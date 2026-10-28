#ifndef CASCADED_PID_H
#define CASCADED_PID_H /* 头文件保护 */

#include <stdbool.h>

#include "pid.h"

/* 两级串级 PID 的非拥有配置。 */
typedef struct {
    pid_f32_t *outer_pid;
    pid_f32_t *inner_pid;
    float intermediate_min;
    float intermediate_max;
} cascaded_pid_f32_config_t;

/* 一次串级计算的完整输入。 */
typedef struct {
    float outer_setpoint;
    float outer_measurement;
    float inner_measurement;
    float dt;
} cascaded_pid_f32_input_t;

/* 串级实例；子 PID 和存储均由调用方拥有。 */
typedef struct {
    pid_f32_t *outer_pid;
    pid_f32_t *inner_pid;
    float intermediate_min;
    float intermediate_max;
    float intermediate_output;
    float output;
    bool initialized;
} cascaded_pid_f32_t;

foundation_status_t cascaded_pid_f32_init(cascaded_pid_f32_t *controller,
    const cascaded_pid_f32_config_t *config);
foundation_status_t cascaded_pid_f32_reset(cascaded_pid_f32_t *controller, float outer_measurement,
    float outer_error, float inner_measurement, float inner_error);
foundation_status_t cascaded_pid_f32_calculate(cascaded_pid_f32_t *controller,
    const cascaded_pid_f32_input_t *input, float *output);
foundation_status_t cascaded_pid_f32_get_intermediate(const cascaded_pid_f32_t *controller,
    float *output);
foundation_status_t cascaded_pid_f32_get_output(const cascaded_pid_f32_t *controller,
    float *output);

#endif /* CASCADED_PID_H */
