#ifndef IEEE754_BYTE_ORDER_H
#define IEEE754_BYTE_ORDER_H /* 头文件保护 */

#include <stddef.h>
#include <stdint.h>

#include "foundation_status.h"

/* IEEE-754 binary32/binary64 的显式大小端编解码。 */
foundation_status_t ieee754_byte_order_read_f32_le(const uint8_t *, size_t, float *);
foundation_status_t ieee754_byte_order_read_f32_be(const uint8_t *, size_t, float *);
foundation_status_t ieee754_byte_order_write_f32_le(uint8_t *, size_t, float);
foundation_status_t ieee754_byte_order_write_f32_be(uint8_t *, size_t, float);
foundation_status_t ieee754_byte_order_read_f64_le(const uint8_t *, size_t, double *);
foundation_status_t ieee754_byte_order_read_f64_be(const uint8_t *, size_t, double *);
foundation_status_t ieee754_byte_order_write_f64_le(uint8_t *, size_t, double);
foundation_status_t ieee754_byte_order_write_f64_be(uint8_t *, size_t, double);

#endif /* IEEE754_BYTE_ORDER_H */
