/**
 * @file queue_port.c
 * @brief 固定大小复制队列及其等待端口实现。
 */

#include "queue_port.h"

#include <string.h>

typedef enum {
    QUEUE_PORT_STATE_UNINITIALIZED, /* 对象尚未初始化。 */
    QUEUE_PORT_STATE_READY,         /* 对象允许正常操作。 */
    QUEUE_PORT_STATE_STOPPED,       /* 对象已停止并取消等待。 */
    QUEUE_PORT_STATE_DEINITIALIZED, /* 对象已释放绑定。 */
} queue_port_state_t;

typedef struct {
    uint8_t *storage;
    size_t element_size;
    size_t capacity;
    size_t head;
    size_t size;
    const critical_section_port_t *critical;
    const waiter_port_t *wait_port;
    queue_port_state_t state;
} queue_port_impl_t;

_Static_assert(sizeof(queue_port_impl_t) <= QUEUE_PORT_STORAGE_SIZE,
    "queue_port_storage_t is too small");

/**
 * @brief 获取队列私有实现对象。
 * @param queue 队列对象。
 * @return 队列私有实现地址。
 */
static queue_port_impl_t *queue_impl(queue_port_t *queue)
{
    // NOLINTNEXTLINE(bugprone-casting-through-void)
    return (queue_port_impl_t *)(void *)queue->storage.bytes;
}

/**
 * @brief 获取队列私有实现的只读视图。
 * @param queue 队列对象。
 * @param waiter 等待者对象。
 * @return 队列私有实现只读地址。
 */
static const queue_port_impl_t *queue_const_impl(const queue_port_t *queue)
{
    // NOLINTNEXTLINE(bugprone-casting-through-void)
    return (const queue_port_impl_t *)(const void *)queue->storage.bytes;
}

/**
 * @brief 通知等待策略队列状态已经变化。
 * @param impl 队列私有实现对象。
 */
static void queue_signal(const queue_port_impl_t *impl)
{
    if (impl->wait_port->signal_one != NULL) {
        (void)impl->wait_port->signal_one(impl->wait_port->context);
    }
}

/**
 * @brief 进入队列状态保护临界区。
 * @param impl 队列私有实现对象。
 * @param token 用于接收恢复 token 的地址。
 * @retval FOUNDATION_STATUS_OK 进入成功。
 * @retval 其他状态码 临界区端口拒绝进入。
 */
static foundation_status_t queue_enter(const queue_port_impl_t *impl,
    critical_section_token_t *token)
{
    return critical_section_enter(impl->critical, token);
}

/**
 * @brief 退出队列状态保护临界区。
 * @param impl 队列私有实现对象。
 * @param token 进入操作返回的 token。
 * @retval FOUNDATION_STATUS_OK 退出成功。
 * @retval 其他状态码 临界区端口拒绝退出。
 */
static foundation_status_t queue_leave(const queue_port_impl_t *impl,
    critical_section_token_t token)
{
    return critical_section_exit(impl->critical, token);
}

/**
 * @brief 计算队列槽位地址。
 * @param impl 队列私有实现对象。
 * @param index 槽位索引。
 * @return 槽位首地址。
 */
static uint8_t *queue_slot(const queue_port_impl_t *impl, size_t index)
{
    return impl->storage + (index * impl->element_size);
}

/**
 * @brief 在已持有临界区时复制一个元素入队。
 * @param impl 队列私有实现对象。
 * @param data 待复制元素地址。
 * @retval FOUNDATION_STATUS_OK 入队成功。
 * @retval FOUNDATION_STATUS_FULL 队列已满。
 */
