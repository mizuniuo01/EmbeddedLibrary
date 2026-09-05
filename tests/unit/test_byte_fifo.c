/**
 * @file test_byte_fifo.c
 * @brief 验证有界容器正常路径、边界和失败原子性。
 */
#include "test_support.h"
#include <string.h>
#include <stdint.h>
#include "byte_fifo.h"

/**
 * @brief 执行容器确定性测试。
 * @return 全部断言成立时返回 0，否则返回 1。
 */
int main(void)
{
    byte_fifo_t fifo = {0};
    uint8_t storage[3] = {0};
    uint8_t data[] = {10U, 20U, 30U, 40U};
    uint8_t output[4] = {99U, 99U, 99U, 99U};
    size_t count = 99U;
    size_t index;
    TEST_ASSERT_STATUS(byte_fifo_size(&fifo, &count), FOUNDATION_STATUS_NOT_INITIALIZED);
    TEST_ASSERT(count == 99U);
    TEST_ASSERT_STATUS(byte_fifo_init(&fifo, storage, 0U), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(byte_fifo_init(&fifo, storage, sizeof(storage)), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_fifo_capacity(&fifo, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U);
    TEST_ASSERT_STATUS(byte_fifo_peek(&fifo, NULL, 0U), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_fifo_read(&fifo, NULL, 0U), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_fifo_write_some(&fifo, NULL, 0U, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 0U);
    count = 99U;
    TEST_ASSERT_STATUS(byte_fifo_read_some(&fifo, output, 1U, &count), FOUNDATION_STATUS_EMPTY);
    TEST_ASSERT(count == 99U && output[0] == 99U);
    TEST_ASSERT_STATUS(byte_fifo_write(&fifo, data, 4U), FOUNDATION_STATUS_FULL);
    TEST_ASSERT_STATUS(byte_fifo_size(&fifo, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 0U);
    TEST_ASSERT_STATUS(byte_fifo_write_some(&fifo, data, 4U, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U);
    TEST_ASSERT_STATUS(byte_fifo_init(&fifo, NULL, 3U), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(byte_fifo_read(&fifo, output, 4U), FOUNDATION_STATUS_EMPTY);
    TEST_ASSERT(output[0] == 99U);
    TEST_ASSERT_STATUS(byte_fifo_write(&fifo, storage, 1U), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(byte_fifo_read(&fifo, storage, 1U), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(byte_fifo_size(&fifo, &fifo.size), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(byte_fifo_peek(&fifo, output, 3U), FOUNDATION_STATUS_OK);
    TEST_ASSERT(memcmp(output, data, 3U) == 0);
    TEST_ASSERT_STATUS(byte_fifo_discard(&fifo, 4U), FOUNDATION_STATUS_EMPTY);
    TEST_ASSERT_STATUS(byte_fifo_discard(&fifo, 2U), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_fifo_write(&fifo, data, 2U), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_fifo_read(&fifo, output, 3U), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output[0] == 30U && output[1] == 10U && output[2] == 20U);
    for (index = 0U; index < 1000U; index++) {
        uint8_t value = (uint8_t)(index % 256U);
        TEST_ASSERT_STATUS(byte_fifo_push(&fifo, value), FOUNDATION_STATUS_OK);
        TEST_ASSERT_STATUS(byte_fifo_peek_byte(&fifo, output), FOUNDATION_STATUS_OK);
        TEST_ASSERT(output[0] == value);
        TEST_ASSERT_STATUS(byte_fifo_pop(&fifo, output), FOUNDATION_STATUS_OK);
        TEST_ASSERT(output[0] == value);
    }
    TEST_ASSERT_STATUS(byte_fifo_high_water_mark(&fifo, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U);
    TEST_ASSERT_STATUS(byte_fifo_rejected_count(&fifo, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 2U);
    /* 白盒注入仅用于验证饱和边界，不是调用方使用示例。 */
    fifo.rejected_count = SIZE_MAX;
    TEST_ASSERT_STATUS(byte_fifo_write(&fifo, data, 4U), FOUNDATION_STATUS_FULL);
    TEST_ASSERT(fifo.rejected_count == SIZE_MAX);
    TEST_ASSERT_STATUS(byte_fifo_init(&fifo, storage, 1U), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_fifo_push(&fifo, 7U), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(byte_fifo_push(&fifo, 8U), FOUNDATION_STATUS_FULL);
    TEST_ASSERT_STATUS(byte_fifo_pop(&fifo, output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output[0] == 7U);
    TEST_ASSERT_STATUS(byte_fifo_free(&fifo, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 1U);

    return 0;
}
