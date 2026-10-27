# EmbeddedLibrary 生产文件清单

本清单同时记录储备库文件和后续计划。只记录生产代码文件，不记录测试、文档、构建工具和临时产物。

状态含义：`现有` 表示文件已经存在；`进行中` 表示当前阶段正在实现；`无` 表示目标文件尚未创建。

计划动作：`保留`、`重写`、`成熟库替换`、`按需创建`、`归档`。

同一组件的 `.h/.c` 目标路径放在同一目录。当前部分旧组件仍使用 `include/` 和 `src/`，阶段重写时再迁移到目标路径。

## 裸机基础与数据

| 目标文件 | 层级 | 状态 | 计划动作 | 职责 |
| --- | --- | --- | --- | --- |
| `services/time_port/time_port.h` | Services | 现有 | 保留 | 单调时间来源边界 |
| `services/time_port/time_port.c` | Services | 现有 | 保留 | 单调时间来源边界 |
| `services/critical_section/critical_section.h` | Services | 现有 | 保留 | ISR/主循环短临界区边界 |
| `services/critical_section/critical_section.c` | Services | 现有 | 保留 | ISR/主循环短临界区边界 |
| `libraries/foundation_status/foundation_status.h` | Libraries | 现有 | 保留 | 基础状态结果 |
| `libraries/numeric/numeric.h` | Libraries | 现有 | 保留 | 数值检查和辅助 |
| `libraries/numeric/numeric.c` | Libraries | 现有 | 保留 | 数值检查和辅助 |
| `libraries/tick32/tick32.h` | Libraries | 现有 | 保留 | tick 计算 |
| `libraries/tick32/tick32.c` | Libraries | 现有 | 保留 | tick 计算 |
| `libraries/mizu_fifo/mizu_fifo.h` | Libraries | 现有 | 自研 | 单生产者/单消费者通信字节暂存 |
| `libraries/mizu_fifo/mizu_fifo.c` | Libraries | 现有 | 自研 | 单生产者/单消费者通信字节暂存 |
| `libraries/snapshot_buffer/snapshot_buffer.h` | Libraries | 现有 | 保留 | 快照发布储备 |
| `libraries/snapshot_buffer/snapshot_buffer.c` | Libraries | 现有 | 保留 | 快照发布储备 |
| `libraries/mizu_status/mizu_status.h` | Libraries | 现有 | 自研 | 仓库公共操作结果 |

## 控制算法

| 目标文件 | 层级 | 状态 | 计划动作 | 职责 |
| --- | --- | --- | --- | --- |
| `libraries/pid/pid.h` | Libraries | 现有 | 重写 | 基础 PID |
| `libraries/pid/pid.c` | Libraries | 现有 | 重写 | 基础 PID |
| `libraries/cascaded_pid/cascaded_pid.h` | Libraries | 现有 | 重写 | 串级 PID |
| `libraries/cascaded_pid/cascaded_pid.c` | Libraries | 现有 | 重写 | 串级 PID |
| `libraries/first_order_filter/first_order_filter.h` | Libraries | 现有 | 整理阶段 | 一阶滤波 |
| `libraries/first_order_filter/first_order_filter.c` | Libraries | 现有 | 整理阶段 | 一阶滤波 |
| `libraries/moving_average/moving_average.h` | Libraries | 现有 | 整理阶段 | 滑动平均 |
| `libraries/moving_average/moving_average.c` | Libraries | 现有 | 整理阶段 | 滑动平均 |
| `libraries/hysteresis/hysteresis.h` | Libraries | 现有 | 整理阶段 | 迟滞 |
| `libraries/hysteresis/hysteresis.c` | Libraries | 现有 | 整理阶段 | 迟滞 |
| `libraries/deadband/deadband.h` | Libraries | 现有 | 整理阶段 | 死区 |
| `libraries/deadband/deadband.c` | Libraries | 现有 | 整理阶段 | 死区 |
| `libraries/rate_limiter/rate_limiter.h` | Libraries | 现有 | 整理阶段 | 速率限制 |
| `libraries/rate_limiter/rate_limiter.c` | Libraries | 现有 | 整理阶段 | 速率限制 |

## 通信与横向 Services

