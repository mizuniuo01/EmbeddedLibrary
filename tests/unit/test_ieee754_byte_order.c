/**
 * @file    test_ieee754_byte_order.c
 * @brief   测试 IEEE-754 浮点位模式的大小端编解码。
 */

#include "test_support.h"

#include <math.h>

#include "ieee754_byte_order.h"

/**
 * @brief 验证浮点特殊值和位模式往返。
 * @return 测试通过时返回 0，否则返回 1。
 */
int main(void)
{
    uint8_t bytes[8];
    float value;
    double double_value;
    TEST_ASSERT_STATUS(ieee754_byte_order_write_f32_be(bytes, sizeof(bytes), -0.0F),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(bytes[0] == 0x80U);
    TEST_ASSERT_STATUS(ieee754_byte_order_read_f32_be(bytes, sizeof(bytes), &value),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(signbit(value));
    TEST_ASSERT_STATUS(ieee754_byte_order_write_f64_le(bytes, sizeof(bytes), INFINITY),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(ieee754_byte_order_read_f64_le(bytes, sizeof(bytes), &double_value),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(isinf(double_value) != 0);
    TEST_ASSERT_STATUS(ieee754_byte_order_read_f64_le(bytes, 7U, &double_value),
        FOUNDATION_STATUS_BUFFER_TOO_SMALL);
    return 0;
}