static foundation_status_t queue_push_locked(queue_port_impl_t *impl, const void *data)
{
    size_t index;
    if (impl->size == impl->capacity) {
        return FOUNDATION_STATUS_FULL;
    }
    index = (impl->head + impl->size) % impl->capacity;
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    (void)memcpy(queue_slot(impl, index), data, impl->element_size);
    impl->size++;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 在已持有临界区时复制一个元素出队。
 * @param impl 队列私有实现对象。
 * @param data 元素输出地址。
 * @retval FOUNDATION_STATUS_OK 出队成功。
 * @retval FOUNDATION_STATUS_EMPTY 队列为空。
 */
static foundation_status_t queue_pop_locked(queue_port_impl_t *impl, void *data)
{
    if (impl->size == 0U) {
        return FOUNDATION_STATUS_EMPTY;
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    (void)memcpy(data, queue_slot(impl, impl->head), impl->element_size);
    impl->head = (impl->head + 1U) % impl->capacity;
    impl->size--;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 执行普通或 ISR 立即入队。
 * @param queue 队列对象。
 * @param data 待复制元素地址。
 * @param is_isr 是否使用 ISR 语义。
 * @param should_yield ISR 路径的调度请求输出地址。
 * @retval FOUNDATION_STATUS_OK 入队成功。
 * @retval FOUNDATION_STATUS_FULL 队列已满。
 */
static foundation_status_t queue_try_push_internal(queue_port_t *queue, const void *data,
    bool is_isr, bool *should_yield)
{
    queue_port_impl_t *impl;
    critical_section_token_t token;
    foundation_status_t status;
    if ((queue == NULL) || (data == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (is_isr && (should_yield == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = queue_impl(queue);
    if (impl->state != QUEUE_PORT_STATE_READY) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    status = queue_enter(impl, &token);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    status = queue_push_locked(impl, data);
    if (is_isr) {
        *should_yield = false;
    }
    (void)queue_leave(impl, token);
    if (status == FOUNDATION_STATUS_OK) {
        queue_signal(impl);
    }
    return status;
}

/**
 * @brief 执行普通或 ISR 立即出队。
 * @param queue 队列对象。
 * @param data 元素输出地址。
 * @param is_isr 是否使用 ISR 语义。
 * @param should_yield ISR 路径的调度请求输出地址。
 * @retval FOUNDATION_STATUS_OK 出队成功。
 * @retval FOUNDATION_STATUS_EMPTY 队列为空。
 */
static foundation_status_t queue_try_pop_internal(queue_port_t *queue, void *data, bool is_isr,
    bool *should_yield)
{
    queue_port_impl_t *impl;
    critical_section_token_t token;
    foundation_status_t status;
    if ((queue == NULL) || (data == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (is_isr && (should_yield == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = queue_impl(queue);
    if (impl->state != QUEUE_PORT_STATE_READY) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    status = queue_enter(impl, &token);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    status = queue_pop_locked(impl, data);
    if (is_isr) {
        *should_yield = false;
    }
    (void)queue_leave(impl, token);
    if (status == FOUNDATION_STATUS_OK) {
        queue_signal(impl);
    }
    return status;
}

/**
 * @brief 初始化固定大小复制队列。
 * @param queue 队列对象。
 * @param storage 元素存储区。
 * @param storage_size 存储区字节数。
 * @param element_size 单个元素字节数。
 * @param capacity 元素容量。
 * @param critical 临界区端口。
 * @param wait_port 统一等待端口。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数非法。
 * @retval FOUNDATION_STATUS_OVERFLOW 容量乘法溢出。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 存储区不足。
 * @retval FOUNDATION_STATUS_BUSY 对象仍处于活动状态。
 */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
foundation_status_t queue_port_init(queue_port_t *queue, void *storage, size_t storage_size,
    size_t element_size, size_t capacity, const critical_section_port_t *critical,
    const waiter_port_t *wait_port)
{
    queue_port_impl_t *impl;
    size_t required;
    if ((queue == NULL) || (storage == NULL) || (critical == NULL) || (critical->enter == NULL) ||
        (critical->exit == NULL) || (wait_port == NULL) || (wait_port->wait == NULL) ||
        (wait_port->signal_one == NULL) || (wait_port->cancel_all == NULL) ||
        (element_size == 0U) || (capacity == 0U)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (capacity > (SIZE_MAX / element_size)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    required = element_size * capacity;
    if (storage_size < required) {
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    }
    impl = queue_impl(queue);
    if ((impl->state == QUEUE_PORT_STATE_READY) || (impl->state == QUEUE_PORT_STATE_STOPPED)) {
        return FOUNDATION_STATUS_BUSY;
    }
    impl->storage = storage;
    impl->element_size = element_size;
    impl->capacity = capacity;
    impl->head = 0U;
    impl->size = 0U;
    impl->critical = critical;
    impl->wait_port = wait_port;
    impl->state = QUEUE_PORT_STATE_READY;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 重新启动已停止的队列并清空运行状态。
 * @param queue 队列对象。
 * @retval FOUNDATION_STATUS_OK 启动成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 STOPPED 状态。
 */
foundation_status_t queue_port_start(queue_port_t *queue)
{
    queue_port_impl_t *impl;
    if (queue == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = queue_impl(queue);
    if (impl->state != QUEUE_PORT_STATE_STOPPED) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    impl->head = 0U;
    impl->size = 0U;
    impl->state = QUEUE_PORT_STATE_READY;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 停止队列、清空元素并取消等待者。
 * @param queue 队列对象。
 * @retval FOUNDATION_STATUS_OK 停止成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 READY 状态。
 */
foundation_status_t queue_port_stop(queue_port_t *queue)
{
    queue_port_impl_t *impl;
    if (queue == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = queue_impl(queue);
    if (impl->state != QUEUE_PORT_STATE_READY) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    impl->state = QUEUE_PORT_STATE_STOPPED;
    impl->head = 0U;
    impl->size = 0U;
    (void)impl->wait_port->cancel_all(impl->wait_port->context);
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 清空 READY 队列的运行状态。
 * @param queue 队列对象。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 READY 状态。
 */
foundation_status_t queue_port_reset(queue_port_t *queue)
{
    queue_port_impl_t *impl;
    if (queue == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = queue_impl(queue);
    if (impl->state != QUEUE_PORT_STATE_READY) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    impl->head = 0U;
    impl->size = 0U;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 解除 STOPPED 队列的运行绑定。
 * @param queue 队列对象。
 * @retval FOUNDATION_STATUS_OK 解除成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUSY 队列仍处于 READY 状态。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象状态不允许解除。
 */
foundation_status_t queue_port_deinit(queue_port_t *queue)
{
    queue_port_impl_t *impl;
    if (queue == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = queue_impl(queue);
    if (impl->state == QUEUE_PORT_STATE_READY) {
        return FOUNDATION_STATUS_BUSY;
    }
    if (impl->state != QUEUE_PORT_STATE_STOPPED) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    impl->state = QUEUE_PORT_STATE_DEINITIALIZED;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 立即复制一个元素入队。
 * @param queue 队列对象。
 * @param data 待复制元素地址。
 * @retval FOUNDATION_STATUS_OK 入队成功。
 * @retval FOUNDATION_STATUS_FULL 队列已满。
 */
foundation_status_t queue_port_try_push(queue_port_t *queue, const void *data)
{
    return queue_try_push_internal(queue, data, false, NULL);
}

/**
 * @brief 在有限 deadline 前等待并复制一个元素入队。
 * @param queue 队列对象。
 * @param waiter 等待者对象。
 * @param data 待复制元素地址。
 * @param deadline 绝对 deadline。
 * @retval FOUNDATION_STATUS_OK 入队成功。
 * @retval FOUNDATION_STATUS_TIMEOUT 等待超时。
 * @retval FOUNDATION_STATUS_CANCELLED 等待被停止取消。
 */
foundation_status_t queue_port_push_until(queue_port_t *queue, waiter_t *waiter, const void *data,
    uint32_t deadline)
{
    foundation_status_t status;
    if (waiter == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    status = waiter_begin(waiter, queue);
    if (status != FOUNDATION_STATUS_OK)
        return status;
    do {
        status = queue_port_try_push(queue, data);
        if ((status == FOUNDATION_STATUS_INVALID_STATE) &&
            (queue_impl(queue)->state == QUEUE_PORT_STATE_STOPPED)) {
            (void)waiter_complete(waiter, FOUNDATION_STATUS_CANCELLED);
            return FOUNDATION_STATUS_CANCELLED;
        }
        if (status != FOUNDATION_STATUS_FULL) {
            (void)waiter_complete(waiter, status);
            return status;
        }
        status = queue_impl(queue)->wait_port->wait(queue_impl(queue)->wait_port->context, waiter,
            deadline);
    } while (status == FOUNDATION_STATUS_OK);
    (void)waiter_complete(waiter, status);
    return status;
}

/**
 * @brief 在 ISR 上下文立即复制一个元素入队。
 * @param queue 队列对象。
 * @param data 待复制元素地址。
 * @param should_yield 用于接收调度请求的地址。
 * @retval FOUNDATION_STATUS_OK 入队成功。
 * @retval FOUNDATION_STATUS_FULL 队列已满。
 */
foundation_status_t queue_port_push_isr(queue_port_t *queue, const void *data, bool *should_yield)
{
    return queue_try_push_internal(queue, data, true, should_yield);
}

/**
 * @brief 立即复制一个元素出队。
 * @param queue 队列对象。
 * @param data 元素输出地址。
 * @retval FOUNDATION_STATUS_OK 出队成功。
 * @retval FOUNDATION_STATUS_EMPTY 队列为空。
 */
foundation_status_t queue_port_try_pop(queue_port_t *queue, void *data)
{
    return queue_try_pop_internal(queue, data, false, NULL);
}

/**
 * @brief 在有限 deadline 前等待并复制一个元素出队。
 * @param queue 队列对象。
 * @param waiter 等待者对象。
 * @param data 元素输出地址。
 * @param deadline 绝对 deadline。
 * @retval FOUNDATION_STATUS_OK 出队成功。
 * @retval FOUNDATION_STATUS_TIMEOUT 等待超时。
 * @retval FOUNDATION_STATUS_CANCELLED 等待被停止取消。
 */
foundation_status_t queue_port_pop_until(queue_port_t *queue, waiter_t *waiter, void *data,
    uint32_t deadline)
{
    foundation_status_t status;
    if (waiter == NULL)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    status = waiter_begin(waiter, queue);
    if (status != FOUNDATION_STATUS_OK)
        return status;
    do {
        status = queue_port_try_pop(queue, data);
        if ((status == FOUNDATION_STATUS_INVALID_STATE) &&
            (queue_impl(queue)->state == QUEUE_PORT_STATE_STOPPED)) {
            (void)waiter_complete(waiter, FOUNDATION_STATUS_CANCELLED);
            return FOUNDATION_STATUS_CANCELLED;
        }
        if (status != FOUNDATION_STATUS_EMPTY) {
            (void)waiter_complete(waiter, status);
            return status;
        }
        status = queue_impl(queue)->wait_port->wait(queue_impl(queue)->wait_port->context, waiter,
            deadline);
    } while (status == FOUNDATION_STATUS_OK);
    (void)waiter_complete(waiter, status);
    return status;
}

/**
 * @brief 在 ISR 上下文立即复制一个元素出队。
 * @param queue 队列对象。
 * @param data 元素输出地址。
 * @param should_yield 用于接收调度请求的地址。
 * @retval FOUNDATION_STATUS_OK 出队成功。
 * @retval FOUNDATION_STATUS_EMPTY 队列为空。
 */
foundation_status_t queue_port_pop_isr(queue_port_t *queue, void *data, bool *should_yield)
{
    return queue_try_pop_internal(queue, data, true, should_yield);
}

/**
 * @brief 查询 READY 队列中的元素数量。
 * @param queue 队列对象。
 * @param size 元素数量输出地址。
 * @retval FOUNDATION_STATUS_OK 查询成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象不是 READY 状态。
 */
foundation_status_t queue_port_size(const queue_port_t *queue, size_t *size)
{
    const queue_port_impl_t *impl;
    if ((queue == NULL) || (size == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = queue_const_impl(queue);
    if (impl->state != QUEUE_PORT_STATE_READY) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    *size = impl->size;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询队列容量。
 * @param queue 队列对象。
 * @param capacity 容量输出地址。
 * @retval FOUNDATION_STATUS_OK 查询成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_STATE 对象尚未初始化或已释放。
 */
foundation_status_t queue_port_capacity(const queue_port_t *queue, size_t *capacity)
{
    const queue_port_impl_t *impl;
    if ((queue == NULL) || (capacity == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    impl = queue_const_impl(queue);
    if ((impl->state != QUEUE_PORT_STATE_READY) && (impl->state != QUEUE_PORT_STATE_STOPPED)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    *capacity = impl->capacity;
    return FOUNDATION_STATUS_OK;
}
