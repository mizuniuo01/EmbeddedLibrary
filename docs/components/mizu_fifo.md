# mizu_fifo 组件说明

`mizu_fifo` 属于 Libraries 层，只负责有界字节暂存和顺序读写。它参考 UART/DMA 环形缓冲语义，但不依赖 UART、DMA、BSP、HAL 或 RTOS。

## 所有权和生命周期

- FIFO 对象和 storage 由上层 Services/communication 或 Composition Root 提供。
- FIFO 对象在调用 `mizu_fifo_init()` 前不需要清零；初始化会完整建立对象状态。
- `mizu_fifo_init()` 在装配阶段只调用一次；重复初始化不属于接口契约，不提供 reset、deinit 或运行期更换 storage。
- storage 大小为 `storage_size`，必须至少为 2；实现保留一个空槽，因此实际可用容量是 `storage_size - 1`。
- 调用方保证 FIFO 对象、storage 和读写数据区互不重叠，并保证 storage 生命周期有效。

## 并发

组件按单生产者/单消费者使用：一个上下文写入，一个上下文读取。组件不提供原子操作、内存屏障、临界区或 RTOS 同步；平台和上层负责保证索引访问与数据可见性。

## 数据语义

- 写入不覆盖未读数据。
- 空间不足时完整写入返回 `MIZU_STATUS_FULL`，FIFO 状态不变。
- 数据不足时完整读取返回 `MIZU_STATUS_EMPTY`，FIFO 状态和输出不变。
- 零长度读写返回 `MIZU_STATUS_OK`，不访问数据指针，不移动游标。
- `push/pop` 是长度为 1 的薄包装。
- 帧头、帧尾、校验、超时和解析状态属于后续 frame/communication 层。
