/**
 * @file byte_fifo.c
 * @brief 外部同步的有界字节 FIFO，不覆盖未读数据。
 */
#include "byte_fifo.h"

#include <stdbool.h>
#include <string.h>

/**
 * @brief 按平坦地址 ABI 判断两个有效内存区间是否相交。
 * @param left 第一个区间起点。
 * @param left_size 第一个区间长度。
 * @param right 第二个区间起点。
 * @param right_size 第二个区间长度。
 * @return 非空区间重叠时返回 true。
 * @note 依赖 uintptr_t 保序平坦地址，使用差值避免末地址加法溢出。
 */
static bool overlaps(const void *left, size_t left_size, const void *right, size_t right_size)
{
    uintptr_t a = (uintptr_t)left;
    uintptr_t b = (uintptr_t)right;
    if ((left_size == 0U) || (right_size == 0U)) {
        return false;
    }
    return (a <= b) ? ((b - a) < left_size) : ((a - b) < right_size);
}

/**
 * @brief 计算环形索引，避免索引相加溢出。
 * @param index 当前索引。
 * @param count 推进量，不超过容量。
 * @param capacity 环形容量。
 * @return 推进后的索引。
 */
/* 私有环形运算按索引、步长、容量固定顺序调用。 */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static size_t advance(size_t index, size_t count, size_t capacity)
{
    size_t tail = capacity - index;
    return (count >= tail) ? (count - tail) : (index + count);
}
/**
 * @brief 校验零初始化实例及环形状态。
 * @param fifo FIFO 实例。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 */
