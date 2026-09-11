# mutex_port

提供不递归的资源互斥能力。对象由调用方静态分配，使用显式 `owner_token` 识别持有者；重复
获取、错误释放以及在 ISR 中使用互斥均被拒绝。普通获取支持立即尝试和有限的 32 位绝对 tick
deadline，等待和取消由注入的等待策略负责。

初始化配置声明适配器是否支持优先级继承以及调用方是否强制要求该能力。要求继承而适配器不支持
时返回 `FOUNDATION_STATUS_UNSUPPORTED`。停止不会强制释放持有者；仍被持有时返回 `BUSY`，
否则取消等待者并进入 STOPPED。组件不包含 FreeRTOS、POSIX 或任务句柄。
