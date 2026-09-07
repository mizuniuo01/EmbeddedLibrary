/**
 * @file test_first_order_filter.c
 * @brief 验证一阶滤波边界和失败原子性。
 */
#include "test_support.h"
#include "first_order_filter.h"
#include <math.h>
/** @brief 执行一阶滤波测试。
 * @return 测试通过时返回 0，否则返回 1。
 */
int main(void)
{
    first_order_filter_f32_t filter = {0};
    float output = 99.0F;
    TEST_ASSERT_STATUS(first_order_filter_f32_init(&filter, 0.5F, 0.0F), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(first_order_filter_f32_update(&filter, 2.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 1.0F);
    TEST_ASSERT_STATUS(first_order_filter_f32_update(&filter, NAN, &output),
        FOUNDATION_STATUS_INVALID_DATA);
    TEST_ASSERT(output == 1.0F);
    TEST_ASSERT_STATUS(first_order_filter_f32_reset(&filter, 4.0F), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(first_order_filter_f32_init(&filter, 1.0F, 3.0F), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(first_order_filter_f32_update(&filter, 9.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 9.0F);
    return 0;
}
