/**
 * @file test_posix_queue_wait.c
 * @brief 使用 POSIX 条件变量验证队列等待和唤醒。
 */

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _POSIX_C_SOURCE 200809L /* 启用 clock_gettime 和 nanosleep 的 POSIX 声明。 */

#include <pthread.h>
#include <time.h>

#include "queue_port.h"
#include "test_support.h"

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t condition;
} wait_context_t;

/**
 * @brief 测试临界区进入。
 * @param context POSIX 等待上下文。
 * @param token 临界区 token 输出地址。
 * @retval FOUNDATION_STATUS_OK 进入成功。
 */
static foundation_status_t enter_critical(void *context, critical_section_token_t *token)
{
    wait_context_t *state = context;
    (void)pthread_mutex_lock(&state->lock);
    *token = 1U;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 测试临界区退出。
 * @param context POSIX 等待上下文。
 * @param token 临界区 token。
 * @retval FOUNDATION_STATUS_OK 退出成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT token 错误。
 */
static foundation_status_t exit_critical(void *context, critical_section_token_t token)
{
    wait_context_t *state = context;
    if (token != 1U) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    (void)pthread_mutex_unlock(&state->lock);
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 等待条件变量信号。
 * @param context POSIX 等待上下文。
 * @param waiter waiter 对象。
 * @param deadline 相对毫秒等待上限。
 * @retval FOUNDATION_STATUS_OK 收到信号。
 * @retval FOUNDATION_STATUS_TIMEOUT 等待超时。
 */
static foundation_status_t wait_condition(void *context, waiter_t *waiter, uint32_t deadline)
{
    wait_context_t *state = context;
    struct timespec timeout;
    (void)waiter;
    (void)clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += (time_t)(deadline / 1000U);
    timeout.tv_nsec += (long)((deadline % 1000U) * 1000000L);
    if (timeout.tv_nsec >= 1000000000L) {
        timeout.tv_sec++;
        timeout.tv_nsec -= 1000000000L;
    }
    (void)pthread_mutex_lock(&state->lock);
    if (pthread_cond_timedwait(&state->condition, &state->lock, &timeout) != 0) {
        (void)pthread_mutex_unlock(&state->lock);
        return FOUNDATION_STATUS_TIMEOUT;
    }
    (void)pthread_mutex_unlock(&state->lock);
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 唤醒一个队列等待者。
 * @param context POSIX 等待上下文。
 * @retval FOUNDATION_STATUS_OK 唤醒请求成功。
 */
static foundation_status_t signal_one(void *context)
{
    wait_context_t *state = context;
    (void)pthread_mutex_lock(&state->lock);
    (void)pthread_cond_signal(&state->condition);
    (void)pthread_mutex_unlock(&state->lock);
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 唤醒全部队列等待者。
 * @param context POSIX 等待上下文。
 * @retval FOUNDATION_STATUS_OK 唤醒请求成功。
 */
static foundation_status_t signal_all(void *context)
{
    wait_context_t *state = context;
    (void)pthread_mutex_lock(&state->lock);
    (void)pthread_cond_broadcast(&state->condition);
    (void)pthread_mutex_unlock(&state->lock);
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 取消全部队列等待者。
 * @param context POSIX 等待上下文。
 * @retval FOUNDATION_STATUS_OK 取消请求成功。
 */
static foundation_status_t cancel_all(void *context)
{
    return signal_all(context);
}

typedef struct {
    queue_port_t *queue;
    waiter_t *waiter;
    foundation_status_t result;
    uint32_t value;
} consumer_argument_t;

/**
 * @brief 在线程中等待并消费队列元素。
 * @param argument 消费者参数。
 * @return 始终返回 NULL。
 */
static void *consume(void *argument)
{
    consumer_argument_t *consumer = argument;
    consumer->result =
        queue_port_pop_until(consumer->queue, consumer->waiter, &consumer->value, 1000U);
    return NULL;
}

int main(void)
{
    wait_context_t context;
    const critical_section_port_t critical = {enter_critical, exit_critical, &context};
    const waiter_port_t wait = {wait_condition, signal_one, signal_all, cancel_all, &context};
    queue_port_t queue = {0};
    waiter_t waiter = {0};
    unsigned char storage[sizeof(uint32_t)] = {0};
    consumer_argument_t consumer = {&queue, &waiter, FOUNDATION_STATUS_INTERNAL_ERROR, 0U};
    pthread_t thread;
    uint32_t value = 42U;

    TEST_ASSERT(pthread_mutex_init(&context.lock, NULL) == 0);
    TEST_ASSERT(pthread_cond_init(&context.condition, NULL) == 0);
    TEST_ASSERT(waiter_init(&waiter) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(queue_port_init(&queue, storage, sizeof(storage), sizeof(value), 1U, &critical,
                    &wait) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(pthread_create(&thread, NULL, consume, &consumer) == 0);
    struct timespec pause_time = {0, 1000000L};
    (void)nanosleep(&pause_time, NULL);
    TEST_ASSERT(queue_port_try_push(&queue, &value) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(pthread_join(thread, NULL) == 0);
    TEST_ASSERT(consumer.result == FOUNDATION_STATUS_OK);
    TEST_ASSERT(consumer.value == value);
    TEST_ASSERT(waiter_reset(&waiter) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(queue_port_stop(&queue) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(queue_port_deinit(&queue) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(pthread_cond_destroy(&context.condition) == 0);
    TEST_ASSERT(pthread_mutex_destroy(&context.lock) == 0);
    return 0;
}
