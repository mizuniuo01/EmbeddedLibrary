#ifndef QUEUE_PORT_H
#define QUEUE_PORT_H /* 头文件保护 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "critical_section.h"
#include "foundation_status.h"
#include "waiter.h"

#define QUEUE_PORT_STORAGE_SIZE ((size_t)128U) /* 队列 opaque 存储字节数。 */
#define QUEUE_PORT_STORAGE_ALIGNMENT ((size_t)_Alignof(max_align_t)) /* 队列存储对齐。 */

/* 调用方提供的队列静态存储。 */
typedef union {
    max_align_t alignment;
    unsigned char bytes[QUEUE_PORT_STORAGE_SIZE];
} queue_port_storage_t;

/* 队列句柄，由调用方静态分配。 */
typedef struct {
    queue_port_storage_t storage;
} queue_port_t;

/* 生命周期和配置接口。 */
foundation_status_t queue_port_init(queue_port_t *queue, void *storage, size_t storage_size,
    size_t element_size, size_t capacity, const critical_section_port_t *critical,
    const waiter_port_t *wait_port);
foundation_status_t queue_port_start(queue_port_t *queue);
foundation_status_t queue_port_stop(queue_port_t *queue);
foundation_status_t queue_port_reset(queue_port_t *queue);
foundation_status_t queue_port_deinit(queue_port_t *queue);

/* 普通上下文队列操作。 */
foundation_status_t queue_port_try_push(queue_port_t *queue, const void *data);
foundation_status_t queue_port_push_until(queue_port_t *queue, waiter_t *waiter, const void *data,
    uint32_t deadline);
foundation_status_t queue_port_push_isr(queue_port_t *queue, const void *data, bool *should_yield);
foundation_status_t queue_port_try_pop(queue_port_t *queue, void *data);
foundation_status_t queue_port_pop_until(queue_port_t *queue, waiter_t *waiter, void *data,
    uint32_t deadline);

/* ISR 上下文队列操作。 */
foundation_status_t queue_port_pop_isr(queue_port_t *queue, void *data, bool *should_yield);

/* 查询接口。 */
foundation_status_t queue_port_size(const queue_port_t *queue, size_t *size);
foundation_status_t queue_port_capacity(const queue_port_t *queue, size_t *capacity);

#endif /* QUEUE_PORT_H */
