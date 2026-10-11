# tick32

所属层级：Libraries

职责：提供 uint32_t 单调 tick 的回绕安全经过时间和 deadline 判断。

平台绑定方式：无；tick 单位和来源由调用方定义。

公共 API：`elapsed`、`duration_is_valid`、`has_elapsed`、`deadline_add`、`deadline_reached`。

契约：可比较的 duration 不得超过 `INT32_MAX` tick；使用 uint32_t 模减法；deadline 相隔恰好半个范围时返回 `OUT_OF_RANGE`。

资源与实时性：所有操作 O(1)，无动态内存、无阻塞、无内部状态。

并发：纯函数式计算，可重入。

测试：覆盖普通时间、零 duration、回绕、半范围边界和非法输出参数。
