/**
 * @file    test_cobs.c
 * @brief   测试 COBS 编解码边界和原地解码。
 */

#include "test_support.h"

#include <string.h>

#include "cobs.h"

/**
 * @brief 验证 COBS 空数据、零字节、原地解码和重叠拒绝。
 * @return 测试通过时返回 0，否则返回 1。
 */
int main(void)
{
    uint8_t input[] = {1U, 0U, 2U, 3U};
    uint8_t encoded[8];
    uint8_t decoded[8];
    size_t encoded_size;
    size_t decoded_size;
    uint8_t long_input[255];
    uint8_t long_encoded[260];
    uint8_t long_decoded[255];
    size_t index;
    TEST_ASSERT_STATUS(cobs_encode(input, sizeof(input), encoded, sizeof(encoded), &encoded_size),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(cobs_decode(encoded, encoded_size, decoded, sizeof(decoded), &decoded_size),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(decoded_size == sizeof(input));
    TEST_ASSERT(memcmp(input, decoded, sizeof(input)) == 0);
    TEST_ASSERT_STATUS(cobs_decode(encoded, encoded_size, encoded, sizeof(encoded), &decoded_size),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(decoded_size == sizeof(input));
    TEST_ASSERT_STATUS(cobs_encode(input, sizeof(input), input, sizeof(input), &encoded_size),
        FOUNDATION_STATUS_INVALID_ARGUMENT);
    encoded[0] = 2U;
    encoded[1] = 1U;
    TEST_ASSERT_STATUS(cobs_decode(encoded, 2U, &encoded[1], sizeof(encoded) - 1U, &decoded_size),
        FOUNDATION_STATUS_INVALID_ARGUMENT);
    encoded[0] = 0U;
    decoded_size = 77U;
    decoded[0] = 0x5AU;
    TEST_ASSERT_STATUS(cobs_decode(encoded, 1U, decoded, sizeof(decoded), &decoded_size),
        FOUNDATION_STATUS_INVALID_DATA);
    TEST_ASSERT(decoded_size == 77U);
    TEST_ASSERT(decoded[0] == 0x5AU);
    for (index = 0U; index < sizeof(long_input); index++) {
        long_input[index] = 0xA5U;
    }
    TEST_ASSERT_STATUS(cobs_encode(long_input, 254U, long_encoded, sizeof(long_encoded),
                           &encoded_size),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(encoded_size == 256U);
    TEST_ASSERT_STATUS(cobs_decode(long_encoded, encoded_size, long_decoded, sizeof(long_decoded),
                           &decoded_size),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(decoded_size == 254U);
    TEST_ASSERT(memcmp(long_input, long_decoded, 254U) == 0);
    TEST_ASSERT_STATUS(cobs_encode(long_input, 255U, long_encoded, sizeof(long_encoded),
                           &encoded_size),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(encoded_size == 257U);
    return 0;
}
