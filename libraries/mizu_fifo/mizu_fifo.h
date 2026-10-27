#ifndef MIZU_FIFO_H
#define MIZU_FIFO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mizu_status.h"

/* 单生产者/单消费者的通信字节暂存区；成员仅用于静态分配，不得直接访问。 */
typedef struct {
    uint8_t *storage;
    size_t storage_size;
    size_t read_index;
    size_t write_index;
    bool initialized;
} mizu_fifo_t;

/**
 * mizu_fifo_init - 绑定调用方提供的存储区。
 * @fifo: 待初始化的 FIFO 对象，不要求预先清零。
 * @storage: 调用方长期持有的字节存储区。
 * @storage_size: @storage 的字节数，必须至少为 2。
 *
 * 仅在装配阶段调用一次；对象和存储区不得重叠。函数不阻塞。
 * 返回值：成功返回 MIZU_STATUS_OK；参数无效返回 MIZU_STATUS_INVALID_ARGUMENT。
 */
mizu_status_t mizu_fifo_init(mizu_fifo_t *fifo, uint8_t *storage, size_t storage_size);

/**
 * mizu_fifo_write - 完整写入一组字节。
 * @fifo: 已初始化的 FIFO 对象。
 * @data: 生产者提供的输入数据；@data_size 为 0 时可以为空。
 * @data_size: 本次写入的字节数。
 *
 * 仅由一个生产者调用，函数不阻塞。空间不足时不写入任何数据。
 * 返回值：成功返回 MIZU_STATUS_OK；空间不足返回 MIZU_STATUS_FULL；对象或参数
 * 无效时返回对应状态。
 */
mizu_status_t mizu_fifo_write(mizu_fifo_t *fifo, const uint8_t *data, size_t data_size);

/**
 * mizu_fifo_read - 完整读取一组字节。
 * @fifo: 已初始化的 FIFO 对象。
 * @data: 消费者提供的输出缓冲区；@data_size 为 0 时可以为空。
 * @data_size: 本次读取的字节数。
 *
 * 仅由一个消费者调用，函数不阻塞。数据不足时不修改 FIFO 和输出缓冲区。
 * 返回值：成功返回 MIZU_STATUS_OK；数据不足返回 MIZU_STATUS_EMPTY；对象或参数
 * 无效时返回对应状态。
 */
mizu_status_t mizu_fifo_read(mizu_fifo_t *fifo, uint8_t *data, size_t data_size);

/**
 * mizu_fifo_push - 写入一个字节。
 * @fifo: 已初始化的 FIFO 对象。
 * @value: 待写入的字节。
 *
 * 返回值：与 mizu_fifo_write() 写入一个字节时相同。
 */
mizu_status_t mizu_fifo_push(mizu_fifo_t *fifo, uint8_t value);

/**
 * mizu_fifo_pop - 读取一个字节。
 * @fifo: 已初始化的 FIFO 对象。
 * @value: 调用方提供的输出地址。
 *
 * 返回值：与 mizu_fifo_read() 读取一个字节时相同。
 */
mizu_status_t mizu_fifo_pop(mizu_fifo_t *fifo, uint8_t *value);

#endif /* MIZU_FIFO_H */
