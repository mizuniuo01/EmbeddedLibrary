/**
 * @file event_group.c
 * @brief 32 位事件位组实现。
 */
#include "event_group.h"
#include <stddef.h>
typedef enum {
    EVENT_GROUP_UNINITIALIZED,
    EVENT_GROUP_READY,
    EVENT_GROUP_STOPPED,
    EVENT_GROUP_DEINITIALIZED
} event_group_state_t;
typedef struct {
    uint32_t bits;
    const critical_section_port_t *critical;
    const waiter_port_t *wait_port;
    event_group_state_t state;
} event_group_impl_t;
_Static_assert(sizeof(event_group_impl_t) <= EVENT_GROUP_STORAGE_SIZE,
    "event_group storage too small");
/**
 * @brief 获取事件组私有状态。
 * @param group 事件组对象。
 * @return 私有状态地址。
 */
static event_group_impl_t *impl(event_group_t *group)
{
    // NOLINTNEXTLINE(bugprone-casting-through-void)
    return (event_group_impl_t *)(void *)group->storage.bytes;
}
/**
 * @brief 获取事件组只读私有状态。
 * @param group 事件组对象。
 * @return 只读状态地址。
 */
static const event_group_impl_t *cimpl(const event_group_t *group)
{
    // NOLINTNEXTLINE(bugprone-casting-through-void)
    return (const event_group_impl_t *)(const void *)group->storage.bytes;
}
/**
 * @brief 在临界区内设置或清除事件位。
 * @param group 事件组对象。
 * @param mask 事件位掩码。
 * @param set true 表示设置，false 表示清除。
 * @param isr true 表示 ISR 路径。
 * @param yield ISR 调度请求输出地址。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 */
