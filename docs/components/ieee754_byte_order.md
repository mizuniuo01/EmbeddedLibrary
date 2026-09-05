# ieee754_byte_order

所属层级：Libraries。职责：按 LE/BE 顺序搬运 IEEE-754 binary32/binary64 位模式。

组件独立于整数 `byte_order`，只通过 `memcpy` 搬运位模式；原样保留 NaN、Inf、负零和 NaN payload，不负责有限性判断。

要求 `float` 为 4 字节、`double` 为 8 字节二进制格式。所有操作 O(1)、无阻塞、无动态内存、可重入。

测试覆盖普通值、特殊值、位模式往返、容量不足和空指针。
