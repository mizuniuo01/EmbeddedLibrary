/* Services 层的裸机协作调度器。
 * 任务只在主循环中执行；时间由调用方提供，不访问硬件、ISR、DMA 或 RTOS。
 * 任务回调必须非阻塞，调度器只维护固定任务的周期状态和执行统计。
 * 配置表和运行槽由调用方长期持有，scheduler 只修改运行槽状态。
 */

#include "scheduler.h"

#include <limits.h>

#define SCHEDULER_MAX_INTERVAL_TICKS ((uint32_t)INT32_MAX)

/* uint32_t tick 只有在相邻采样间隔小于半个计数空间时才能确定先后关系。 */

static bool scheduler_is_valid(const scheduler_t *scheduler)
{
    return (scheduler != NULL) && scheduler->initialized;
}

static void scheduler_increment_counter(uint32_t *counter, uint32_t amount)
{
    if (UINT32_MAX - *counter < amount) {
        *counter = UINT32_MAX;
    } else {
        *counter += amount;
    }
}

static bool scheduler_slot_follows(const scheduler_t *scheduler, scheduler_id_t left,
                                   scheduler_id_t right)
{
    const scheduler_task_config_t *left_config = &scheduler->configs[left];
    const scheduler_task_config_t *right_config = &scheduler->configs[right];

    return (left_config->priority > right_config->priority) ||
           ((left_config->priority == right_config->priority) && (left > right));
}

static void scheduler_swap_slots(scheduler_slot_t *left, scheduler_slot_t *right)
{
    scheduler_slot_t temporary = *left;

    *left = *right;
    *right = temporary;
}

static void scheduler_sift_down(scheduler_t *scheduler, size_t root, size_t count)
{
    while (root < count / 2U) {
        size_t child = root * 2U + 1U;

        if ((child + 1U < count) &&
            scheduler_slot_follows(scheduler, scheduler->slots[child + 1U].id,
                                   scheduler->slots[child].id)) {
            child++;
        }
        if (!scheduler_slot_follows(scheduler, scheduler->slots[child].id,
                                    scheduler->slots[root].id)) {
            break;
        }
        scheduler_swap_slots(&scheduler->slots[root], &scheduler->slots[child]);
        root = child;
    }
}

static void scheduler_sort_slots(scheduler_t *scheduler)
{
    /* 初始化时按优先级和原配置 ID 排序，使运行期派发只需线性扫描。 */
    for (size_t start = scheduler->task_count / 2U; start > 0U; --start) {
        scheduler_sift_down(scheduler, start - 1U, scheduler->task_count);
    }
    for (size_t end = scheduler->task_count; end > 1U; --end) {
        scheduler_swap_slots(&scheduler->slots[0], &scheduler->slots[end - 1U]);
        scheduler_sift_down(scheduler, 0U, end - 1U);
    }
}

