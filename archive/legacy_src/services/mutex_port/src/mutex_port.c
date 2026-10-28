/**
 * @file mutex_port.c
 * @brief 非递归互斥端口实现。
 */

#include "mutex_port.h"

typedef enum {
    MUTEX_STATE_UNINITIALIZED, /* 对象尚未初始化。 */
    MUTEX_STATE_READY,         /* 对象允许锁操作。 */
    MUTEX_STATE_STOPPED,       /* 对象已停止并取消等待。 */
    MUTEX_STATE_DEINITIALIZED, /* 对象已解除绑定。 */
} mutex_state_t;

typedef struct {
    bool locked;
    bool priority_inheritance;
    uintptr_t owner_token;
    const critical_section_port_t *critical;
    const waiter_port_t *wait_port;
    mutex_state_t state;
} mutex_impl_t;

_Static_assert(sizeof(mutex_impl_t) <= MUTEX_PORT_STORAGE_SIZE, "mutex storage too small");

/**
 * @brief 获取互斥对象私有状态。
 * @param mutex 互斥对象。
 * @return 私有状态地址。
 */
static mutex_impl_t *mutex_impl(mutex_port_t *mutex)
{
    // NOLINTNEXTLINE(bugprone-casting-through-void)
    return (mutex_impl_t *)(void *)mutex->storage.bytes;
}

/**
 * @brief 尝试在临界区内获取互斥。
 * @param mutex 私有互斥状态。
 * @param owner_token 调用方持有者 token。
 * @retval FOUNDATION_STATUS_OK 获取成功。
 * @retval FOUNDATION_STATUS_BUSY 已被其他持有者占用。
 * @retval FOUNDATION_STATUS_INVALID_STATE 当前状态不允许操作。
 */
static foundation_status_t mutex_try_lock_internal(mutex_impl_t *mutex, uintptr_t owner_token)
{
    critical_section_token_t token;
    foundation_status_t status;

    status = critical_section_enter(mutex->critical, &token);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!mutex->locked) {
        mutex->locked = true;
        mutex->owner_token = owner_token;
        status = FOUNDATION_STATUS_OK;
    } else if (mutex->owner_token == owner_token) {
        status = FOUNDATION_STATUS_INVALID_STATE;
    } else {
        status = FOUNDATION_STATUS_BUSY;
    }
    (void)critical_section_exit(mutex->critical, token);
    return status;
}

/**
 * @brief 初始化非递归互斥对象。
 * @param mutex 互斥对象。
 * @param critical 临界区端口。
 * @param wait 等待策略端口。
 * @param config 优先级继承能力配置。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数或必需函数缺失。
 * @retval FOUNDATION_STATUS_UNSUPPORTED 适配器不支持所要求能力。
 * @retval FOUNDATION_STATUS_BUSY 对象仍处于活动状态。
 */
