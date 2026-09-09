#ifndef COUNT_NOTIFICATION_H
#define COUNT_NOTIFICATION_H /* 头文件保护 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "critical_section.h"
#include "foundation_status.h"

#define COUNT_NOTIFICATION_STORAGE_SIZE ((size_t)64U) /* 通知 opaque 存储字节数。 */
#define COUNT_NOTIFICATION_STORAGE_ALIGNMENT ((size_t)_Alignof(max_align_t)) /* 通知存储对齐。 */

/* 调用方提供的计数通知静态存储。 */
typedef struct {
    max_align_t alignment;
    unsigned char bytes[COUNT_NOTIFICATION_STORAGE_SIZE - sizeof(max_align_t)];
} count_notification_storage_t;

/* 等待策略函数表。 */
typedef foundation_status_t (*count_notification_wait_fn)(void *, uint32_t);
typedef foundation_status_t (*count_notification_signal_fn)(void *);
typedef foundation_status_t (*count_notification_cancel_fn)(void *);

/* 计数通知等待、唤醒和取消能力。 */
typedef struct {
    count_notification_wait_fn wait;
    count_notification_signal_fn signal;
    count_notification_cancel_fn cancel;
    void *context;
} count_notification_wait_strategy_t;

/* 计数通知句柄，由调用方静态分配。 */
typedef struct {
    count_notification_storage_t storage;
} count_notification_t;

/* 生命周期接口。 */
foundation_status_t count_notification_init(count_notification_t *notification, uint32_t maximum,
    const critical_section_port_t *critical, const count_notification_wait_strategy_t *wait);
foundation_status_t count_notification_start(count_notification_t *notification);
foundation_status_t count_notification_stop(count_notification_t *notification);
foundation_status_t count_notification_reset(count_notification_t *notification);
foundation_status_t count_notification_deinit(count_notification_t *notification);

/* 计数操作接口。 */
foundation_status_t count_notification_give(count_notification_t *notification);
foundation_status_t count_notification_give_isr(count_notification_t *notification,
    bool *should_yield);
foundation_status_t count_notification_take(count_notification_t *notification);
foundation_status_t count_notification_take_until(count_notification_t *notification,
    uint32_t deadline);
#endif /* COUNT_NOTIFICATION_H */
