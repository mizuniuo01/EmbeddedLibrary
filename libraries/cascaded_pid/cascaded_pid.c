/* Libraries 层的固定两级串级 PID 组合器；不包含项目控制语义。 */

#include "cascaded_pid.h"

#include <stddef.h>

static mizu_status_t mizu_cascaded_pid_check(const mizu_cascaded_pid_t *controller)
{
    if (controller == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!controller->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_cascaded_pid_init(mizu_cascaded_pid_t *controller,
                                     const mizu_cascaded_pid_config_t *config)
{
    mizu_cascaded_pid_t candidate = {0};
    mizu_status_t status;

    if ((controller == NULL) || (config == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    status = mizu_pid_init(&candidate.outer_pid, &config->outer_config);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    status = mizu_pid_init(&candidate.inner_pid, &config->inner_config);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    candidate.initialized = true;
    *controller = candidate;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_cascaded_pid_reset(mizu_cascaded_pid_t *controller, float outer_measurement,
                                      float inner_measurement)
{
    mizu_cascaded_pid_t candidate;
    mizu_status_t status = mizu_cascaded_pid_check(controller);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    candidate = *controller;
    status = mizu_pid_reset(&candidate.outer_pid, outer_measurement);
    if (status == MIZU_STATUS_OK) {
        status = mizu_pid_reset(&candidate.inner_pid, inner_measurement);
    }
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    *controller = candidate;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_cascaded_pid_calculate(mizu_cascaded_pid_t *controller,
                                          const mizu_cascaded_pid_input_t *input, float *output)
{
    mizu_cascaded_pid_t candidate;
    mizu_status_t status = mizu_cascaded_pid_check(controller);
    float intermediate;
    float final_output;

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if ((input == NULL) || (output == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }

    candidate = *controller;
    status = mizu_pid_calculate(&candidate.outer_pid, input->outer_setpoint,
                                input->outer_measurement, input->dt, &intermediate);
    if (status == MIZU_STATUS_OK) {
        status = mizu_pid_calculate(&candidate.inner_pid, intermediate, input->inner_measurement,
                                    input->dt, &final_output);
    }
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    *controller = candidate;
    *output = final_output;
    return MIZU_STATUS_OK;
}
