/**
 * @file    crc.c
 * @brief   固定 CRC 算法的无表增量计算。
 */

#include "crc.h"

/**
 * @brief 初始化 CRC-8/SMBUS 状态。
 * @param context 计算状态。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT context 为空。
 */
foundation_status_t crc8_smbus_init(crc8_smbus_context_t *context)
{
    if (!context) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    context->value = 0U;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 更新 CRC-8/SMBUS 状态。
 * @param context 计算状态。
 * @param data 输入字节；size 为零时允许为空。
 * @param size 输入字节数。
 * @retval FOUNDATION_STATUS_OK 更新成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数无效。
 */
foundation_status_t crc8_smbus_update(crc8_smbus_context_t *context, const uint8_t *data,
    size_t size)
{
    size_t i;
    uint8_t bit;
    if (!context || ((!data) && (size > 0U))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    for (i = 0U; i < size; i++) {
        context->value ^= data[i];
        for (bit = 0U; bit < 8U; bit++) {
            context->value =
                (uint8_t)(((uint16_t)context->value << 1U) ^
                          ((context->value & 0x80U) ? UINT16_C(0x07) : UINT16_C(0x00)));
        }
    }
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 获取 CRC-8/SMBUS 结果。
 * @param context 计算状态。
 * @param value 接收结果；成功时有效。
 * @retval FOUNDATION_STATUS_OK 获取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 */
foundation_status_t crc8_smbus_finalize(const crc8_smbus_context_t *context, uint8_t *value)
{
    if (!context || !value) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *value = context->value;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 一次性计算 CRC-8/SMBUS。
 * @param data 输入字节；size 为零时允许为空。
 * @param size 输入字节数。
 * @param value 接收结果；成功时有效。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数无效。
 */
foundation_status_t crc8_smbus_calculate(const uint8_t *data, size_t size, uint8_t *value)
{
    crc8_smbus_context_t context;
    foundation_status_t status = crc8_smbus_init(&context);
    if (status == FOUNDATION_STATUS_OK) {
        status = crc8_smbus_update(&context, data, size);
    }
    if (status == FOUNDATION_STATUS_OK) {
        status = crc8_smbus_finalize(&context, value);
    }
    return status;
}

/**
 * @brief 初始化 CRC-16/CCITT-FALSE 状态。
 * @param context 计算状态。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT context 为空。
 */
foundation_status_t crc16_ccitt_false_init(crc16_ccitt_false_context_t *context)
{
    if (!context) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    context->value = 0xFFFFU;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 更新 CRC-16/CCITT-FALSE 状态。
 * @param context 计算状态。
 * @param data 输入字节；size 为零时允许为空。
 * @param size 输入字节数。
 * @retval FOUNDATION_STATUS_OK 更新成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数无效。
 */
foundation_status_t crc16_ccitt_false_update(crc16_ccitt_false_context_t *context,
    const uint8_t *data, size_t size)
{
    size_t i;
    uint8_t bit;
    if (!context || ((!data) && (size > 0U))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    for (i = 0U; i < size; i++) {
        context->value ^= (uint16_t)data[i] << 8U;
        for (bit = 0U; bit < 8U; bit++) {
            context->value =
                (uint16_t)(((uint32_t)context->value << 1U) ^
                           ((context->value & 0x8000U) ? UINT32_C(0x1021) : UINT32_C(0x0000)));
        }
    }
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 获取 CRC-16/CCITT-FALSE 结果。
 * @param context 计算状态。
 * @param value 接收结果；成功时有效。
 * @retval FOUNDATION_STATUS_OK 获取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 */
foundation_status_t crc16_ccitt_false_finalize(const crc16_ccitt_false_context_t *context,
    uint16_t *value)
{
    if (!context || !value) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *value = context->value;
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 一次性计算 CRC-16/CCITT-FALSE。
 * @param data 输入字节；size 为零时允许为空。
 * @param size 输入字节数。
 * @param value 接收结果；成功时有效。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数无效。
 */
foundation_status_t crc16_ccitt_false_calculate(const uint8_t *data, size_t size, uint16_t *value)
{
    crc16_ccitt_false_context_t context;
    foundation_status_t status = crc16_ccitt_false_init(&context);
    if (status == FOUNDATION_STATUS_OK) {
        status = crc16_ccitt_false_update(&context, data, size);
    }
    if (status == FOUNDATION_STATUS_OK) {
        status = crc16_ccitt_false_finalize(&context, value);
    }
    return status;
}

/**
 * @brief 初始化 CRC-32/ISO-HDLC 状态。
 * @param context 计算状态。
 * @retval FOUNDATION_STATUS_OK 初始化成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT context 为空。
 */
foundation_status_t crc32_iso_hdlc_init(crc32_iso_hdlc_context_t *context)
{
    if (!context) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    context->value = UINT32_C(0xFFFFFFFF);
    return FOUNDATION_STATUS_OK;
}
/**
 * @brief 更新 CRC-32/ISO-HDLC 状态。
 * @param context 计算状态。
 * @param data 输入字节；size 为零时允许为空。
 * @param size 输入字节数。
 * @retval FOUNDATION_STATUS_OK 更新成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数无效。
 */
foundation_status_t crc32_iso_hdlc_update(crc32_iso_hdlc_context_t *context, const uint8_t *data,
    size_t size)
{
    size_t i;
    uint8_t bit;
    if (!context || ((!data) && (size > 0U))) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    for (i = 0U; i < size; i++) {
        context->value ^= data[i];
        for (bit = 0U; bit < 8U; bit++) {
            context->value =
                (context->value >> 1U) ^ ((context->value & 1U) ? UINT32_C(0xEDB88320) : 0U);
        }
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 获取 CRC-32/ISO-HDLC 结果。
 * @param context 计算状态。
 * @param value 接收结果；成功时有效。
 * @retval FOUNDATION_STATUS_OK 获取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空。
 */
foundation_status_t crc32_iso_hdlc_finalize(const crc32_iso_hdlc_context_t *context,
    uint32_t *value)
{
    if (!context || !value) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    *value = context->value ^ UINT32_C(0xFFFFFFFF);
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 一次性计算 CRC-32/ISO-HDLC。
 * @param data 输入字节；size 为零时允许为空。
 * @param size 输入字节数。
 * @param value 接收结果；成功时有效。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数无效。
 */
foundation_status_t crc32_iso_hdlc_calculate(const uint8_t *data, size_t size, uint32_t *value)
{
    crc32_iso_hdlc_context_t context;
    foundation_status_t status = crc32_iso_hdlc_init(&context);
    if (status == FOUNDATION_STATUS_OK) {
        status = crc32_iso_hdlc_update(&context, data, size);
    }
    if (status == FOUNDATION_STATUS_OK) {
        status = crc32_iso_hdlc_finalize(&context, value);
    }
    return status;
}
