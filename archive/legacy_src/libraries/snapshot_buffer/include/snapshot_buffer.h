#ifndef SNAPSHOT_BUFFER_H
#define SNAPSHOT_BUFFER_H /* 头文件保护 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation_status.h"

/* 读借用必须原地址归还，禁止复制、修改或提前销毁。 */
typedef struct {
    const uint8_t *data;
    size_t size;
    uint32_t sequence;
    uint32_t timestamp;
} snapshot_buffer_lease_t;

/* 写借用必须由 begin 获取，publish/cancel 后指针失效。 */
typedef struct {
    uint8_t *data;
    size_t capacity;
} snapshot_buffer_writer_t;

/* 一次发布的元数据。 */
typedef struct {
    size_t size;
    uint32_t sequence;
    uint32_t timestamp;
} snapshot_buffer_info_t;

/* 实例首次使用前零初始化；各槽允许一个未归还读借用。 */
typedef struct {
    uint8_t *slots[2];
    size_t capacity;
    size_t active;
    size_t writing;
    bool published;
    snapshot_buffer_writer_t *writer;
    snapshot_buffer_lease_t *readers[2];
    snapshot_buffer_info_t info[2];
} snapshot_buffer_t;

/* 生命周期与生产操作。 */
foundation_status_t snapshot_buffer_init(snapshot_buffer_t *buffer, uint8_t *first, uint8_t *second,
    size_t capacity);
foundation_status_t snapshot_buffer_begin(snapshot_buffer_t *buffer,
    snapshot_buffer_writer_t *writer);
foundation_status_t snapshot_buffer_publish(snapshot_buffer_t *buffer,
    snapshot_buffer_writer_t *writer, size_t size, uint32_t timestamp);
foundation_status_t snapshot_buffer_cancel(snapshot_buffer_t *buffer,
    snapshot_buffer_writer_t *writer);

/* 读取和借用归还；所有 API 调用需外部串行化。 */
foundation_status_t snapshot_buffer_acquire(snapshot_buffer_t *buffer,
    snapshot_buffer_lease_t *lease);
foundation_status_t snapshot_buffer_release(snapshot_buffer_t *buffer,
    snapshot_buffer_lease_t *lease);
foundation_status_t snapshot_buffer_copy(const snapshot_buffer_t *buffer, uint8_t *data,
    size_t capacity, snapshot_buffer_info_t *result);
foundation_status_t snapshot_buffer_sequence(const snapshot_buffer_t *buffer, uint32_t *result);
foundation_status_t snapshot_buffer_timestamp(const snapshot_buffer_t *buffer, uint32_t *result);

#endif /* SNAPSHOT_BUFFER_H */
