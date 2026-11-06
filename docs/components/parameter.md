# mizu_parameter_service 组件说明

`mizu_parameter_service` 属于 Services 层，是调试菜单的参数后端。它不依赖菜单或命令，也不拥有业务变量。

## 协议和所有权

- 调用方在装配阶段提供静态分组、描述符、字符串、getter/setter 和 context。
- 服务只借用这些对象，不复制、不释放、不保存业务值副本。
- getter 必须存在；只读参数没有 setter；可写参数必须有 setter。
- setter 负责实际参数校验、原子应用和跨字段一致性。
- 服务只在主循环或普通任务上下文串行调用，不提供锁或 ISR 安全。
- `init()` 只调用一次，成功后注册表在整个服务生命周期内保持不变；已初始化对象再次 init 应返回 `MIZU_STATUS_INVALID_STATE` 并保持旧注册表。当前实现尚待修复，见 `REVIEW-S5-001`。

## 参数模型

- 当前支持 `float`、`int32`、`uint32` 和 `bool`。
- 每个参数拥有稳定 ID、名称、单位、类型、范围、默认步长、显示小数位数和读写权限。
- 参数按静态分组组织；分组只负责名称、稳定 ID 和枚举顺序，不表达业务权限或模式。
- 菜单通过分组视图获取元数据，通过参数稳定 ID 读写和调整。
- `decimals` 规定 float 的显示小数位数和运行期最小步长，范围为 0～4；整数和 bool 必须为 0。
- 文本格式化和编码仍由菜单输出适配器负责，parameter 只提供强类型值和精度元数据。

## 调整和错误

- 数值参数按步长调整并钳制到范围；bool 的正步数设为 true，负步数设为 false，零步数不改变。
- 初始化会读取并校验每个参数当前值。
- getter/setter 返回的状态原样向上传播。
- getter 每次直接读取业务 owner 的已生效配置，不使用 service 当前值缓存；getter 必须同步、轻量且非阻塞。实时传感器或监测数据不注册为 parameter。
- setter 成功返回前必须已经把新值应用到真实业务对象或活动控制器；只更新显示副本不构成成功。
- parameter service 只执行单参数的类型、范围、步长和权限校验。跨字段整体校验或事务提交只在业务 owner 存在实际关联约束时由 owner 实现，不作为 service 的统一要求。
- `get_descriptor_by_id()` 返回注册表中的只读借用描述符。
- `adjust_value()` 和 `scale_step()` 是无状态纯变换：不读取或提交业务参数，也不拥有菜单草稿。
- `adjust_value()` 的类型或步长组合错误返回 `MIZU_STATUS_INVALID_ARGUMENT`，当前值越界返回
  `MIZU_STATUS_OUT_OF_RANGE`；当前值或步长为 NaN/Inf 返回 `MIZU_STATUS_INVALID_DATA`，失败时输出不变。
- `scale_step()` 只接受与描述符类型一致且处于合法步长范围内的运行期步长；浮点步长还必须有限，
  不合法输入被拒绝且不修改输出。
- 参数服务自身使用 `MIZU_STATUS_INVALID_ARGUMENT`、`MIZU_STATUS_INVALID_DATA`、
  `MIZU_STATUS_INVALID_STATE`、`MIZU_STATUS_NOT_INITIALIZED` 和 `MIZU_STATUS_OUT_OF_RANGE`。
