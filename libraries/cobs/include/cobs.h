#ifndef COBS_H
#define COBS_H /* 头文件保护 */

#include <stddef.h>
#include <stdint.h>

#include "foundation_status.h"

/* COBS 编码长度和无状态编解码接口；编码结果不包含零分隔符。 */
foundation_status_t cobs_max_encoded_size(size_t input_size, size_t *encoded_size);
foundation_status_t cobs_decoded_size(const uint8_t *encoded, size_t encoded_size,
    size_t *decoded_size);
foundation_status_t cobs_encode(const uint8_t *input, size_t input_size, uint8_t *output,
    size_t output_size, size_t *encoded_size);
foundation_status_t cobs_decode(const uint8_t *input, size_t input_size, uint8_t *output,
    size_t output_size, size_t *decoded_size);

#endif /* COBS_H */
