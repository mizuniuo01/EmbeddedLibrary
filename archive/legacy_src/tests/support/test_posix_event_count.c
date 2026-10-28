/**
 * @file test_posix_event_count.c
 * @brief 使用 POSIX 条件变量验证事件组和计数通知的等待唤醒。
 */

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _POSIX_C_SOURCE 200809L /* 启用 POSIX 条件变量和时钟声明。 */

#include <pthread.h>
#include <time.h>

#include "count_notification.h"
#include "event_group.h"
#include "test_support.h"

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t condition;
} posix_wait_t;

static foundation_status_t enter(void *context, critical_section_token_t *token)
{
    posix_wait_t *state = context;
    (void)pthread_mutex_lock(&state->lock);
    *token = 1U;
    return FOUNDATION_STATUS_OK;
}

static foundation_status_t leave(void *context, critical_section_token_t token)
{
    posix_wait_t *state = context;
    if (token != 1U)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    (void)pthread_mutex_unlock(&state->lock);
    return FOUNDATION_STATUS_OK;
}

static foundation_status_t wait_cond(void *context, waiter_t *waiter, uint32_t deadline)
{
    posix_wait_t *state = context;
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
    int result = pthread_cond_timedwait(&state->condition, &state->lock, &timeout);
    (void)pthread_mutex_unlock(&state->lock);
    return result == 0 ? FOUNDATION_STATUS_OK : FOUNDATION_STATUS_TIMEOUT;
}

static foundation_status_t signal_one(void *context)
{
    posix_wait_t *state = context;
    (void)pthread_mutex_lock(&state->lock);
    (void)pthread_cond_signal(&state->condition);
    (void)pthread_mutex_unlock(&state->lock);
    return FOUNDATION_STATUS_OK;
}

static foundation_status_t signal_all(void *context)
{
    posix_wait_t *state = context;
    (void)pthread_mutex_lock(&state->lock);
    (void)pthread_cond_broadcast(&state->condition);
    (void)pthread_mutex_unlock(&state->lock);
    return FOUNDATION_STATUS_OK;
}

static foundation_status_t cancel_all(void *context)
{
    return signal_all(context);
}

typedef struct {
    event_group_t *group;
    waiter_t *waiter;
    foundation_status_t result;
} event_arg_t;

static void *wait_event(void *argument)
{
    event_arg_t *arg = argument;
    uint32_t matched = 0U;
    arg->result = event_group_wait_any(arg->group, arg->waiter, 1U, true, 1000U, &matched);
    return NULL;
}

typedef struct {
    count_notification_t *notification;
    waiter_t *waiter;
    foundation_status_t result;
} count_arg_t;

static void *wait_count(void *argument)
{
    count_arg_t *arg = argument;
    arg->result = count_notification_take_until(arg->notification, arg->waiter, 1000U);
    return NULL;
}

int main(void)
{
    posix_wait_t context;
    const critical_section_port_t critical = {enter, leave, &context};
    const waiter_port_t wait = {wait_cond, signal_one, signal_all, cancel_all, &context};
    event_group_t group = {0};
    count_notification_t notification = {0};
    waiter_t event_waiter = {0};
    waiter_t count_waiter = {0};
    pthread_t event_thread;
    pthread_t count_thread;
    event_arg_t event_arg = {&group, &event_waiter, FOUNDATION_STATUS_INTERNAL_ERROR};
    count_arg_t count_arg = {&notification, &count_waiter, FOUNDATION_STATUS_INTERNAL_ERROR};

    TEST_ASSERT(pthread_mutex_init(&context.lock, NULL) == 0);
    TEST_ASSERT(pthread_cond_init(&context.condition, NULL) == 0);
    TEST_ASSERT(waiter_init(&event_waiter) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(waiter_init(&count_waiter) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(event_group_init(&group, &critical, &wait) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_notification_init(&notification, 2U, &critical, &wait) ==
                FOUNDATION_STATUS_OK);
    TEST_ASSERT(pthread_create(&event_thread, NULL, wait_event, &event_arg) == 0);
    TEST_ASSERT(pthread_create(&count_thread, NULL, wait_count, &count_arg) == 0);
    struct timespec pause_time = {0, 1000000L};
    (void)nanosleep(&pause_time, NULL);
    TEST_ASSERT(event_group_set(&group, 1U) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_notification_give(&notification) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(pthread_join(event_thread, NULL) == 0);
    TEST_ASSERT(pthread_join(count_thread, NULL) == 0);
    TEST_ASSERT(event_arg.result == FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_arg.result == FOUNDATION_STATUS_OK);
    TEST_ASSERT(waiter_reset(&event_waiter) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(waiter_reset(&count_waiter) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(event_group_stop(&group) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_notification_stop(&notification) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(event_group_deinit(&group) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(count_notification_deinit(&notification) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(pthread_cond_destroy(&context.condition) == 0);
    TEST_ASSERT(pthread_mutex_destroy(&context.lock) == 0);
    return 0;
}
