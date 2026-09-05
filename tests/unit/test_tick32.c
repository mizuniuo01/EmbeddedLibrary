/**
 * @file test_tick32.c
 * @brief tick32 的普通、回绕和非法范围测试。
 */

#include "test_support.h"

#include <limits.h>
#include <stdint.h>

#include "tick32.h"

/**
 * @brief 执行 tick32 的正常、回绕和非法范围测试。
 * @return 所有断言通过时返回 0，否则返回 1。
 */
int main(void)
{
    bool result;
    uint32_t output;

    TEST_ASSERT(TICK32_MAX_INTERVAL_TICKS == (uint32_t)INT32_MAX);
    TEST_ASSERT(tick32_duration_is_valid(0U));
    TEST_ASSERT(tick32_duration_is_valid(TICK32_MAX_INTERVAL_TICKS));
    TEST_ASSERT(!tick32_duration_is_valid(UINT32_MAX));

    TEST_ASSERT_STATUS(tick32_elapsed(100U, 40U, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 60U);
    TEST_ASSERT_STATUS(tick32_elapsed(10U, UINT32_MAX - 4U, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 15U);
    output = 123U;
    TEST_ASSERT_STATUS(tick32_elapsed(1U, 0U, NULL), FOUNDATION_STATUS_INVALID_ARGUMENT);

    TEST_ASSERT_STATUS(tick32_has_elapsed(100U, 40U, 60U, &result), FOUNDATION_STATUS_OK);
    TEST_ASSERT(result);
    TEST_ASSERT_STATUS(tick32_has_elapsed(99U, 40U, 60U, &result), FOUNDATION_STATUS_OK);
    TEST_ASSERT(!result);
    TEST_ASSERT_STATUS(tick32_has_elapsed(0U, 0U, UINT32_MAX, &result),
        FOUNDATION_STATUS_OUT_OF_RANGE);
    TEST_ASSERT_STATUS(tick32_has_elapsed(0U, 0U, 0U, NULL), FOUNDATION_STATUS_INVALID_ARGUMENT);

    TEST_ASSERT_STATUS(tick32_deadline_add(UINT32_MAX - 4U, 10U, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 5U);
    TEST_ASSERT_STATUS(tick32_deadline_add(0U, UINT32_MAX, &output),
        FOUNDATION_STATUS_OUT_OF_RANGE);

    TEST_ASSERT_STATUS(tick32_deadline_reached(5U, 5U, &result), FOUNDATION_STATUS_OK);
    TEST_ASSERT(result);
    TEST_ASSERT_STATUS(tick32_deadline_reached(4U, 5U, &result), FOUNDATION_STATUS_OK);
    TEST_ASSERT(!result);
    TEST_ASSERT_STATUS(tick32_deadline_reached(5U, UINT32_MAX - 4U, &result), FOUNDATION_STATUS_OK);
    TEST_ASSERT(result);
    TEST_ASSERT_STATUS(tick32_deadline_reached(0U, (uint32_t)INT32_MIN, &result),
        FOUNDATION_STATUS_OUT_OF_RANGE);
    TEST_ASSERT_STATUS(tick32_deadline_reached(0U, 0U, NULL), FOUNDATION_STATUS_INVALID_ARGUMENT);

    return 0;
}
