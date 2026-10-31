#ifndef MIZU_CASCADED_PID_H
#define MIZU_CASCADED_PID_H

#include <stdbool.h>

#include "pid.h"

typedef struct {
    mizu_pid_config_t outer_config;
    mizu_pid_config_t inner_config;
} mizu_cascaded_pid_config_t;

typedef struct {
    float outer_setpoint;
    float outer_measurement;
    float inner_measurement;
    float dt;
} mizu_cascaded_pid_input_t;

/* 调用方拥有的两级 PID 实例；成员仅由组件维护，不得直接修改或依赖布局。 */
typedef struct {
    mizu_pid_t outer_pid;
    mizu_pid_t inner_pid;
    bool initialized;
} mizu_cascaded_pid_t;

/**
 * mizu_cascaded_pid_init - 初始化内部外环和内环 PID。
 * @controller: 调用方拥有的串级实例，不要求预先清零。
 * @config: 两级 PID 配置；函数复制配置，不保存其地址。
 *
 * 可对已初始化实例重新调用；成功后两级历史全部清除。函数不阻塞。
 * 返回值：成功返回 MIZU_STATUS_OK；空指针或子 PID 配置结构错误返回
 * MIZU_STATUS_INVALID_ARGUMENT；配置数值非法返回 MIZU_STATUS_INVALID_DATA；失败时对象不变。
 */
mizu_status_t mizu_cascaded_pid_init(mizu_cascaded_pid_t *controller,
                                     const mizu_cascaded_pid_config_t *config);

/**
 * mizu_cascaded_pid_reset - 同时重置外环和内环的测量历史。
 * @controller: 已成功初始化的串级实例。
 * @outer_measurement: 外环当前测量值。
 * @inner_measurement: 内环当前测量值。
 *
 * 返回值：成功返回 MIZU_STATUS_OK；参数、状态或测量值无效时返回对应状态，失败时对象不变。
 */
mizu_status_t mizu_cascaded_pid_reset(mizu_cascaded_pid_t *controller, float outer_measurement,
                                      float inner_measurement);

/**
 * mizu_cascaded_pid_calculate - 执行一次外环到内环的串级计算。
 * @controller: 已成功初始化的串级实例。
 * @input: 外环目标、两级测量值和共用时间步长。
 * @output: 成功时接收内环最终输出，必须与 @controller 及其成员独立。
 *
 * 外环最终输出直接作为内环目标；两级使用同一个 dt。组件不检测输出指针重叠。
 * 计算在两个局部 PID 副本上进行，任一级失败时原对象和输出保持不变。函数不阻塞，
 * 同一实例不得并发或重入使用。
 * 返回值：成功返回 MIZU_STATUS_OK；输入为空返回 MIZU_STATUS_INVALID_ARGUMENT；未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED；子 PID 的其他错误状态原样传播。
 */
mizu_status_t mizu_cascaded_pid_calculate(mizu_cascaded_pid_t *controller,
                                          const mizu_cascaded_pid_input_t *input, float *output);

#endif /* MIZU_CASCADED_PID_H */
