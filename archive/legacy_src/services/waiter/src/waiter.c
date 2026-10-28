/**
 * @file waiter.c
 * @brief 并发对象等待者的生命周期和绑定状态管理。
 */

#include "waiter.h"

typedef struct {
    waiter_state_t state;
    const void *owner;
    foundation_status_t result;
} waiter_impl_t;

_Static_assert(sizeof(waiter_impl_t) <= WAITER_STORAGE_SIZE, "waiter storage too small");

/**
 * @brief 获取 waiter 私有状态。
 * @param waiter waiter 对象。
 * @return 私有状态地址。
 */
static waiter_impl_t *waiter_impl(waiter_t *waiter)
{
    // NOLINTNEXTLINE(bugprone-casting-through-void)
    return (waiter_impl_t *)(void *)waiter->storage.bytes;
}

/**
 * @brief 获取 waiter 私有状态只读视图。
 * @param waiter waiter 对象。
 * @return 私有状态只读地址。
 */
static const waiter_impl_t *waiter_const_impl(const waiter_t *waiter)
{
    // NOLINTNEXTLINE(bugprone-casting-through-void)
    return (const waiter_impl_t *)(const void *)waiter->storage.bytes;
}

/**
 * @brief 初始化 waiter。
 * @param waiter waiter 对象。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUSY waiter 已初始化。
 */
foundation_status_t waiter_init(waiter_t *waiter)
{
    waiter_impl_t *impl;
    if (waiter == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = waiter_impl(waiter);
    if (impl->state != WAITER_STATE_UNINITIALIZED) {
        return FOUNDATION_STATUS_BUSY;
    }
    impl->state = WAITER_STATE_IDLE;
    impl->owner = NULL;
    impl->result = FOUNDATION_STATUS_OK;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 将 waiter 绑定到一个等待对象。
 * @param waiter waiter 对象。
 * @param owner 等待对象标识。
 * @retval FOUNDATION_STATUS_OK 绑定成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE waiter 不在 IDLE 状态。
 */
foundation_status_t waiter_begin(waiter_t *waiter, const void *owner)
{
    waiter_impl_t *impl;
    if ((waiter == NULL) || (owner == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = waiter_impl(waiter);
    if (impl->state != WAITER_STATE_IDLE) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    impl->owner = owner;
    impl->state = WAITER_STATE_WAITING;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 完成或取消一次等待。
 * @param waiter waiter 对象。
 * @param result 等待结果。
 * @retval FOUNDATION_STATUS_OK 完成成功。
 * @retval FOUNDATION_STATUS_CANCELLED 等待被取消。
 * @retval FOUNDATION_STATUS_INVALID_STATE waiter 不在 WAITING 状态。
 */
foundation_status_t waiter_complete(waiter_t *waiter, foundation_status_t result)
{
    waiter_impl_t *impl;
    if (waiter == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = waiter_impl(waiter);
    if (impl->state != WAITER_STATE_WAITING) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    impl->result = result;
    impl->state =
        (result == FOUNDATION_STATUS_CANCELLED) ? WAITER_STATE_CANCELLED : WAITER_STATE_COMPLETED;
    return result;
}

/**
 * @brief 将完成或取消的 waiter 恢复为 IDLE。
 * @param waiter waiter 对象。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE waiter 尚未完成或取消。
 */
foundation_status_t waiter_reset(waiter_t *waiter)
{
    waiter_impl_t *impl;
    if (waiter == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = waiter_impl(waiter);
    if ((impl->state != WAITER_STATE_COMPLETED) && (impl->state != WAITER_STATE_CANCELLED)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    impl->owner = NULL;
    impl->state = WAITER_STATE_IDLE;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询 waiter 状态。
 * @param waiter waiter 对象。
 * @param state 状态输出地址。
 * @retval FOUNDATION_STATUS_OK 查询成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 */
foundation_status_t waiter_state(const waiter_t *waiter, waiter_state_t *state)
{
    if ((waiter == NULL) || (state == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *state = waiter_const_impl(waiter)->state;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询 waiter 绑定的 owner。
 * @param waiter waiter 对象。
 * @param owner owner 输出地址。
 * @retval FOUNDATION_STATUS_OK 查询成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 */
foundation_status_t waiter_owner(const waiter_t *waiter, const void **owner)
{
    if ((waiter == NULL) || (owner == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *owner = waiter_const_impl(waiter)->owner;
    return FOUNDATION_STATUS_OK;
}
