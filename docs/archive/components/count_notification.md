# count_notification

有界计数通知，计数上限在初始化时固定。`give` 增加计数，`take` 消耗计数，达到上限返回
`OVERFLOW`，零计数返回 `EMPTY`；有限等待由等待策略端口实现。ISR 只能调用非阻塞 give，停止
清零计数并取消等待者，组件不绑定任何任务句柄。
