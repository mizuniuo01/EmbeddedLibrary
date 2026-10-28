/**
 * @file    byte_order.c
 * @brief   固定宽度整数的显式大小端编解码。
 */

#include "byte_order.h"

#include <stdbool.h>
#include <string.h>

/**
 * @brief 按指定字节序读取无符号位模式。
 * @param source 输入字节。
 * @param source_size 输入容量。
 * @param width 读取宽度，单位为字节。
 * @param big_endian 是否采用大端顺序。
 * @param value 接收位模式；成功时有效。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
static foundation_status_t byte_order_read_unsigned(const uint8_t *source, size_t source_size,
    size_t width, bool big_endian, uint64_t *value)
{
    size_t index;
    uint64_t result = 0U;

    if (!value || ((!source) && (source_size > 0U))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (source_size < width) {
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    }
    for (index = 0U; index < width; index++) {
        size_t shift_index = big_endian ? (width - 1U - index) : index;
        result |= ((uint64_t)source[index]) << (shift_index * 8U);
    }
    *value = result;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 按指定字节序写入无符号位模式。
 * @param destination 输出字节。
 * @param destination_size 输出容量。
 * @param width 写入宽度，单位为字节。
 * @param big_endian 是否采用大端顺序。
 * @param value 待写入位模式。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
static foundation_status_t byte_order_write_unsigned(uint8_t *destination, size_t destination_size,
    size_t width, bool big_endian, uint64_t value)
{
    size_t index;

    if (!destination) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (destination_size < width) {
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    }
    for (index = 0U; index < width; index++) {
        size_t shift_index = big_endian ? (width - 1U - index) : index;
        destination[index] = (uint8_t)(value >> (shift_index * 8U));
    }
    return FOUNDATION_STATUS_OK;
}

/* 固定宽度位模式搬运使用 memcpy，长度均由 sizeof 确定。 */
// NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
/**
 * @brief 将有符号二进制补码位模式从字节序列转换到目标对象。
 * @param source 输入字节。
 * @param source_size 输入容量。
 * @param width 读取宽度，单位为字节。
 * @param big_endian 是否采用大端顺序。
 * @param value 接收有符号值；成功时有效。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
static foundation_status_t byte_order_read_signed(const uint8_t *source, size_t source_size,
    size_t width, bool big_endian, void *value)
{
    uint64_t unsigned_value;
    foundation_status_t status;

    if (!value) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    status = byte_order_read_unsigned(source, source_size, width, big_endian, &unsigned_value);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (width == sizeof(int16_t)) {
        int16_t signed_value;
        uint16_t bits = (uint16_t)unsigned_value;
        (void)memcpy(&signed_value, &bits, sizeof(signed_value));
        (void)memcpy(value, &signed_value, sizeof(signed_value));
    } else if (width == sizeof(int32_t)) {
        int32_t signed_value;
        uint32_t bits = (uint32_t)unsigned_value;
        (void)memcpy(&signed_value, &bits, sizeof(signed_value));
        (void)memcpy(value, &signed_value, sizeof(signed_value));
    } else {
        int64_t signed_value;
        (void)memcpy(&signed_value, &unsigned_value, sizeof(signed_value));
        (void)memcpy(value, &signed_value, sizeof(signed_value));
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 将有符号二进制补码位模式写入字节序列。
 * @param destination 输出字节。
 * @param destination_size 输出容量。
 * @param width 写入宽度，单位为字节。
 * @param big_endian 是否采用大端顺序。
 * @param value 待写入有符号值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
static foundation_status_t byte_order_write_signed(uint8_t *destination, size_t destination_size,
    size_t width, bool big_endian, const void *value)
{
    uint64_t bits = 0U;

    if (!value) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (width == sizeof(int16_t)) {
        int16_t signed_value;
        uint16_t unsigned_value;
        (void)memcpy(&signed_value, value, sizeof(signed_value));
        (void)memcpy(&unsigned_value, &signed_value, sizeof(unsigned_value));
        bits = unsigned_value;
    } else if (width == sizeof(int32_t)) {
        int32_t signed_value;
        uint32_t unsigned_value;
        (void)memcpy(&signed_value, value, sizeof(signed_value));
        (void)memcpy(&unsigned_value, &signed_value, sizeof(unsigned_value));
        bits = unsigned_value;
    } else {
        int64_t signed_value;
        (void)memcpy(&signed_value, value, sizeof(signed_value));
        (void)memcpy(&bits, &signed_value, sizeof(bits));
    }
    return byte_order_write_unsigned(destination, destination_size, width, big_endian, bits);
}
/* NOLINTEND(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) */