foundation_status_t mutex_port_init(mutex_port_t *mutex, const critical_section_port_t *critical,
    const waiter_port_t *wait_port, const mutex_port_config_t *config)
{
    mutex_impl_t *impl;

    if ((mutex == NULL) || (critical == NULL) || (critical->enter == NULL) ||
        (critical->exit == NULL) || (wait_port == NULL) || (config == NULL) ||
        (wait_port->wait == NULL) || (wait_port->signal_one == NULL) ||
        (wait_port->cancel_all == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = mutex_impl(mutex);
    if ((impl->state == MUTEX_STATE_READY) || (impl->state == MUTEX_STATE_STOPPED)) {
        return FOUNDATION_STATUS_BUSY;
    }
    if (config->require_priority_inheritance && !config->adapter_supports_priority_inheritance) {
        return FOUNDATION_STATUS_UNSUPPORTED;
    }
    impl->locked = false;
    impl->priority_inheritance = config->adapter_supports_priority_inheritance;
    impl->owner_token = 0U;
    impl->critical = critical;
    impl->wait_port = wait_port;
    impl->state = MUTEX_STATE_READY;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 启动已停止互斥并清除持有状态。
 * @param mutex 互斥对象。
 * @retval FOUNDATION_STATUS_OK 启动成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 STOPPED 状态。
 */
foundation_status_t mutex_port_start(mutex_port_t *mutex)
{
    mutex_impl_t *impl;
    if (mutex == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = mutex_impl(mutex);
    if (impl->state != MUTEX_STATE_STOPPED) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    impl->locked = false;
    impl->owner_token = 0U;
    impl->state = MUTEX_STATE_READY;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 停止未持有的互斥并取消等待者。
 * @param mutex 互斥对象。
 * @retval FOUNDATION_STATUS_OK 停止成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUSY 互斥仍被持有。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 READY 状态。
 */
foundation_status_t mutex_port_stop(mutex_port_t *mutex)
{
    mutex_impl_t *impl;
    if (mutex == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = mutex_impl(mutex);
    if (impl->state != MUTEX_STATE_READY) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    if (impl->locked) {
        return FOUNDATION_STATUS_BUSY;
    }
    impl->state = MUTEX_STATE_STOPPED;
    (void)impl->wait_port->cancel_all(impl->wait_port->context);
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 重置 READY 且未持有的互斥。
 * @param mutex 互斥对象。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUSY 互斥仍被持有。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 READY 状态。
 */
foundation_status_t mutex_port_reset(mutex_port_t *mutex)
{
    mutex_impl_t *impl;
    if (mutex == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = mutex_impl(mutex);
    if (impl->state != MUTEX_STATE_READY) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    if (impl->locked) {
        return FOUNDATION_STATUS_BUSY;
    }
    impl->owner_token = 0U;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 解除 STOPPED 互斥的绑定。
 * @param mutex 互斥对象。
 * @retval FOUNDATION_STATUS_OK 解除成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象状态不允许解除。
 */
foundation_status_t mutex_port_deinit(mutex_port_t *mutex)
{
    mutex_impl_t *impl;
    if (mutex == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = mutex_impl(mutex);
    if (impl->state != MUTEX_STATE_STOPPED) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    impl->state = MUTEX_STATE_DEINITIALIZED;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 立即获取非递归互斥。
 * @param mutex 互斥对象。
 * @param owner_token 调用方持有者 token。
 * @retval FOUNDATION_STATUS_OK 获取成功。
 * @retval FOUNDATION_STATUS_BUSY 已被其他持有者占用。
 * @retval FOUNDATION_STATUS_INVALID_STATE 重复获取或状态非法。
 */
foundation_status_t mutex_port_try_lock(mutex_port_t *mutex, uintptr_t owner_token)
{
    if (mutex == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (mutex_impl(mutex)->state != MUTEX_STATE_READY) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    return mutex_try_lock_internal(mutex_impl(mutex), owner_token);
}

/**
 * @brief 在有限 deadline 前等待并获取互斥。
 * @param mutex 互斥对象。
 * @param waiter 等待者对象。
 * @param owner_token 调用方持有者 token。
 * @param deadline 绝对 deadline。
 * @retval FOUNDATION_STATUS_OK 获取成功。
 * @retval FOUNDATION_STATUS_TIMEOUT 等待超时。
 * @retval FOUNDATION_STATUS_CANCELLED 等待被停止取消。
 */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
foundation_status_t mutex_port_lock_until(mutex_port_t *mutex, waiter_t *waiter,
    uintptr_t owner_token, uint32_t deadline)
{
    mutex_impl_t *impl;
    foundation_status_t status;
    if ((mutex == NULL) || (waiter == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    status = waiter_begin(waiter, mutex);
    if (status != FOUNDATION_STATUS_OK)
        return status;
    impl = mutex_impl(mutex);
    if (impl->state != MUTEX_STATE_READY) {
        status = (impl->state == MUTEX_STATE_STOPPED) ? FOUNDATION_STATUS_CANCELLED
                                                      : FOUNDATION_STATUS_INVALID_STATE;
        (void)waiter_complete(waiter, status);
        return status;
    }
    do {
        status = mutex_try_lock_internal(impl, owner_token);
        if (status != FOUNDATION_STATUS_BUSY) {
            (void)waiter_complete(waiter, status);
            return status;
        }
        status = impl->wait_port->wait(impl->wait_port->context, waiter, deadline);
    } while (status == FOUNDATION_STATUS_OK);
    (void)waiter_complete(waiter, status);
    return status;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

/**
 * @brief 由指定持有者释放互斥。
 * @param mutex 互斥对象。
 * @param owner_token 持有者 token。
 * @retval FOUNDATION_STATUS_OK 释放成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或 token 不匹配。
 * @retval FOUNDATION_STATUS_INVALID_STATE 互斥未持有或状态非法。
 */
foundation_status_t mutex_port_unlock(mutex_port_t *mutex, uintptr_t owner_token)
{
    mutex_impl_t *impl;
    critical_section_token_t token;
    foundation_status_t status;
    if (mutex == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = mutex_impl(mutex);
    if (impl->state != MUTEX_STATE_READY) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    status = critical_section_enter(impl->critical, &token);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!impl->locked || (impl->owner_token != owner_token)) {
        status = FOUNDATION_STATUS_INVALID_ARGUMENT;
    } else {
        impl->locked = false;
        impl->owner_token = 0U;
        status = FOUNDATION_STATUS_OK;
    }
    (void)critical_section_exit(impl->critical, token);
    if (status == FOUNDATION_STATUS_OK) {
        (void)impl->wait_port->signal_one(impl->wait_port->context);
    }
    return status;
}
