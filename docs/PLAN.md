# EmbeddedLibrary 总体建设计划

> 计划状态：P1 第一、第二批已完成；第三批本地实现与验证完成，等待远端 CI
>
> 规范基线：架构规范 v1.3.0、代码风格规范 v1.2.0、仓库工作流规范 v1.0.0

## 1. 计划使用规则

本文件记录总体方向、阶段边界、依赖顺序、完成标准和状态，不替代阶段性设计。

1. 只有用户明确指定阶段编号或名称时，才进入该阶段。
2. 阶段开始后，先进行独立的阶段性 Plan，冻结 API、所有权、生命周期、并发、错误、资源预算、测试用例和验收条件。
3. 阶段性 Plan 未确认前，不修改生产代码或测试代码。
4. 阶段性 Plan 确认后，才实现该阶段并执行验证。
5. 阶段完成后，更新本文件的状态、证据、成熟度和遗留风险。
6. 未经用户手动开启，不自动进入下一个阶段。

状态：`未开始`、`规划中`、`进行中`、`已完成`、`阻塞`。

## 2. 总体边界

本仓库是可复用嵌入式 C 组件储存库，不是某一个具体产品固件工程。

纳入范围：与硬件、板卡、厂商 SDK 和具体业务无关的 Libraries 与 Services；可通过最小端口适配裸机、FreeRTOS 和其他运行环境的公共能力；主机测试、替身、构建检查和架构文档。

暂不直接纳入：具体 BSP、MCU 外设 Driver、厂商 HAL、寄存器和 DMA 实现；参考工程中的 Application、Domain、控制模式和产品参数；具体 OLED、蓝牙芯片、UART 外设和固件示例。

## 3. 仓库组织

采用组件包优先结构：

```text
libraries/<component>/{include,src}
services/<component>/{include,src}
adapters/freertos/{include,src}
tests/{unit,integration,adapters,fixtures}
docs/PLAN.md
docs/components/<component>.md
```

每个组件必须有公共头文件、实现、独立构建目标、主机测试和组件设计说明。端口由需要能力的组件定义，适配器实现端口；RTOS 类型不得泄漏到平台无关核心。

## 4. 阶段路线

### P0：治理与构建基础 — 已完成

建立 README、CMake/CTest 主机入口、Presets、格式检查、严格告警、静态分析入口，以及组件命名、目录、构建目标、测试命名、成熟度和文档模板。不实现公共组件。

重新验证原因：首次 P0 CI 的 GCC/Clang 任务因缺少 `clang-format-21` 未进入构建；原
`host-sanitize` Preset 只设置了未被 CMake 使用的变量，没有实际启用 ASan/UBSan。修复后须由
GCC 13、Clang 21、真实 ASan/UBSan 和 CMake 3.20.6 四项 CI 验证共同闭环。P0 未加入公共组件、
BSP、Driver 或 RTOS 实现。

本地修复验证：`sh tools/check.sh host-gcc`、`sh tools/check.sh host-clang` 和
`sh tools/check.sh host-sanitize` 均通过；Sanitizer 编译/链接命令包含 ASan/UBSan，
`repository.sanitizer_probe` 通过；使用 Kitware CMake 3.20.6 的临时工具链构建通过。GitHub
Actions 运行 `33846771581` 中的 `host (gcc)`、`host (clang)`、`sanitizer` 和 `cmake-minimum`
四项任务均成功，P0 验证闭环完成。

### P1：基础类型、时间和数据结构 — 进行中

实现稳定状态码、单调时间与 tick 回绕计算、限幅和有限值检查、字节 FIFO、固定元素队列、对象池、双缓冲快照、CRC、显式大小端编解码及必要的 COBS/SLIP 等基础能力。

阶段设计与批次证据见 [docs/plans/P1_FOUNDATION.md](plans/P1_FOUNDATION.md)。第一、第二批已完成，
其远端 CI 运行 `33955779208` 的五项任务均成功。
第三批有界容器已实现，完成本地 GCC/Clang、Sanitizer、静态分析及 ARM compile-only 验证，
等待提交后的远端五项 CI（含 CMake 3.20.6）确认。P1 最终收尾仍须用户手动推进。

