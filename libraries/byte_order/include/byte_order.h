#ifndef BYTE_ORDER_H
#define BYTE_ORDER_H /* 头文件保护 */

#include <stddef.h>
#include <stdint.h>

#include "foundation_status.h"

/* 无符号整数读取接口。 */
foundation_status_t byte_order_read_u16_le(const uint8_t *source, size_t source_size,
    uint16_t *value);
foundation_status_t byte_order_read_u16_be(const uint8_t *source, size_t source_size,
    uint16_t *value);
foundation_status_t byte_order_read_u32_le(const uint8_t *source, size_t source_size,
    uint32_t *value);
foundation_status_t byte_order_read_u32_be(const uint8_t *source, size_t source_size,
    uint32_t *value);
foundation_status_t byte_order_read_u64_le(const uint8_t *source, size_t source_size,
    uint64_t *value);
foundation_status_t byte_order_read_u64_be(const uint8_t *source, size_t source_size,
    uint64_t *value);

/* 有符号二进制补码整数读取接口。 */
foundation_status_t byte_order_read_i16_le(const uint8_t *source, size_t source_size,
    int16_t *value);
foundation_status_t byte_order_read_i16_be(const uint8_t *source, size_t source_size,
    int16_t *value);
foundation_status_t byte_order_read_i32_le(const uint8_t *source, size_t source_size,
    int32_t *value);
foundation_status_t byte_order_read_i32_be(const uint8_t *source, size_t source_size,
    int32_t *value);
foundation_status_t byte_order_read_i64_le(const uint8_t *source, size_t source_size,
    int64_t *value);
foundation_status_t byte_order_read_i64_be(const uint8_t *source, size_t source_size,
    int64_t *value);

/* 无符号整数写入接口。 */
foundation_status_t byte_order_write_u16_le(uint8_t *destination, size_t destination_size,
    uint16_t value);
foundation_status_t byte_order_write_u16_be(uint8_t *destination, size_t destination_size,
    uint16_t value);
foundation_status_t byte_order_write_u32_le(uint8_t *destination, size_t destination_size,
    uint32_t value);
foundation_status_t byte_order_write_u32_be(uint8_t *destination, size_t destination_size,
    uint32_t value);
foundation_status_t byte_order_write_u64_le(uint8_t *destination, size_t destination_size,
    uint64_t value);
foundation_status_t byte_order_write_u64_be(uint8_t *destination, size_t destination_size,
    uint64_t value);

/* 有符号二进制补码整数写入接口。 */
foundation_status_t byte_order_write_i16_le(uint8_t *destination, size_t destination_size,
    int16_t value);
foundation_status_t byte_order_write_i16_be(uint8_t *destination, size_t destination_size,
    int16_t value);
foundation_status_t byte_order_write_i32_le(uint8_t *destination, size_t destination_size,
    int32_t value);
foundation_status_t byte_order_write_i32_be(uint8_t *destination, size_t destination_size,
    int32_t value);
foundation_status_t byte_order_write_i64_le(uint8_t *destination, size_t destination_size,
    int64_t value);
foundation_status_t byte_order_write_i64_be(uint8_t *destination, size_t destination_size,
    int64_t value);

#endif /* BYTE_ORDER_H */
