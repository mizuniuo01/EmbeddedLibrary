/**
 * @file moving_average.c
 * @brief 固定窗口单精度滑动平均。
 */

#include "moving_average.h"

#include <stddef.h>
#include <stdint.h>

#include "numeric.h"

/**
 * @brief 初始化空的逻辑窗口，不读取或清零调用方样本区。
 * @param filter 滤波器实例。
 * @param storage 调用方提供的浮点样本数组。
 * @param capacity 窗口容量。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、容量为零或存储与实例重叠。
 * @retval FOUNDATION_STATUS_OVERFLOW 样本区字节数溢出。
 */
foundation_status_t moving_average_f32_init(moving_average_f32_t *filter, float *storage,
    size_t capacity)
{
    uintptr_t filter_address = (uintptr_t)filter;
    uintptr_t storage_address = (uintptr_t)storage;
    size_t storage_size;

    if ((filter == NULL) || (storage == NULL) || (capacity == 0U)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (capacity > (SIZE_MAX / sizeof(*storage))) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    storage_size = capacity * sizeof(*storage);
    if (((filter_address <= storage_address) &&
            ((storage_address - filter_address) < sizeof(*filter))) ||
        ((storage_address < filter_address) &&
            ((filter_address - storage_address) < storage_size))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *filter = (moving_average_f32_t){
        .storage = storage,
        .capacity = capacity,
        .initialized = true,
    };
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 清除逻辑窗口但不要求清零样本区。
 * @param filter 滤波器实例。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 实例为空。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 */
foundation_status_t moving_average_f32_reset(moving_average_f32_t *filter)
{
    if (filter == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!filter->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    filter->index = 0U;
    filter->count = 0U;
    filter->sum = 0.0F;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 添加样本并计算已有样本的平均值。
 * @param filter 滤波器实例。
 * @param input 新样本。
 * @param output 成功时接收平均值，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 更新成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或输出覆盖实例。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输入不是有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 * @retval FOUNDATION_STATUS_OVERFLOW 总和或平均值不是有限值。
 */
foundation_status_t moving_average_f32_update(moving_average_f32_t *filter, float input,
    float *output)
{
    float sum;
    size_t next_count;

    if ((filter == NULL) || (output == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!filter->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if ((((uintptr_t)output >= (uintptr_t)filter->storage) &&
            (((uintptr_t)output - (uintptr_t)filter->storage) <
                (filter->capacity * sizeof(*filter->storage)))) ||
        (output == &filter->sum)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!numeric_f32_is_finite(input)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    sum = filter->sum;
    next_count = (filter->count < filter->capacity) ? filter->count + 1U : filter->count;
    if (filter->count == filter->capacity) {
        sum -= filter->storage[filter->index];
    }
    sum += input;
    if (!numeric_f32_is_finite(sum) || !numeric_f32_is_finite(sum / (float)next_count)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    *output = sum / (float)next_count;
    filter->storage[filter->index] = input;
    filter->sum = sum;
    filter->index = (filter->index + 1U) % filter->capacity;
    filter->count = next_count;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询当前有效样本数。
 * @param filter 滤波器实例。
 * @param count 成功时接收样本数。
 * @retval FOUNDATION_STATUS_OK 查询成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 */
foundation_status_t moving_average_f32_count(const moving_average_f32_t *filter, size_t *count)
{
    if ((filter == NULL) || (count == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!filter->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    *count = filter->count;
    return FOUNDATION_STATUS_OK;
}
