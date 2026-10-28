/**
 * @file    cobs.c
 * @brief   Consistent Overhead Byte Stuffing 无状态编解码。
 */

#include "cobs.h"

#include <stdint.h>

/**
 * @brief 判断两个字节区间是否重叠。
 * @param left 第一个区间起始地址。
 * @param left_size 第一个区间长度。
 * @param right 第二个区间起始地址。
 * @param right_size 第二个区间长度。
 * @return 区间重叠或地址计算溢出时返回非零，否则返回零。
 */
static int cobs_ranges_overlap(const uint8_t *left, size_t left_size, const uint8_t *right,
    size_t right_size)
{
    uintptr_t left_address;
    uintptr_t right_address;
    if ((left_size == 0U) || (right_size == 0U)) {
        return 0;
    }
    left_address = (uintptr_t)left;
    right_address = (uintptr_t)right;
    if (left_address > UINTPTR_MAX - left_size || right_address > UINTPTR_MAX - right_size) {
        return 1;
    }
    return (left_address < right_address + right_size) &&
           (right_address < left_address + left_size);
}

/**
 * @brief 计算 COBS 编码在不含分隔符时的最大长度。
 * @param input_size 输入字节数。
 * @param encoded_size 接收最大编码长度；成功时有效。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 输出参数为空。
 * @retval FOUNDATION_STATUS_OVERFLOW 长度计算溢出。
 */
foundation_status_t cobs_max_encoded_size(size_t input_size, size_t *encoded_size)
{
    size_t overhead;
    if (!encoded_size) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    overhead = input_size / 254U + 1U;
    if (input_size > SIZE_MAX - overhead) {
        return FOUNDATION_STATUS_OVERFLOW;
    }
    *encoded_size = input_size + overhead;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 校验 COBS 编码并计算解码长度。
 * @param encoded 编码数据；encoded_size 为零时不允许为空。
 * @param encoded_size 编码长度。
 * @param decoded_size 接收解码长度；成功时有效。
 * @retval FOUNDATION_STATUS_OK 校验成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数无效。
 * @retval FOUNDATION_STATUS_INVALID_DATA 编码格式非法。
 */
foundation_status_t cobs_decoded_size(const uint8_t *encoded, size_t encoded_size,
    size_t *decoded_size)
{
    size_t index = 0U;
    size_t result = 0U;
    if (!decoded_size || ((!encoded) && (encoded_size > 0U))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (encoded_size == 0U) {
        return FOUNDATION_STATUS_INVALID_DATA;
    }
    while (index < encoded_size) {
        size_t code = encoded[index++];
        size_t copy_size = code - 1U;
        if ((code == 0U) || (copy_size > encoded_size - index)) {
            return FOUNDATION_STATUS_INVALID_DATA;
        }
        if (result > SIZE_MAX - copy_size) {
            return FOUNDATION_STATUS_OVERFLOW;
        }
        result += copy_size;
        index += copy_size;
        if ((code != 0xFFU) && (index < encoded_size)) {
            result++;
        }
    }
    *decoded_size = result;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 编码一个 COBS 数据块。
 * @param input 输入数据；input_size 为零时允许为空。
 * @param input_size 输入长度。
 * @param output 输出缓冲区。
 * @param output_size 输出容量。
 * @param encoded_size 接收编码长度；成功时有效。
 * @retval FOUNDATION_STATUS_OK 编码成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数或地址重叠非法。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t cobs_encode(const uint8_t *input, size_t input_size, uint8_t *output,
    size_t output_size, size_t *encoded_size)
{
    size_t read_index = 0U, write_index = 1U, code_index = 0U;
    uint8_t code = 1U;
    size_t required;
    if (!encoded_size || !output || (!input && input_size > 0U)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (cobs_ranges_overlap(input, input_size, output, output_size)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (cobs_max_encoded_size(input_size, &required) != FOUNDATION_STATUS_OK ||
        output_size < required)
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    while (read_index < input_size) {
        if (input[read_index] == 0U) {
            output[code_index] = code;
            code_index = write_index++;
            code = 1U;
        } else {
            output[write_index++] = input[read_index];
            code++;
            if (code == 0xFFU) {
                output[code_index] = code;
                code_index = write_index++;
                code = 1U;
            }
        }
        read_index++;
    }
    output[code_index] = code;
    *encoded_size = write_index;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 解码一个 COBS 数据块。
 * @param input 编码数据。
 * @param input_size 编码长度。
 * @param output 输出缓冲区。
 * @param output_size 输出容量。
 * @param decoded_size 接收解码长度；成功时有效。
 * @retval FOUNDATION_STATUS_OK 解码成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数非法。
 * @retval FOUNDATION_STATUS_INVALID_DATA 编码格式非法。
 * @retval FOUNDATION_STATUS_BUFFER_TOO_SMALL 输出容量不足。
 */
foundation_status_t cobs_decode(const uint8_t *input, size_t input_size, uint8_t *output,
    size_t output_size, size_t *decoded_size)
{
    size_t required, in_index = 0U, out_index = 0U;
    foundation_status_t status;
    if (!decoded_size || !output || ((!input) && input_size > 0U)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if ((input != output) && cobs_ranges_overlap(input, input_size, output, output_size)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    status = cobs_decoded_size(input, input_size, &required);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (output_size < required) {
        return FOUNDATION_STATUS_BUFFER_TOO_SMALL;
    }
    while (in_index < input_size) {
        size_t code = input[in_index++], copy_size = code - 1U;
        size_t index;
        for (index = 0U; index < copy_size; index++) {
            output[out_index++] = input[in_index++];
        }
        if (code != 0xFFU && in_index < input_size) {
            output[out_index++] = 0U;
        }
    }
    *decoded_size = out_index;
    return FOUNDATION_STATUS_OK;
}
