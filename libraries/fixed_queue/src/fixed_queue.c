/**
 * @file fixed_queue.c
 * @brief 以字节复制保存固定大小元素，不要求存储对齐。
 */
#include "fixed_queue.h"

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
 * @brief 校验队列状态。
 * @param queue 队列实例。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 */
static foundation_status_t check(const fixed_queue_t *queue)
{
    if (!queue) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!queue->storage || (queue->capacity == 0U) || (queue->element_size == 0U)) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if ((queue->capacity > SIZE_MAX / queue->element_size) || (queue->size > queue->capacity) ||
        (queue->head >= queue->capacity)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 检查数据不与队列对象及存储区重叠。
 * @param queue 队列实例。
 * @param data 读写数据区。
 * @param size 字节数。
 * @return 参数满足约束时返回 true，否则返回 false。
 */
static bool valid_data(const fixed_queue_t *queue, const void *data, size_t size)
{
    return ((size == 0U) || data) && !overlaps(data, size, queue, sizeof(*queue)) &&
           !overlaps(data, size, queue->storage, queue->capacity * queue->element_size);
}

/**
 * @brief 校验大小后绑定存储，不修改存储中的原始字节。
 * @param queue 队列实例。
 * @param storage 调用方提供的存储区。
 * @param storage_size 调用方数据区的可用字节数。
 * @param element_size 每个队列元素的字节数，必须非零。
 * @param capacity 容量上限。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_OVERFLOW 容量运算或发布序号溢出。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 提供的存储容量不足。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 */
foundation_status_t fixed_queue_init(fixed_queue_t *queue, uint8_t *storage, size_t storage_size,
    size_t element_size, size_t capacity)
{
    if (!queue || !storage || (element_size == 0U) || (capacity == 0U)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (capacity > SIZE_MAX / element_size) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    if (storage_size < capacity * element_size) {
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    }
    if (overlaps(queue, sizeof(*queue), storage, storage_size)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *queue =
        (fixed_queue_t){.storage = storage, .element_size = element_size, .capacity = capacity};
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 预检整个请求后复制完整元素，部分进展返回成功。
 * @param queue 队列实例。
 * @param input 输入对象表示，调用期间保持有效。
 * @param output 输出存储区，容量至少满足请求长度。
 * @param count 元素数。
 * @param result 仅成功时写入的结果。
 * @param writing true 表示写入，false 表示读取。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_OVERFLOW 容量运算或发布序号溢出。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_FULL 没有足够容量。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
static foundation_status_t transfer(fixed_queue_t *queue, const void *input, void *output,
    size_t count, size_t *result, bool writing)
{
    foundation_status_t status = check(queue);
    const void *data = writing ? input : output;
    size_t available;
    size_t amount;
    size_t index;
    size_t bytes;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (count > SIZE_MAX / queue->element_size) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    bytes = count * queue->element_size;
    if (!valid_data(queue, data, bytes) || !result || !valid_data(queue, result, sizeof(*result)) ||
        overlaps(data, bytes, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    available = writing ? (queue->capacity - queue->size) : queue->size;
    amount = (available < count) ? available : count;
    if (writing && (count > available) && (queue->rejected_count < SIZE_MAX)) {
        queue->rejected_count++;
    }
    if ((count != 0U) && (amount == 0U)) {
        return writing ? FOUNDATION_STATUS_FULL : FOUNDATION_STATUS_EMPTY;
    }
    for (index = 0U; index < amount; index++) {
        if (writing) {
            size_t slot = advance(queue->head, queue->size, queue->capacity);
            /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
            // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
            (void)memcpy(&queue->storage[slot * queue->element_size],
                (const uint8_t *)input + index * queue->element_size, queue->element_size);
            queue->size++;
        } else {
            /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
            // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
            (void)memcpy((uint8_t *)output + index * queue->element_size,
                &queue->storage[queue->head * queue->element_size], queue->element_size);
            queue->head = advance(queue->head, 1U, queue->capacity);
            queue->size--;
        }
    }
    if (queue->size > queue->high_water_mark) {
        queue->high_water_mark = queue->size;
    }
    *result = amount;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 复制单个元素，失败不改变队列或元素。
 * @param queue 队列实例。
 * @param data 读写数据区。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_OVERFLOW 容量运算或发布序号溢出。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_FULL 没有足够容量。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_push(fixed_queue_t *queue, const void *data)
{
    size_t result;
    return transfer(queue, data, NULL, 1U, &result, true);
}

/**
 * @brief 批量复制完整元素，无进展时保持数量输出。
 * @param queue 队列实例。
 * @param data 读写数据区。
 * @param count 元素数。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_OVERFLOW 容量运算或发布序号溢出。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_FULL 没有足够容量。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_push_some(fixed_queue_t *queue, const void *data, size_t count,
    size_t *result)
{
    return transfer(queue, data, NULL, count, result, true);
}

/**
 * @brief 复制单个元素，失败不改变队列或元素。
 * @param queue 队列实例。
 * @param data 读写数据区。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_OVERFLOW 容量运算或发布序号溢出。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_pop(fixed_queue_t *queue, void *data)
{
    size_t result;
    return transfer(queue, NULL, data, 1U, &result, false);
}

/**
 * @brief 批量复制完整元素，无进展时保持数量输出。
 * @param queue 队列实例。
 * @param data 读写数据区。
 * @param count 元素数。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_OVERFLOW 容量运算或发布序号溢出。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_pop_some(fixed_queue_t *queue, void *data, size_t count,
    size_t *result)
{
    return transfer(queue, NULL, data, count, result, false);
}

/**
 * @brief 按相对队头的索引复制元素，不移除元素。
 * @param queue 队列实例。
 * @param index 逻辑索引。
 * @param data 读写数据区。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_OUT_OF_RANGE 逻辑索引越界。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_peek_at(const fixed_queue_t *queue, size_t index, void *data)
{
    foundation_status_t status = check(queue);
    size_t slot;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!valid_data(queue, data, queue->element_size)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (index >= queue->size) {
        return FOUNDATION_STATUS_OUT_OF_RANGE;
    }
    slot = advance(queue->head, index, queue->capacity);
    /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    (void)memcpy(data, &queue->storage[slot * queue->element_size], queue->element_size);
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 复制队头元素，空队列返回 EMPTY。
 * @param queue 队列实例。
 * @param data 读写数据区。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_peek(const fixed_queue_t *queue, void *data)
{
    foundation_status_t status = check(queue);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!valid_data(queue, data, queue->element_size)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (queue->size == 0U) {
        return FOUNDATION_STATUS_EMPTY;
    }
    return fixed_queue_peek_at(queue, 0U, data);
}

/**
 * @brief 查询当前占用量，失败保持输出。
 * @param queue 队列实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_size(const fixed_queue_t *queue, size_t *result)
{
    foundation_status_t status = check(queue);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_data(queue, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = queue->size;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询总容量，失败保持输出。
 * @param queue 队列实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_capacity(const fixed_queue_t *queue, size_t *result)
{
    foundation_status_t status = check(queue);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_data(queue, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = queue->capacity;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询剩余容量，失败保持输出。
 * @param queue 队列实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_free(const fixed_queue_t *queue, size_t *result)
{
    foundation_status_t status = check(queue);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_data(queue, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = queue->capacity - queue->size;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询拒绝请求次数，失败保持输出。
 * @param queue 队列实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_rejected_count(const fixed_queue_t *queue, size_t *result)
{
    foundation_status_t status = check(queue);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_data(queue, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = queue->rejected_count;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询历史最大占用量，失败保持输出。
 * @param queue 队列实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t fixed_queue_high_water_mark(const fixed_queue_t *queue, size_t *result)
{
    foundation_status_t status = check(queue);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_data(queue, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = queue->high_water_mark;
    return FOUNDATION_STATUS_OK;
}
