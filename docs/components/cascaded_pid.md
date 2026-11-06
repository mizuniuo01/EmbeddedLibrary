# mizu_cascaded_pid 组件说明

`mizu_cascaded_pid` 属于 Libraries 层，只负责固定两级 PID 的通用组合。它不包含航向角、差速、左右轮、电机、传感器或项目模式语义。

## 所有权和生命周期

- 串级对象内部拥有外环和内环两个 `mizu_pid_t`，调用方只提供一个串级对象和两份配置。
- `mizu_cascaded_pid_init()` 可以重复调用；成功后两级配置和动态历史全部替换。
- `mizu_cascaded_pid_reset()` 同时设置两级测量历史并清除两级积分和输出。
- 同一实例不得并发或重入使用；组件不提供同步。

## 典型调用流程

```text
Composition Root/Application 创建串级对象和两级配置
        ↓
mizu_cascaded_pid_init()
        ↓
可选：mizu_cascaded_pid_reset(两级当前测量值)
        ↓
Domain 每周期组装 outer_setpoint、outer_measurement、inner_measurement、dt
        ↓
mizu_cascaded_pid_calculate()
        ↓
Domain/Application 应用最终内环输出或处理错误
```

串级对象内部拥有两个 `mizu_pid_t`。配置更新沿用基础 PID 的整体替换语义：构造完整新配置后重新调用 `init()`，成功后两级动态历史清零。在线调参是否保留历史、无扰切换或重新开始，由 Domain/Application 决定。

## 计算语义

- 外环接收 `outer_setpoint` 和 `outer_measurement`。
- 外环 PID 的最终输出直接作为内环目标；不设置第二套级间限幅。
- 内环接收外环输出和 `inner_measurement`，产生最终输出。
- 两级使用同一个 `dt`。
- 公共接口只返回内环最终输出；外环输出和内环目标不作为通用公共结果暴露，项目诊断如有需要由 Domain/Application 自行组织。
- 输入和输出结构由调用方拥有，输出必须与串级对象及其成员独立。
- 计算使用两个局部 PID 副本；任一级失败时原对象和输出保持不变。

外环和内环的具体业务含义由 Domain 决定。该组件不绑定位置、角度、速度、电机、差速或项目模式。

## 错误结果

- 串级自身参数错误返回 `MIZU_STATUS_INVALID_ARGUMENT`。
- 未初始化返回 `MIZU_STATUS_NOT_INITIALIZED`。
- 子 PID 的错误状态原样向上传播。
