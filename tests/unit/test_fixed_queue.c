/**
 * @file test_fixed_queue.c
 * @brief 验证有界容器正常路径、边界和失败原子性。
 */
#include "test_support.h"
#include <string.h>
#include <stdint.h>
#include "fixed_queue.h"

/**
 * @brief 执行容器确定性测试。
 * @return 全部断言成立时返回 0，否则返回 1。
 */
int main(void)
{
    fixed_queue_t queue = {0};
    uint8_t storage[13] = {0};
    uint32_t input[] = {10U, 20U, 30U, 40U};
    uint32_t output[4] = {99U, 99U, 99U, 99U};
    size_t count = 99U;
    size_t index;
    TEST_ASSERT_STATUS(fixed_queue_size(&queue, &count), FOUNDATION_STATUS_NOT_INITIALIZED);
    TEST_ASSERT_STATUS(fixed_queue_init(&queue, storage, sizeof(storage), 2U, SIZE_MAX),
        FOUNDATION_STATUS_OVERFLOW);
    TEST_ASSERT_STATUS(fixed_queue_init(&queue, storage, 3U, 4U, 1U),
        FOUNDATION_STATUS_BUFFER_TOO_SMALL);
    TEST_ASSERT_STATUS(fixed_queue_init(&queue, storage + 1U, 12U, sizeof(uint32_t), 3U),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(fixed_queue_peek(&queue, output), FOUNDATION_STATUS_EMPTY);
    TEST_ASSERT_STATUS(fixed_queue_push_some(&queue, NULL, 0U, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 0U);
    TEST_ASSERT_STATUS(fixed_queue_push_some(&queue, input, 4U, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U);
    TEST_ASSERT_STATUS(fixed_queue_push(&queue, input), FOUNDATION_STATUS_FULL);
    TEST_ASSERT_STATUS(fixed_queue_peek_at(&queue, 2U, output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output[0] == 30U);
    TEST_ASSERT_STATUS(fixed_queue_peek_at(&queue, 3U, output), FOUNDATION_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(output[0] == 30U);
    TEST_ASSERT_STATUS(fixed_queue_push(&queue, storage + 1U), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(fixed_queue_pop(&queue, storage + 1U), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(fixed_queue_pop_some(&queue, output, SIZE_MAX, &count),
        FOUNDATION_STATUS_OVERFLOW);
    TEST_ASSERT_STATUS(fixed_queue_init(&queue, storage, 0U, 4U, 3U),
        FOUNDATION_STATUS_BUFFER_TOO_SMALL);
    TEST_ASSERT_STATUS(fixed_queue_pop(&queue, output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output[0] == 10U);
    TEST_ASSERT_STATUS(fixed_queue_push(&queue, &input[3]), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(fixed_queue_pop_some(&queue, output, 4U, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U && output[0] == 20U && output[1] == 30U && output[2] == 40U);
    count = 99U;
    TEST_ASSERT_STATUS(fixed_queue_pop_some(&queue, output, 1U, &count), FOUNDATION_STATUS_EMPTY);
    TEST_ASSERT(count == 99U);
    for (index = 0U; index < 1000U; index++) {
        uint32_t value = (uint32_t)index;
        TEST_ASSERT_STATUS(fixed_queue_push(&queue, &value), FOUNDATION_STATUS_OK);
        TEST_ASSERT_STATUS(fixed_queue_pop(&queue, output), FOUNDATION_STATUS_OK);
        TEST_ASSERT(output[0] == value);
    }
    TEST_ASSERT_STATUS(fixed_queue_rejected_count(&queue, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 2U);
    TEST_ASSERT_STATUS(fixed_queue_high_water_mark(&queue, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U);
    queue.rejected_count = SIZE_MAX;
    TEST_ASSERT_STATUS(fixed_queue_push_some(&queue, input, 4U, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(queue.rejected_count == SIZE_MAX);
    TEST_ASSERT_STATUS(fixed_queue_capacity(&queue, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U);
    TEST_ASSERT_STATUS(fixed_queue_free(&queue, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 0U);
    TEST_ASSERT_STATUS(fixed_queue_size(&queue, &queue.size), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(fixed_queue_init(&queue, storage, sizeof(storage), 1U, 1U),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(fixed_queue_push(&queue, input), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(fixed_queue_push(&queue, input), FOUNDATION_STATUS_FULL);

    return 0;
}
