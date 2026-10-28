/**
 * @file hysteresis.c
 * @brief 带上下阈值的二值单精度迟滞。
 */
#include "hysteresis.h"

#include <stddef.h>

#include "numeric.h"

/**
 * @brief 初始化二值迟滞。
 * @param hysteresis 迟滞实例。
 * @param lower 低阈值。
 * @param upper 高阈值。
 * @param low_output 低状态输出。
 * @param high_output 高状态输出。
 * @param initial_high 初始状态。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或阈值反向。
 * @retval FOUNDATION_STATUS_INVALID_DATA 参数不是有限值。
 */
foundation_status_t hysteresis_f32_init(hysteresis_f32_t *hysteresis, float lower, float upper,
    float low_output, float high_output, bool initial_high)
{
    if (hysteresis == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!numeric_f32_is_finite(lower) || !numeric_f32_is_finite(upper) ||
        !numeric_f32_is_finite(low_output) || !numeric_f32_is_finite(high_output)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    if (lower > upper) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *hysteresis = (hysteresis_f32_t){lower, upper, low_output, high_output, initial_high, true};
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 重置迟滞状态。
 * @param hysteresis 迟滞实例。
 * @param initial_high 新的初始状态。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 实例为空。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 */
foundation_status_t hysteresis_f32_reset(hysteresis_f32_t *hysteresis, bool initial_high)
{
    if (hysteresis == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!hysteresis->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    hysteresis->high = initial_high;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 根据输入跨越阈值更新二值状态。
 * @param hysteresis 迟滞实例。
 * @param input 当前输入。
 * @param output 成功时接收当前状态输出。
 * @retval FOUNDATION_STATUS_OK 更新成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或输出覆盖实例。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输入不是有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 */
foundation_status_t hysteresis_f32_update(hysteresis_f32_t *hysteresis, float input, float *output)
{
    if ((hysteresis == NULL) || (output == NULL) || (output == &hysteresis->lower) ||
        (output == &hysteresis->upper) || (output == &hysteresis->low_output) ||
        (output == &hysteresis->high_output)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!hysteresis->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if (!numeric_f32_is_finite(input)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    if (input >= hysteresis->upper) {
        hysteresis->high = true;
    } else if (input <= hysteresis->lower) {
        hysteresis->high = false;
    }
    *output = hysteresis->high ? hysteresis->high_output : hysteresis->low_output;
    return FOUNDATION_STATUS_OK;
}
