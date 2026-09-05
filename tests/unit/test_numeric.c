/**
 * @file test_numeric.c
 * @brief numeric 的有限值和整数/浮点限幅测试。
 */

#include "test_support.h"

#include <math.h>
#include <stdint.h>

#include "numeric.h"

/**
 * @brief 执行 numeric 的有限值和闭区间限幅测试。
 * @return 所有断言通过时返回 0，否则返回 1。
 */
int main(void)
{
    float f_result = 12.0F;
    int32_t i_result = 12;
    uint32_t u_result = 12U;

    TEST_ASSERT(numeric_f32_is_finite(0.0F));
    TEST_ASSERT(!numeric_f32_is_finite(NAN));
    TEST_ASSERT(!numeric_f32_is_finite(INFINITY));

    TEST_ASSERT_STATUS(numeric_f32_clamp(-1.0F, 0.0F, 1.0F, &f_result), FOUNDATION_STATUS_OK);
    TEST_ASSERT(f_result == 0.0F);
    TEST_ASSERT_STATUS(numeric_f32_clamp(2.0F, 0.0F, 1.0F, &f_result), FOUNDATION_STATUS_OK);
    TEST_ASSERT(f_result == 1.0F);
    f_result = 12.0F;
    TEST_ASSERT_STATUS(numeric_f32_clamp(NAN, 0.0F, 1.0F, &f_result),
        FOUNDATION_STATUS_INVALID_DATA);
    TEST_ASSERT(f_result == 12.0F);
    TEST_ASSERT_STATUS(numeric_f32_clamp(0.0F, 2.0F, 1.0F, &f_result),
        FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(numeric_f32_clamp(0.0F, 0.0F, 1.0F, NULL),
        FOUNDATION_STATUS_INVALID_ARGUMENT);

    TEST_ASSERT_STATUS(numeric_i32_clamp(INT32_MIN, -5, 5, &i_result), FOUNDATION_STATUS_OK);
    TEST_ASSERT(i_result == -5);
    TEST_ASSERT_STATUS(numeric_i32_clamp(INT32_MAX, -5, 5, &i_result), FOUNDATION_STATUS_OK);
    TEST_ASSERT(i_result == 5);
    TEST_ASSERT_STATUS(numeric_i32_clamp(0, 2, 1, &i_result), FOUNDATION_STATUS_INVALID_ARGUMENT);

    TEST_ASSERT_STATUS(numeric_u32_clamp(UINT32_MAX, 2U, 5U, &u_result), FOUNDATION_STATUS_OK);
    TEST_ASSERT(u_result == 5U);
    TEST_ASSERT_STATUS(numeric_u32_clamp(0U, 2U, 1U, &u_result),
        FOUNDATION_STATUS_INVALID_ARGUMENT);

    return 0;
}
