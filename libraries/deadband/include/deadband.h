#ifndef DEADBAND_H
#define DEADBAND_H /* 头文件保护 */

#include "foundation_status.h"

/* 中心回零死区配置。 */
typedef struct {
    float center;
    float half_width;
    int initialized;
} deadband_f32_t;

foundation_status_t deadband_f32_init(deadband_f32_t *deadband, float center, float half_width);
foundation_status_t deadband_f32_update(const deadband_f32_t *deadband, float input, float *output);

#endif /* DEADBAND_H */
