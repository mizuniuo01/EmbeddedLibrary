# mizu_command_service 组件说明

`mizu_command_service` 属于 Services 层，是菜单调试体系中的静态动作兼容层。它不依赖菜单、参数服务、蓝牙、串口或具体业务模块。

## 协议和所有权

- 调用方提供静态命令分组、名称、handler 和 context。
- 服务只借用注册表、字符串和 context，不复制、不释放。
- handler 只接收 context，不接收运行时参数；具体动作由调用方注册。
- 参数调整必须由 parameter 服务负责，不得通过 command 内置参数操作绕行。
- handler 必须非阻塞、执行时间有界；command 不提供锁、ISR 安全或 RTOS 同步。
- 服务只能在主循环或普通任务上下文串行调用；同一实例不得并发或重入使用。

## 生命周期和执行

- `init()` 在装配阶段只调用一次；已初始化对象再次 init 应返回 `MIZU_STATUS_INVALID_STATE` 并保持旧命令表和快照。当前实现尚待修复，见 `REVIEW-S6-001`。不支持运行期注册、注销或替换。
- `execute()` 按稳定命令 ID 同步调用一次 handler。
- handler 返回 `BUSY` 时原样返回并记录；command 不自动轮询或保存活动命令。
- `MIZU_STATUS_BUSY` 表示动作尚未完成，不表示 command 服务正在阻塞等待。
- 未注册 ID 返回 `MIZU_STATUS_OUT_OF_RANGE`，并同样记录到最近结果快照。
- 最近快照只保存命令 ID、状态、序号和是否已有结果。
- `get_descriptor_by_id()` 供菜单等调用方按稳定 ID 借用只读命令描述符。
- TSan 或主机测试通过不代表 command 支持并发调用。

## 菜单关系

S8 菜单服务通过命令分组枚举名称和顺序，在用户确认后调用 `execute()`；菜单负责显示 `OK`、`BUSY` 或错误反馈。command 不处理菜单导航或显示。
