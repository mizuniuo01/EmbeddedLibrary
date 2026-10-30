# mizu_pid 组件说明

`mizu_pid` 属于 Libraries 层，只负责平台无关的单精度 PID 计算。调用方提供目标值、测量值和时间步长；组件不读取时间、不访问硬件、不依赖调度器或 RTOS。

## 所有权和生命周期

- PID 对象和配置由调用方提供，组件复制配置，不保存配置指针。
- PID 对象支持静态或局部分配；成员不得直接修改、复制或依赖布局，这是 C 语言静态分配下的模块约束。
- `mizu_pid_init()` 可以重复调用；成功后覆盖旧配置并清除动态状态。
- `mizu_pid_reset()` 和 `mizu_pid_calculate()` 只能在 `init()` 成功后调用；调用方不得在未初始化对象上调用其他 API。
- `mizu_pid_reset()` 清除积分和输出，并把当前测量值设为微分历史。
- 同一实例不得并发或重入使用；组件不提供同步。
- `mizu_pid_calculate()` 的输出缓冲区必须独立于 PID 对象及其成员；组件不检测重叠。
- 失败操作不提交对象状态，也不修改输出参数。

## 参数和计算

- `kp`、`ki`、`kd` 必须为有限值，允许负值和零。
- 输出限幅和积分限幅由调用方分别提供最小值和最大值。
- 导数固定作用于测量值，避免目标值阶跃造成微分冲击。
- `dt` 每次计算显式提供，必须为有限正数。
- 抗饱和策略可选条件积分或 back-calculation，两者不叠加。
- `tracking_gain` 由调用方配置；条件积分模式不使用它。

## 错误结果

- 未初始化返回 `MIZU_STATUS_NOT_INITIALIZED`。
- 空指针或组合参数非法返回 `MIZU_STATUS_INVALID_ARGUMENT`。
- 反向限幅、非法抗饱和枚举或负 `tracking_gain` 返回 `MIZU_STATUS_INVALID_ARGUMENT`。
- 非有限输入或非法数值返回 `MIZU_STATUS_INVALID_DATA`。
- 中间计算超出有限浮点范围返回 `MIZU_STATUS_OVERFLOW`。
