/**
 * @file test_deadband.c
 * @brief 验证中心回零死区边界。
 */
#include "test_support.h"
#include "deadband.h"
/** @brief 执行死区测试。
 * @return 测试通过时返回 0，否则返回 1。
 */
int main(void)
{
    deadband_f32_t deadband = {0};
    float output = 99.0F;
    TEST_ASSERT_STATUS(deadband_f32_init(&deadband, 10.0F, 2.0F), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(deadband_f32_update(&deadband, 10.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 0.0F);
    TEST_ASSERT_STATUS(deadband_f32_update(&deadband, 15.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 3.0F);
    TEST_ASSERT_STATUS(deadband_f32_update(&deadband, 5.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == -3.0F);
    return 0;
}
