/**
 * @file test_posix_mutex.c
 * @brief 使用 POSIX 互斥验证并发组件的跨线程所有权路径。
 */

#include <pthread.h>

#include "mutex_port.h"
#include "test_support.h"

typedef struct {
    pthread_mutex_t lock;
} posix_context_t;

static foundation_status_t posix_enter(void *context, critical_section_token_t *token)
{
    posix_context_t *state = context;
    (void)pthread_mutex_lock(&state->lock);
    *token = 1U;
    return FOUNDATION_STATUS_OK;
}

static foundation_status_t posix_exit(void *context, critical_section_token_t token)
{
    posix_context_t *state = context;
    if (token != 1U)
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    (void)pthread_mutex_unlock(&state->lock);
    return FOUNDATION_STATUS_OK;
}

static foundation_status_t posix_wait(void *context, waiter_t *waiter, uint32_t deadline)
{
    (void)context;
    (void)waiter;
    (void)deadline;
    return FOUNDATION_STATUS_TIMEOUT;
}

static foundation_status_t posix_signal(void *context)
{
    (void)context;
    return FOUNDATION_STATUS_OK;
}

static foundation_status_t posix_cancel(void *context)
{
    (void)context;
    return FOUNDATION_STATUS_OK;
}

typedef struct {
    mutex_port_t *mutex;
    foundation_status_t result;
} thread_argument_t;

static void *try_lock_thread(void *argument)
{
    thread_argument_t *thread = argument;
    thread->result = mutex_port_try_lock(thread->mutex, 2U);
    return NULL;
}

int main(void)
{
    posix_context_t context;
    const critical_section_port_t critical = {posix_enter, posix_exit, &context};
    const waiter_port_t wait = {posix_wait, posix_signal, posix_signal, posix_cancel, NULL};
    const mutex_port_config_t config = {false, false};
    mutex_port_t mutex = {0};
    pthread_t thread;
    thread_argument_t argument = {&mutex, FOUNDATION_STATUS_INTERNAL_ERROR};

    TEST_ASSERT(pthread_mutex_init(&context.lock, NULL) == 0);
    TEST_ASSERT(mutex_port_init(&mutex, &critical, &wait, &config) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(mutex_port_try_lock(&mutex, 1U) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(pthread_create(&thread, NULL, try_lock_thread, &argument) == 0);
    TEST_ASSERT(pthread_join(thread, NULL) == 0);
    TEST_ASSERT(argument.result == FOUNDATION_STATUS_BUSY);
    TEST_ASSERT(mutex_port_unlock(&mutex, 1U) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(mutex_port_stop(&mutex) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(mutex_port_deinit(&mutex) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(pthread_mutex_destroy(&context.lock) == 0);
    return 0;
}