static foundation_status_t mutate(event_group_t *group, uint32_t mask, bool set, bool isr,
    bool *yield)
{
    critical_section_token_t token;
    foundation_status_t status;
    event_group_impl_t *state;
    if ((group == NULL) || (mask == 0U) || (isr && (yield == NULL)))
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    state = impl(group);
    if (state->state != EVENT_GROUP_READY)
        return FOUNDATION_STATUS_INVALID_STATE;
    status = critical_section_enter(state->critical, &token);
    if (status != FOUNDATION_STATUS_OK)
        return status;
    if (set)
        state->bits |= mask;
    else
        state->bits &= ~mask;
    if (isr)
        *yield = false;
    (void)critical_section_exit(state->critical, token);
    (void)state->wait_port->signal_all(state->wait_port->context);
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 初始化事件位组。
 * @param group 事件组对象。
 * @param critical 临界区端口。
 * @param wait 等待策略端口。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数非法。
 */
foundation_status_t event_group_init(event_group_t *group, const critical_section_port_t *critical,
    const waiter_port_t *wait_port)
{
    event_group_impl_t *state;
    if ((group == NULL) || (critical == NULL) || (critical->enter == NULL) ||
        (critical->exit == NULL) || (wait_port == NULL) || (wait_port->wait == NULL) ||
        (wait_port->signal_all == NULL) || (wait_port->cancel_all == NULL))
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    state = impl(group);
    if ((state->state == EVENT_GROUP_READY) || (state->state == EVENT_GROUP_STOPPED))
        return FOUNDATION_STATUS_BUSY;
    state->bits = 0U;
    state->critical = critical;
    state->wait_port = wait_port;
    state->state = EVENT_GROUP_READY;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 启动已停止事件组并清空位。
 * @param group 事件组对象。
 * @retval FOUNDATION_STATUS_OK 启动成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 STOPPED 状态。
 */
foundation_status_t event_group_start(event_group_t *group)
{
    event_group_impl_t *s;
    if (group == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(group);
    if (s->state != EVENT_GROUP_STOPPED)
        return FOUNDATION_STATUS_INVALID_STATE;
    s->bits = 0U;
    s->state = EVENT_GROUP_READY;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 停止事件组并取消等待者。
 * @param group 事件组对象。
 * @retval FOUNDATION_STATUS_OK 停止成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 READY 状态。
 */
foundation_status_t event_group_stop(event_group_t *group)
{
    event_group_impl_t *s;
    if (group == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(group);
    if (s->state != EVENT_GROUP_READY)
        return FOUNDATION_STATUS_INVALID_STATE;
    s->bits = 0U;
    s->state = EVENT_GROUP_STOPPED;
    (void)s->wait_port->cancel_all(s->wait_port->context);
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 清空 READY 事件组的全部位。
 * @param group 事件组对象。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 READY 状态。
 */
foundation_status_t event_group_reset(event_group_t *group)
{
    event_group_impl_t *s;
    if (group == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(group);
    if (s->state != EVENT_GROUP_READY)
        return FOUNDATION_STATUS_INVALID_STATE;
    s->bits = 0U;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 解除 STOPPED 事件组的绑定。
 * @param group 事件组对象。
 * @retval FOUNDATION_STATUS_OK 解除成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUSY 对象仍处于 READY 状态。
 */
foundation_status_t event_group_deinit(event_group_t *group)
{
    event_group_impl_t *s;
    if (group == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(group);
    if (s->state == EVENT_GROUP_READY)
        return FOUNDATION_STATUS_BUSY;
    if (s->state != EVENT_GROUP_STOPPED)
        return FOUNDATION_STATUS_INVALID_STATE;
    s->state = EVENT_GROUP_DEINITIALIZED;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 在普通上下文设置事件位。
 * @param g 事件组对象。
 * @param m 事件位掩码。
 * @retval FOUNDATION_STATUS_OK 设置成功。
 */
foundation_status_t event_group_set(event_group_t *g, uint32_t m)
{
    return mutate(g, m, true, false, NULL);
}
/**
 * @brief 在 ISR 上下文设置事件位。
 * @param g 事件组对象。
 * @param m 事件位掩码。
 * @param y 调度请求输出地址。
 * @retval FOUNDATION_STATUS_OK 设置成功。
 */
foundation_status_t event_group_set_isr(event_group_t *g, uint32_t m, bool *y)
{
    return mutate(g, m, true, true, y);
}
/**
 * @brief 在普通上下文清除事件位。
 * @param g 事件组对象。
 * @param m 事件位掩码。
 * @retval FOUNDATION_STATUS_OK 清除成功。
 */
foundation_status_t event_group_clear(event_group_t *g, uint32_t m)
{
    return mutate(g, m, false, false, NULL);
}
/**
 * @brief 在 ISR 上下文清除事件位。
 * @param g 事件组对象。
 * @param m 事件位掩码。
 * @param y 调度请求输出地址。
 * @retval FOUNDATION_STATUS_OK 清除成功。
 */
foundation_status_t event_group_clear_isr(event_group_t *g, uint32_t m, bool *y)
{
    return mutate(g, m, false, true, y);
}
/**
 * @brief 等待事件位满足任意或全部条件。
 * @param group 事件组对象。
 * @param mask 目标位掩码。
 * @param all true 表示全部满足。
 * @param clear true 表示成功后清除目标位。
 * @param deadline 绝对 deadline。
 * @param matched 实际满足位输出地址。
 * @retval FOUNDATION_STATUS_OK 条件满足。
 */
static foundation_status_t wait_bits(event_group_t *group, waiter_t *waiter, uint32_t mask,
    bool all, bool clear, uint32_t deadline, uint32_t *matched)
{
    event_group_impl_t *s;
    critical_section_token_t token;
    foundation_status_t w;
    foundation_status_t status;
    if ((group == NULL) || (waiter == NULL) || (mask == 0U) || (matched == NULL))
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(group);
    status = waiter_begin(waiter, group);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (s->state != EVENT_GROUP_READY) {
        (void)waiter_complete(waiter, FOUNDATION_STATUS_INVALID_STATE);
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    for (;;) {
        uint32_t bits;
        if (s->state == EVENT_GROUP_STOPPED) {
            (void)waiter_complete(waiter, FOUNDATION_STATUS_CANCELLED);
            return FOUNDATION_STATUS_CANCELLED;
        }
        status = critical_section_enter(s->critical, &token);
        if (status != FOUNDATION_STATUS_OK) {
            (void)waiter_complete(waiter, status);
            return status;
        }
        bits = cimpl(group)->bits;
        if ((all && ((bits & mask) == mask)) || (!all && ((bits & mask) != 0U))) {
            *matched = bits & mask;
            if (clear)
                s->bits &= ~mask;
            (void)critical_section_exit(s->critical, token);
            (void)waiter_complete(waiter, FOUNDATION_STATUS_OK);
            return FOUNDATION_STATUS_OK;
        }
        (void)critical_section_exit(s->critical, token);
        w = s->wait_port->wait(s->wait_port->context, waiter, deadline);
        if (w != FOUNDATION_STATUS_OK) {
            (void)waiter_complete(waiter, w);
            return w;
        }
    }
}
/**
 * @brief 等待任意目标事件位。
 * @param g 事件组对象。
 * @param waiter 等待者对象。
 * @param m 目标掩码。
 * @param c 成功后是否清除。
 * @param d 绝对 deadline。
 * @param r 满足位输出地址。
 * @retval FOUNDATION_STATUS_OK 条件满足。
 */
foundation_status_t event_group_wait_any(event_group_t *g, waiter_t *waiter, uint32_t m, bool c,
    uint32_t d, uint32_t *r)
{
    return wait_bits(g, waiter, m, false, c, d, r);
}
/**
 * @brief 等待全部目标事件位。
 * @param g 事件组对象。
 * @param waiter 等待者对象。
 * @param m 目标掩码。
 * @param c 成功后是否清除。
 * @param d 绝对 deadline。
 * @param r 满足位输出地址。
 * @retval FOUNDATION_STATUS_OK 条件满足。
 */
foundation_status_t event_group_wait_all(event_group_t *g, waiter_t *waiter, uint32_t m, bool c,
    uint32_t d, uint32_t *r)
{
    return wait_bits(g, waiter, m, true, c, d, r);
}
