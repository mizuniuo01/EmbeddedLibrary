/* Libraries 层的通信字节暂存 FIFO。
 * FIFO 使用外部 storage、双游标和单生产者/单消费者约束，不提供同步和协议解析。
 */

#include "mizu_fifo.h"

static size_t mizu_fifo_advance(const mizu_fifo_t *fifo, size_t count, bool writing)
{
    const size_t index = writing ? fifo->write_index : fifo->read_index;
    const size_t storage_size = fifo->storage_size;
    const size_t tail = storage_size - index;

    return (count >= tail) ? (count - tail) : (index + count);
}

static mizu_status_t mizu_fifo_check(const mizu_fifo_t *fifo)
{
    if (fifo == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!fifo->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    if ((fifo->storage == NULL) || (fifo->storage_size < 2U) ||
        (fifo->read_index >= fifo->storage_size) || (fifo->write_index >= fifo->storage_size)) {
        return MIZU_STATUS_INVALID_STATE;
    }
    return MIZU_STATUS_OK;
}

static size_t mizu_fifo_used(const mizu_fifo_t *fifo)
{
    if (fifo->write_index >= fifo->read_index) {
        return fifo->write_index - fifo->read_index;
    }
    return fifo->storage_size - fifo->read_index + fifo->write_index;
}

static size_t mizu_fifo_free(const mizu_fifo_t *fifo)
{
    return (fifo->storage_size - 1U) - mizu_fifo_used(fifo);
}

mizu_status_t mizu_fifo_init(mizu_fifo_t *fifo, uint8_t *storage, size_t storage_size)
{
    if ((fifo == NULL) || (storage == NULL) || (storage_size < 2U)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    fifo->storage = storage;
    fifo->storage_size = storage_size;
    fifo->read_index = 0U;
    fifo->write_index = 0U;
    fifo->initialized = true;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_fifo_write(mizu_fifo_t *fifo, const uint8_t *data, size_t data_size)
{
    mizu_status_t status = mizu_fifo_check(fifo);
    size_t first;

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (data_size == 0U) {
        return MIZU_STATUS_OK;
    }
    if (data == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (data_size > mizu_fifo_free(fifo)) {
        return MIZU_STATUS_FULL;
    }

    first = fifo->storage_size - fifo->write_index;
    if (first > data_size) {
        first = data_size;
    }
    for (size_t index = 0U; index < first; ++index) {
        fifo->storage[fifo->write_index + index] = data[index];
    }
    for (size_t index = first; index < data_size; ++index) {
        fifo->storage[index - first] = data[index];
    }
    fifo->write_index = mizu_fifo_advance(fifo, data_size, true);
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_fifo_read(mizu_fifo_t *fifo, uint8_t *data, size_t data_size)
{
    mizu_status_t status = mizu_fifo_check(fifo);
    size_t first;

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (data_size == 0U) {
        return MIZU_STATUS_OK;
    }
    if (data == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (data_size > mizu_fifo_used(fifo)) {
        return MIZU_STATUS_EMPTY;
    }

    first = fifo->storage_size - fifo->read_index;
    if (first > data_size) {
        first = data_size;
    }
    for (size_t index = 0U; index < first; ++index) {
        data[index] = fifo->storage[fifo->read_index + index];
    }
    for (size_t index = first; index < data_size; ++index) {
        data[index] = fifo->storage[index - first];
    }
    fifo->read_index = mizu_fifo_advance(fifo, data_size, false);
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_fifo_push(mizu_fifo_t *fifo, uint8_t value)
{
    return mizu_fifo_write(fifo, &value, 1U);
}

mizu_status_t mizu_fifo_pop(mizu_fifo_t *fifo, uint8_t *value)
{
    return mizu_fifo_read(fifo, value, 1U);
}