/**
 * @brief 按小端读取 uint16_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 2 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_u16_le(const uint8_t *source, size_t source_size,
    uint16_t *value)
{
    uint64_t decoded;
    foundation_status_t status =
        byte_order_read_unsigned(source, source_size, sizeof(uint16_t), false, &decoded);
    if (status == FOUNDATION_STATUS_OK) {
        *value = (uint16_t)decoded;
    }
    return status;
}
/**
 * @brief 按大端读取 uint16_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 2 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_u16_be(const uint8_t *source, size_t source_size,
    uint16_t *value)
{
    uint64_t decoded;
    foundation_status_t status =
        byte_order_read_unsigned(source, source_size, sizeof(uint16_t), true, &decoded);
    if (status == FOUNDATION_STATUS_OK) {
        *value = (uint16_t)decoded;
    }
    return status;
}
/**
 * @brief 按小端读取 uint32_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 4 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_u32_le(const uint8_t *source, size_t source_size,
    uint32_t *value)
{
    uint64_t decoded;
    foundation_status_t status =
        byte_order_read_unsigned(source, source_size, sizeof(uint32_t), false, &decoded);
    if (status == FOUNDATION_STATUS_OK) {
        *value = (uint32_t)decoded;
    }
    return status;
}
/**
 * @brief 按大端读取 uint32_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 4 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_u32_be(const uint8_t *source, size_t source_size,
    uint32_t *value)
{
    uint64_t decoded;
    foundation_status_t status =
        byte_order_read_unsigned(source, source_size, sizeof(uint32_t), true, &decoded);
    if (status == FOUNDATION_STATUS_OK) {
        *value = (uint32_t)decoded;
    }
    return status;
}
/**
 * @brief 按小端读取 uint64_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 8 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_u64_le(const uint8_t *source, size_t source_size,
    uint64_t *value)
{
    return byte_order_read_unsigned(source, source_size, sizeof(uint64_t), false, value);
}
/**
 * @brief 按大端读取 uint64_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 8 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_u64_be(const uint8_t *source, size_t source_size,
    uint64_t *value)
{
    return byte_order_read_unsigned(source, source_size, sizeof(uint64_t), true, value);
}
/**
 * @brief 按小端写入 uint16_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 2 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_u16_le(uint8_t *destination, size_t destination_size,
    uint16_t value)
{
    return byte_order_write_unsigned(destination, destination_size, sizeof(uint16_t), false, value);
}
/**
 * @brief 按大端写入 uint16_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 2 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_u16_be(uint8_t *destination, size_t destination_size,
    uint16_t value)
{
    return byte_order_write_unsigned(destination, destination_size, sizeof(uint16_t), true, value);
}
/**
 * @brief 按小端写入 uint32_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 4 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_u32_le(uint8_t *destination, size_t destination_size,
    uint32_t value)
{
    return byte_order_write_unsigned(destination, destination_size, sizeof(uint32_t), false, value);
}
/**
 * @brief 按大端写入 uint32_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 4 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_u32_be(uint8_t *destination, size_t destination_size,
    uint32_t value)
{
    return byte_order_write_unsigned(destination, destination_size, sizeof(uint32_t), true, value);
}
/**
 * @brief 按小端写入 uint64_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 8 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_u64_le(uint8_t *destination, size_t destination_size,
    uint64_t value)
{
    return byte_order_write_unsigned(destination, destination_size, sizeof(uint64_t), false, value);
}
/**
 * @brief 按大端写入 uint64_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 8 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_u64_be(uint8_t *destination, size_t destination_size,
    uint64_t value)
{
    return byte_order_write_unsigned(destination, destination_size, sizeof(uint64_t), true, value);
}

/**
 * @brief 按小端读取 int16_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 2 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_i16_le(const uint8_t *source, size_t source_size,
    int16_t *value)
{
    return byte_order_read_signed(source, source_size, sizeof(int16_t), false, value);
}
/**
 * @brief 按大端读取 int16_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 2 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_i16_be(const uint8_t *source, size_t source_size,
    int16_t *value)
{
    return byte_order_read_signed(source, source_size, sizeof(int16_t), true, value);
}
/**
 * @brief 按小端读取 int32_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 4 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_i32_le(const uint8_t *source, size_t source_size,
    int32_t *value)
{
    return byte_order_read_signed(source, source_size, sizeof(int32_t), false, value);
}
/**
 * @brief 按大端读取 int32_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 4 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_i32_be(const uint8_t *source, size_t source_size,
    int32_t *value)
{
    return byte_order_read_signed(source, source_size, sizeof(int32_t), true, value);
}
/**
 * @brief 按小端读取 int64_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 8 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_i64_le(const uint8_t *source, size_t source_size,
    int64_t *value)
{
    return byte_order_read_signed(source, source_size, sizeof(int64_t), false, value);
}
/**
 * @brief 按大端读取 int64_t。
 * @param source 输入字节。
 * @param source_size 输入容量，至少为 8 字节。
 * @param value 接收结果；成功时有效，失败时保持不变。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t byte_order_read_i64_be(const uint8_t *source, size_t source_size,
    int64_t *value)
{
    return byte_order_read_signed(source, source_size, sizeof(int64_t), true, value);
}
/**
 * @brief 按小端写入 int16_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 2 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_i16_le(uint8_t *destination, size_t destination_size,
    int16_t value)
{
    return byte_order_write_signed(destination, destination_size, sizeof(int16_t), false, &value);
}
/**
 * @brief 按大端写入 int16_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 2 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_i16_be(uint8_t *destination, size_t destination_size,
    int16_t value)
{
    return byte_order_write_signed(destination, destination_size, sizeof(int16_t), true, &value);
}
/**
 * @brief 按小端写入 int32_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 4 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_i32_le(uint8_t *destination, size_t destination_size,
    int32_t value)
{
    return byte_order_write_signed(destination, destination_size, sizeof(int32_t), false, &value);
}
/**
 * @brief 按大端写入 int32_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 4 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_i32_be(uint8_t *destination, size_t destination_size,
    int32_t value)
{
    return byte_order_write_signed(destination, destination_size, sizeof(int32_t), true, &value);
}
/**
 * @brief 按小端写入 int64_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 8 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_i64_le(uint8_t *destination, size_t destination_size,
    int64_t value)
{
    return byte_order_write_signed(destination, destination_size, sizeof(int64_t), false, &value);
}
/**
 * @brief 按大端写入 int64_t。
 * @param destination 输出字节。
 * @param destination_size 输出容量，至少为 8 字节。
 * @param value 待写入值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t byte_order_write_i64_be(uint8_t *destination, size_t destination_size,
    int64_t value)
{
    return byte_order_write_signed(destination, destination_size, sizeof(int64_t), true, &value);
}
