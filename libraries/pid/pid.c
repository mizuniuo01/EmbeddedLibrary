/* Libraries 层的单精度 PID 控制器；不依赖硬件、时间源、调度器或 RTOS。 */

#include "pid.h"

#include <math.h>
#include <stddef.h>

static bool mizu_pid_is_finite(float value)
{
    return isfinite(value) != 0;
}

static mizu_status_t mizu_pid_validate_config(const mizu_pid_config_t *config)
{
    if (config == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!mizu_pid_is_finite(config->kp) || !mizu_pid_is_finite(config->ki) ||
        !mizu_pid_is_finite(config->kd) || !mizu_pid_is_finite(config->output_min) ||
        !mizu_pid_is_finite(config->output_max) || !mizu_pid_is_finite(config->integral_min) ||
        !mizu_pid_is_finite(config->integral_max) || !mizu_pid_is_finite(config->tracking_gain)) {
        return MIZU_STATUS_INVALID_DATA;
    }
    if ((config->output_min > config->output_max) ||
        (config->integral_min > config->integral_max) || (config->tracking_gain < 0.0F) ||
        ((config->anti_windup != MIZU_PID_ANTI_WINDUP_CONDITIONAL) &&
         (config->anti_windup != MIZU_PID_ANTI_WINDUP_BACK_CALCULATION))) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    return MIZU_STATUS_OK;
}

static mizu_status_t mizu_pid_check(const mizu_pid_t *pid)
{
    if (pid == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!pid->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    if ((mizu_pid_validate_config(&pid->config) != MIZU_STATUS_OK) ||
        !mizu_pid_is_finite(pid->integral) || !mizu_pid_is_finite(pid->previous_measurement) ||
        !mizu_pid_is_finite(pid->output)) {
        return MIZU_STATUS_INVALID_STATE;
    }
    return MIZU_STATUS_OK;
}

static float mizu_pid_clamp(float value, float minimum, float maximum)
{
    return fminf(fmaxf(value, minimum), maximum);
}

static float mizu_pid_raw_output(const mizu_pid_config_t *config, float error, float integral,
                                 float derivative)
{
    return config->kp * error + config->ki * integral + config->kd * derivative;
}

mizu_status_t mizu_pid_init(mizu_pid_t *pid, const mizu_pid_config_t *config)
{
    if (pid == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (config == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    {
        const mizu_status_t status = mizu_pid_validate_config(config);

        if (status != MIZU_STATUS_OK) {
            return status;
        }
    }

    pid->config = *config;
    pid->integral = 0.0F;
    pid->previous_measurement = 0.0F;
    pid->output = 0.0F;
    pid->initialized = true;
    pid->history_valid = false;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_pid_reset(mizu_pid_t *pid, float measurement)
{
    const mizu_status_t status = mizu_pid_check(pid);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (!mizu_pid_is_finite(measurement)) {
        return MIZU_STATUS_INVALID_DATA;
    }

    pid->integral = 0.0F;
    pid->previous_measurement = measurement;
    pid->output = 0.0F;
    pid->history_valid = true;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_pid_calculate(mizu_pid_t *pid, float setpoint, float measurement, float dt,
                                 float *output)
{
    const mizu_status_t status = mizu_pid_check(pid);
    const mizu_pid_config_t *config;
    float error;
    float derivative;
    float integral_candidate;
    float raw_output;
    float limited_output;
    float corrected_integral;

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (output == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!mizu_pid_is_finite(setpoint) || !mizu_pid_is_finite(measurement) ||
        !mizu_pid_is_finite(dt) || (dt <= 0.0F)) {
        return MIZU_STATUS_INVALID_DATA;
    }

    config = &pid->config;
    error = setpoint - measurement;
    derivative = pid->history_valid ? -(measurement - pid->previous_measurement) / dt : 0.0F;
    integral_candidate = pid->integral + error * dt;
    if (!mizu_pid_is_finite(error) || !mizu_pid_is_finite(derivative) ||
        !mizu_pid_is_finite(integral_candidate)) {
        return MIZU_STATUS_OVERFLOW;
    }
    integral_candidate =
        mizu_pid_clamp(integral_candidate, config->integral_min, config->integral_max);
    raw_output = mizu_pid_raw_output(config, error, integral_candidate, derivative);
    if (!mizu_pid_is_finite(raw_output)) {
        return MIZU_STATUS_OVERFLOW;
    }

    if (config->anti_windup == MIZU_PID_ANTI_WINDUP_CONDITIONAL &&
        (((raw_output > config->output_max) && (config->ki * error > 0.0F)) ||
         ((raw_output < config->output_min) && (config->ki * error < 0.0F)))) {
        integral_candidate = pid->integral;
        raw_output = mizu_pid_raw_output(config, error, integral_candidate, derivative);
        if (!mizu_pid_is_finite(raw_output)) {
            return MIZU_STATUS_OVERFLOW;
        }
    }

    limited_output = mizu_pid_clamp(raw_output, config->output_min, config->output_max);
    corrected_integral = integral_candidate;
    if (config->anti_windup == MIZU_PID_ANTI_WINDUP_BACK_CALCULATION) {
        corrected_integral += config->tracking_gain * (limited_output - raw_output) * dt;
        if (!mizu_pid_is_finite(corrected_integral)) {
            return MIZU_STATUS_OVERFLOW;
        }
        corrected_integral =
            mizu_pid_clamp(corrected_integral, config->integral_min, config->integral_max);
        raw_output = mizu_pid_raw_output(config, error, corrected_integral, derivative);
        if (!mizu_pid_is_finite(raw_output)) {
            return MIZU_STATUS_OVERFLOW;
        }
        limited_output = mizu_pid_clamp(raw_output, config->output_min, config->output_max);
    }

    if (!mizu_pid_is_finite(limited_output)) {
        return MIZU_STATUS_OVERFLOW;
    }
    pid->integral = corrected_integral;
    pid->previous_measurement = measurement;
    pid->output = limited_output;
    pid->history_valid = true;
    *output = limited_output;
    return MIZU_STATUS_OK;
}
