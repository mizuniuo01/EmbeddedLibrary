#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H /* 头文件保护 */

#include <stddef.h>
#include <stdint.h>

#include "foundation_status.h"

/* 调用方提供相互独立的数据区与每槽一字节的状态区。 */
typedef struct {
    void *storage;
    size_t storage_size;
    uint8_t *states;
    size_t states_size;
    size_t slot_size;
    size_t capacity;
    size_t alignment;
} object_pool_config_t;

/* 首次使用前零初始化；字段由组件维护，禁止复制。 */
typedef struct {
    uint8_t *storage;
    uint8_t *states;
    size_t slot_size;
    size_t capacity;
    size_t available;
    size_t lowest_free_index;
    size_t exhausted_count;
} object_pool_t;

/* 生命周期与所有权转移。 */
foundation_status_t object_pool_init(object_pool_t *pool, const object_pool_config_t *config);
foundation_status_t object_pool_acquire(object_pool_t *pool, void **result, size_t *index);
foundation_status_t object_pool_release(object_pool_t *pool, void *data);

/* 只读诊断。 */
foundation_status_t object_pool_capacity(const object_pool_t *pool, size_t *result);
foundation_status_t object_pool_available(const object_pool_t *pool, size_t *result);
foundation_status_t object_pool_lowest_free_index(const object_pool_t *pool, size_t *result);
foundation_status_t object_pool_exhausted_count(const object_pool_t *pool, size_t *result);

#endif /* OBJECT_POOL_H */
