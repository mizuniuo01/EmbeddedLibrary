/**
 * @file first_order_filter.c
 * @brief 单精度一阶低通滤波器。
 */

#include "first_order_filter.h"

#include <stddef.h>

#include "numeric.h"

/**
 * @brief 初始化一阶滤波器，失败时不改变旧实例。
 * @param filter 滤波器实例。
 * @param alpha 更新系数，范围为闭区间 [0, 1]。
 * @param initial_output 初始输出。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 实例为空或 alpha 越界。
 * @retval FOUNDATION_STATUS_INVALID_DATA 配置包含非有限值。
 */
foundation_status_t first_order_filter_f32_init(first_order_filter_f32_t *filter, float alpha,
    float initial_output)
{
    if (filter == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if ((!numeric_f32_is_finite(alpha)) || (!numeric_f32_is_finite(initial_output))) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    if ((alpha < 0.0F) || (alpha > 1.0F)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *filter = (first_order_filter_f32_t){
        .alpha = alpha,
        .output = initial_output,
        .initialized = true,
    };
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 重置滤波器输出但保留更新系数。
 * @param filter 滤波器实例。
 * @param output 新输出。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 实例为空。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输出不是有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 */
foundation_status_t first_order_filter_f32_reset(first_order_filter_f32_t *filter, float output)
{
    if (filter == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!filter->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if (!numeric_f32_is_finite(output)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    filter->output = output;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 使用当前样本更新滤波输出。
 * @param filter 滤波器实例。
 * @param input 当前输入。
 * @param output 成功时接收新输出，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 更新成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 实例或输出为空，或输出与实例重叠。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输入不是有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 * @retval FOUNDATION_STATUS_OVERFLOW 中间结果不是有限值。
 */
foundation_status_t first_order_filter_f32_update(first_order_filter_f32_t *filter, float input,
    float *output)
{
    float next_output;

    if ((filter == NULL) || (output == NULL) || (output == &filter->alpha) ||
        (output == &filter->output)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!filter->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if (!numeric_f32_is_finite(input)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    next_output = filter->output + filter->alpha * (input - filter->output);
    if (!numeric_f32_is_finite(next_output)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    filter->output = next_output;
    *output = next_output;
    return FOUNDATION_STATUS_OK;
}
