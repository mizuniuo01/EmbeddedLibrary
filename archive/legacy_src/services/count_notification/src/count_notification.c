/**
 * @file count_notification.c
 * @brief 有界计数通知实现。
 */
#include "count_notification.h"
typedef enum {
    COUNT_UNINITIALIZED, /* 对象尚未初始化。 */
    COUNT_READY,         /* 对象允许正常操作。 */
    COUNT_STOPPED,       /* 对象已停止并取消等待。 */
    COUNT_DEINITIALIZED, /* 对象已解除绑定。 */
} count_state_t;
typedef struct {
    uint32_t count;
    uint32_t maximum;
    const critical_section_port_t *critical;
    const waiter_port_t *wait_port;
    count_state_t state;
} count_impl_t;
_Static_assert(sizeof(count_impl_t) <= COUNT_NOTIFICATION_STORAGE_SIZE, "count storage too small");
/**
 * @brief 获取计数通知私有状态。
 * @param n 通知对象。
 * @return 私有状态地址。
 */
static count_impl_t *impl(count_notification_t *n)
{
    // NOLINTNEXTLINE(bugprone-casting-through-void)
    return (count_impl_t *)(void *)n->storage.bytes;
}
/**
 * @brief 执行普通或 ISR 计数增加。
 * @param n 通知对象。
 * @param isr 是否使用 ISR 语义。
 * @param y ISR 调度请求输出地址。
 * @retval FOUNDATION_STATUS_OK 计数增加成功。
 * @retval FOUNDATION_STATUS_OVERFLOW 已达到计数上限。
 */
static foundation_status_t give_internal(count_notification_t *n, bool isr, bool *y)
{
    count_impl_t *s;
    critical_section_token_t t;
    foundation_status_t r;
    if ((n == NULL) || (isr && (y == NULL)))
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(n);
    if (s->state != COUNT_READY)
        return FOUNDATION_STATUS_INVALID_STATE;
    r = critical_section_enter(s->critical, &t);
    if (r != FOUNDATION_STATUS_OK)
        return r;
    if (s->count == s->maximum)
        r = FOUNDATION_STATUS_OVERFLOW;
    else {
        s->count++;
        r = FOUNDATION_STATUS_OK;
    }
    if (isr)
        *y = false;
    (void)critical_section_exit(s->critical, t);
    if (r == FOUNDATION_STATUS_OK)
        (void)s->wait_port->signal_one(s->wait_port->context);
    return r;
}
/**
 * @brief 初始化有界计数通知。
 * @param n 通知对象。
 * @param maximum 最大计数。
 * @param c 临界区端口。
 * @param w 等待策略端口。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数非法。
 */
