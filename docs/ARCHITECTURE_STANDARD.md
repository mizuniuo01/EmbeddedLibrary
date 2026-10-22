# 嵌入式架构规范说明

> 版本：v1.0.0<br>
> 状态：Active<br>
> 适用范围：采用 `docs/ENGINEERING_PROFILE.yaml` 的 MCU 裸机/RTOS 项目

本文档是架构规范的解释入口，不复制完整的架构或 RTOS 手册。`ENGINEERING_PROFILE.yaml` 是 Agent skills 使用的机器可读架构规则和项目上下文来源；本文档解释职责、边界和审查方法。Profile 的显式配置优先。

## 1. 参考来源

- 架构描述参考：[ISO/IEC/IEEE 42010](https://www.iso.org/standard/74393.html)。
- 嵌入式分层、接口和质量属性参考成熟 MCU 工程实践；项目在 Profile 或架构说明中记录实际采用的来源。
- FreeRTOS 项目参考 [FreeRTOS 官方文档](https://www.freertos.org/Documentation/00-Overview) 和目标移植层文档。
- 裸机项目参考目标 MCU、启动代码、厂商 SDK 和硬件手册；平台事实优先于抽象示例。

## 2. 个人架构规则

默认职责分层如下：

```text
Composition Root（装配边界）
├── Application
├── Domain
├── Services
├── Libraries
├── Middleware
└── BSP
    └── Drivers
        └── Vendor SDK / HAL / Registers
```

Composition Root 负责实例创建、依赖绑定、配置装配和启动/关闭顺序。Application 负责产品流程、模式、命令编排和任务/主循环注册。Domain 负责产品规则、领域模型、状态机和算法协调。Services 提供时间、日志、错误、参数、存储和诊断等横向能力。Libraries 提供平台无关算法和数据结构。Middleware 承载 FreeRTOS、网络栈、文件系统、GUI 和第三方协议栈；第三方栈适配属于 Middleware 内部职责，不承载产品规则。BSP 负责板级资源绑定、时钟、电源、DMA、IRQ、外设实例和板级安全。Drivers 负责外设和器件机制，不决定产品模式和业务策略。

Composition Root 不是运行时业务层。它只负责创建对象、绑定依赖、选择执行模型和安排启动/关闭顺序；运行时业务流程由 Application 负责。

允许依赖矩阵：

| 分区 | 可依赖 | 不得直接依赖 |
| --- | --- | --- |
| Composition Root | 所有公共接口和配置 | 各模块私有状态 |
| Application | Domain、Services、Libraries、Middleware 公共 API、BSP 能力接口 | Drivers 私有类型、Vendor/HAL、寄存器 |
| Domain | Services、Libraries、项目端口 | Application、BSP、Drivers、RTOS 类型 |
| Services | Libraries、Middleware 公共 API、BSP 能力接口、注入端口 | Application 流程、Domain 私有状态、Vendor/HAL |
| Libraries | 标准库、同级基础库、注入端口 | Application、Domain、BSP、Drivers |
| Middleware | Middleware API、BSP、Drivers、Vendor/HAL | 产品规则和 Application 私有状态 |
| BSP | Drivers、Vendor/HAL、板级配置 | Application 流程、Domain 规则 |
| Drivers | Vendor/HAL、寄存器、底层端口 | Application、Domain、产品策略 |

项目必须在 Profile 的 `architecture.directory_map` 中把实际目录或厂商框架映射到上述职责；偏离矩阵时必须登记例外。

### 分层

- 业务/领域逻辑不得直接依赖寄存器、引脚、厂商 HAL 或 FreeRTOS 类型。
- 硬件能力由 BSP/Driver 或明确适配边界提供。
- 裸机主循环、协作调度器和 FreeRTOS 任务都是执行模型，不改变领域逻辑的职责边界。
- 任务按实时约束、阻塞行为、故障隔离和资源所有权划分，不按源文件数量创建。

### 接口设计

- 接口面向调用者，暴露最小且稳定的能力。
- 接口必须说明生命周期、所有权、可修改性、阻塞性、执行上下文、并发限制和错误结果。
- 每项硬件资源和可变数据有唯一所有者；其他模块使用命令、事件、只读快照或明确同步协议。
- C/C++ 公共边界只能使用双方都能表达且 ABI 已验证的类型。

同一分层同时支持裸机和 FreeRTOS。Domain 和 Libraries 必须保持执行模型无关，除非 Profile 登记例外；Services 通过时间、事件和同步端口适配两种模型；Application 在裸机中注册主循环/协作任务，在 FreeRTOS 中注册任务入口和优先级。任务按实时约束、阻塞行为、故障隔离和资源所有权划分，不按文件数量创建。

公共接口最低说明生命周期、所有权、参数方向、阻塞性/执行上下文、并发限制和错误结果。

除上述规则外，采用的命名、同步、任务、错误、资源和测试原则来自 Profile 声明的行业参考；不在本文档复制完整规则。

## 3. 裸机与 FreeRTOS

Profile 选择 `bare_metal` 时，agent 检查主循环、协作调度、ISR/DMA 下半部、非阻塞状态机、时间源和低功耗边界。

Profile 选择 `freertos` 时，agent 检查任务边界、阻塞等待、队列/通知/信号量/互斥锁选择、ISR-safe API、tick/超时、栈堆预算和对象生命周期。FreeRTOS API 细节以官方文档和目标移植层为准，不由本文件复制。

## 4. Agent 执行约定

agent 生成或审查架构时，先读取 `docs/ENGINEERING_PROFILE.yaml`，确认裸机或 FreeRTOS 执行模型，再读取本文件和 Profile 声明的参考来源，按分层、接口、所有权、生命周期、并发和资源边界输出结果。

没有 Profile 时，不得假设项目使用 FreeRTOS、特定分层或某种任务模型。

## 5. 架构产物

项目架构说明必须记录：Profile、分区映射、公共接口、资源所有者、执行模型、任务/ISR 表、关键时序、例外和验证证据。项目不复制本文件或外部标准全文。

## Changelog

### v1.0.0 — 2026-10-10

- 将架构规范收敛为参考来源、个人分层/接口规则和裸机/FreeRTOS 使用边界。
- 增加 Profile 驱动和 agent 执行约定。
