#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H /* 头文件保护 */

#include "foundation_status.h"

/* 速率限制器状态。 */
typedef struct {
    float rising_rate;
    float falling_rate;
    float output;
    int initialized;
} rate_limiter_f32_t;

foundation_status_t rate_limiter_f32_init(rate_limiter_f32_t *limiter, float rising_rate,
    float falling_rate, float initial_output);
foundation_status_t rate_limiter_f32_reset(rate_limiter_f32_t *limiter, float output);
foundation_status_t rate_limiter_f32_update(rate_limiter_f32_t *limiter, float input, float dt,
    float *output);

#endif /* RATE_LIMITER_H */
