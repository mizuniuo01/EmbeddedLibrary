# mizu_parameter_service 组件说明

`mizu_parameter_service` 属于 Services 层，是调试菜单的参数后端。它不依赖菜单或命令，也不拥有业务变量。

## 协议和所有权

- 调用方在装配阶段提供静态分组、描述符、字符串、getter/setter 和 context。
- 服务只借用这些对象，不复制、不释放、不保存业务值副本。
- getter 必须存在；只读参数没有 setter；可写参数必须有 setter。
- setter 负责实际参数校验、原子应用和跨字段一致性。
- 服务只在主循环或普通任务上下文串行调用，不提供锁或 ISR 安全。
- `init()` 只调用一次，成功后注册表在整个服务生命周期内保持不变。

## 参数模型

- 当前支持 `float`、`int32`、`uint32` 和 `bool`。
- 每个参数拥有稳定 ID、名称、单位、类型、范围、步长和读写权限。
- 参数按静态分组组织；分组只负责名称、稳定 ID 和枚举顺序，不表达业务权限或模式。
- 菜单通过分组视图获取元数据，通过参数稳定 ID 读写和调整。
- float 显示格式不属于参数服务，由菜单或项目格式化层决定。

## 调整和错误

- 数值参数按步长调整并钳制到范围；bool 的正步数设为 true，负步数设为 false，零步数不改变。
- 初始化会读取并校验每个参数当前值。
- getter/setter 返回的状态原样向上传播。
- 参数服务自身使用 `MIZU_STATUS_INVALID_ARGUMENT`、`MIZU_STATUS_INVALID_DATA`、
  `MIZU_STATUS_INVALID_STATE`、`MIZU_STATUS_NOT_INITIALIZED` 和 `MIZU_STATUS_OUT_OF_RANGE`。
