/**
 * @file test_rate_limiter.c
 * @brief 验证上升和下降速率限制。
 */
#include "test_support.h"
#include "rate_limiter.h"
/** @brief 执行速率限制测试。
 * @return 测试通过时返回 0，否则返回 1。
 */
int main(void)
{
    rate_limiter_f32_t limiter = {0};
    float output = 99.0F;
    TEST_ASSERT_STATUS(rate_limiter_f32_init(&limiter, 2.0F, 4.0F, 0.0F), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(rate_limiter_f32_update(&limiter, 10.0F, 1.0F, &output),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 2.0F);
    TEST_ASSERT_STATUS(rate_limiter_f32_update(&limiter, -10.0F, 0.5F, &output),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 0.0F);
    TEST_ASSERT_STATUS(rate_limiter_f32_update(&limiter, 0.0F, 0.0F, &output),
        FOUNDATION_STATUS_INVALID_DATA);
    TEST_ASSERT(output == 0.0F);
    return 0;
}