### P2：控制与算法库 — 未开始

实现通用 PID、通用串级/组合控制器、一阶滤波、滑动平均、迟滞、死区和速率限制。算法不包含航向、左右轮、滚球等项目语义，全部支持多实例、静态分配和主机测试。

### P3：并发端口与 FreeRTOS 适配 — 未开始

定义时间、事件、队列、锁、临界区和任务通知端口，区分普通上下文与 ISR-safe 操作，提供裸机/主机替身和 FreeRTOS 适配器。FreeRTOS 具体版本由首个新项目确定后锁定，适配器记录最低支持版本、当前验证版本和配置要求。未经用户开启 P3，不实现适配器代码。

### P4：运行时 Services — 未开始

实现时间服务、协作式调度器、事件队列/必要的发布订阅、错误与故障服务、日志服务、任务健康/看门狗决策、参数服务和设置/存储 Repository 端口。

### P5：通信与协议 Services — 未开始

按“字节传输 → RX/TX 缓冲 → 帧解析 → 协议会话 → 命令/数据模型”的边界实现传输、背压、长度/版本/CRC/超时、会话、序号、有限重试、命令路由、文本编码器和权限/速率限制接口。

### P6：调试、诊断、显示、菜单和蓝牙协议 — 未开始

实现诊断快照与 Telemetry、调试输出/显示 sink、菜单状态机和蓝牙协议适配。显示不得依赖蓝牙；蓝牙只是一种可替换的传输/协议/诊断输出适配器。OLED 字模和具体屏幕 Driver 不纳入。

### P7：条件能力与后续储备 — 未开始

实现可主机验证的安全输出门控、恢复检查点、固件镜像元数据、低功耗/时钟状态机、DMA/cache/多核同步契约和多板变体构建说明。Bootloader、cache 维护、功耗控制和 HIL 实现留给实际项目。

## 5. 参考工程映射

- `pid`：重写后进入 Libraries；串级 PID 只保留通用组合思想。
- `scheduler_service`、`parameter_service`、`error_service`、`command_service`：按实例、端口、静态容量和明确错误契约重写。
- `driver_uart_stream`：拆成 FIFO、字节传输和平台适配三部分。
- `bluetooth_service`、`display`：拆成传输、协议、命令、诊断输出和显示 sink。
- `menu_service`：保留交互目标，重写为硬件无关菜单状态机。
- `motion_control`、`ball_controller`、`control_mode_manager` 以及其他 App/Domain、BSP、Drivers 和 OLED 字模不直接迁入。

参考工程只提供候选内容和问题样本；当前规范是唯一标准。

## 6. 阶段完成门槛

每个阶段必须依次完成：阶段性 Plan 并经用户确认；组件设计说明和公共 API；实现与构建目标；正常、边界、错误、超时、容量和并发测试；格式、严格告警、静态分析和依赖检查；资源、生命周期、所有权、执行上下文和恢复说明；最后更新本文件状态和验证证据。

成熟度使用：`host-tested`、`rtos-tested`、`hardware-unverified`。未经过目标板验证的代码不得宣称硬件成熟度。

## 7. RTOS 版本策略

- 公共组件版本与 FreeRTOS 版本独立管理；
- 核心代码不使用 FreeRTOS 版本宏或条件分支；
- 版本差异只存在于适配器，并记录最低支持版本、当前验证版本和配置 profile；
- 具体版本在首个新项目确定后锁定；
- 兼容性扩展更新适配器，接口或语义不兼容时提升适配器主版本。

## 8. 变更记录

| 日期 | 变更 | 原因 |
| --- | --- | --- |
| 2026-09-03 | 建立总体路线和手动阶段推进规则 | 明确公共组件、RTOS 和主机测试边界 |
