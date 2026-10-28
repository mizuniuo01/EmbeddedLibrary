#ifndef MUTEX_PORT_H
#define MUTEX_PORT_H /* 头文件保护 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "critical_section.h"
#include "foundation_status.h"
#include "waiter.h"

#define MUTEX_PORT_STORAGE_SIZE ((size_t)64U) /* 互斥对象 opaque 存储字节数。 */
#define MUTEX_PORT_STORAGE_ALIGNMENT ((size_t)_Alignof(max_align_t)) /* 互斥存储对齐。 */

/* 调用方提供的互斥对象静态存储。 */
typedef union {
    max_align_t alignment;
    unsigned char bytes[MUTEX_PORT_STORAGE_SIZE];
} mutex_port_storage_t;

/* 互斥句柄，由调用方静态分配。 */
typedef struct {
    mutex_port_storage_t storage;
} mutex_port_t;

/* 互斥初始化能力配置。 */
typedef struct {
    bool adapter_supports_priority_inheritance;
    bool require_priority_inheritance;
} mutex_port_config_t;

/* 生命周期接口。 */
foundation_status_t mutex_port_init(mutex_port_t *mutex, const critical_section_port_t *critical,
    const waiter_port_t *wait_port, const mutex_port_config_t *config);
foundation_status_t mutex_port_start(mutex_port_t *mutex);
foundation_status_t mutex_port_stop(mutex_port_t *mutex);
foundation_status_t mutex_port_reset(mutex_port_t *mutex);
foundation_status_t mutex_port_deinit(mutex_port_t *mutex);

/* 锁操作接口。 */
foundation_status_t mutex_port_try_lock(mutex_port_t *mutex, uintptr_t owner_token);
foundation_status_t mutex_port_lock_until(mutex_port_t *mutex, waiter_t *waiter,
    uintptr_t owner_token, uint32_t deadline);
foundation_status_t mutex_port_unlock(mutex_port_t *mutex, uintptr_t owner_token);

#endif /* MUTEX_PORT_H */
