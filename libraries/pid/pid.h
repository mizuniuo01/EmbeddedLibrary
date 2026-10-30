#ifndef MIZU_PID_H
#define MIZU_PID_H

#include <stdbool.h>

#include "mizu_status.h"

typedef enum {
    MIZU_PID_ANTI_WINDUP_CONDITIONAL = 0,
    MIZU_PID_ANTI_WINDUP_BACK_CALCULATION
} mizu_pid_anti_windup_t;

typedef struct {
    float kp;
    float ki;
    float kd;
    float output_min;
    float output_max;
    float integral_min;
    float integral_max;
    float tracking_gain;
    mizu_pid_anti_windup_t anti_windup;
} mizu_pid_config_t;

/* 调用方拥有的 PID 实例；成员仅由组件维护，不得直接修改、复制或依赖布局。 */
typedef struct {
    mizu_pid_config_t config;
    float integral;
    float previous_measurement;
    float output;
    bool initialized;
    bool history_valid;
} mizu_pid_t;

/**
 * mizu_pid_init - 复制配置并清除 PID 动态状态。
 * @pid: 调用方拥有的 PID 实例，不要求预先清零。
 * @config: 调用方提供的初始化配置，函数只复制其内容。
 *
 * 可对已初始化实例重新调用；成功后旧配置和历史全部失效。函数不阻塞。
 * 返回值：成功返回 MIZU_STATUS_OK；空指针或配置结构错误返回 MIZU_STATUS_INVALID_ARGUMENT；
 * 配置包含非有限值时返回 MIZU_STATUS_INVALID_DATA；失败时对象保持不变。
 */
mizu_status_t mizu_pid_init(mizu_pid_t *pid, const mizu_pid_config_t *config);

/**
 * mizu_pid_reset - 清除积分和输出历史并设置当前测量值。
 * @pid: 已成功初始化的 PID 实例。
 * @measurement: 当前有限测量值，作为下一次微分计算的历史值。
 *
 * 返回值：成功返回 MIZU_STATUS_OK；空指针返回 MIZU_STATUS_INVALID_ARGUMENT；未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED；测量值非有限时返回 MIZU_STATUS_INVALID_DATA，失败时状态不变。
 */
mizu_status_t mizu_pid_reset(mizu_pid_t *pid, float measurement);

/**
 * mizu_pid_calculate - 按显式时间步长执行一次 PID 计算。
 * @pid: 已成功初始化的 PID 实例。
 * @setpoint: 有限目标值。
 * @measurement: 有限测量值。
 * @dt: 大于零的有限时间步长。
 * @output: 成功时接收限幅后的输出值，必须指向独立存储，不得与 @pid 或其成员重叠。
 *
 * 导数固定作用于 measurement；首次未 reset 时导数按零处理。输入或中间计算失败时，
 * PID 状态和输出参数保持不变。组件不检测指针重叠；函数不阻塞，同一实例不得并发或重入使用。
 * 返回值：成功返回 MIZU_STATUS_OK；输出为空返回 MIZU_STATUS_INVALID_ARGUMENT；未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED；非有限输入或非正 dt 返回 MIZU_STATUS_INVALID_DATA；中间计算非有限
 * 返回 MIZU_STATUS_OVERFLOW。
 */
mizu_status_t mizu_pid_calculate(mizu_pid_t *pid, float setpoint, float measurement, float dt,
                                 float *output);

#endif /* MIZU_PID_H */
