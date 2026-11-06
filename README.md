# EmbeddedLibrary

面向 MCU 项目的 C11 通用组件储备库。这里保存可跨项目复用、可在主机环境验证、且不绑定
具体芯片、BSP、Driver、HAL、RTOS 或业务规则的 Libraries 和 Services 层代码。

本仓库不是一次性建设完整框架。组件只在存在明确复用价值、完成阶段性设计并经过验证后进入
主线；具体 Application、Domain、BSP、Driver、项目协议和产品页面留在实际项目中。

## 当前主线

S1～S8 已完成实现、使用方式复核和生命周期契约确认。

| 层级 | 组件 | 职责 |
| --- | --- | --- |
| Libraries | `mizu_status` | 当前 C API 的公共结果类型 |
| Libraries | `mizu_fifo` | 有界 SPSC 字节暂存，不负责同步或帧解析 |
| Libraries | `mizu_pid` | 显式时间步长的单精度 PID |
| Libraries | `mizu_cascaded_pid` | 固定两级 PID 组合 |
| Services | `scheduler` | 裸机协作式周期任务调度 |
| Services | `mizu_parameter_service` | 静态参数注册和 getter/setter 路由 |
| Services | `mizu_command_service` | 静态命令注册、同步执行和结果快照 |
| Services | `mizu_status_service` | 显式上报的当前错误状态记录 |
| Services | `mizu_menu` | 硬件无关的调试菜单状态机和语义视图 |

各组件的资源所有权、初始化、调用时序、并发边界和错误语义见
[组件说明](docs/components/)；S1～S8 的设计确认记录见
[使用与设计复核计划](docs/REVIEW_PLAN.md)。完整生产文件清单见
[FILE_CATALOG.md](docs/FILE_CATALOG.md)。

## 接入原则

- 组件不分配动态内存；对象、数组、缓存和 `context` 由调用方或明确的 owner 模块持有。
- 除 PID 和串级 PID 外，当前主线组件首次初始化前必须零初始化，且只允许一次成功初始化。
  重复初始化返回 `MIZU_STATUS_INVALID_STATE`，不会改变原有资源绑定或运行状态。
- Libraries 不包含项目规则或硬件访问；Services 只通过公共 API 和注入的 callback/context
  接入项目能力。
- 并发、ISR/DMA 交接、RTOS 同步、错误恢复和安全动作由实际项目的 owner、Domain 或
  Application 决定；组件不会隐式提供锁或硬件策略。
- 调试菜单只接收逻辑事件、提供语义视图；输入适配、渲染、蓝牙协议和实时展示数据由项目
  自行装配。

## 不在主线预建的内容

以下能力按实际项目的稳定重复需求单独立项，不作为通用空壳预先创建：

- BSP、Driver、HAL、寄存器访问、UART/DMA/蓝牙适配；
- FreeRTOS 任务、队列、锁和 ISR-safe 适配；
- 项目帧协议、通信组合、显示渲染、字库、日志和持久化；
- 仅有单一项目使用场景的小算法或数据结构。

CRC、字节序、IEEE754、COBS 等编码能力优先采用成熟库或项目现成实现。完整范围和采用原则见
[PLAN.md](docs/PLAN.md)。

## 构建与验证

主机验证基线为 Linux/WSL。需要 CMake 3.20+、Ninja、GCC 13、Clang 21、clang-format 21、
clang-tidy 21、ripgrep 和 cppcheck；ARM compile-only 还需要 `arm-none-eabi-gcc`。

常用完整检查：

```sh
./tools/check.sh host-gcc
./tools/check.sh host-clang
./tools/check.sh host-sanitize
./tools/check.sh host-tsan

cmake --preset arm-cortex-m0plus-gcc
cmake --build --preset arm-cortex-m0plus-gcc
```

`tools/check.sh` 会执行格式检查、主机构建、单元测试、外部接入 smoke test、clang-tidy 和
cppcheck。`host-sanitize` 启用 ASan/UBSan，`host-tsan` 启用 TSan。构建产物位于 `build/`，不提交。

当前验证不替代目标板 HIL、实际 FreeRTOS 移植层、WCET、堆栈预算或具体硬件资源验证；这些由
采用组件的项目负责。

## 文档入口

- [工程 Profile](docs/ENGINEERING_PROFILE.yaml)：机器可读的工程约束和上下文。
- [架构规范](docs/ARCHITECTURE_STANDARD.md)：分层、接口和执行模型边界。
- [代码风格](docs/CODING_STYLE.md)：C11、命名、注释和静态分析约束。
- [总体建设计划](docs/PLAN.md)：仓库范围、阶段路线和按需能力。
- [组件说明](docs/components/)：每个主线组件的使用契约。
- [使用与设计复核计划](docs/REVIEW_PLAN.md)：S1～S8 的资源、生命周期和确认结论。
- [仓库工作流](docs/REPOSITORY_WORKFLOW.md)：Git、CI 和仓库实践。
