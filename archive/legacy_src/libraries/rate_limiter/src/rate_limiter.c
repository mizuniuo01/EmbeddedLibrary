/**
 * @file rate_limiter.c
 * @brief 显式时间步长的单精度上升/下降速率限制器。
 */
#include "rate_limiter.h"

#include <stddef.h>

#include "numeric.h"

/**
 * @brief 初始化速率限制器。
 * @param limiter 速率限制器实例。
 * @param rising_rate 最大上升速率。
 * @param falling_rate 最大下降速率。
 * @param initial_output 初始输出。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或速率为负。
 * @retval FOUNDATION_STATUS_INVALID_DATA 参数不是有限值。
 */
foundation_status_t rate_limiter_f32_init(rate_limiter_f32_t *limiter, float rising_rate,
    float falling_rate, float initial_output)
{
    if (limiter == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!numeric_f32_is_finite(rising_rate) || !numeric_f32_is_finite(falling_rate) ||
        !numeric_f32_is_finite(initial_output)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    if ((rising_rate < 0.0F) || (falling_rate < 0.0F)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *limiter = (rate_limiter_f32_t){rising_rate, falling_rate, initial_output, 1};
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 重置速率限制器输出。
 * @param limiter 速率限制器实例。
 * @param output 新输出。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 实例为空。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输出不是有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 */
foundation_status_t rate_limiter_f32_reset(rate_limiter_f32_t *limiter, float output)
{
    if (limiter == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!limiter->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if (!numeric_f32_is_finite(output)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    limiter->output = output;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 按显式时间步长限制输出变化。
 * @param limiter 速率限制器实例。
 * @param input 目标输入。
 * @param dt 正时间步长。
 * @param output 成功时接收新输出。
 * @retval FOUNDATION_STATUS_OK 更新成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或输出覆盖实例。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输入或 dt 非法。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 * @retval FOUNDATION_STATUS_OVERFLOW 允许变化量溢出。
 */
foundation_status_t rate_limiter_f32_update(rate_limiter_f32_t *limiter, float input, float dt,
    float *output)
{
    float delta;
    float limit;
    float next;
    if ((limiter == NULL) || (output == NULL) || (output == &limiter->rising_rate) ||
        (output == &limiter->falling_rate) || (output == &limiter->output)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!limiter->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if (!numeric_f32_is_finite(input) || !numeric_f32_is_finite(dt) || (dt <= 0.0F)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    delta = input - limiter->output;
    limit = ((delta >= 0.0F) ? limiter->rising_rate : limiter->falling_rate) * dt;
    if (!numeric_f32_is_finite(delta) || !numeric_f32_is_finite(limit)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    next = limiter->output;
    if (delta > limit) {
        next += limit;
    } else if (delta < -limit) {
        next -= limit;
    } else {
        next = input;
    }
    if (!numeric_f32_is_finite(next)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    limiter->output = next;
    *output = next;
    return FOUNDATION_STATUS_OK;
}
