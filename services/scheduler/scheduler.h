#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mizu_status.h"

typedef uint16_t scheduler_id_t;

typedef enum { SCHEDULER_TRIGGER_PERIODIC = 0 } scheduler_trigger_t;

/* 回调在主循环中执行，必须非阻塞；context 由调用方拥有并保持有效。 */
typedef mizu_status_t (*scheduler_task_fn)(void *context, scheduler_trigger_t trigger);

/* 编译期固定的任务配置；调用方长期持有，scheduler 只读取。 */
typedef struct {
    scheduler_task_fn callback;
    void *context;
    uint8_t priority;
    uint32_t period_ticks;
} scheduler_task_config_t;

/* 调用方提供的运行存储；只分配和传入，不得直接读写成员。 */
typedef struct {
    scheduler_id_t id;
    uint32_t elapsed_ticks;
    uint32_t run_count;
    uint32_t error_count;
    uint32_t missed_count;
    mizu_status_t last_status;
    scheduler_trigger_t last_trigger;
    bool ready;
} scheduler_slot_t;

/* scheduler_get_stats() 写入的内部统计副本。 */
typedef struct {
    uint32_t run_count;
    uint32_t error_count;
    uint32_t missed_count;
    mizu_status_t last_status;
    scheduler_trigger_t last_trigger;
} scheduler_task_stats_t;

/* 调度器实例由调用方拥有；配置表和槽数组须覆盖整个实例生命周期。 */
typedef struct {
    scheduler_slot_t *slots;
    const scheduler_task_config_t *configs;
    size_t task_count;
    uint32_t last_tick;
    bool time_initialized;
    bool initialized;
} scheduler_t;

/* 初始化阶段调用；其余 API 只在同一主循环上下文调用，不支持 ISR、并发或重入。
 * 回调不得阻塞、重入 scheduler 或修改配置表和运行槽。
 */

/**
 * scheduler_init - 绑定固定任务配置和运行存储。
 * @scheduler: 调用方拥有、调用前已清零的实例。
 * @slots: 调用方提供的运行槽数组。
 * @slot_capacity: @slots 的实际元素数量。
 * @configs: 调用方长期持有的只读配置表。
 * @task_count: @configs 的实际元素数量，必须等于 @slot_capacity。
 *
 * 调用方须使用实际数组长度提供两个数量；C 接口无法验证指针背后的容量。
 * 返回值：成功返回 MIZU_STATUS_OK；参数无效返回 MIZU_STATUS_INVALID_ARGUMENT；
 * 数量或周期越界返回 MIZU_STATUS_OUT_OF_RANGE。
 */
mizu_status_t scheduler_init(scheduler_t *scheduler, scheduler_slot_t *slots, size_t slot_capacity,
                             const scheduler_task_config_t *configs, size_t task_count);

/**
 * scheduler_update - 按调用方提供的单调 tick 更新周期状态。
 * @scheduler: 已初始化的实例。
 * @now_tick: 与所有任务使用同一单位的当前 tick。
 *
 * 首次调用只建立基准；时间间隔达到 uint32_t 半范围时重建基准，不推进任务。
 * 返回值：成功返回 MIZU_STATUS_OK；未初始化返回 MIZU_STATUS_NOT_INITIALIZED；
 * 时间间隔越界返回 MIZU_STATUS_OUT_OF_RANGE。
 */
mizu_status_t scheduler_update(scheduler_t *scheduler, uint32_t now_tick);

/**
 * scheduler_run - 按优先级执行当前就绪的非阻塞任务。
 * @scheduler: 已初始化的实例。
 *
 * 调用方应在同一主循环中先调用 scheduler_update()，再调用本函数。
 * 返回值：成功返回 MIZU_STATUS_OK；返回本轮第一个回调错误，未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED。
 */
mizu_status_t scheduler_run(scheduler_t *scheduler);

/**
 * scheduler_get_stats - 复制指定任务的统计快照。
 * @scheduler: 已初始化的实例。
 * @id: 原配置表索引。
 * @stats: 调用方提供的输出副本。
 *
 * 返回值：成功返回 MIZU_STATUS_OK；输出地址无效返回 MIZU_STATUS_INVALID_ARGUMENT；
 * 未初始化返回 MIZU_STATUS_NOT_INITIALIZED；ID 越界返回 MIZU_STATUS_OUT_OF_RANGE。
 */
mizu_status_t scheduler_get_stats(const scheduler_t *scheduler, scheduler_id_t id,
                                  scheduler_task_stats_t *stats);

#endif /* SCHEDULER_H */
