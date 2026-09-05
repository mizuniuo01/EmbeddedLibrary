# P1 基础类型、时间与数据结构阶段设计

状态：进行中（第一、第二批完成；第三批本地实现与验证完成，等待远端 CI）
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

第二批证据：GCC/Clang/Sanitizer 三套检查通过，COBS 重叠与 254/255 字节长块测试通过，CRC 增量接口和失败输出测试通过，IEEE-754 特殊值测试通过，Cortex-M0+ compile-only 已通过。GitHub Actions 运行 `33955779208` 中五项任务全部成功。第二批完成，第三批为有界容器。

## 6. 第三批确认设计与实现记录

用户确认：以更稳健的边界检查减少调用方隐式责任；FIFO 提供批量及单字节接口，
队列采用不要求对齐的字节复制，对象池使用指针与一字节槽状态，快照采用显式借用/归还。
详细 API、错误优先级涉及的状态、所有权和资源上限分别见：

- [byte_fifo](../components/byte_fifo.md)
- [fixed_queue](../components/fixed_queue.md)
- [object_pool](../components/object_pool.md)
- [snapshot_buffer](../components/snapshot_buffer.md)

### 6.1 固定契约

- 四组件各有公共头、实现、独立构建目标、单元测试和组件文档，只依赖 foundation_status。
- 首次使用前实例为 {0}；所有 API 均需有效指针和真实存储长度。
  外部同步、存储存活、禁止复制实例仍是不可由普通 C 库自动消除的前提。
- 初始化完整校验后提交新绑定；对象池有借出对象或快照有未结束读写借用时，重新初始化返回 BUSY。
- 非 OK 不写普通输出或有效数据；FULL 的饱和诊断计数是显式例外。
  FIFO/队列统计不能全部接受的写请求次数，包括 some 部分成功，不统计字节/元素数。
- FIFO write/read 为 exact，some 部分成功返回 OK 和数量；完全无进展返回 FULL/EMPTY，
  数量输出不变。零请求允许 NULL 并返回 OK 和零。peek/discard 不允许部分完成。
- 队列 push/pop 复制单元素，push_some/pop_some 为完整元素的部分批量；
  peek_at 从队头算零基逻辑索引。元素大小乘容量溢出和存储不足显式区分。
  值复制不是深拷贝，元素中的指针不会变成库拥有的对象。
- 对象池 alignment 显式指定，必须为非零二次幂；每槽步长满足对齐。
  acquire 返回最低空闲槽指针与索引，release 校验当前占用槽首地址。
  lowest_free_index == capacity 表示无空闲槽。保持无代次句柄设计，不检测地址重用后的旧指针误释放。
- 快照 writer 和 lease 绑定原对象地址，禁止复制凭据；每槽最多一个未归还读借用。
  更多读者采用 copy 或串行借用。读借用阻止槽复用，不阻止另一槽完成发布。
  BUSY 在 begin 获取不到写槽时报告；publish 使用已经独占的写槽。
- 快照 size 可为零；sequence 由库从 1 递增，UINT32_MAX 后 OVERFLOW 并保留写事务；
  timestamp 由调用方按统一单位传入 uint32_t 单调 tick，不调用平台时钟。
  publish/cancel/release 成功均清空对应凭据；失败保持凭据与已发布快照。
- 所有 API 由外部串行化；快照读/写载荷可在各自借用存活期内、在 API 临界区外访问。
  这不是无锁或 C 原子实现；P3 才提供 RTOS 适配。
- 区间/对齐检查使用有记录的 uintptr_t 平坦地址 ABI，不做无关对象指针相减。
  复制区间和数量输出互相重叠时拒绝；不宣称能检测悬空指针、虚报容量或任意内存破坏。

### 6.2 资源与验证门槛

FIFO/队列读写时间由实际字节数约束，池 acquire 最坏扫描固定槽数；
快照 copy 按有效字节数复制，其余借用操作 O(1)。全部栈 O(1)，无堆、VLA 或动态读者列表。
RAM 公式和资源释放责任见组件文档；ARM compile-only 不提供目标 WCET 或上板证据。

本地证据（第三批当前工作树）：

- GCC 13：`sh tools/check.sh host-gcc` 通过，11 项 CTest 通过。
- Clang 21：`sh tools/check.sh host-clang` 通过，11 项 CTest 通过。
- ASan/UBSan：`sh tools/check.sh host-sanitize` 通过，12 项 CTest（含 sanitizer probe）通过。
- clang-format 21、Clang-Tidy 和 Cppcheck 全部通过。
- Cortex-M0+：`cmake --preset arm-cortex-m0plus-gcc` 及对应 build 通过。
- 测试覆盖满空、单容量、跨尾复制、反复回绕、零长度、部分操作、失败原子性、
  输入输出重叠、池对齐/容量/重复释放、快照借用背压、复制凭据、取消和序号/统计极值。

成熟度：`host-tested / hardware-unverified`，不是 `rtos-tested`。
远端五项 CI 及最低 CMake 3.20.6 的本次结果尚未取得；提交并推送后验证，记录对应 commit/run，
不能沿用第二批 run 作为第三批证据。第三批远端验收和 P1 最终收尾尚未完成，不自动进入 P2/P3。
