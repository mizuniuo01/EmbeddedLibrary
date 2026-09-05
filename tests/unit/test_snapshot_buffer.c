/**
 * @file test_snapshot_buffer.c
 * @brief 验证有界容器正常路径、边界和失败原子性。
 */
#include "test_support.h"
#include <string.h>
#include <stdint.h>
#include "snapshot_buffer.h"

/**
 * @brief 执行容器确定性测试。
 * @return 全部断言成立时返回 0，否则返回 1。
 */
int main(void)
{
    snapshot_buffer_t buffer = {0};
    snapshot_buffer_writer_t writer = {0};
    snapshot_buffer_writer_t fake_writer;
    snapshot_buffer_lease_t old = {0};
    snapshot_buffer_lease_t current = {0};
    snapshot_buffer_lease_t fake;
    snapshot_buffer_info_t info = {.size = 99U};
    uint8_t first[3] = {0};
    uint8_t second[3] = {0};
    uint8_t output[3] = {99U, 99U, 99U};
    uint32_t value = 99U;
    TEST_ASSERT_STATUS(snapshot_buffer_sequence(&buffer, &value),
        FOUNDATION_STATUS_NOT_INITIALIZED);
    TEST_ASSERT_STATUS(snapshot_buffer_init(&buffer, first, first, 3U),
        FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(snapshot_buffer_init(&buffer, first, second, 3U), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(snapshot_buffer_acquire(&buffer, &old), FOUNDATION_STATUS_EMPTY);
    TEST_ASSERT_STATUS(snapshot_buffer_begin(&buffer, &writer), FOUNDATION_STATUS_OK);
    writer.data[0] = 42U;
    TEST_ASSERT_STATUS(snapshot_buffer_init(&buffer, first, second, 3U), FOUNDATION_STATUS_BUSY);
    TEST_ASSERT_STATUS(snapshot_buffer_begin(&buffer, &fake_writer), FOUNDATION_STATUS_BUSY);
    fake_writer = writer;
    TEST_ASSERT_STATUS(snapshot_buffer_publish(&buffer, &fake_writer, 1U, 10U),
        FOUNDATION_STATUS_INVALID_STATE);
    TEST_ASSERT_STATUS(snapshot_buffer_publish(&buffer, &writer, 4U, 10U),
        FOUNDATION_STATUS_BUFFER_TOO_SMALL);
    TEST_ASSERT_STATUS(snapshot_buffer_publish(&buffer, &writer, 1U, 10U), FOUNDATION_STATUS_OK);
    TEST_ASSERT(writer.data == NULL);
    TEST_ASSERT_STATUS(snapshot_buffer_acquire(&buffer, &old), FOUNDATION_STATUS_OK);
    TEST_ASSERT(old.size == 1U && old.sequence == 1U && old.timestamp == 10U && old.data[0] == 42U);
    TEST_ASSERT_STATUS(snapshot_buffer_acquire(&buffer, &current), FOUNDATION_STATUS_BUSY);
    fake = old;
    TEST_ASSERT_STATUS(snapshot_buffer_release(&buffer, &fake), FOUNDATION_STATUS_INVALID_STATE);
    TEST_ASSERT_STATUS(snapshot_buffer_begin(&buffer, &writer), FOUNDATION_STATUS_OK);
    writer.data[0] = 43U;
    TEST_ASSERT_STATUS(snapshot_buffer_publish(&buffer, &writer, 1U, 20U), FOUNDATION_STATUS_OK);
    TEST_ASSERT(old.data[0] == 42U && old.sequence == 1U);
    TEST_ASSERT_STATUS(snapshot_buffer_acquire(&buffer, &current), FOUNDATION_STATUS_OK);
    TEST_ASSERT(current.data[0] == 43U && current.sequence == 2U);
    TEST_ASSERT_STATUS(snapshot_buffer_begin(&buffer, &writer), FOUNDATION_STATUS_BUSY);
    TEST_ASSERT_STATUS(snapshot_buffer_copy(&buffer, output, 0U, &info),
        FOUNDATION_STATUS_BUFFER_TOO_SMALL);
    TEST_ASSERT(output[0] == 99U && info.size == 99U);
    TEST_ASSERT_STATUS(snapshot_buffer_copy(&buffer, output, 3U, &info), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output[0] == 43U && info.sequence == 2U && info.timestamp == 20U);
    TEST_ASSERT_STATUS(snapshot_buffer_copy(&buffer, first, 3U, &info),
        FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(snapshot_buffer_release(&buffer, &old), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(snapshot_buffer_release(&buffer, &old), FOUNDATION_STATUS_INVALID_STATE);
    TEST_ASSERT_STATUS(snapshot_buffer_begin(&buffer, &writer), FOUNDATION_STATUS_OK);
    writer.data[0] = 44U;
    TEST_ASSERT_STATUS(snapshot_buffer_cancel(&buffer, &writer), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(snapshot_buffer_cancel(&buffer, &writer), FOUNDATION_STATUS_INVALID_STATE);
    TEST_ASSERT(current.data[0] == 43U);
    TEST_ASSERT_STATUS(snapshot_buffer_timestamp(&buffer, &value), FOUNDATION_STATUS_OK);
    TEST_ASSERT(value == 20U);
    TEST_ASSERT_STATUS(snapshot_buffer_release(&buffer, &current), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(snapshot_buffer_begin(&buffer, &writer), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(snapshot_buffer_publish(&buffer, &writer, 0U, UINT32_MAX),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(snapshot_buffer_copy(&buffer, NULL, 0U, &info), FOUNDATION_STATUS_OK);
    TEST_ASSERT(info.size == 0U && info.sequence == 3U);
    /* 白盒注入序列号极值以验证禁止回绕及失败事务保留。 */
    buffer.info[buffer.active].sequence = UINT32_MAX;
    TEST_ASSERT_STATUS(snapshot_buffer_begin(&buffer, &writer), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(snapshot_buffer_publish(&buffer, &writer, 0U, 0U),
        FOUNDATION_STATUS_OVERFLOW);
    TEST_ASSERT(writer.data != NULL);
    TEST_ASSERT_STATUS(snapshot_buffer_cancel(&buffer, &writer), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(snapshot_buffer_init(&buffer, first, second, 1U), FOUNDATION_STATUS_OK);

    return 0;
}
