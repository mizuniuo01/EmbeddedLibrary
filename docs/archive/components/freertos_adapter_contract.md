# FreeRTOS 适配器契约（P3）

本文件只冻结平台适配边界，不包含 FreeRTOS 头文件或实现。适配器未来位于
`adapters/freertos`，公共 Services 和 Libraries 不得依赖 FreeRTOS 类型、版本宏或任务句柄。

| 公共能力 | 映射方向 |
| --- | --- |
| 32 位时间 | 项目时间端口与 `TickType_t` 映射 |
| 有限 deadline | 转换为有限阻塞 tick |
| 队列 | 静态队列对象 |
| 事件位组 | 静态事件组对象 |
| 计数通知 | 计数信号量或项目确认的通知映射 |
| 非递归互斥 | 带优先级继承的静态 mutex |
| ISR API | 对应 `FromISR` API 和 `should_yield` |
| stop/cancel | 适配器关闭状态和等待唤醒策略 |
| waiter | 调用方静态等待上下文 |

最低支持版本、当前验证版本和配置 Profile 待首个实际项目确定。版本差异只能存在于适配器；
接口或语义不兼容时提升适配器主版本。P3 完成本文件不等于 `rtos-tested`。
