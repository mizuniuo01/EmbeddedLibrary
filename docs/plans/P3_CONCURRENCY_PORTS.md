# P3 并发端口与运行环境适配阶段设计

状态：进行中（第三批本地验证完成，等待远端 CI）

P3 分三批完成：第一批为 32/64 位单调时间、token 型临界区和公共错误契约；第二批为固定元素
复制队列、32 位事件位组和计数通知；第三批为非递归互斥、POSIX 测试适配、并发验证和
FreeRTOS 适配契约收尾。

生产接口不包含 FreeRTOS、POSIX、HAL 或任务类型，不提供任务创建、调度器和永久等待。对象使用
调用方静态工作区，普通等待只接受有限 deadline，ISR API 明确使用 `_isr` 后缀且不接受超时。
FreeRTOS 实现等待首个项目锁定版本后进入 `adapters/freertos`，因此 P3 完成时仍只能标记为
`host-tested / hardware-unverified`。

## 第一批证据

已实现 `time_port`、`critical_section` 和状态码 `INVALID_CONTEXT`/`UNSUPPORTED`。时间端口提供
32 位回绕安全差值、64 位差值、绝对 deadline 包装和测试专用手动时钟替身；临界区端口使用
enter/exit token。GCC、Clang、ASan/UBSan、clang-format、Clang-Tidy、Cppcheck、外部构建和
Cortex-M0+ compile-only 均通过。当前仓库共有 21 项普通测试，Sanitizer 为 22 项（含 probe）。

第一批不提供真实 MCU 临界区实现，也不宣称 64 位时间由公共库扩展；两者均由后续平台适配器
提供并承担原子性、屏蔽范围和时钟连续性契约。

## 第二批证据

已建立独立的 `queue_port`、`event_group` 和 `count_notification` 组件，采用调用方存储、
临界区端口和统一 waiter 端口。GCC/Clang 各 22 项 CTest、Sanitizer 23 项 CTest（含 probe）、
clang-format、Clang-Tidy、Cppcheck、外部接入构建和 Cortex-M0+ compile-only 均通过。
等待策略已统一为 `waiter_port_t`；真实 FreeRTOS 适配和目标板验证不在 P3 范围内。

## 第三批当前进度

已新增 `mutex_port` 非递归互斥组件：调用方显式提供 owner token，支持 try/有限 deadline lock、
unlock、stop/start/reset/deinit，并通过配置声明优先级继承能力。GCC/Clang、Sanitizer、外部接入、
clang-format、Clang-Tidy、Cppcheck 和 ARM compile-only 已通过。

已新增独立 `waiter` 公共组件，提供静态存储、owner 绑定和完成/取消后复用状态；queue/event/
notification/mutex 的阻塞 API 已切换为显式 waiter 参数，等待端口统一为 `waiter_port_t`，并区分
`signal_one`、`signal_all` 和 `cancel_all`。事件组采用广播后由各 waiter 重新检查条件的模型，不在
核心保存跨线程 waiter 指针，避免生命周期竞态。POSIX 测试已覆盖 mutex 竞争、queue 阻塞唤醒、
event wait-any 和 notification 等待唤醒；GCC/Clang 各 26 项、Sanitizer 27 项、TSan 26 项测试，
外部接入、clang-format、Clang-Tidy、Cppcheck 和 Cortex-M0+ compile-only 均在本地通过。

第三批本地闭环完成。仍待用户提交推送后的远端 CI；P3 完成后成熟度仍为
`host-tested / hardware-unverified`，FreeRTOS 适配器版本和实际 RTOS 运行验证留待首个项目。