static foundation_status_t check(const byte_fifo_t *fifo)
{
    if (!fifo) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!fifo->storage || (fifo->capacity == 0U)) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if ((fifo->size > fifo->capacity) || (fifo->read_index >= fifo->capacity) ||
        (fifo->write_index >= fifo->capacity)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 校验传输区间不覆盖实例或内部存储。
 * @param fifo FIFO 实例。
 * @param data 读写数据区。
 * @param size 字节数。
 * @return 参数满足约束时返回 true，否则返回 false。
 */
static bool valid_data(const byte_fifo_t *fifo, const void *data, size_t size)
{
    return ((size == 0U) || data) && !overlaps(data, size, fifo, sizeof(*fifo)) &&
           !overlaps(data, size, fifo->storage, fifo->capacity);
}

/**
 * @brief 初始化空 FIFO，校验失败不改变旧实例。
 * @param fifo FIFO 实例。
 * @param storage 调用方提供的存储区。
 * @param capacity 容量上限。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 */
foundation_status_t byte_fifo_init(byte_fifo_t *fifo, uint8_t *storage, size_t capacity)
{
    if (!fifo || !storage || (capacity == 0U) || overlaps(fifo, sizeof(*fifo), storage, capacity)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *fifo = (byte_fifo_t){.storage = storage, .capacity = capacity};
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 预检后执行完整或部分字节传输。
 * @param fifo FIFO 实例。
 * @param input 输入对象表示，调用期间保持有效。
 * @param output 输出存储区，容量至少满足请求长度。
 * @param size 字节数。
 * @param result 仅成功时写入的结果。
 * @param writing true 表示写入，false 表示读取。
 * @param exact true 要求完整传输，false 允许部分进展。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_FULL 没有足够容量。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
static foundation_status_t transfer(byte_fifo_t *fifo, const uint8_t *input, uint8_t *output,
    size_t size, size_t *result, bool writing, bool exact)
{
    foundation_status_t status = check(fifo);
    const void *data = writing ? (const void *)input : (const void *)output;
    size_t available;
    size_t amount;
    size_t position;
    size_t first;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!valid_data(fifo, data, size) || !result || !valid_data(fifo, result, sizeof(*result)) ||
        overlaps(data, size, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    available = writing ? (fifo->capacity - fifo->size) : fifo->size;
    amount = (size < available) ? size : available;
    if ((size > available) && writing && (fifo->rejected_count < SIZE_MAX)) {
        fifo->rejected_count++;
    }
    if ((size != 0U) && ((amount == 0U) || (exact && (size > available)))) {
        return writing ? FOUNDATION_STATUS_FULL : FOUNDATION_STATUS_EMPTY;
    }
    if (amount == 0U) {
        *result = 0U;
        return FOUNDATION_STATUS_OK;
    }
    position = writing ? fifo->write_index : fifo->read_index;
    first = fifo->capacity - position;
    if (first > amount) {
        first = amount;
    }
    if (writing) {
        /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        (void)memcpy(&fifo->storage[position], input, first);
        if (amount > first) {
            /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
            // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
            (void)memcpy(fifo->storage, &input[first], amount - first);
        }
        fifo->write_index = advance(position, amount, fifo->capacity);
        fifo->size += amount;
        if (fifo->size > fifo->high_water_mark) {
            fifo->high_water_mark = fifo->size;
        }
    } else {
        /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        (void)memcpy(output, &fifo->storage[position], first);
        if (amount > first) {
            /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
            // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
            (void)memcpy(&output[first], fifo->storage, amount - first);
        }
        fifo->read_index = advance(position, amount, fifo->capacity);
        fifo->size -= amount;
    }
    *result = amount;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 完整写入指定字节数。
 * @param fifo FIFO 实例。
 * @param data 读写数据区。
 * @param size 字节数。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_FULL 没有足够容量。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_write(byte_fifo_t *fifo, const uint8_t *data, size_t size)
{
    size_t result;
    return transfer(fifo, data, NULL, size, &result, true, true);
}

/**
 * @brief 尽可能写入字节，无进展时保持数量输出。
 * @param fifo FIFO 实例。
 * @param data 读写数据区。
 * @param size 字节数。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_FULL 没有足够容量。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_write_some(byte_fifo_t *fifo, const uint8_t *data, size_t size,
    size_t *result)
{
    return transfer(fifo, data, NULL, size, result, true, false);
}

/**
 * @brief 完整读取指定字节数。
 * @param fifo FIFO 实例。
 * @param data 读写数据区。
 * @param size 字节数。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_read(byte_fifo_t *fifo, uint8_t *data, size_t size)
{
    size_t result;
    return transfer(fifo, NULL, data, size, &result, false, true);
}

/**
 * @brief 尽可能读取字节，无进展时保持数量输出。
 * @param fifo FIFO 实例。
 * @param data 读写数据区。
 * @param size 字节数。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_read_some(byte_fifo_t *fifo, uint8_t *data, size_t size,
    size_t *result)
{
    return transfer(fifo, NULL, data, size, result, false, false);
}

/**
 * @brief 复制队头数据但不移动读索引。
 * @param fifo FIFO 实例。
 * @param data 读写数据区。
 * @param size 字节数。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_peek(const byte_fifo_t *fifo, uint8_t *data, size_t size)
{
    foundation_status_t status = check(fifo);
    size_t first;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!valid_data(fifo, data, size)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (size > fifo->size) {
        return FOUNDATION_STATUS_EMPTY;
    }
    if (size == 0U) {
        return FOUNDATION_STATUS_OK;
    }
    first = fifo->capacity - fifo->read_index;
    if (first > size) {
        first = size;
    }
    /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    (void)memcpy(data, &fifo->storage[fifo->read_index], first);
    if (size > first) {
        /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        (void)memcpy(&data[first], fifo->storage, size - first);
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 完整丢弃队头字节，不清除历史统计。
 * @param fifo FIFO 实例。
 * @param size 字节数。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_discard(byte_fifo_t *fifo, size_t size)
{
    foundation_status_t status = check(fifo);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (size > fifo->size) {
        return FOUNDATION_STATUS_EMPTY;
    }
    fifo->read_index = advance(fifo->read_index, size, fifo->capacity);
    fifo->size -= size;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 单字节快捷操作，沿用批量接口契约。
 * @param fifo FIFO 实例。
 * @param value 单字节数据。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_FULL 没有足够容量。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_push(byte_fifo_t *fifo, uint8_t value)
{
    return byte_fifo_write(fifo, &value, 1U);
}

/**
 * @brief 单字节快捷操作，沿用批量接口契约。
 * @param fifo FIFO 实例。
 * @param value 单字节数据。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_pop(byte_fifo_t *fifo, uint8_t *value)
{
    return byte_fifo_read(fifo, value, 1U);
}

/**
 * @brief 单字节快捷操作，沿用批量接口契约。
 * @param fifo FIFO 实例。
 * @param value 单字节数据。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_peek_byte(const byte_fifo_t *fifo, uint8_t *value)
{
    return byte_fifo_peek(fifo, value, 1U);
}

/**
 * @brief 查询当前占用量，失败保持输出。
 * @param fifo FIFO 实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_size(const byte_fifo_t *fifo, size_t *result)
{
    foundation_status_t status = check(fifo);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_data(fifo, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = fifo->size;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询总容量，失败保持输出。
 * @param fifo FIFO 实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_capacity(const byte_fifo_t *fifo, size_t *result)
{
    foundation_status_t status = check(fifo);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_data(fifo, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = fifo->capacity;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询剩余容量，失败保持输出。
 * @param fifo FIFO 实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_free(const byte_fifo_t *fifo, size_t *result)
{
    foundation_status_t status = check(fifo);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_data(fifo, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = fifo->capacity - fifo->size;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询拒绝请求次数，失败保持输出。
 * @param fifo FIFO 实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_rejected_count(const byte_fifo_t *fifo, size_t *result)
{
    foundation_status_t status = check(fifo);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_data(fifo, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = fifo->rejected_count;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询历史最大占用量，失败保持输出。
 * @param fifo FIFO 实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t byte_fifo_high_water_mark(const byte_fifo_t *fifo, size_t *result)
{
    foundation_status_t status = check(fifo);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_data(fifo, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = fifo->high_water_mark;
    return FOUNDATION_STATUS_OK;
}
