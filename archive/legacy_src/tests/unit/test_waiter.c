/**
 * @file test_waiter.c
 * @brief 验证 waiter 绑定、完成、取消和复用生命周期。
 */

#include "test_support.h"

#include "waiter.h"

int main(void)
{
    waiter_t waiter = {0};
    waiter_state_t state;
    const int owner = 0;
    const void *owner_view;

    TEST_ASSERT(waiter_init(&waiter) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(waiter_state(&waiter, &state) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(state == WAITER_STATE_IDLE);
    TEST_ASSERT(waiter_begin(&waiter, &owner) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(waiter_begin(&waiter, &owner) == FOUNDATION_STATUS_INVALID_STATE);
    TEST_ASSERT(waiter_owner(&waiter, &owner_view) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(owner_view == &owner);
    TEST_ASSERT(waiter_complete(&waiter, FOUNDATION_STATUS_CANCELLED) ==
                FOUNDATION_STATUS_CANCELLED);
    TEST_ASSERT(waiter_reset(&waiter) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(waiter_state(&waiter, &state) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(state == WAITER_STATE_IDLE);
    return 0;
}
