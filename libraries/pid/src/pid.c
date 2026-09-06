/**
 * @file pid.c
 * @brief 与时钟和执行器无关的单精度 PID 控制器。
 */
#include "pid.h"

#include <math.h>
#include <stddef.h>

#include "numeric.h"

/**
 * @brief 判断单精度数值是否有限。
 * @param value 待检查的数值。
 * @return 有限时返回 true，否则返回 false。
 */
static bool finite_value(float value)
{
    return numeric_f32_is_finite(value);
}

/**
 * @brief 校验 PID 配置的范围和枚举值。
 * @param config 待校验配置。
 * @return 配置合法时返回 OK，否则返回具体错误。
 */
static foundation_status_t validate_config(const pid_f32_config_t *config)
{
    if (config == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!finite_value(config->kp) || !finite_value(config->ki) || !finite_value(config->kd) ||
        !finite_value(config->output_min) || !finite_value(config->output_max) ||
        !finite_value(config->integral_min) || !finite_value(config->integral_max) ||
        !finite_value(config->tracking_gain)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    if ((config->output_min > config->output_max) ||
        (config->integral_min > config->integral_max) || (config->tracking_gain < 0.0F) ||
        (config->derivative_mode > PID_DERIVATIVE_ON_ERROR) ||
        (config->anti_windup_mode > PID_ANTI_WINDUP_BACK_CALCULATION)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 校验已经初始化的 PID 实例及其历史状态。
 * @param pid PID 实例。
 * @return 状态有效时返回 OK，否则返回具体错误。
 */
static foundation_status_t check_pid(const pid_f32_t *pid)
{
    foundation_status_t status;
    if (pid == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!pid->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    status = validate_config(&pid->config);
    if (status != FOUNDATION_STATUS_OK) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    if (!finite_value(pid->integral) || !finite_value(pid->previous_measurement) ||
        !finite_value(pid->previous_error) || !finite_value(pid->output)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 初始化 PID 实例并清除历史状态。
 * @param pid PID 实例。
 * @param config 初始化配置。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空、范围或枚举非法。
 * @retval FOUNDATION_STATUS_INVALID_DATA 配置包含非有限数值。
 */
foundation_status_t pid_f32_init(pid_f32_t *pid, const pid_f32_config_t *config)
{
    foundation_status_t status = validate_config(config);
    if ((status != FOUNDATION_STATUS_OK) || (pid == NULL)) {
        return (pid == NULL) ? FOUNDATION_STATUS_INVALID_ARGUMENT : status;
    }
    *pid = (pid_f32_t){.config = *config, .initialized = true};
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 重置积分和历史输入，避免下一次计算产生导数冲击。
 * @param pid PID 实例。
 * @param measurement 当前测量值。
 * @param error 当前误差。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输入不是有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 */
foundation_status_t pid_f32_reset(pid_f32_t *pid, float measurement, float error)
{
    foundation_status_t status = check_pid(pid);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!finite_value(measurement) || !finite_value(error)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    pid->integral = 0.0F;
    pid->previous_measurement = measurement;
    pid->previous_error = error;
    pid->output = 0.0F;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 按显式时间步长执行一次 PID 计算。
 * @param pid PID 实例。
 * @param setpoint 目标值。
 * @param measurement 当前测量值。
 * @param dt 本次计算的正时间步长。
 * @param output 成功时写入限幅后的控制输出。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或配置非法。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输入或时间步长不是合法有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 实例尚未初始化。
 * @retval FOUNDATION_STATUS_OVERFLOW 中间计算超出有限浮点范围。
 */
foundation_status_t pid_f32_calculate(pid_f32_t *pid, float setpoint, float measurement, float dt,
    float *output)
{
    foundation_status_t status = check_pid(pid);
    float error;
    float derivative;
    float candidate_integral;
    float raw_output;
    float limited_output;
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (output == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!finite_value(setpoint) || !finite_value(measurement) || !finite_value(dt) ||
        (dt <= 0.0F)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    error = setpoint - measurement;
    derivative = (pid->config.derivative_mode == PID_DERIVATIVE_ON_MEASUREMENT)
                     ? (pid->previous_measurement - measurement) / dt
                     : (error - pid->previous_error) / dt;
    candidate_integral = pid->integral + error * dt;
    if (!finite_value(error) || !finite_value(derivative) || !finite_value(candidate_integral)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    if (pid->config.anti_windup_mode == PID_ANTI_WINDUP_CONDITIONAL) {
        raw_output = pid->config.kp * error + pid->config.ki * candidate_integral +
                     pid->config.kd * derivative;
        if (((raw_output > pid->config.output_max) && (error > 0.0F)) ||
            ((raw_output < pid->config.output_min) && (error < 0.0F))) {
            candidate_integral = pid->integral;
        }
    }
    candidate_integral =
        fminf(fmaxf(candidate_integral, pid->config.integral_min), pid->config.integral_max);
    raw_output =
        pid->config.kp * error + pid->config.ki * candidate_integral + pid->config.kd * derivative;
    if (!finite_value(raw_output)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    limited_output = fminf(fmaxf(raw_output, pid->config.output_min), pid->config.output_max);
    if (pid->config.anti_windup_mode == PID_ANTI_WINDUP_BACK_CALCULATION) {
        candidate_integral += pid->config.tracking_gain * (limited_output - raw_output) * dt;
        candidate_integral =
            fminf(fmaxf(candidate_integral, pid->config.integral_min), pid->config.integral_max);
    }
    if (!finite_value(candidate_integral) || !finite_value(limited_output)) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    pid->integral = candidate_integral;
    pid->previous_measurement = measurement;
    pid->previous_error = error;
    pid->output = limited_output;
    *output = limited_output;
    return FOUNDATION_STATUS_OK;
}
