/**
 * @file snapshot_buffer.c
 * @brief 双槽快照，使用地址绑定的借用凭据保护槽复用。
 */
#include "snapshot_buffer.h"

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
 * @brief 检查快照实例状态。
 * @param buffer 快照实例。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 */
static foundation_status_t check(const snapshot_buffer_t *buffer)
{
    if (!buffer) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!buffer->slots[0] || !buffer->slots[1] || (buffer->capacity == 0U)) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if ((buffer->active > 1U) || (buffer->writing > 1U)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 检查输出不覆盖实例、槽或现有借用凭据。
 * @param buffer 快照实例。
 * @param data 读写数据区。
 * @param size 字节数。
 * @return 参数满足约束时返回 true，否则返回 false。
 */
static bool valid_output(const snapshot_buffer_t *buffer, const void *data, size_t size)
{
    size_t index;
    if (((size != 0U) && !data) || overlaps(data, size, buffer, sizeof(*buffer)) ||
        overlaps(data, size, buffer->slots[0], buffer->capacity) ||
        overlaps(data, size, buffer->slots[1], buffer->capacity)) {
        return false;
    }
    if (buffer->writer && overlaps(data, size, buffer->writer, sizeof(*buffer->writer))) {
        return false;
    }
    for (index = 0U; index < 2U; index++) {
        if (buffer->readers[index] &&
            overlaps(data, size, buffer->readers[index], sizeof(*buffer->readers[index]))) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 无借用及写事务时绑定两个不重叠数据槽。
 * @param buffer 快照实例。
 * @param first 第一个数据槽，至少包含 capacity 字节。
 * @param second 第二个数据槽，与第一个槽不重叠。
 * @param capacity 容量上限。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_BUSY 资源尚未归还或事务正在进行。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 */
foundation_status_t snapshot_buffer_init(snapshot_buffer_t *buffer, uint8_t *first, uint8_t *second,
    size_t capacity)
{
    if (!buffer || !first || !second || (capacity == 0U) ||
        overlaps(first, capacity, second, capacity) ||
        overlaps(buffer, sizeof(*buffer), first, capacity) ||
        overlaps(buffer, sizeof(*buffer), second, capacity)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (buffer->writer || buffer->readers[0] || buffer->readers[1]) {
        return FOUNDATION_STATUS_BUSY;
    }
    *buffer = (snapshot_buffer_t){.slots = {first, second}, .capacity = capacity};
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 借出非活动且未被读取的槽，不复制或清零旧载荷。
 * @param buffer 快照实例。
 * @param writer 原地址保存的写借用凭据，完成或取消前保持有效。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_BUSY 资源尚未归还或事务正在进行。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t snapshot_buffer_begin(snapshot_buffer_t *buffer,
    snapshot_buffer_writer_t *writer)
{
    foundation_status_t status = check(buffer);
    size_t slot;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!writer) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (buffer->writer) {
        return FOUNDATION_STATUS_BUSY;
    }
    if (!valid_output(buffer, writer, sizeof(*writer))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    slot = buffer->published ? (1U - buffer->active) : 0U;
    if (buffer->readers[slot]) {
        return FOUNDATION_STATUS_BUSY;
    }
    buffer->writing = slot;
    buffer->writer = writer;
    *writer = (snapshot_buffer_writer_t){.data = buffer->slots[slot], .capacity = buffer->capacity};
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 验证写凭据由本次 begin 在同一地址产生。
 * @param buffer 快照实例。
 * @param writer 原地址保存的写借用凭据，完成或取消前保持有效。
 * @return 参数满足约束时返回 true，否则返回 false。
 */
static bool valid_writer(const snapshot_buffer_t *buffer, const snapshot_buffer_writer_t *writer)
{
    return writer && (buffer->writer == writer) &&
           (writer->data == buffer->slots[buffer->writing]) &&
           (writer->capacity == buffer->capacity);
}

/**
 * @brief 发布完整快照并自动递增序号；失败保留写事务。
 * @param buffer 快照实例。
 * @param writer 原地址保存的写借用凭据，完成或取消前保持有效。
 * @param size 字节数。
 * @param timestamp 调用方提供的单调时刻。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 提供的存储容量不足。
 * @retval FOUNDATION_STATUS_OVERFLOW 容量运算或发布序号溢出。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 */
foundation_status_t snapshot_buffer_publish(snapshot_buffer_t *buffer,
    snapshot_buffer_writer_t *writer, size_t size, uint32_t timestamp)
{
    foundation_status_t status = check(buffer);
    uint32_t sequence;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!writer) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!valid_writer(buffer, writer)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    if (size > buffer->capacity) {
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    }
    sequence = buffer->published ? buffer->info[buffer->active].sequence : 0U;
    if (sequence == UINT32_MAX) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    buffer->info[buffer->writing] =
        (snapshot_buffer_info_t){.size = size, .sequence = sequence + 1U, .timestamp = timestamp};
    buffer->active = buffer->writing;
    buffer->published = true;
    buffer->writer = NULL;
    *writer = (snapshot_buffer_writer_t){0};
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 放弃写事务，保留已发布快照及其元数据。
 * @param buffer 快照实例。
 * @param writer 原地址保存的写借用凭据，完成或取消前保持有效。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 */
foundation_status_t snapshot_buffer_cancel(snapshot_buffer_t *buffer,
    snapshot_buffer_writer_t *writer)
{
    foundation_status_t status = check(buffer);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!writer) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!valid_writer(buffer, writer)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    buffer->writer = NULL;
    *writer = (snapshot_buffer_writer_t){0};
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 登记当前发布槽的只读借用，同槽已有借用时返回忙。
 * @param buffer 快照实例。
 * @param lease 调用方持有的借用凭据。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_BUSY 资源尚未归还或事务正在进行。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t snapshot_buffer_acquire(snapshot_buffer_t *buffer,
    snapshot_buffer_lease_t *lease)
{
    foundation_status_t status = check(buffer);
    size_t slot;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!lease) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!buffer->published) {
        return FOUNDATION_STATUS_EMPTY;
    }
    slot = buffer->active;
    if (buffer->readers[slot]) {
        return FOUNDATION_STATUS_BUSY;
    }
    if (!valid_output(buffer, lease, sizeof(*lease))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *lease = (snapshot_buffer_lease_t){.data = buffer->slots[slot],
        .size = buffer->info[slot].size,
        .sequence = buffer->info[slot].sequence,
        .timestamp = buffer->info[slot].timestamp};
    buffer->readers[slot] = lease;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 归还原地址读凭据，拒绝复制或已归还的凭据。
 * @param buffer 快照实例。
 * @param lease 调用方持有的借用凭据。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 */
foundation_status_t snapshot_buffer_release(snapshot_buffer_t *buffer,
    snapshot_buffer_lease_t *lease)
{
    foundation_status_t status = check(buffer);
    size_t slot;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!lease) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    for (slot = 0U; slot < 2U; slot++) {
        if (buffer->readers[slot] == lease) {
            if ((lease->data != buffer->slots[slot]) || (lease->size != buffer->info[slot].size) ||
                (lease->sequence != buffer->info[slot].sequence) ||
                (lease->timestamp != buffer->info[slot].timestamp)) {
                return FOUNDATION_STATUS_INVALID_STATE;
            }
            buffer->readers[slot] = NULL;
            *lease = (snapshot_buffer_lease_t){0};
            return FOUNDATION_STATUS_OK;
        }
    }
    return FOUNDATION_STATUS_INVALID_STATE;
}

/**
 * @brief 在调用方同步保护下复制完整快照与同次元数据。
 * @param buffer 快照实例。
 * @param data 读写数据区。
 * @param capacity 容量上限。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 提供的存储容量不足。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t snapshot_buffer_copy(const snapshot_buffer_t *buffer, uint8_t *data,
    size_t capacity, snapshot_buffer_info_t *result)
{
    foundation_status_t status = check(buffer);
    snapshot_buffer_info_t info;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_output(buffer, result, sizeof(*result)) ||
        !valid_output(buffer, data, capacity) ||
        overlaps(data, capacity, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!buffer->published) {
        return FOUNDATION_STATUS_EMPTY;
    }
    info = buffer->info[buffer->active];
    if (capacity < info.size) {
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    }
    if (info.size != 0U) {
        /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        (void)memcpy(data, buffer->slots[buffer->active], info.size);
    }
    *result = info;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询当前快照的发布序号，未发布时返回 EMPTY。
 * @param buffer 快照实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t snapshot_buffer_sequence(const snapshot_buffer_t *buffer, uint32_t *result)
{
    foundation_status_t status = check(buffer);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_output(buffer, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!buffer->published) {
        return FOUNDATION_STATUS_EMPTY;
    }
    *result = buffer->info[buffer->active].sequence;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询当前快照的发布时间戳，未发布时返回 EMPTY。
 * @param buffer 快照实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_EMPTY 没有足够数据。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t snapshot_buffer_timestamp(const snapshot_buffer_t *buffer, uint32_t *result)
{
    foundation_status_t status = check(buffer);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!result || !valid_output(buffer, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!buffer->published) {
        return FOUNDATION_STATUS_EMPTY;
    }
    *result = buffer->info[buffer->active].timestamp;
    return FOUNDATION_STATUS_OK;
}
