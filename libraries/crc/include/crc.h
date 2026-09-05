#ifndef CRC_H
#define CRC_H /* 头文件保护 */

#include <stddef.h>
#include <stdint.h>

#include "foundation_status.h"

/* 三种固定 CRC 算法的增量计算状态。 */
typedef struct {
    uint8_t value;
} crc8_smbus_context_t;
typedef struct {
    uint16_t value;
} crc16_ccitt_false_context_t;
typedef struct {
    uint32_t value;
} crc32_iso_hdlc_context_t;

/* CRC-8/SMBUS：多项式 0x07，初值 0，结果异或 0。 */
foundation_status_t crc8_smbus_init(crc8_smbus_context_t *context);
foundation_status_t crc8_smbus_update(crc8_smbus_context_t *context, const uint8_t *data,
    size_t size);
foundation_status_t crc8_smbus_finalize(const crc8_smbus_context_t *context, uint8_t *value);
foundation_status_t crc8_smbus_calculate(const uint8_t *data, size_t size, uint8_t *value);

/* CRC-16/CCITT-FALSE：多项式 0x1021，初值 0xFFFF。 */
foundation_status_t crc16_ccitt_false_init(crc16_ccitt_false_context_t *context);
foundation_status_t crc16_ccitt_false_update(crc16_ccitt_false_context_t *context,
    const uint8_t *data, size_t size);
foundation_status_t crc16_ccitt_false_finalize(const crc16_ccitt_false_context_t *context,
    uint16_t *value);
foundation_status_t crc16_ccitt_false_calculate(const uint8_t *data, size_t size, uint16_t *value);

/* CRC-32/ISO-HDLC：多项式 0xEDB88320（反射表示），初值和结果异或均为 0xFFFFFFFF。 */
foundation_status_t crc32_iso_hdlc_init(crc32_iso_hdlc_context_t *context);
foundation_status_t crc32_iso_hdlc_update(crc32_iso_hdlc_context_t *context, const uint8_t *data,
    size_t size);
foundation_status_t crc32_iso_hdlc_finalize(const crc32_iso_hdlc_context_t *context,
    uint32_t *value);
foundation_status_t crc32_iso_hdlc_calculate(const uint8_t *data, size_t size, uint32_t *value);

#endif /* CRC_H */
