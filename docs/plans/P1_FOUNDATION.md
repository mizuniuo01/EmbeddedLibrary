# P1 基础类型、时间与数据结构阶段设计

状态：进行中（基础契约批完成，规范整改已完成）  
规范基线：架构规范 v1.3.0、代码风格规范 v1.2.0

## 1. 已冻结的总体约束

- 公共符号只使用职责前缀，不使用仓库名；组件可单独复制。
- 允许所有基础组件共同依赖轻量 `foundation_status`；浮点字节序另依赖整数 `byte_order`。
- 实例由调用方静态分配并提供 backing storage；公开结构体字段属于内部状态，不得直接修改。
- 已初始化实例禁止按值复制；`init` 先完整校验，成功后允许重新绑定，失败保持旧实例不变。
- 不使用堆、VLA、平台头文件、RTOS 类型、全局可变状态或隐式主机字节序。
- FIFO、队列、对象池和快照均为外部同步模型；不宣称 ISR/任务并发安全。
- 非 `OK` 不修改普通输出参数或目标数据；允许更新的只有明确规定的饱和诊断计数。
- `NULL + 0` 对字节输入合法；`NULL + 非零长度` 非法。

## 2. 组件与批次

第一批基础契约：

- `foundation_status`：稳定显式状态码 0～16，后续只允许末尾追加。
- `tick32`：uint32_t 模回绕时间计算，合法比较间隔不超过 INT32_MAX。
- `numeric`：f32 有限值检查及 f32/i32/u32 闭区间限幅。

第二批编码算法：

- `byte_order`：u16/u32/u64 和 i16/i32/i64 的 LE/BE 有界编解码。
- `ieee754_byte_order`：独立的 IEEE-754 binary32/binary64 LE/BE 位级编解码。
- `crc`：CRC-8/SMBUS、CRC-16/CCITT-FALSE、CRC-32/ISO-HDLC，无表逐位实现。
- `cobs`：事务式 COBS 编解码、容量计算和完整输入验证。

第三批有界容器：

- `byte_fifo`：exact/some、peek、discard、拒绝计数和 high-water mark。
- `fixed_queue`：固定元素复制、逻辑索引 peek、静态容量推导和统计。
- `object_pool`：显式对齐、每槽状态字节、最低空闲索引和耗尽统计。
- `snapshot_buffer`：双槽 begin/publish/cancel、copy/view、sequence/timestamp。

## 3. 第一批接口契约

### foundation_status

`foundation_status_t` 成员固定为：OK、INVALID_ARGUMENT、NOT_INITIALIZED、INVALID_STATE、
OUT_OF_RANGE、EMPTY、FULL、BUFFER_TOO_SMALL、INVALID_DATA、OVERFLOW、BUSY、TIMEOUT、
IO_ERROR、UNAVAILABLE、NOT_FOUND、CANCELLED、INTERNAL_ERROR，数值为 0～16。

### tick32

接口为 `elapsed`、`duration_is_valid`、`has_elapsed`、`deadline_add`、`deadline_reached`。
elapsed 使用 uint32_t 模减法；零 duration 立即到期；half-range 对应的 deadline 差值返回
OUT_OF_RANGE；所有失败不写输出。

### numeric

接口为 `numeric_f32_is_finite` 和三个 `numeric_*_clamp`。边界为闭区间；反向边界为
INVALID_ARGUMENT；NaN/Inf 为 INVALID_DATA；失败不改 result。

## 4. 资源、并发与成熟度

第一批均为 O(1)、无阻塞、无动态分配、无内部共享状态；可重入。当前成熟度为
`host-tested / hardware-unverified`。P1 后续 ARM 任务只编译生产组件，不运行目标程序，不替代上板验证。

## 5. 第一批证据

- GCC 13 `sh tools/check.sh host-gcc`：通过。
- Clang 21 `sh tools/check.sh host-clang`：通过。
- ASan/UBSan `sh tools/check.sh host-sanitize`：通过；4 个测试通过。
- Clang-Tidy、Cppcheck、clang-format：通过。
- 第一批测试覆盖正常、零值、极值、tick 回绕、half-range、非法参数、NaN/Inf、反向边界和失败输出不变。
- 注释整改：`.h` 仅保留普通接口分组注释；`.c` 文件包含文件头和函数 Doxygen；宏与枚举成员使用逐项中文普通注释。
- 结构检查：禁止头文件文件头/函数 Doxygen，要求 C 文件文件头和所有新增文件格式符合规范。

第二批本地证据：GCC/Clang/Sanitizer 三套检查通过，COBS 重叠与 254/255 字节长块测试通过，CRC 增量接口和失败输出测试通过，IEEE-754 特殊值测试通过，Cortex-M0+ compile-only 已通过。第二批本地整改完成，剩余验收为远端 CI 和最终阶段状态更新。
