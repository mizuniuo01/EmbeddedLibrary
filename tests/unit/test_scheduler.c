/* 裸机调度器的初始化、时间边界、派发顺序和统计测试。 */

#include "test_support.h"

#include <limits.h>
#include <stdint.h>

#include "scheduler.h"

typedef struct {
    uint8_t id;
    uint8_t *order;
    size_t *order_count;
    size_t order_capacity;
    scheduler_trigger_t last_trigger;
    mizu_status_t result;
} test_task_context_t;

static mizu_status_t test_record_task(void *context, scheduler_trigger_t trigger)
{
    test_task_context_t *task = context;

    task->last_trigger = trigger;
    if (*task->order_count >= task->order_capacity) {
        return MIZU_STATUS_INTERNAL_ERROR;
    }
    task->order[*task->order_count] = task->id;
    (*task->order_count)++;
    return task->result;
}

static int test_initialization(void)
{
    scheduler_t scheduler = {0};
    scheduler_slot_t slots[2] = {{0}};
    uint8_t order[1] = {0U};
    size_t order_count = 0U;
    test_task_context_t context =
        {0U, order, &order_count, 1U, SCHEDULER_TRIGGER_PERIODIC, MIZU_STATUS_OK};
    scheduler_task_config_t config = {test_record_task, &context, 0U, 1U};

    slots[0].run_count = 123U;
    slots[1].run_count = 456U;
    TEST_ASSERT(scheduler_init(NULL, slots, 1U, &config, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(scheduler_init(&scheduler, slots, 1U, &config, 2U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(slots[0].run_count == 123U);
    TEST_ASSERT(slots[1].run_count == 456U);
    TEST_ASSERT(scheduler_update(&scheduler, 0U) == MIZU_STATUS_NOT_INITIALIZED);
    TEST_ASSERT(scheduler_init(&scheduler, NULL, 1U, &config, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(scheduler_init(&scheduler, slots, 1U, NULL, 1U) == MIZU_STATUS_INVALID_ARGUMENT);

    config.callback = NULL;
    TEST_ASSERT(scheduler_init(&scheduler, slots, 1U, &config, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    config.callback = test_record_task;
    config.period_ticks = 0U;
    TEST_ASSERT(scheduler_init(&scheduler, slots, 1U, &config, 1U) == MIZU_STATUS_OUT_OF_RANGE);
    config.period_ticks = (uint32_t)INT32_MAX + 1U;
    TEST_ASSERT(scheduler_init(&scheduler, slots, 1U, &config, 1U) == MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(scheduler_init(&scheduler, slots, (size_t)UINT16_MAX + 1U, &config,
                               (size_t)UINT16_MAX + 1U) == MIZU_STATUS_OUT_OF_RANGE);

    TEST_ASSERT(scheduler_init(&scheduler, NULL, 0U, NULL, 0U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, 0U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_run(&scheduler) == MIZU_STATUS_OK);
    return 0;
}

static int test_order_and_errors(void)
{
    scheduler_t scheduler = {0};
    scheduler_slot_t slots[4];
    scheduler_task_stats_t stats;
    uint8_t order[4] = {0U};
    size_t order_count = 0U;
    test_task_context_t tasks[4] =
        {{0U, order, &order_count, 4U, SCHEDULER_TRIGGER_PERIODIC, MIZU_STATUS_OK},
         {1U, order, &order_count, 4U, SCHEDULER_TRIGGER_PERIODIC, MIZU_STATUS_INTERNAL_ERROR},
         {2U, order, &order_count, 4U, SCHEDULER_TRIGGER_PERIODIC, MIZU_STATUS_OUT_OF_RANGE},
         {3U, order, &order_count, 4U, SCHEDULER_TRIGGER_PERIODIC, MIZU_STATUS_OK}};
    const scheduler_task_config_t configs[4] = {{test_record_task, &tasks[0], 3U, 10U},
                                                {test_record_task, &tasks[1], 1U, 10U},
                                                {test_record_task, &tasks[2], 1U, 10U},
                                                {test_record_task, &tasks[3], 0U, 10U}};

    TEST_ASSERT(scheduler_init(&scheduler, slots, 4U, configs, 4U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_get_stats(&scheduler, 1U, &stats) == MIZU_STATUS_OK);
    TEST_ASSERT(stats.run_count == 0U);
    TEST_ASSERT(stats.last_status == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, 100U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_run(&scheduler) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, 110U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_run(&scheduler) == MIZU_STATUS_INTERNAL_ERROR);
    TEST_ASSERT(order_count == 4U);
    TEST_ASSERT(order[0] == 3U);
    TEST_ASSERT(order[1] == 1U);
    TEST_ASSERT(order[2] == 2U);
    TEST_ASSERT(order[3] == 0U);
    TEST_ASSERT(tasks[1].last_trigger == SCHEDULER_TRIGGER_PERIODIC);
    TEST_ASSERT(scheduler_get_stats(&scheduler, 1U, &stats) == MIZU_STATUS_OK);
    TEST_ASSERT(stats.run_count == 1U);
    TEST_ASSERT(stats.error_count == 1U);
    TEST_ASSERT(stats.last_status == MIZU_STATUS_INTERNAL_ERROR);
    stats.run_count = 99U;
    TEST_ASSERT(scheduler_get_stats(&scheduler, 1U, &stats) == MIZU_STATUS_OK);
    TEST_ASSERT(stats.run_count == 1U);
    TEST_ASSERT(scheduler_get_stats(&scheduler, 2U, &stats) == MIZU_STATUS_OK);
    TEST_ASSERT(stats.last_status == MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(scheduler_get_stats(&scheduler, 4U, &stats) == MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(scheduler_get_stats(&scheduler, 0U, NULL) == MIZU_STATUS_INVALID_ARGUMENT);
    return 0;
}

static int test_period_and_recovery(void)
{
    scheduler_t scheduler = {0};
    scheduler_slot_t slot;
    scheduler_task_stats_t stats;
    uint8_t order[3] = {0U};
    size_t order_count = 0U;
    test_task_context_t task =
        {7U, order, &order_count, 3U, SCHEDULER_TRIGGER_PERIODIC, MIZU_STATUS_OK};
    const scheduler_task_config_t config = {test_record_task, &task, 0U, 10U};

    TEST_ASSERT(scheduler_init(&scheduler, &slot, 1U, &config, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, 0U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, 25U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, 35U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_get_stats(&scheduler, 0U, &stats) == MIZU_STATUS_OK);
    TEST_ASSERT(stats.missed_count == 2U);
    TEST_ASSERT(slot.elapsed_ticks == 5U);
    TEST_ASSERT(scheduler_run(&scheduler) == MIZU_STATUS_OK);
    TEST_ASSERT(order_count == 1U);
    TEST_ASSERT(scheduler_update(&scheduler, 40U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_run(&scheduler) == MIZU_STATUS_OK);
    TEST_ASSERT(order_count == 2U);
    TEST_ASSERT(scheduler_update(&scheduler, (uint32_t)INT32_MAX + 41U) ==
                MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(scheduler_get_stats(&scheduler, 0U, &stats) == MIZU_STATUS_OK);
    TEST_ASSERT(stats.missed_count == 2U);
    TEST_ASSERT(scheduler_update(&scheduler, (uint32_t)INT32_MAX + 51U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_run(&scheduler) == MIZU_STATUS_OK);
    TEST_ASSERT(order_count == 3U);
    return 0;
}

static int test_wrap_and_saturation(void)
{
    scheduler_t scheduler = {0};
    scheduler_slot_t slot;
    scheduler_task_stats_t stats;
    uint8_t order[2] = {0U};
    size_t order_count = 0U;
    test_task_context_t task =
        {1U, order, &order_count, 2U, SCHEDULER_TRIGGER_PERIODIC, MIZU_STATUS_INTERNAL_ERROR};
    const scheduler_task_config_t config = {test_record_task, &task, 0U, 5U};

    TEST_ASSERT(scheduler_init(&scheduler, &slot, 1U, &config, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, UINT32_MAX - 2U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, 2U) == MIZU_STATUS_OK);
    slot.run_count = UINT32_MAX;
    slot.error_count = UINT32_MAX;
    slot.missed_count = UINT32_MAX;
    TEST_ASSERT(scheduler_run(&scheduler) == MIZU_STATUS_INTERNAL_ERROR);
    TEST_ASSERT(scheduler_get_stats(&scheduler, 0U, &stats) == MIZU_STATUS_OK);
    TEST_ASSERT(stats.run_count == UINT32_MAX);
    TEST_ASSERT(stats.error_count == UINT32_MAX);
    TEST_ASSERT(stats.missed_count == UINT32_MAX);
    TEST_ASSERT(order_count == 1U);
    return 0;
}

static int test_large_period_accumulation(void)
{
    scheduler_t scheduler = {0};
    scheduler_slot_t slot;
    uint8_t order[1] = {0U};
    size_t order_count = 0U;
    test_task_context_t task =
        {4U, order, &order_count, 1U, SCHEDULER_TRIGGER_PERIODIC, MIZU_STATUS_OK};
    const scheduler_task_config_t config = {test_record_task, &task, 0U, INT32_MAX};

    TEST_ASSERT(scheduler_init(&scheduler, &slot, 1U, &config, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, 0U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, (uint32_t)INT32_MAX - 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(scheduler_update(&scheduler, UINT32_MAX - 2U) == MIZU_STATUS_OK);
    TEST_ASSERT(slot.elapsed_ticks == (uint32_t)INT32_MAX - 1U);
    TEST_ASSERT(scheduler_run(&scheduler) == MIZU_STATUS_OK);
    TEST_ASSERT(order_count == 1U);
    return 0;
}

int main(void)
{
    int (* const cases[])(void) = {test_initialization, test_order_and_errors,
                                   test_period_and_recovery, test_wrap_and_saturation,
                                   test_large_period_accumulation};

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        int result = cases[index]();

        if (result != 0) {
            return result;
        }
    }
    return 0;
}
