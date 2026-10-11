# numeric

所属层级：Libraries

职责：提供 f32 有限值检查以及 f32/i32/u32 闭区间限幅。

平台绑定方式：依赖 C11 `<math.h>`；不依赖硬件或 RTOS。

公共 API：`numeric_f32_is_finite`、`numeric_*_clamp`。

契约：区间为闭区间；反向边界返回 `INVALID_ARGUMENT`；浮点 NaN/Inf 返回 `INVALID_DATA`；失败不修改输出。

资源与实时性：O(1)，无动态内存、无阻塞、无内部状态。

并发：纯函数式计算，可重入。

测试：覆盖边界、极值、NaN、Inf、反向区间、空输出和失败输出保持不变。