| 目标文件 | 层级 | 状态 | 计划动作 | 职责 |
| --- | --- | --- | --- | --- |
| `services/scheduler/scheduler.h` | Services | 现有 | 自研 | 裸机协作调度 |
| `services/scheduler/scheduler.c` | Services | 现有 | 自研 | 裸机协作调度 |
| `services/frame/frame.h` | Services | 无 | 建立骨架 | 帧边界和处理状态 |
| `services/frame/frame.c` | Services | 无 | 建立骨架 | 帧边界和处理状态 |
| `services/communication/communication.h` | Services | 无 | 建立骨架 | 通信组合和错误边界 |
| `services/communication/communication.c` | Services | 无 | 建立骨架 | 通信组合和错误边界 |
| `services/error/error.h` | Services | 无 | 自研 | 最小错误模型 |
| `services/error/error.c` | Services | 无 | 自研 | 最小错误模型 |
| `services/diagnostic/diagnostic.h` | Services | 无 | 自研 | 诊断快照和 Telemetry |
| `services/diagnostic/diagnostic.c` | Services | 无 | 自研 | 诊断快照和 Telemetry |
| `services/parameter/parameter.h` | Services | 无 | 自研 | 静态参数注册表 |
| `services/parameter/parameter.c` | Services | 无 | 自研 | 静态参数注册表 |
| `services/menu/menu.h` | Services | 无 | 自研 | 菜单状态机 |
| `services/menu/menu.c` | Services | 无 | 自研 | 菜单状态机 |
| `services/command/command.h` | Services | 无 | 自研 | 静态命令路由 |
| `services/command/command.c` | Services | 无 | 自研 | 静态命令路由 |

S1 的 scheduler 使用项目提供的静态配置表和运行槽数组，只在主循环调用；不处理 ISR、DMA 或 RTOS 事件。

`byte_stream` 不建立自研文件，优先采用成熟实现、平台能力或项目传输接口。

## 成熟库能力

以下能力进入储备范围，但不预建新的自研文件。具体第三方文件路径要等实际选型后确定：

| 能力/目标路径 | 层级 | 状态 | 计划动作 | 说明 |
| --- | --- | --- | --- | --- |
| `third_party/<selected_crc_library>/...` | Middleware | 无 | 成熟库替换 | CRC |
| `third_party/<selected_byte_order_library>/...` | Middleware | 无 | 成熟库替换 | 字节序和 IEEE754 编解码 |
| `third_party/<selected_encoding_library>/...` | Middleware | 无 | 成熟库替换 | COBS 等编码 |
| `third_party/<selected_logging_library>/...` | Middleware | 无 | 成熟库替换 | 日志 |
| `third_party/<selected_pool_library>/...` | Middleware | 无 | 成熟库替换 | 对象池和固定队列 |
| `third_party/<selected_storage_library>/...` | Middleware | 无 | 成熟库替换 | Repository 后端 |
| `third_party/<selected_bluetooth_stack>/...` | Middleware | 无 | 成熟库替换 | 蓝牙协议栈 |
| `third_party/<selected_display_library>/...` | Middleware | 无 | 成熟库替换 | 显示驱动、字库和渲染 |

FreeRTOS 的 queue、notification、event group、mutex 和 semaphore 直接使用项目锁定版本的内核文件，不在本仓库预建第三方副本。

## 按需契约文件

| 目标文件 | 层级 | 状态 | 计划动作 |
| --- | --- | --- | --- |
| `adapters/freertos/freertos_contract.h` | Middleware | 无 | 按需创建 |
| `adapters/bluetooth/bluetooth_contract.h` | Middleware | 无 | 按需创建 |
| `adapters/display/display_contract.h` | Middleware | 无 | 按需创建 |

## 排除和历史资产

以下路径不进入新清单：

- `waiter`、`queue_port`、`event_group`、`count_notification`、`mutex_port`；
- 具体 UART/DMA、蓝牙芯片、OLED/LCD、Flash/NVM 和 FreeRTOS 实现；
- 项目 Application、Domain、BSP、Drivers 和具体产品页面。

旧说明和旧阶段记录已归档到 `docs/archive/`；历史生产源码是否清理，须另行执行维护动作。
