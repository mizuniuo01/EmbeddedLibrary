#ifndef NUMERIC_H
#define NUMERIC_H /* 头文件保护 */

#include <stdbool.h>
#include <stdint.h>

#include "foundation_status.h"

/* 有限值检查 */
bool numeric_f32_is_finite(float value);

/* 浮点限幅 */
foundation_status_t numeric_f32_clamp(float value, float minimum, float maximum, float *result);

/* 有符号整数限幅 */
foundation_status_t numeric_i32_clamp(int32_t value, int32_t minimum, int32_t maximum,
    int32_t *result);

/* 无符号整数限幅 */
foundation_status_t numeric_u32_clamp(uint32_t value, uint32_t minimum, uint32_t maximum,
    uint32_t *result);

#endif /* NUMERIC_H */
