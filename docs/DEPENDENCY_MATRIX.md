# 能力与成熟库评估矩阵

本表用于 R0。已完成实现仍需评估，但在结论确认前不删除、不替换。

| 能力 | 当前资产 | 候选成熟库 | 结论 | 依据/风险 |
| --- | --- | --- | --- | --- |
| 时间数学、状态码 | P1 | 无直接候选 | 暂缓替换 | 标准 C 运算和仓库错误契约较小，成熟库收益不足 |
| FIFO、队列、对象池、快照 | P1/P3 | ETL（C++，MIT）/FreeRTOS 原生队列 | 待定，高优先级评估 | 候选能力成熟，但语言、线程模型和 API 兼容性需先确认 |
| CRC、字节序、COBS/SLIP | P1 | nanopb（协议编码，BSD-3-Clause）等 | 分项评估 | nanopb 适合 Protobuf，不直接替代 CRC/字节序/COBS |
| PID 与滤波算法 | P2 | CMSIS-DSP（BSD-3-Clause） | 待定，中优先级评估 | ARM 优化有价值，但需比较浮点、Cortex-M0 和接口语义 |
| 并发端口与 waiter | P3 | FreeRTOS Kernel（MIT） | 保留端口，适配器待定 | 原生机制绑定 RTOS，不能替代平台无关核心接口 |
| 协作式调度器 | 无 | 不预设 | 自研 | 属于本仓库架构核心 |
| 日志、错误、诊断 | 无 | 待调查 | 待定 | 需结构化记录、固定容量和 sink 隔离 |
| 参数与存储 | 无 | 待调查 | 待定 | 需类型化 API、版本和完整性 |
| 通信与协议 | 无 | nanopb（BSD-3-Clause） | 待定 | 仅作为结构化编码候选，传输/会话仍需另评估 |
| 菜单与蓝牙 | 无 | 待调查 | 待定 | 不得绑定显示或具体芯片 |

候选依赖必须记录源码来源、锁定版本、许可证、维护状态、资源预算、规范差距、适配职责和验证平台。

## 第一轮候选证据

- ETL 官方文档说明其固定容量容器不使用堆，并提供嵌入式容器集合：<https://www.etlcpp.com/docs/containers/>。
- FreeRTOS 官方资料确认队列、事件组、通知和互斥量属于内核同步机制：<https://freertos.org/RTOS_task_notifications.html>。
- nanopb 官方仓库定位为嵌入式 Protocol Buffers，并支持静态字段分配：<https://github.com/nanopb/nanopb>。
- CMSIS-DSP 官方仓库由 Arm 维护，候选算法实现需单独核对具体模块和 Cortex-M0 支持：<https://github.com/ARM-software/CMSIS-DSP>。

上述候选仅完成初筛，不代表已采用；版本、许可证正文、资源测量和行为等价测试仍需在具体替换阶段确认。
