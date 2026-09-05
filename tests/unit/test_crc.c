/**
 * @file    test_crc.c
 * @brief   测试固定 CRC 算法的标准校验向量。
 */

#include "test_support.h"

#include "crc.h"

/**
 * @brief 验证三种 CRC 的标准字符串校验值、分块等价性和错误返回。
 * @return 测试通过时返回 0，否则返回 1。
 */
int main(void)
{
    static const uint8_t input[] = "123456789";
    crc8_smbus_context_t crc8_context;
    crc16_ccitt_false_context_t context;
    uint8_t crc8;
    uint16_t crc16;
    uint32_t crc32;
    TEST_ASSERT_STATUS(crc8_smbus_calculate(input, sizeof(input) - 1U, &crc8),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(crc8 == 0xF4U);
    TEST_ASSERT_STATUS(crc16_ccitt_false_calculate(input, sizeof(input) - 1U, &crc16),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(crc16 == 0x29B1U);
    TEST_ASSERT_STATUS(crc32_iso_hdlc_calculate(input, sizeof(input) - 1U, &crc32),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(crc32 == UINT32_C(0xCBF43926));
    TEST_ASSERT_STATUS(crc16_ccitt_false_init(&context), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(crc16_ccitt_false_update(&context, input, 3U), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(crc16_ccitt_false_update(&context, &input[3], sizeof(input) - 4U),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(crc16_ccitt_false_finalize(&context, &crc16), FOUNDATION_STATUS_OK);
    TEST_ASSERT(crc16 == 0x29B1U);
    crc8 = 0xA5U;
    TEST_ASSERT_STATUS(crc8_smbus_update(NULL, input, 1U), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(crc8_smbus_init(&crc8_context), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(crc8_smbus_update(&crc8_context, NULL, 1U),
        FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(crc8_smbus_finalize(&crc8_context, NULL),
        FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(crc8_smbus_calculate(NULL, 0U, &crc8), FOUNDATION_STATUS_OK);
    TEST_ASSERT(crc8 == 0U);
    crc16 = 0x5AA5U;
    TEST_ASSERT_STATUS(crc16_ccitt_false_finalize(NULL, &crc16),
        FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(crc16 == 0x5AA5U);
    return 0;
}
