/* 验证 mizu_fifo 的双游标、容量边界、完整读写和单字节包装。 */

#include "test_support.h"

#include <stdint.h>

#include "mizu_fifo.h"

static int test_initialization(void)
{
    mizu_fifo_t fifo;
    mizu_fifo_t uninitialized = {0};
    uint8_t storage[3] = {0U};

    TEST_ASSERT(mizu_fifo_write(&uninitialized, storage, 1U) == MIZU_STATUS_NOT_INITIALIZED);
    TEST_ASSERT(mizu_fifo_init(&fifo, NULL, sizeof(storage)) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_fifo_init(&fifo, storage, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_fifo_init(&fifo, storage, sizeof(storage)) == MIZU_STATUS_OK);
    TEST_ASSERT(fifo.storage == storage);
    TEST_ASSERT(fifo.storage_size == sizeof(storage));
    TEST_ASSERT(fifo.read_index == 0U);
    TEST_ASSERT(fifo.write_index == 0U);
    TEST_ASSERT(fifo.initialized);
    return 0;
}

static int test_full_and_empty(void)
{
    mizu_fifo_t fifo = {0};
    uint8_t storage[4] = {0U};
    uint8_t input[3] = {1U, 2U, 3U};
    uint8_t oversized[2] = {4U, 5U};
    uint8_t output[4] = {9U, 9U, 9U, 9U};
    uint8_t extra = 4U;
    size_t read_index;
    size_t write_index;

    TEST_ASSERT(mizu_fifo_init(&fifo, storage, sizeof(storage)) == MIZU_STATUS_OK);
    read_index = fifo.read_index;
    write_index = fifo.write_index;
    TEST_ASSERT(mizu_fifo_read(&fifo, output, 2U) == MIZU_STATUS_EMPTY);
    TEST_ASSERT(fifo.read_index == read_index);
    TEST_ASSERT(fifo.write_index == write_index);
    TEST_ASSERT(output[0] == 9U && output[1] == 9U);

    TEST_ASSERT(mizu_fifo_write(&fifo, input, sizeof(input)) == MIZU_STATUS_OK);
    read_index = fifo.read_index;
    write_index = fifo.write_index;
    TEST_ASSERT(mizu_fifo_write(&fifo, oversized, sizeof(oversized)) == MIZU_STATUS_FULL);
    TEST_ASSERT(fifo.read_index == read_index);
    TEST_ASSERT(fifo.write_index == write_index);
    TEST_ASSERT(mizu_fifo_push(&fifo, extra) == MIZU_STATUS_FULL);
    TEST_ASSERT(fifo.read_index == read_index);
    TEST_ASSERT(fifo.write_index == write_index);
    TEST_ASSERT(mizu_fifo_read(&fifo, output, sizeof(input)) == MIZU_STATUS_OK);
    TEST_ASSERT(output[0] == 1U);
    TEST_ASSERT(output[1] == 2U);
    TEST_ASSERT(output[2] == 3U);
    extra = 9U;
    TEST_ASSERT(mizu_fifo_pop(&fifo, &extra) == MIZU_STATUS_EMPTY);
    TEST_ASSERT(extra == 9U);
    return 0;
}

static int test_wraparound(void)
{
    mizu_fifo_t fifo = {0};
    uint8_t storage[5] = {0U};
    uint8_t input[3] = {1U, 2U, 3U};
    uint8_t output[3] = {0U};

    TEST_ASSERT(mizu_fifo_init(&fifo, storage, sizeof(storage)) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_fifo_write(&fifo, input, sizeof(input)) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_fifo_read(&fifo, output, 2U) == MIZU_STATUS_OK);
    TEST_ASSERT(output[0] == 1U && output[1] == 2U);
    input[0] = 4U;
    input[1] = 5U;
    TEST_ASSERT(mizu_fifo_write(&fifo, input, 2U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_fifo_read(&fifo, output, 3U) == MIZU_STATUS_OK);
    TEST_ASSERT(output[0] == 3U && output[1] == 4U && output[2] == 5U);
    return 0;
}

static int test_invalid_arguments(void)
{
    mizu_fifo_t fifo = {0};
    uint8_t storage[3] = {0U};

    TEST_ASSERT(mizu_fifo_init(&fifo, storage, sizeof(storage)) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_fifo_write(&fifo, NULL, 0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_fifo_read(&fifo, NULL, 0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_fifo_write(&fifo, NULL, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_fifo_read(&fifo, NULL, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    return 0;
}

int main(void)
{
    TEST_ASSERT(test_initialization() == 0);
    TEST_ASSERT(test_full_and_empty() == 0);
    TEST_ASSERT(test_wraparound() == 0);
    TEST_ASSERT(test_invalid_arguments() == 0);
    return 0;
}
