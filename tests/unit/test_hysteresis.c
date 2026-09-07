/**
 * @file test_hysteresis.c
 * @brief 验证二值迟滞阈值和初态。
 */
#include "test_support.h"
#include "hysteresis.h"
/** @brief 执行迟滞测试。
 * @return 测试通过时返回 0，否则返回 1。
 */
int main(void)
{
    hysteresis_f32_t hysteresis = {0};
    float output = 0.0F;
    TEST_ASSERT_STATUS(hysteresis_f32_init(&hysteresis, -1.0F, 1.0F, 0.0F, 10.0F, false),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(hysteresis_f32_update(&hysteresis, 0.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 0.0F);
    TEST_ASSERT_STATUS(hysteresis_f32_update(&hysteresis, 1.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 10.0F);
    TEST_ASSERT_STATUS(hysteresis_f32_update(&hysteresis, 0.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 10.0F);
    TEST_ASSERT_STATUS(hysteresis_f32_update(&hysteresis, -1.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 0.0F);
    return 0;
}
