#ifndef MIZU_STATUS_H
#define MIZU_STATUS_H

/* 仅用于当前 C API；不用于协议、持久化或跨 ABI 编码。
 * 已有枚举值的数值和语义保持稳定，新增结果只能追加。
 */
typedef enum {
    MIZU_STATUS_OK = 0,             /* 操作成功。 */
    MIZU_STATUS_INVALID_ARGUMENT,   /* 参数、指针或组合非法。 */
    MIZU_STATUS_INVALID_STATE,      /* 对象当前状态不允许操作。 */
    MIZU_STATUS_NOT_INITIALIZED,    /* 对象尚未成功初始化。 */
    MIZU_STATUS_OUT_OF_RANGE,       /* 索引、时间或数值超出支持范围。 */
    MIZU_STATUS_INTERNAL_ERROR,     /* 组件内部不变量被破坏。 */
    MIZU_STATUS_FULL,               /* 容器没有足够空间完成写入。 */
    MIZU_STATUS_EMPTY               /* 容器没有足够数据完成读取。 */
} mizu_status_t;

#endif /* MIZU_STATUS_H */
