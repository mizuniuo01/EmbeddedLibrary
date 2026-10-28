/**
 * @file numeric.c
 * @brief 常用有限值检查与闭区间限幅函数实现。
 */

#include "numeric.h"

#include <math.h>

/**
 * @brief 判断单精度浮点数是否为有限值。
 * @param value 待检查的单精度浮点数。
 * @return value 不是 NaN 或正负无穷时返回 true，否则返回 false。
 */
bool numeric_f32_is_finite(float value)
{
    return isfinite(value) != 0;
}

/**
 * @brief 将单精度浮点数限制在闭区间内。
 * @param value 待限幅的数值。
 * @param minimum 闭区间下限。
 * @param maximum 闭区间上限。
 * @param result 用于接收限幅结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 限幅成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 输出参数为空或区间反向。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输入值或边界不是有限值。
 */
foundation_status_t numeric_f32_clamp(float value, float minimum, float maximum, float *result)
{
    float clamped_value;

    if (!result) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if ((!numeric_f32_is_finite(value)) || (!numeric_f32_is_finite(minimum)) ||
        (!numeric_f32_is_finite(maximum))) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    if (minimum > maximum) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }

    clamped_value = value;
    if (clamped_value < minimum) {
        clamped_value = minimum;
    } else if (clamped_value > maximum) {
        clamped_value = maximum;
    }

    *result = clamped_value;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 将 int32_t 数值限制在闭区间内。
 * @param value 待限幅的数值。
 * @param minimum 闭区间下限。
 * @param maximum 闭区间上限。
 * @param result 用于接收限幅结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 限幅成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 输出参数为空或区间反向。
 */
/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters)：value、minimum、maximum 是公开接口顺序。 */
foundation_status_t numeric_i32_clamp(int32_t value, int32_t minimum, int32_t maximum,
    int32_t *result)
{
    int32_t clamped_value;

    if (!result) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (minimum > maximum) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }

    clamped_value = value;
    if (clamped_value < minimum) {
        clamped_value = minimum;
    } else if (clamped_value > maximum) {
        clamped_value = maximum;
    }

    *result = clamped_value;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 将 uint32_t 数值限制在闭区间内。
 * @param value 待限幅的数值。
 * @param minimum 闭区间下限。
 * @param maximum 闭区间上限。
 * @param result 用于接收限幅结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 限幅成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 输出参数为空或区间反向。
 */
/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters)：value、minimum、maximum 是公开接口顺序。 */
foundation_status_t numeric_u32_clamp(uint32_t value, uint32_t minimum, uint32_t maximum,
    uint32_t *result)
{
    uint32_t clamped_value;

    if (!result) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (minimum > maximum) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }

    clamped_value = value;
    if (clamped_value < minimum) {
        clamped_value = minimum;
    } else if (clamped_value > maximum) {
        clamped_value = maximum;
    }

    *result = clamped_value;
    return FOUNDATION_STATUS_OK;
}
