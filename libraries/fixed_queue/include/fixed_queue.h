#ifndef FIXED_QUEUE_H
#define FIXED_QUEUE_H /* 头文件保护 */

#include <stddef.h>
#include <stdint.h>

#include "foundation_status.h"

/* 调用方零初始化并静态分配；字段仅供组件使用，初始化后禁止复制。 */
typedef struct {
    uint8_t *storage;
    size_t element_size;
    size_t capacity;
    size_t head;
    size_t size;
    size_t rejected_count;
    size_t high_water_mark;
} fixed_queue_t;

/* 生命周期、元素复制与只读统计。 */
foundation_status_t fixed_queue_init(fixed_queue_t *queue, uint8_t *storage, size_t storage_size,
    size_t element_size, size_t capacity);
foundation_status_t fixed_queue_push(fixed_queue_t *queue, const void *data);
foundation_status_t fixed_queue_push_some(fixed_queue_t *queue, const void *data, size_t count,
    size_t *result);
foundation_status_t fixed_queue_pop(fixed_queue_t *queue, void *data);
foundation_status_t fixed_queue_pop_some(fixed_queue_t *queue, void *data, size_t count,
    size_t *result);
foundation_status_t fixed_queue_peek_at(const fixed_queue_t *queue, size_t index, void *data);
foundation_status_t fixed_queue_peek(const fixed_queue_t *queue, void *data);
foundation_status_t fixed_queue_size(const fixed_queue_t *queue, size_t *result);
foundation_status_t fixed_queue_capacity(const fixed_queue_t *queue, size_t *result);
foundation_status_t fixed_queue_free(const fixed_queue_t *queue, size_t *result);
foundation_status_t fixed_queue_rejected_count(const fixed_queue_t *queue, size_t *result);
foundation_status_t fixed_queue_high_water_mark(const fixed_queue_t *queue, size_t *result);

#endif /* FIXED_QUEUE_H */
