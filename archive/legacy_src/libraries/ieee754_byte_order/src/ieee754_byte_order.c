/**
 * @file    ieee754_byte_order.c
 * @brief   IEEE-754 浮点位模式的显式大小端编解码。
 */

#include "ieee754_byte_order.h"

#include <float.h>
#include <string.h>

_Static_assert(sizeof(float) == 4U, "f32 requires four-byte float");
_Static_assert(sizeof(double) == 8U, "f64 requires eight-byte double");
_Static_assert(FLT_RADIX == 2, "f32 requires binary radix");
_Static_assert(FLT_RADIX == 2, "f64 requires binary radix");

/* NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)：memcpy 仅用于固定宽度浮点位模式搬运。 */
/**
 * @brief 从字节序列读取浮点位模式。
 * @param source 输入字节。
 * @param size 输入容量。
 * @param width 浮点位宽，必须为 4 或 8 字节。
 * @param big_endian 是否采用大端顺序。
 * @param value 接收结果；成功时有效。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
static foundation_status_t ieee_read(const uint8_t *source, size_t size, size_t width,
    int big_endian, void *value)
{
    uint64_t bits = 0U;
    size_t index;
    if (!value || ((!source) && (size > 0U))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (size < width) {
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    }
    for (index = 0U; index < width; index++) {
        size_t shift = big_endian ? width - 1U - index : index;
        bits |= ((uint64_t)source[index]) << (shift * 8U);
    }
    if (width == 4U) {
        uint32_t fbits = (uint32_t)bits;
        (void)memcpy(value, &fbits, sizeof(fbits));
    } else {
        (void)memcpy(value, &bits, sizeof(bits));
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 将浮点位模式写入字节序列。
 * @param destination 输出字节。
 * @param size 输出容量。
 * @param width 浮点位宽，必须为 4 或 8 字节。
 * @param big_endian 是否采用大端顺序。
 * @param value 待读取的浮点值。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
static foundation_status_t ieee_write(uint8_t *destination, size_t size, size_t width,
    int big_endian, const void *value)
{
    uint64_t bits = 0U;
    size_t index;
    if (!destination || !value) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (size < width) {
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    }
    if (width == 4U) {
        uint32_t fbits;
        (void)memcpy(&fbits, value, sizeof(fbits));
        bits = fbits;
    } else {
        (void)memcpy(&bits, value, sizeof(bits));
    }
    for (index = 0U; index < width; index++) {
        size_t shift = big_endian ? width - 1U - index : index;
        destination[index] = (uint8_t)(bits >> (shift * 8U));
    }
    return FOUNDATION_STATUS_OK;
}
/* NOLINTEND(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) */

/**
 * @brief 按小端读取 IEEE-754 binary32。
 * @param s 输入字节；长度不足时不读取。
 * @param n 输入缓冲区容量，至少为 4 字节。
 * @param v 接收浮点位模式；成功时有效。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t ieee754_byte_order_read_f32_le(const uint8_t *s, size_t n, float *v)
{
    return ieee_read(s, n, 4U, 0, v);
}
/**
 * @brief 按大端读取 IEEE-754 binary32。
 * @param s 输入字节；长度不足时不读取。
 * @param n 输入缓冲区容量，至少为 4 字节。
 * @param v 接收浮点位模式；成功时有效。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t ieee754_byte_order_read_f32_be(const uint8_t *s, size_t n, float *v)
{
    return ieee_read(s, n, 4U, 1, v);
}
/**
 * @brief 按小端写入 IEEE-754 binary32。
 * @param d 输出字节。
 * @param n 输出缓冲区容量，至少为 4 字节。
 * @param v 待写入浮点位模式。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t ieee754_byte_order_write_f32_le(uint8_t *d, size_t n, float v)
{
    return ieee_write(d, n, 4U, 0, &v);
}
/**
 * @brief 按大端写入 IEEE-754 binary32。
 * @param d 输出字节。
 * @param n 输出缓冲区容量，至少为 4 字节。
 * @param v 待写入浮点位模式。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t ieee754_byte_order_write_f32_be(uint8_t *d, size_t n, float v)
{
    return ieee_write(d, n, 4U, 1, &v);
}
/**
 * @brief 按小端读取 IEEE-754 binary64。
 * @param s 输入字节；长度不足时不读取。
 * @param n 输入缓冲区容量，至少为 8 字节。
 * @param v 接收浮点位模式；成功时有效。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t ieee754_byte_order_read_f64_le(const uint8_t *s, size_t n, double *v)
{
    return ieee_read(s, n, 8U, 0, v);
}
/**
 * @brief 按大端读取 IEEE-754 binary64。
 * @param s 输入字节；长度不足时不读取。
 * @param n 输入缓冲区容量，至少为 8 字节。
 * @param v 接收浮点位模式；成功时有效。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输入容量不足。
 */
foundation_status_t ieee754_byte_order_read_f64_be(const uint8_t *s, size_t n, double *v)
{
    return ieee_read(s, n, 8U, 1, v);
}
/**
 * @brief 按小端写入 IEEE-754 binary64。
 * @param d 输出字节。
 * @param n 输出缓冲区容量，至少为 8 字节。
 * @param v 待写入浮点位模式。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t ieee754_byte_order_write_f64_le(uint8_t *d, size_t n, double v)
{
    return ieee_write(d, n, 8U, 0, &v);
}
/**
 * @brief 按大端写入 IEEE-754 binary64。
 * @param d 输出字节。
 * @param n 输出缓冲区容量，至少为 8 字节。
 * @param v 待写入浮点位模式。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t ieee754_byte_order_write_f64_be(uint8_t *d, size_t n, double v)
{
    return ieee_write(d, n, 8U, 1, &v);
}
