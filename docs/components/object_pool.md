# object_pool

运行期借出固定数量对象；采用指针所有权契约和每槽一个字节状态。

## 公共边界

所属层级：Libraries；唯一组件依赖为 `foundation_status`，使用标准 C11。
实例由调用方以 `{0}` 初始化并提供静态存储，字段仅供实现使用；实例初始化后禁止按值复制。
所有传入指针必须指向存活对象，所报长度不得超过真实对象大小。C 无法检测悬空指针或虚报容量。
存储和输出区不得与实例、内部存储或同次其他输出重叠；重叠检查在任何复制之前完成。
失败不写普通输出，不改变有效数据；只有明确的饱和诊断计数可在容量不足时改变。
成功重新初始化会清空逻辑内容及统计，失败保持旧绑定；实例及存储均由调用方拥有，不释放内存。

同一实例的全部 API（包括查询）必须由调用方串行化。不同实例使用不重叠存储时可重入。
没有内置锁、等待、超时、回调或任务创建，不保证直接用于 ISR/任务并发；
P3 才实现 RTOS 同步适配。主机测试不替代目标 MCU 原子性、WCET、DMA/cache 或上板验证。

内存区间检查是明确的 ABI 边界：依赖目标提供 `uintptr_t`，对象地址转换后保序，
字节地址差对应对象内字节距离。当前验证目标为 Linux GCC/Clang 和 ARM GCC 的平坦地址 ABI。
不适用于未经验证的分段地址或特殊指针平台；不依赖主机字节序。
内存函数采用标准 C11，长度及不重叠已预检；仅相应调用局部抑制 Annex K 建议，
不要求可选 `memcpy_s/memset_s`，不关闭全局分析规则。

## 配置、所有权与接口

`object_pool_config_t` 显式提供 storage/storage_size、states/states_size、
slot_size、capacity 和 alignment。alignment 必须为非零二次幂，存储首地址和槽步长
必须满足该对齐；容量与槽大小均非零，乘法溢出返回 OVERFLOW，任一存储区不足返回 BUFFER_TOO_SMALL。
配置、实例、状态区、数据区不得重叠。状态区每槽一个字节，0 空闲、1 占用。
init 成功仅清零状态区，不清零对象数据；仍有对象借出时返回 BUSY，禁止丢失所有权。

建议直接以真实对象数组作为数据区，用 sizeof(对象) 和 _Alignof(对象) 配置，
这样同时满足 C 有效类型与对齐要求。仅对齐的 uint8_t 数组不能凭对齐就强转成任意对象；
若使用原始字节区，调用方通过 memcpy 读写对象表示。

- `acquire(pool, result, index)`：返回最低空闲槽的 void 指针和从零开始的索引。
  两个输出均必需，不得互相重叠或覆盖池资源。新借出对象内容未初始化，需调用方设置。
- 耗尽返回 FULL，输出不变，exhausted_count 按请求次数增加并在 SIZE_MAX 饱和。
- `release(pool, data)`：只接受池内占用槽首地址；池外/非槽首地址为 INVALID_ARGUMENT，
  已空闲槽为 INVALID_STATE。释放不清零载荷；释放前由对象所有者清理内部资源与链接。
- `capacity/available/lowest_free_index/exhausted_count`：状态码加 size_t 输出；
  没有空闲槽时 lowest_free_index == capacity，避免无效索引被当作可用槽。
- 指针释放并重新分配到同一地址后，库无法识别旧指针再次释放的误用。
  本批按确认方案不引入代次句柄，不宣称能检测该情况；归还后旧引用立即失效。

## 资源与验证

RAM 为 sizeof(object_pool_t) + capacity × slot_size + capacity 字节。
init O(capacity)，acquire 最坏 O(capacity) 以寻找下一最低空闲槽；
release 和查询 O(1)，栈 O(1)。扫描有固定上限，但不是恒定时间分配算法。
测试覆盖对齐、大小溢出、状态区容量、耗尽、最小槽选择、非法/重复释放、
有借用时重绑 BUSY、统计饱和。无 RTOS/ISR 时间保证，WCET 留待实际目标测量。
证据见 [P1 阶段计划](../plans/P1_FOUNDATION.md)。
