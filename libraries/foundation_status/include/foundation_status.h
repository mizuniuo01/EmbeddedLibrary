#ifndef FOUNDATION_STATUS_H
#define FOUNDATION_STATUS_H /* 头文件保护 */

/* 基础组件公共操作结果；仅用于 C API，不直接编码为协议或持久化字段。 */
typedef enum {
    FOUNDATION_STATUS_OK = 0,               /* 操作成功。 */
    FOUNDATION_STATUS_INVALID_ARGUMENT = 1, /* 参数为空、重叠或组合非法。 */
    FOUNDATION_STATUS_NOT_INITIALIZED = 2,  /* 实例尚未初始化。 */
    FOUNDATION_STATUS_INVALID_STATE = 3,    /* 当前生命周期状态不允许操作。 */
    FOUNDATION_STATUS_OUT_OF_RANGE = 4,     /* 数值、索引或地址超出支持范围。 */
    FOUNDATION_STATUS_EMPTY = 5,            /* 容器或数据源为空。 */
    FOUNDATION_STATUS_FULL = 6,             /* 容器或资源池已满。 */
    FOUNDATION_STATUS_BUFFER_TOO_SMALL = 7, /* 输出缓冲区容量不足。 */
    FOUNDATION_STATUS_INVALID_DATA = 8,     /* 输入数据格式或数值无效。 */
    FOUNDATION_STATUS_OVERFLOW = 9,         /* 算术或容量计算发生溢出。 */
    FOUNDATION_STATUS_BUSY = 10,            /* 能力当前正被占用。 */
    FOUNDATION_STATUS_TIMEOUT = 11,         /* 操作未在约定时间内完成。 */
    FOUNDATION_STATUS_IO_ERROR = 12,        /* 外部输入输出能力报告错误。 */
    FOUNDATION_STATUS_UNAVAILABLE = 13,     /* 当前环境不提供请求的能力。 */
    FOUNDATION_STATUS_NOT_FOUND = 14,       /* 请求的对象或记录不存在。 */
    FOUNDATION_STATUS_CANCELLED = 15,       /* 操作已被明确取消。 */
    FOUNDATION_STATUS_INTERNAL_ERROR = 16,  /* 组件内部不变量被破坏。 */
} foundation_status_t;

#endif /* FOUNDATION_STATUS_H */
