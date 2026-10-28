#ifndef HYSTERESIS_H
#define HYSTERESIS_H /* 头文件保护 */

#include <stdbool.h>

#include "foundation_status.h"

/* 二值迟滞状态；初始化后字段由组件维护。 */
typedef struct {
    float lower;
    float upper;
    float low_output;
    float high_output;
    bool high;
    bool initialized;
} hysteresis_f32_t;

foundation_status_t hysteresis_f32_init(hysteresis_f32_t *hysteresis, float lower, float upper,
    float low_output, float high_output, bool initial_high);
foundation_status_t hysteresis_f32_reset(hysteresis_f32_t *hysteresis, bool initial_high);
foundation_status_t hysteresis_f32_update(hysteresis_f32_t *hysteresis, float input, float *output);

#endif /* HYSTERESIS_H */