foundation_status_t count_notification_init(count_notification_t *n, uint32_t maximum,
    const critical_section_port_t *c, const waiter_port_t *wait_port)
{
    count_impl_t *s;
    if ((n == NULL) || (maximum == 0U) || (c == NULL) || (c->enter == NULL) || (c->exit == NULL) ||
        (wait_port == NULL) || (wait_port->wait == NULL) || (wait_port->signal_one == NULL) ||
        (wait_port->cancel_all == NULL))
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(n);
    if ((s->state == COUNT_READY) || (s->state == COUNT_STOPPED))
        return FOUNDATION_STATUS_BUSY;
    s->count = 0U;
    s->maximum = maximum;
    s->critical = c;
    s->wait_port = wait_port;
    s->state = COUNT_READY;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 启动已停止通知并清零计数。
 * @param n 通知对象。
 * @retval FOUNDATION_STATUS_OK 启动成功。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 STOPPED 状态。
 */
foundation_status_t count_notification_start(count_notification_t *n)
{
    count_impl_t *s;
    if (n == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(n);
    if (s->state != COUNT_STOPPED)
        return FOUNDATION_STATUS_INVALID_STATE;
    s->count = 0U;
    s->state = COUNT_READY;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 停止通知、清零计数并取消等待者。
 * @param n 通知对象。
 * @retval FOUNDATION_STATUS_OK 停止成功。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 READY 状态。
 */
foundation_status_t count_notification_stop(count_notification_t *n)
{
    count_impl_t *s;
    if (n == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(n);
    if (s->state != COUNT_READY)
        return FOUNDATION_STATUS_INVALID_STATE;
    s->count = 0U;
    s->state = COUNT_STOPPED;
    (void)s->wait_port->cancel_all(s->wait_port->context);
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 清零 READY 通知的运行计数。
 * @param n 通知对象。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 READY 状态。
 */
foundation_status_t count_notification_reset(count_notification_t *n)
{
    count_impl_t *s;
    if (n == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(n);
    if (s->state != COUNT_READY)
        return FOUNDATION_STATUS_INVALID_STATE;
    s->count = 0U;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 解除 STOPPED 通知的绑定。
 * @param n 通知对象。
 * @retval FOUNDATION_STATUS_OK 解除成功。
 * @retval FOUNDATION_STATUS_BUSY 对象仍处于 READY 状态。
 */
foundation_status_t count_notification_deinit(count_notification_t *n)
{
    count_impl_t *s;
    if (n == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(n);
    if (s->state == COUNT_READY)
        return FOUNDATION_STATUS_BUSY;
    if (s->state != COUNT_STOPPED)
        return FOUNDATION_STATUS_INVALID_STATE;
    s->state = COUNT_DEINITIALIZED;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 在普通上下文增加一个通知计数。
 * @param n 通知对象。
 * @retval FOUNDATION_STATUS_OK 增加成功。
 * @retval FOUNDATION_STATUS_OVERFLOW 已达到上限。
 */
foundation_status_t count_notification_give(count_notification_t *n)
{
    return give_internal(n, false, NULL);
}
/**
 * @brief 在 ISR 上下文增加一个通知计数。
 * @param n 通知对象。
 * @param y 调度请求输出地址。
 * @retval FOUNDATION_STATUS_OK 增加成功。
 * @retval FOUNDATION_STATUS_OVERFLOW 已达到上限。
 */
foundation_status_t count_notification_give_isr(count_notification_t *n, bool *y)
{
    return give_internal(n, true, y);
}
/**
 * @brief 立即消耗一个通知计数。
 * @param n 通知对象。
 * @retval FOUNDATION_STATUS_OK 消耗成功。
 * @retval FOUNDATION_STATUS_EMPTY 当前没有计数。
 */
foundation_status_t count_notification_take(count_notification_t *n)
{
    count_impl_t *s;
    critical_section_token_t t;
    foundation_status_t r;
    if (n == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    s = impl(n);
    if (s->state != COUNT_READY)
        return FOUNDATION_STATUS_INVALID_STATE;
    r = critical_section_enter(s->critical, &t);
    if (r != FOUNDATION_STATUS_OK)
        return r;
    if (s->count == 0U)
        r = FOUNDATION_STATUS_EMPTY;
    else {
        s->count--;
        r = FOUNDATION_STATUS_OK;
    }
    (void)critical_section_exit(s->critical, t);
    return r;
}
/**
 * @brief 在有限 deadline 前等待并消耗一个通知计数。
 * @param n 通知对象。
 * @param waiter 等待者对象。
 * @param d 绝对 deadline。
 * @retval FOUNDATION_STATUS_OK 消耗成功。
 * @retval FOUNDATION_STATUS_TIMEOUT 等待超时。
 * @retval FOUNDATION_STATUS_CANCELLED 等待被停止取消。
 */
foundation_status_t count_notification_take_until(count_notification_t *n, waiter_t *waiter,
    uint32_t d)
{
    foundation_status_t r;
    if ((n == NULL) || (waiter == NULL))
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    r = waiter_begin(waiter, n);
    if (r != FOUNDATION_STATUS_OK)
        return r;
    do {
        r = count_notification_take(n);
        if ((r == FOUNDATION_STATUS_INVALID_STATE) && (impl(n)->state == COUNT_STOPPED)) {
            (void)waiter_complete(waiter, FOUNDATION_STATUS_CANCELLED);
            return FOUNDATION_STATUS_CANCELLED;
        }
        if (r != FOUNDATION_STATUS_EMPTY) {
            (void)waiter_complete(waiter, r);
            return r;
        }
        r = impl(n)->wait_port->wait(impl(n)->wait_port->context, waiter, d);
    } while (r == FOUNDATION_STATUS_OK);
    (void)waiter_complete(waiter, r);
    return r;
}
