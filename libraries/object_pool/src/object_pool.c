/**
 * @file object_pool.c
 * @brief 外部同步的固定对象池。
 */
#include "object_pool.h"

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
 * @brief 检查对象池状态。
 * @param pool 对象池实例。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 */
static foundation_status_t check(const object_pool_t *pool)
{
    if (!pool) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!pool->storage || !pool->states || (pool->capacity == 0U) || (pool->slot_size == 0U)) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if ((pool->capacity > SIZE_MAX / pool->slot_size) || (pool->available > pool->capacity) ||
        (pool->lowest_free_index > pool->capacity)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 检查输出不覆盖内部状态。
 * @param pool 对象池实例。
 * @param data 读写数据区。
 * @param size 字节数。
 * @return 参数满足约束时返回 true，否则返回 false。
 */
static bool valid_output(const object_pool_t *pool, const void *data, size_t size)
{
    return data && !overlaps(data, size, pool, sizeof(*pool)) &&
           !overlaps(data, size, pool->states, pool->capacity) &&
           !overlaps(data, size, pool->storage, pool->capacity * pool->slot_size);
}

/**
 * @brief 校验配置后绑定存储，有对象借出时拒绝重新绑定。
 * @param pool 对象池实例。
 * @param config 初始化配置。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_OVERFLOW 容量运算或发布序号溢出。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 提供的存储容量不足。
 * @retval FOUNDATION_STATUS_BUSY 资源尚未归还或事务正在进行。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 */
foundation_status_t object_pool_init(object_pool_t *pool, const object_pool_config_t *config)
{
    size_t bytes;
    if (!pool || !config || !config->storage || !config->states || (config->capacity == 0U) ||
        (config->slot_size == 0U) || (config->alignment == 0U) ||
        ((config->alignment & (config->alignment - 1U)) != 0U) ||
        (((uintptr_t)config->storage % config->alignment) != 0U) ||
        ((config->slot_size % config->alignment) != 0U)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (config->capacity > SIZE_MAX / config->slot_size) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    bytes = config->capacity * config->slot_size;
    if ((config->storage_size < bytes) || (config->states_size < config->capacity)) {
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    }
    if (overlaps(pool, sizeof(*pool), config->storage, config->storage_size) ||
        overlaps(pool, sizeof(*pool), config->states, config->states_size) ||
        overlaps(config->storage, config->storage_size, config->states, config->states_size) ||
        overlaps(config, sizeof(*config), config->storage, config->storage_size) ||
        overlaps(config, sizeof(*config), config->states, config->states_size) ||
        overlaps(pool, sizeof(*pool), config, sizeof(*config))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (pool->storage && (pool->available != pool->capacity)) {
        return FOUNDATION_STATUS_BUSY;
    }
    *pool = (object_pool_t){.storage = config->storage,
        .states = config->states,
        .slot_size = config->slot_size,
        .capacity = config->capacity,
        .available = config->capacity};
    /* 已预检长度和不重叠区间；C11 可选 Annex K 不是本库依赖。 */
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    (void)memset(pool->states, 0, pool->capacity);
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 借出最低空闲槽，不初始化载荷。
 * @param pool 对象池实例。
 * @param result 仅成功时写入的结果。
 * @param index 逻辑索引。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_FULL 没有足够容量。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 */
foundation_status_t object_pool_acquire(object_pool_t *pool, void **result, size_t *index)
{
    foundation_status_t status = check(pool);
    size_t slot;
    size_t next;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!valid_output(pool, (const void *)result, sizeof(*result)) ||
        !valid_output(pool, index, sizeof(*index)) ||
        overlaps((const void *)result, sizeof(*result), index, sizeof(*index))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (pool->available == 0U) {
        if (pool->exhausted_count < SIZE_MAX) {
            pool->exhausted_count++;
        }
        return FOUNDATION_STATUS_FULL;
    }
    slot = pool->lowest_free_index;
    if ((slot >= pool->capacity) || (pool->states[slot] != 0U)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    next = slot + 1U;
    while ((next < pool->capacity) && (pool->states[next] != 0U)) {
        next++;
    }
    pool->states[slot] = 1U;
    pool->available--;
    pool->lowest_free_index = next;
    *result = &pool->storage[slot * pool->slot_size];
    *index = slot;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 归还当前占用槽首地址，拒绝重复释放。
 * @param pool 对象池实例。
 * @param data 读写数据区。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 */
foundation_status_t object_pool_release(object_pool_t *pool, void *data)
{
    foundation_status_t status = check(pool);
    uintptr_t address = (uintptr_t)data;
    uintptr_t base;
    uintptr_t offset;
    size_t slot;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    base = (uintptr_t)pool->storage;
    if (!data || (address < base)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    offset = address - base;
    if ((offset >= pool->capacity * pool->slot_size) || ((offset % pool->slot_size) != 0U)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    slot = (size_t)(offset / pool->slot_size);
    if (pool->states[slot] != 1U) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    pool->states[slot] = 0U;
    pool->available++;
    if (slot < pool->lowest_free_index) {
        pool->lowest_free_index = slot;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询总容量，最低空闲索引等于容量表示耗尽。
 * @param pool 对象池实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t object_pool_capacity(const object_pool_t *pool, size_t *result)
{
    foundation_status_t status = check(pool);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!valid_output(pool, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = pool->capacity;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询可用槽数，最低空闲索引等于容量表示耗尽。
 * @param pool 对象池实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t object_pool_available(const object_pool_t *pool, size_t *result)
{
    foundation_status_t status = check(pool);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!valid_output(pool, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = pool->available;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询最低空闲索引，最低空闲索引等于容量表示耗尽。
 * @param pool 对象池实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t object_pool_lowest_free_index(const object_pool_t *pool, size_t *result)
{
    foundation_status_t status = check(pool);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!valid_output(pool, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = pool->lowest_free_index;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询耗尽请求次数，最低空闲索引等于容量表示耗尽。
 * @param pool 对象池实例。
 * @param result 仅成功时写入的结果。
 * @retval FOUNDATION_STATUS_OK 操作成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、重叠或配置非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例未初始化。
 * @retval FOUNDATION_STATUS_INVALID_STATE 内部状态或借用凭据无效。
 */
foundation_status_t object_pool_exhausted_count(const object_pool_t *pool, size_t *result)
{
    foundation_status_t status = check(pool);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!valid_output(pool, result, sizeof(*result))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *result = pool->exhausted_count;
    return FOUNDATION_STATUS_OK;
}
