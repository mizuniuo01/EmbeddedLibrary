# mizu_cascaded_pid 组件说明

`mizu_cascaded_pid` 属于 Libraries 层，只负责固定两级 PID 的通用组合。它不包含航向角、差速、左右轮、电机、传感器或项目模式语义。

## 所有权和生命周期

- 串级对象内部拥有外环和内环两个 `mizu_pid_t`，调用方只提供一个串级对象和两份配置。
- `mizu_cascaded_pid_init()` 可以重复调用；成功后两级配置和动态历史全部替换。
- `mizu_cascaded_pid_reset()` 同时设置两级测量历史并清除两级积分和输出。
- 同一实例不得并发或重入使用；组件不提供同步。

## 计算语义

- 外环接收 `outer_setpoint` 和 `outer_measurement`。
- 外环 PID 的最终输出直接作为内环目标；不设置第二套级间限幅。
- 内环接收外环输出和 `inner_measurement`，产生最终输出。
- 两级使用同一个 `dt`。
- 输入和输出结构由调用方拥有，输出必须与串级对象及其成员独立。
- 计算使用两个局部 PID 副本；任一级失败时原对象和输出保持不变。

## 错误结果

- 串级自身参数错误返回 `MIZU_STATUS_INVALID_ARGUMENT`。
- 未初始化返回 `MIZU_STATUS_NOT_INITIALIZED`。
- 子 PID 的错误状态原样向上传播。
