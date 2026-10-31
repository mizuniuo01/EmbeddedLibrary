# EmbeddedLibrary 生产文件清单

本清单只记录当前主线生产代码和明确的未来阶段目标，不记录测试、文档、构建工具、临时产物或已归档源码。

状态含义：`现有` 表示当前主线文件已经存在；`无` 表示目标文件尚未创建。

## 已完成主线

| 目标文件 | 层级 | 状态 | 计划动作 | 职责 |
| --- | --- | --- | --- | --- |
| `services/scheduler/scheduler.h` | Services | 现有 | 自研 | 裸机协作调度 |
| `services/scheduler/scheduler.c` | Services | 现有 | 自研 | 裸机协作调度 |
| `libraries/mizu_fifo/mizu_fifo.h` | Libraries | 现有 | 自研 | 单生产者/单消费者通信字节暂存 |
| `libraries/mizu_fifo/mizu_fifo.c` | Libraries | 现有 | 自研 | 单生产者/单消费者通信字节暂存 |
| `libraries/mizu_status/mizu_status.h` | Libraries | 现有 | 保留并在 S7 复核 | 当前公共操作结果类型；状态服务方案在 S7 决定 |
| `libraries/pid/pid.h` | Libraries | 现有 | 自研 | 单精度基础 PID |
| `libraries/pid/pid.c` | Libraries | 现有 | 自研 | 单精度基础 PID |
| `libraries/cascaded_pid/cascaded_pid.h` | Libraries | 现有 | 自研 | 固定两级串级 PID |
| `libraries/cascaded_pid/cascaded_pid.c` | Libraries | 现有 | 自研 | 固定两级串级 PID |

## 待建设主线

| 目标文件 | 层级 | 状态 | 阶段 | 职责 |
| --- | --- | --- | --- | --- |
| `services/parameter/parameter.h` | Services | 无 | S5 自研 | 回调绑定的参数注册和访问 |
| `services/parameter/parameter.c` | Services | 无 | S5 自研 | 回调绑定的参数注册和访问 |
| `services/command/command.h` | Services | 无 | S6 自研 | 稳定命令 ID 和静态命令路由 |
| `services/command/command.c` | Services | 无 | S6 自研 | 稳定命令 ID 和静态命令路由 |
| `services/mizu_status/mizu_status.h` | Services | 无 | S7 自研 | 错误状态类型、保存和查询入口，具体方案阶段内决定 |
| `services/mizu_status/mizu_status.c` | Services | 无 | S7 自研 | 错误状态保存和查询实现 |
| `services/menu/menu.h` | Services | 无 | S8 自研 | 硬件无关调试菜单 |
| `services/menu/menu.c` | Services | 无 | S8 自研 | 硬件无关调试菜单 |

`mizu_status` 当前头文件中的结果类型与未来 Services 状态服务的最终依赖关系，在 S7 阶段性 Plan 中解决；不得形成 Libraries 依赖 Services 的反向依赖。

## 按需能力

以下内容不预建仓库文件，真实项目出现稳定重复需求后再单独开启阶段：

- 项目帧协议、通信组合、UART/DMA/蓝牙适配；
- FreeRTOS、蓝牙、显示、日志和持久化适配；
- 参数持久化和项目专属状态展示；
- 一阶滤波、滑动平均、迟滞、死区、速率限制等小型算法；
- 固定队列、对象池、快照缓冲等尚无明确主线使用者的数据结构。

CRC、字节序、IEEE754、COBS 等编码能力优先使用成熟库或项目现成实现，不预建新的自研包装文件。

## 归档说明

不属于上述主线或按需能力边界的旧生产源码统一位于 `archive/legacy_src/`，不参与主构建、主线测试和静态检查。旧组件说明、阶段计划和资产登记位于 `docs/archive/`；历史代码仍可从 Git 历史恢复。

归档类别及职责：

- `time_port`：时间来源和休眠端口；
- `critical_section`：临界区进入/退出端口；
- `numeric`、`tick32`：数值检查、限幅和 tick 回绕计算；
- `foundation_status`：旧公共状态结果类型；
- `snapshot_buffer`、`fixed_queue`、`object_pool`：旧快照、固定队列和固定对象分配；
- 一阶滤波、滑动平均、迟滞、死区、速率限制：旧小型算法；
- 旧 PID 和串级 PID：S3/S4 重写前的历史实现；
- CRC、字节序、IEEE754、COBS：旧自研编码和校验；
- `waiter`、`queue_port`、`event_group`、`count_notification`、`mutex_port`：旧并发服务。

具体 BSP、Driver、Application、Domain、产品协议和产品页面不进入本仓库主线清单。
