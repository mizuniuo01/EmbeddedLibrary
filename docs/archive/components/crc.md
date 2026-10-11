# crc

所属层级：Libraries。职责：提供 CRC-8/SMBUS、CRC-16/CCITT-FALSE 和 CRC-32/ISO-HDLC 的无表计算。

每种算法拥有独立 context，支持一次性和分块计算；`init`、`update`、`finalize` 和 `calculate` 均返回 `foundation_status_t`。数据输入为 `NULL + 0` 时表示空消息；非法输入不得静默改变 context，结果输出仅在成功时写入。

算法状态不含平台或 RTOS 类型，更新操作 O(8×字节数)，无动态内存。测试使用标准 `123456789` check vector、空消息和分块等价性。
