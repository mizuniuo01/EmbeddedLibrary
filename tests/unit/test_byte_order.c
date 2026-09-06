/**
 * @file    test_byte_order.c
 * @brief   测试固定宽度整数的显式大小端编解码。
 */

#include "test_support.h"

#include "byte_order.h"

/**
 * @brief 验证整数大小端黄金字节和缓冲区边界。
 * @return 测试通过时返回 0，否则返回 1。
 */
int main(void)
{
    uint8_t bytes[8];
    uint32_t u32;
    int16_t i16;
    uint16_t u16;
    uint64_t u64;
    int32_t i32;
    int64_t i64;
    TEST_ASSERT_STATUS(byte_order_write_u32_le(bytes, sizeof(bytes), 0x12345678U),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(bytes[0] == 0x78U && bytes[3] == 0x12U);
    TEST_ASSERT_STATUS(byte_order_read_u32_le(bytes, sizeof(bytes), &u32), FOUNDATION_STATUS_OK);
    TEST_ASSERT(u32 == 0x12345678U);
    TEST_ASSERT_STATUS(byte_order_write_i16_be(bytes, sizeof(bytes), -2), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_order_read_i16_be(bytes, sizeof(bytes), &i16), FOUNDATION_STATUS_OK);
    TEST_ASSERT(i16 == -2);
    TEST_ASSERT_STATUS(byte_order_write_u16_be(bytes, sizeof(bytes), 0xA1B2U),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_order_read_u16_be(bytes, sizeof(bytes), &u16), FOUNDATION_STATUS_OK);
    TEST_ASSERT(u16 == 0xA1B2U);
    TEST_ASSERT_STATUS(byte_order_write_u64_le(bytes, sizeof(bytes), UINT64_C(0x0123456789ABCDEF)),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_order_read_u64_le(bytes, sizeof(bytes), &u64), FOUNDATION_STATUS_OK);
    TEST_ASSERT(u64 == UINT64_C(0x0123456789ABCDEF));
    TEST_ASSERT_STATUS(byte_order_write_i32_le(bytes, sizeof(bytes), INT32_MIN),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_order_read_i32_le(bytes, sizeof(bytes), &i32), FOUNDATION_STATUS_OK);
    TEST_ASSERT(i32 == INT32_MIN);
    TEST_ASSERT_STATUS(byte_order_write_i64_be(bytes, sizeof(bytes), INT64_MAX),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_order_read_i64_be(bytes, sizeof(bytes), &i64), FOUNDATION_STATUS_OK);
    TEST_ASSERT(i64 == INT64_MAX);
    TEST_ASSERT_STATUS(byte_order_read_u32_le(bytes, 3U, &u32), FOUNDATION_STATUS_BUFFER_TOO_SMALL);
    return 0;
}
