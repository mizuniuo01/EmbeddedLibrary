/**
 * @file deadband.c
 * @brief 中心回零的单精度死区函数。
 */
#include "deadband.h"

#include <stddef.h>

#include "numeric.h"

/**
 * @brief 初始化死区。
 * @param deadband 死区实例。
 * @param center 死区中心。
 * @param half_width 死区半宽。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或半宽为负。
 * @retval FOUNDATION_STATUS_INVALID_DATA 参数不是有限值。
 */
foundation_status_t deadband_f32_init(deadband_f32_t *deadband, float center, float half_width)
{
    float lower;
    float upper;

    if (deadband == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!numeric_f32_is_finite(center) || !numeric_f32_is_finite(half_width)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    if (half_width < 0.0F) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    lower = center - half_width;
    upper = center + half_width;
    if (!numeric_f32_is_finite(lower) || !numeric_f32_is_finite(upper)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    *deadband = (deadband_f32_t){center, half_width, 1};
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 计算中心回零死区输出。
 * @param deadband 死区实例。
 * @param input 当前输入。
 * @param output 成功时接收输出。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输入或边界计算不是有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 * @retval FOUNDATION_STATUS_OVERFLOW 边界或输出溢出。
 */
foundation_status_t deadband_f32_update(const deadband_f32_t *deadband, float input, float *output)
{
    float lower;
    float upper;
    float result;
    if ((deadband == NULL) || (output == NULL) || (output == &deadband->center) ||
        (output == &deadband->half_width)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!deadband->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if (!numeric_f32_is_finite(input)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    lower = deadband->center - deadband->half_width;
    upper = deadband->center + deadband->half_width;
    if (!numeric_f32_is_finite(lower) || !numeric_f32_is_finite(upper)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    if (input < lower) {
        result = input - lower;
    } else if (input > upper) {
        result = input - upper;
    } else {
        result = 0.0F;
    }
    if (!numeric_f32_is_finite(result)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    *output = result;
    return FOUNDATION_STATUS_OK;
}