static mizu_status_t scheduler_validate_config(const scheduler_task_config_t *configs,
                                               size_t task_count)
{
    if ((task_count > 0U) && (configs == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (task_count > (size_t)UINT16_MAX) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }

    for (size_t index = 0U; index < task_count; ++index) {
        if (configs[index].callback == NULL) {
            return MIZU_STATUS_INVALID_ARGUMENT;
        }
        if ((configs[index].period_ticks == 0U) ||
            (configs[index].period_ticks > SCHEDULER_MAX_INTERVAL_TICKS)) {
            return MIZU_STATUS_OUT_OF_RANGE;
        }
    }
    return MIZU_STATUS_OK;
}

mizu_status_t scheduler_init(scheduler_t *scheduler, scheduler_slot_t *slots, size_t slot_capacity,
                             const scheduler_task_config_t *configs, size_t task_count)
{
    mizu_status_t status;

    if (scheduler == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    scheduler->initialized = false;

    if ((slot_capacity != task_count) || ((task_count > 0U) && (slots == NULL))) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    status = scheduler_validate_config(configs, task_count);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    for (size_t index = 0U; index < task_count; ++index) {
        slots[index].id = (scheduler_id_t)index;
        slots[index].elapsed_ticks = 0U;
        slots[index].run_count = 0U;
        slots[index].error_count = 0U;
        slots[index].missed_count = 0U;
        slots[index].last_status = MIZU_STATUS_OK;
        slots[index].last_trigger = SCHEDULER_TRIGGER_PERIODIC;
        slots[index].ready = false;
    }

    scheduler->slots = slots;
    scheduler->configs = configs;
    scheduler->task_count = task_count;
    scheduler->last_tick = 0U;
    scheduler->time_initialized = false;
    scheduler_sort_slots(scheduler);
    scheduler->initialized = true;
    return MIZU_STATUS_OK;
}

mizu_status_t scheduler_update(scheduler_t *scheduler, uint32_t now_tick)
{
    uint32_t delta;

    if (!scheduler_is_valid(scheduler)) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    if (!scheduler->time_initialized) {
        scheduler->last_tick = now_tick;
        scheduler->time_initialized = true;
        return MIZU_STATUS_OK;
    }

    delta = now_tick - scheduler->last_tick;
    scheduler->last_tick = now_tick;
    if (delta > SCHEDULER_MAX_INTERVAL_TICKS) {
        /* 时间源间隔无法解释时重建基准，不把异常间隔计入任何任务。 */
        return MIZU_STATUS_OUT_OF_RANGE;
    }

    for (size_t index = 0U; index < scheduler->task_count; ++index) {
        scheduler_slot_t *slot = &scheduler->slots[index];
        const uint32_t period = scheduler->configs[slot->id].period_ticks;
        const uint32_t total = slot->elapsed_ticks + delta;
        const uint32_t periods = total / period;

        /* 保留不足一个周期的余数，ready 只合并触发，不追赶执行。 */
        slot->elapsed_ticks = total % period;
        if (periods == 0U) {
            continue;
        }
        if (slot->ready) {
            /* 当前 ready 已代表一个周期，其他完整周期计入 missed。 */
            scheduler_increment_counter(&slot->missed_count, periods);
        } else {
            /* 第一个完整周期形成当前 ready，其余周期计入 missed。 */
            slot->ready = true;
            scheduler_increment_counter(&slot->missed_count, periods - 1U);
        }
    }
    return MIZU_STATUS_OK;
}

mizu_status_t scheduler_run(scheduler_t *scheduler)
{
    mizu_status_t first_error = MIZU_STATUS_OK;
    if (!scheduler_is_valid(scheduler)) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }

    for (size_t index = 0U; index < scheduler->task_count; ++index) {
        scheduler_slot_t *slot = &scheduler->slots[index];
        const scheduler_task_config_t *config = &scheduler->configs[slot->id];
        mizu_status_t status;

        if (!slot->ready) {
            continue;
        }
        /* 先清除本轮触发，再在调度器状态之外执行回调；每轮最多执行一次。 */
        slot->ready = false;
        scheduler_increment_counter(&slot->run_count, 1U);
        slot->last_trigger = SCHEDULER_TRIGGER_PERIODIC;
        status = config->callback(config->context, slot->last_trigger);
        slot->last_status = status;
        if (status != MIZU_STATUS_OK) {
            scheduler_increment_counter(&slot->error_count, 1U);
            if (first_error == MIZU_STATUS_OK) {
                first_error = status;
            }
        }
    }
    return first_error;
}

mizu_status_t scheduler_get_stats(const scheduler_t *scheduler, scheduler_id_t id,
                                  scheduler_task_stats_t *stats)
{
    const scheduler_slot_t *slot;

    if (stats == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!scheduler_is_valid(scheduler)) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    if ((size_t)id >= scheduler->task_count) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }

    slot = NULL;
    for (size_t index = 0U; index < scheduler->task_count; ++index) {
        if (scheduler->slots[index].id == id) {
            slot = &scheduler->slots[index];
            break;
        }
    }
    if (slot == NULL) {
        return MIZU_STATUS_INTERNAL_ERROR;
    }
    stats->run_count = slot->run_count;
    stats->error_count = slot->error_count;
    stats->missed_count = slot->missed_count;
    stats->last_status = slot->last_status;
    stats->last_trigger = slot->last_trigger;
    return MIZU_STATUS_OK;
}
