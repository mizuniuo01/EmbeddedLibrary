# mizu_pid 组件说明

`mizu_pid` 属于 Libraries 层，只负责平台无关的单精度 PID 计算。调用方提供目标值、测量值和时间步长；组件不读取时间、不访问硬件、不依赖调度器或 RTOS。

## 所有权和生命周期

- PID 对象和配置由调用方提供，组件复制配置，不保存配置指针。
- PID 对象支持静态或长期存活的局部分配；项目不使用动态内存。成员不得直接修改、复制或依赖布局，这是 C 语言静态分配下的模块约束。
- `mizu_pid_init()` 可以重复调用；成功后覆盖旧配置并清除动态状态。
- 控制参数变化应由 owner 在安全边界构造完整的新配置，再通过一次 `init()` 切换；不提供运行期逐字段 setter，也不建议直接修改实例内的配置成员。
- `mizu_pid_reset()` 和 `mizu_pid_calculate()` 只能在 `init()` 成功后调用；调用方不得在未初始化对象上调用其他 API。
- `mizu_pid_reset()` 只清除积分和输出，并把当前测量值设为微分历史，不修改当前配置。
- 同一实例不得并发或重入使用；组件不提供同步。
- `mizu_pid_calculate()` 的输出缓冲区必须独立于 PID 对象及其成员；组件不检测重叠。
- 失败操作不提交对象状态，也不修改输出参数。

## 典型调用流程

```text
Composition Root/Application 创建 PID 对象和配置
        ↓
mizu_pid_init()
        ↓
可选：mizu_pid_reset(当前测量值)
        ↓
周期性读取 setpoint、measurement 和 dt
        ↓
mizu_pid_calculate()
        ↓
调用方应用 output 或处理错误状态
```

`mizu_pid` 不拥有 setpoint、measurement、`dt` 或执行器输出；这些数据由调用方在每次计算时提供。`dt` 的来源、单位一致性和最大调用间隔由调用方保证，PID 只验证它是有限正数。

## 在线调参的一致性

PID 的配置必须由控制对象的 owner 统一持有。Parameter、菜单或其他调试入口修改参数时，应先构造并校验完整的新配置，再在同一提交操作中应用到正在运行的 PID 实例；只有活动控制器应用成功后，setter 才能返回成功，getter 也必须读取这份已生效配置。不能只修改显示或配置副本而让活动 PID 继续使用旧参数。应用失败时，权威配置、活动 PID 和对外显示值保持旧状态。

基础 PID 不定义在线调参实验的可比性策略。保留积分/微分历史、清除历史后重新开始、无扰切换以及机器物理初始状态的统一，属于 Domain/Application 的调参流程；`mizu_pid_init()` 和 `mizu_pid_reset()` 不承诺不同调参试验之间具有相同的物理初始条件。

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
- `INVALID_DATA` 或 `OVERFLOW` 不会自动锁定 PID；调用方负责决定记录、降级、停机或进入项目安全状态。
