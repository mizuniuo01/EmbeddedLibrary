# mizu_fifo 组件说明

`mizu_fifo` 属于 Libraries 层，只负责有界字节暂存和顺序读写。它参考 UART/DMA 环形缓冲语义，但不依赖 UART、DMA、BSP、HAL 或 RTOS。

## 所有权和生命周期

- FIFO 对象和 storage 应由上层 Services/communication 或其他明确的 owner 模块在其 `.c` 文件内以
  `static` 持有；业务调用方不直接接触 FIFO 对象、storage 或游标。owner 通过更高层通信 API
  对外提供业务语义。
- FIFO 对象在调用 `mizu_fifo_init()` 前不需要清零；初始化会完整建立对象状态。
- `mizu_fifo_init()` 在装配阶段只调用一次；已确认的目标契约是不提供 reset、deinit 或运行期更换
  storage，重复初始化应返回 `MIZU_STATUS_INVALID_STATE` 并保持原对象不变。当前实现尚待修复，
  见 `REVIEW-S2-001`。
- storage 大小为 `storage_size`，必须至少为 2；实现保留一个空槽，因此实际可用容量是 `storage_size - 1`。
- owner 保证 FIFO 对象、storage、DMA 缓冲区、写入输入区和读取输出区互不重叠，并保证 storage
  生命周期有效；mizu_fifo 不执行通用地址范围检测。

典型 owner 结构如下：

```c
/* communication.c */
static mizu_fifo_t s_rx_fifo;
static uint8_t s_rx_storage[128];

mizu_status_t communication_init(void)
{
    return mizu_fifo_init(&s_rx_fifo, s_rx_storage, sizeof(s_rx_storage));
}

mizu_status_t communication_read(uint8_t *data, size_t size)
{
    return mizu_fifo_read(&s_rx_fifo, data, size);
}
```

外部模块只调用 `communication_init()` 和 `communication_read()` 等 owner API，不直接声明或操作
FIFO 对象。

## 并发

组件按单生产者/单消费者使用：一个上下文写入，一个上下文读取。组件不提供原子操作、内存屏障、临界区或 RTOS 同步；平台和上层负责保证索引访问与数据可见性。

## 数据语义

- 写入不覆盖未读数据。
- 空间不足时完整写入返回 `MIZU_STATUS_FULL`，FIFO 状态不变。
- 数据不足时完整读取返回 `MIZU_STATUS_EMPTY`，FIFO 状态和输出不变。
- 零长度读写返回 `MIZU_STATUS_OK`，不访问数据指针，不移动游标。
- `push/pop` 是长度为 1 的薄包装；owner 对外的业务 API 不要求暴露它们。
- 帧头、帧尾、校验、超时和解析状态属于后续 frame/communication 层。

## 调用方与 owner 保证

- owner 在单一装配路径中完成一次初始化，并保证不会重复初始化同一实例。
- owner 保证 storage、输入缓冲区、输出缓冲区和 DMA 缓冲区不与 FIFO storage 重叠。
- owner 负责在 DMA/ISR 与普通上下文之间建立必要的数据可见性和同步边界。
- mizu_fifo 只提供 SPSC 字节暂存，不负责地址重叠检测、并发同步、协议解析或错误恢复策略。
