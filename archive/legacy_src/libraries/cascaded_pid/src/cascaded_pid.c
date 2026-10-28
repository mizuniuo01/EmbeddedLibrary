/**
 * @file cascaded_pid.c
 * @brief 固定两级、事务式的单精度串级 PID 组合器。
 */
#include "cascaded_pid.h"

#include <stddef.h>

#include "numeric.h"

/**
 * @brief 校验串级实例及其级间边界。
 * @param controller 串级控制器。
 * @return 状态有效时返回 OK，否则返回具体状态。
 */
static foundation_status_t check_controller(const cascaded_pid_f32_t *controller)
{
    if (controller == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!controller->initialized) {
        return FOUNDATION_STATUS_NOT_INITIALIZED;
    }
    if ((controller->outer_pid == NULL) || (controller->inner_pid == NULL)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    if (!numeric_f32_is_finite(controller->intermediate_min) ||
        !numeric_f32_is_finite(controller->intermediate_max) ||
        (controller->intermediate_min > controller->intermediate_max)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    if (!numeric_f32_is_finite(controller->intermediate_output) ||
        !numeric_f32_is_finite(controller->output)) {
        return FOUNDATION_STATUS_INVALID_STATE;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 初始化两级串级 PID。
 * @param controller 串级控制器实例。
 * @param config 非拥有配置。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或边界反向。
 * @retval FOUNDATION_STATUS_INVALID_DATA 边界不是有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 子 PID 尚未初始化。
 */
foundation_status_t cascaded_pid_f32_init(cascaded_pid_f32_t *controller,
    const cascaded_pid_f32_config_t *config)
{
    foundation_status_t outer_status;
    foundation_status_t inner_status;

    if ((controller == NULL) || (config == NULL) || (config->outer_pid == NULL) ||
        (config->inner_pid == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!numeric_f32_is_finite(config->intermediate_min) ||
        !numeric_f32_is_finite(config->intermediate_max)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    if (config->intermediate_min > config->intermediate_max) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    outer_status = pid_f32_snapshot(config->outer_pid, &(pid_f32_snapshot_t){0});
    inner_status = pid_f32_snapshot(config->inner_pid, &(pid_f32_snapshot_t){0});
    if (outer_status != FOUNDATION_STATUS_OK) {
        return outer_status;
    }
    if (inner_status != FOUNDATION_STATUS_OK) {
        return inner_status;
    }
    *controller = (cascaded_pid_f32_t){
        .outer_pid = config->outer_pid,
        .inner_pid = config->inner_pid,
        .intermediate_min = config->intermediate_min,
        .intermediate_max = config->intermediate_max,
        .initialized = true,
    };
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 事务式重置外环和内环 PID。
 * @param controller 串级控制器实例。
 * @param outer_measurement 外环测量值。
 * @param outer_error 外环当前误差。
 * @param inner_measurement 内环测量值。
 * @param inner_error 内环当前误差。
 * @retval FOUNDATION_STATUS_OK 重置成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输入不是有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 控制器尚未初始化。
 */
foundation_status_t cascaded_pid_f32_reset(cascaded_pid_f32_t *controller, float outer_measurement,
    float outer_error, float inner_measurement, float inner_error)
{
    foundation_status_t status = check_controller(controller);
    pid_f32_snapshot_t outer_snapshot;
    pid_f32_snapshot_t inner_snapshot;

    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (!numeric_f32_is_finite(outer_measurement) || !numeric_f32_is_finite(outer_error) ||
        !numeric_f32_is_finite(inner_measurement) || !numeric_f32_is_finite(inner_error)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    status = pid_f32_snapshot(controller->outer_pid, &outer_snapshot);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    status = pid_f32_snapshot(controller->inner_pid, &inner_snapshot);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    status = pid_f32_reset(controller->outer_pid, outer_measurement, outer_error);
    if (status == FOUNDATION_STATUS_OK) {
        status = pid_f32_reset(controller->inner_pid, inner_measurement, inner_error);
    }
    if (status != FOUNDATION_STATUS_OK) {
        (void)pid_f32_restore(controller->outer_pid, &outer_snapshot);
        (void)pid_f32_restore(controller->inner_pid, &inner_snapshot);
    }
    return status;
}

/**
 * @brief 事务式执行两级串级 PID。
 * @param controller 串级控制器实例。
 * @param input 外环目标、两级测量值和时间步长。
 * @param output 成功时接收最终输出，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或输出覆盖控制器。
 * @retval FOUNDATION_STATUS_INVALID_DATA 输入不是有限值。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 控制器尚未初始化。
 * @retval FOUNDATION_STATUS_OVERFLOW 中间值不是有限值。
 * @retval FOUNDATION_STATUS_INVALID_STATE 子 PID 状态无效。
 */
foundation_status_t cascaded_pid_f32_calculate(cascaded_pid_f32_t *controller,
    const cascaded_pid_f32_input_t *input, float *output)
{
    foundation_status_t status = check_controller(controller);
    pid_f32_snapshot_t outer_snapshot;
    pid_f32_snapshot_t inner_snapshot;
    float intermediate;
    float final_output;

    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if ((input == NULL) || (output == NULL) || (output == &controller->intermediate_output) ||
        (output == &controller->output)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!numeric_f32_is_finite(input->outer_setpoint) ||
        !numeric_f32_is_finite(input->outer_measurement) ||
        !numeric_f32_is_finite(input->inner_measurement) || !numeric_f32_is_finite(input->dt) ||
        (input->dt <= 0.0F)) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    status = pid_f32_snapshot(controller->outer_pid, &outer_snapshot);
    if (status == FOUNDATION_STATUS_OK) {
        status = pid_f32_snapshot(controller->inner_pid, &inner_snapshot);
    }
    if (status == FOUNDATION_STATUS_OK) {
        status = pid_f32_calculate(controller->outer_pid, input->outer_setpoint,
            input->outer_measurement, input->dt, &intermediate);
    }
    if (status == FOUNDATION_STATUS_OK) {
        if (intermediate < controller->intermediate_min) {
            intermediate = controller->intermediate_min;
        } else if (intermediate > controller->intermediate_max) {
            intermediate = controller->intermediate_max;
        }
        status = pid_f32_calculate(controller->inner_pid, intermediate, input->inner_measurement,
            input->dt, &final_output);
    }
    if (status != FOUNDATION_STATUS_OK) {
        (void)pid_f32_restore(controller->outer_pid, &outer_snapshot);
        (void)pid_f32_restore(controller->inner_pid, &inner_snapshot);
        return status;
    }
    if (!numeric_f32_is_finite(intermediate) || !numeric_f32_is_finite(final_output)) {
        (void)pid_f32_restore(controller->outer_pid, &outer_snapshot);
        (void)pid_f32_restore(controller->inner_pid, &inner_snapshot);
        return FOUNDATION_STATUS_OVERFLOW;
    }
    controller->intermediate_output = intermediate;
    controller->output = final_output;
    *output = final_output;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询最近一次成功的级间目标。
 * @param controller 串级控制器实例。
 * @param output 成功时接收级间目标。
 * @retval FOUNDATION_STATUS_OK 查询成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 控制器尚未初始化。
 */
foundation_status_t cascaded_pid_f32_get_intermediate(const cascaded_pid_f32_t *controller,
    float *output)
{
    foundation_status_t status = check_controller(controller);
    if ((status != FOUNDATION_STATUS_OK) || (output == NULL)) {
        return (status != FOUNDATION_STATUS_OK) ? status : FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *output = controller->intermediate_output;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 查询最近一次成功的最终输出。
 * @param controller 串级控制器实例。
 * @param output 成功时接收最终输出。
 * @retval FOUNDATION_STATUS_OK 查询成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_NOT_INITIALIZED 控制器尚未初始化。
 */
foundation_status_t cascaded_pid_f32_get_output(const cascaded_pid_f32_t *controller, float *output)
{
    foundation_status_t status = check_controller(controller);
    if ((status != FOUNDATION_STATUS_OK) || (output == NULL)) {
        return (status != FOUNDATION_STATUS_OK) ? status : FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *output = controller->output;
    return FOUNDATION_STATUS_OK;
}
