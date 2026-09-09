/**
 * @file test_concurrency_ports.c
 * @brief 验证队列、事件位组和计数通知的基础契约。
 */

#include "test_support.h"

#include "count_notification.h"
#include "event_group.h"
#include "queue_port.h"

typedef struct {
    uint32_t cancel_count;
} test_wait_context_t;

/**
 * @brief 测试临界区进入回调。
 * @param context 未使用的测试上下文。
 * @param token 临界区 token 输出地址。
 * @retval FOUNDATION_STATUS_OK 始终成功。
 */
static foundation_status_t test_enter(void *context, critical_section_token_t *token)
{
    (void)context;
    *token = 1U;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 测试临界区退出回调。
 * @param context 未使用的测试上下文。
 * @param token 待校验 token。
 * @retval FOUNDATION_STATUS_OK token 正确。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT token 错误。
 */
static foundation_status_t test_exit(void *context, critical_section_token_t token)
{
    (void)context;
    return token == 1U ? FOUNDATION_STATUS_OK : FOUNDATION_STATUS_INVALID_ARGUMENT;
}

/**
 * @brief 测试等待回调，立即返回超时。
 * @param context 未使用的测试上下文。
 * @param deadline 未使用的 deadline。
 * @retval FOUNDATION_STATUS_TIMEOUT 始终返回超时。
 */
static foundation_status_t test_wait(void *context, uint32_t deadline)
{
    (void)context;
    (void)deadline;
    return FOUNDATION_STATUS_TIMEOUT;
}

/**
 * @brief 测试状态变化通知回调。
 * @param context 未使用的测试上下文。
 * @retval FOUNDATION_STATUS_OK 始终成功。
 */
static foundation_status_t test_signal(void *context)
{
    (void)context;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 测试取消等待回调并累计调用次数。
 * @param context 测试等待状态。
 * @retval FOUNDATION_STATUS_OK 始终成功。
 */
static foundation_status_t test_cancel(void *context)
{
    test_wait_context_t *state = context;
    state->cancel_count++;
    return FOUNDATION_STATUS_OK;
}

int main(void)
{
    const critical_section_port_t critical = {test_enter, test_exit, NULL};
    test_wait_context_t wait_context = {0U};
    const queue_port_wait_strategy_t queue_wait = {
        test_wait, test_signal, test_cancel, &wait_context};
    const event_group_wait_strategy_t event_wait = {
        test_wait, test_signal, test_cancel, &wait_context};
    const count_notification_wait_strategy_t count_wait = {
        test_wait, test_signal, test_cancel, &wait_context};
    queue_port_t queue = {0};
    unsigned char queue_storage[sizeof(uint32_t) * 2U] = {0};
    event_group_t events = {0};
    count_notification_t notification = {0};
    uint32_t value = 7U;
    uint32_t output = 0U;
    uint32_t matched = 0U;
    bool should_yield = true;

    TEST_ASSERT(queue_port_init(&queue, queue_storage, sizeof(queue_storage), sizeof(value), 2U,
                    &critical, &queue_wait) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(queue_port_try_push(&queue, &value) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(queue_port_push_isr(&queue, &value, &should_yield) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(!should_yield);
    TEST_ASSERT(queue_port_try_push(&queue, &value) == FOUNDATION_STATUS_FULL);
    TEST_ASSERT(queue_port_try_pop(&queue, &output) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == value);
    TEST_ASSERT(queue_port_try_pop(&queue, &output) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(queue_port_pop_until(&queue, &output, 10U) == FOUNDATION_STATUS_TIMEOUT);
    TEST_ASSERT(queue_port_stop(&queue) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(wait_context.cancel_count == 1U);
    TEST_ASSERT(queue_port_start(&queue) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(queue_port_deinit(&queue) == FOUNDATION_STATUS_BUSY);
    TEST_ASSERT(queue_port_stop(&queue) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(queue_port_deinit(&queue) == FOUNDATION_STATUS_OK);

    TEST_ASSERT(event_group_init(&events, &critical, &event_wait) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(event_group_set(&events, 0x03U) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(event_group_wait_any(&events, 0x05U, true, 10U, &matched) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(matched == 0x01U);
    TEST_ASSERT(event_group_wait_all(&events, 0x03U, false, 10U, &matched) ==
                FOUNDATION_STATUS_TIMEOUT);
    TEST_ASSERT(event_group_stop(&events) == FOUNDATION_STATUS_OK);

    TEST_ASSERT(count_notification_init(&notification, 1U, &critical, &count_wait) ==
                FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_notification_give(&notification) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_notification_give_isr(&notification, &should_yield) ==
                FOUNDATION_STATUS_OVERFLOW);
    TEST_ASSERT(count_notification_take(&notification) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_notification_take_until(&notification, 10U) == FOUNDATION_STATUS_TIMEOUT);
    TEST_ASSERT(count_notification_stop(&notification) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_notification_start(&notification) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_notification_stop(&notification) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_notification_deinit(&notification) == FOUNDATION_STATUS_OK);
    return 0;
}
