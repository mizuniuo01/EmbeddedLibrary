/**
 * @file test_moving_average.c
 * @brief 验证滑动平均预热、覆盖和失败原子性。
 */
#include "test_support.h"
#include "moving_average.h"
/** @brief 执行滑动平均测试。
 * @return 测试通过时返回 0，否则返回 1。
 */
int main(void)
{
    moving_average_f32_t average = {0};
    float storage[3] = {0};
    float output = 99.0F;
    size_t count = 0U;
    TEST_ASSERT_STATUS(moving_average_f32_init(&average, storage, 3U), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(moving_average_f32_update(&average, 3.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 3.0F);
    TEST_ASSERT_STATUS(moving_average_f32_update(&average, 6.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 4.5F);
    TEST_ASSERT_STATUS(moving_average_f32_update(&average, 9.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 6.0F);
    TEST_ASSERT_STATUS(moving_average_f32_update(&average, 12.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 9.0F);
    TEST_ASSERT_STATUS(moving_average_f32_count(&average, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U);
    TEST_ASSERT_STATUS(moving_average_f32_reset(&average), FOUNDATION_STATUS_OK);
    return 0;
}
