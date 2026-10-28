#ifndef WAITER_H
#define WAITER_H /* 头文件保护 */

#include <stddef.h>
#include <stdint.h>

#include "foundation_status.h"

#define WAITER_STORAGE_SIZE ((size_t)64U) /* waiter opaque 存储字节数。 */
#define WAITER_STORAGE_ALIGNMENT ((size_t)_Alignof(max_align_t)) /* waiter 存储对齐。 */

typedef union {
    max_align_t alignment;
    unsigned char bytes[WAITER_STORAGE_SIZE];
} waiter_storage_t;

typedef struct {
    waiter_storage_t storage;
} waiter_t;

typedef enum {
    WAITER_STATE_UNINITIALIZED, /* waiter 尚未初始化。 */
    WAITER_STATE_IDLE,          /* waiter 可以开始等待。 */
    WAITER_STATE_WAITING,       /* waiter 正在等待对象。 */
    WAITER_STATE_COMPLETED,     /* waiter 已完成一次等待。 */
    WAITER_STATE_CANCELLED,     /* waiter 已被取消。 */
} waiter_state_t;

typedef foundation_status_t (*waiter_port_wait_fn)(void *context, waiter_t *waiter,
    uint32_t deadline);
typedef foundation_status_t (*waiter_port_signal_fn)(void *context);
typedef foundation_status_t (*waiter_port_cancel_fn)(void *context);

typedef struct {
    waiter_port_wait_fn wait;
    waiter_port_signal_fn signal_one;
    waiter_port_signal_fn signal_all;
    waiter_port_cancel_fn cancel_all;
    void *context;
} waiter_port_t;

foundation_status_t waiter_init(waiter_t *waiter);
foundation_status_t waiter_begin(waiter_t *waiter, const void *owner);
foundation_status_t waiter_complete(waiter_t *waiter, foundation_status_t result);
foundation_status_t waiter_reset(waiter_t *waiter);
foundation_status_t waiter_state(const waiter_t *waiter, waiter_state_t *state);
foundation_status_t waiter_owner(const waiter_t *waiter, const void **owner);

#endif /* WAITER_H */
