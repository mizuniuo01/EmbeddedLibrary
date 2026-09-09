#ifndef EVENT_GROUP_H
#define EVENT_GROUP_H /* 头文件保护 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "critical_section.h"
#include "foundation_status.h"

#define EVENT_GROUP_STORAGE_SIZE ((size_t)64U) /* 事件组 opaque 存储字节数。 */
#define EVENT_GROUP_STORAGE_ALIGNMENT ((size_t)_Alignof(max_align_t)) /* 事件组存储对齐。 */

/* 调用方提供的事件组静态存储。 */
typedef struct {
    max_align_t alignment;
    unsigned char bytes[EVENT_GROUP_STORAGE_SIZE - sizeof(max_align_t)];
} event_group_storage_t;

/* 等待策略函数表。 */
typedef foundation_status_t (*event_group_wait_fn)(void *, uint32_t);
typedef foundation_status_t (*event_group_signal_fn)(void *);
typedef foundation_status_t (*event_group_cancel_fn)(void *);

/* 事件组等待、唤醒和取消能力。 */
typedef struct {
    event_group_wait_fn wait;
    event_group_signal_fn signal;
    event_group_cancel_fn cancel;
    void *context;
} event_group_wait_strategy_t;

/* 事件组句柄，由调用方静态分配。 */
typedef struct {
    event_group_storage_t storage;
} event_group_t;

/* 生命周期接口。 */
foundation_status_t event_group_init(event_group_t *group, const critical_section_port_t *critical,
    const event_group_wait_strategy_t *wait);
foundation_status_t event_group_start(event_group_t *group);
foundation_status_t event_group_stop(event_group_t *group);
foundation_status_t event_group_reset(event_group_t *group);
foundation_status_t event_group_deinit(event_group_t *group);

/* 事件位修改接口。 */
foundation_status_t event_group_set(event_group_t *group, uint32_t mask);
foundation_status_t event_group_set_isr(event_group_t *group, uint32_t mask, bool *should_yield);
foundation_status_t event_group_clear(event_group_t *group, uint32_t mask);
foundation_status_t event_group_clear_isr(event_group_t *group, uint32_t mask, bool *should_yield);

/* 事件位等待接口。 */
foundation_status_t event_group_wait_any(event_group_t *group, uint32_t mask, bool clear_on_exit,
    uint32_t deadline, uint32_t *matched);
foundation_status_t event_group_wait_all(event_group_t *group, uint32_t mask, bool clear_on_exit,
    uint32_t deadline, uint32_t *matched);

#endif /* EVENT_GROUP_H */
