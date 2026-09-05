#ifndef BYTE_FIFO_H
#define BYTE_FIFO_H /* 头文件保护 */

#include <stddef.h>
#include <stdint.h>

#include "foundation_status.h"

/* 首次使用前零初始化；字段由组件维护，初始化后禁止复制。 */
typedef struct {
    uint8_t *storage;
    size_t capacity;
    size_t read_index;
    size_t write_index;
    size_t size;
    size_t rejected_count;
    size_t high_water_mark;
} byte_fifo_t;

/* 生命周期与完整或部分批量传输。 */
foundation_status_t byte_fifo_init(byte_fifo_t *fifo, uint8_t *storage, size_t capacity);
foundation_status_t byte_fifo_write(byte_fifo_t *fifo, const uint8_t *data, size_t size);
foundation_status_t byte_fifo_read(byte_fifo_t *fifo, uint8_t *data, size_t size);
foundation_status_t byte_fifo_write_some(byte_fifo_t *fifo, const uint8_t *data, size_t size,
    size_t *written);
foundation_status_t byte_fifo_read_some(byte_fifo_t *fifo, uint8_t *data, size_t size,
    size_t *read);
foundation_status_t byte_fifo_peek(const byte_fifo_t *fifo, uint8_t *data, size_t size);
foundation_status_t byte_fifo_discard(byte_fifo_t *fifo, size_t size);
/* 单字节快捷操作。 */
foundation_status_t byte_fifo_push(byte_fifo_t *fifo, uint8_t value);
foundation_status_t byte_fifo_pop(byte_fifo_t *fifo, uint8_t *value);
foundation_status_t byte_fifo_peek_byte(const byte_fifo_t *fifo, uint8_t *value);
/* 只读容量和诊断。 */
foundation_status_t byte_fifo_size(const byte_fifo_t *fifo, size_t *size);
foundation_status_t byte_fifo_capacity(const byte_fifo_t *fifo, size_t *capacity);
foundation_status_t byte_fifo_free(const byte_fifo_t *fifo, size_t *free_size);
foundation_status_t byte_fifo_rejected_count(const byte_fifo_t *fifo, size_t *count);
foundation_status_t byte_fifo_high_water_mark(const byte_fifo_t *fifo, size_t *mark);

#endif /* BYTE_FIFO_H */
