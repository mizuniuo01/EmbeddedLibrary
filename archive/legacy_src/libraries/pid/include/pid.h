#ifndef PID_H
#define PID_H /* 头文件保护 */

#include <stdbool.h>

#include "foundation_status.h"

/* 导数输入来源。 */
typedef enum {
    PID_DERIVATIVE_ON_MEASUREMENT = 0, /* 对测量值求导，避免目标阶跃冲击。 */
    PID_DERIVATIVE_ON_ERROR = 1,       /* 对误差求导，兼容传统 PID 形式。 */
} pid_derivative_mode_t;

/* 积分抗饱和策略。 */
typedef enum {
    PID_ANTI_WINDUP_CONDITIONAL = 0, /* 饱和方向继续推动时暂停积分。 */
    PID_ANTI_WINDUP_BACK_CALCULATION = 1, /* 使用 tracking gain 回算积分状态。 */
} pid_anti_windup_mode_t;

/* PID 初始化配置；所有浮点值必须有限。 */
typedef struct {
    float kp;
    float ki;
    float kd;
    float output_min;
    float output_max;
    float integral_min;
    float integral_max;
    float tracking_gain;
    pid_derivative_mode_t derivative_mode;
    pid_anti_windup_mode_t anti_windup_mode;
} pid_f32_config_t;

/* PID 运行状态；字段由组件维护，初始化后禁止复制或直接修改。 */
typedef struct {
    pid_f32_config_t config;
    float integral;
    float previous_measurement;
    float previous_error;
    float output;
    bool initialized;
} pid_f32_t;

/* 受控事务快照；只能通过 snapshot/restore API 使用。 */
typedef struct {
    pid_f32_config_t config;
    float integral;
    float previous_measurement;
    float previous_error;
    float output;
    bool initialized;
} pid_f32_snapshot_t;

/* 生命周期和计算接口。 */
foundation_status_t pid_f32_init(pid_f32_t *pid, const pid_f32_config_t *config);
foundation_status_t pid_f32_reset(pid_f32_t *pid, float measurement, float error);
foundation_status_t pid_f32_calculate(pid_f32_t *pid, float setpoint, float measurement, float dt,
    float *output);
foundation_status_t pid_f32_snapshot(const pid_f32_t *pid, pid_f32_snapshot_t *snapshot);
foundation_status_t pid_f32_restore(pid_f32_t *pid, const pid_f32_snapshot_t *snapshot);

#endif /* PID_H */
