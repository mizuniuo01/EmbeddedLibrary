#ifndef MOVING_AVERAGE_H
#define MOVING_AVERAGE_H /* 头文件保护 */

#include <stddef.h>
#include <stdbool.h>

#include "foundation_status.h"

/* 固定窗口平均状态；样本区由调用方提供并由组件拥有其内容。 */
typedef struct {
    float *storage;
    size_t capacity;
    size_t index;
    size_t count;
    float sum;
    bool initialized;
} moving_average_f32_t;

foundation_status_t moving_average_f32_init(moving_average_f32_t *filter, float *storage,
    size_t capacity);
foundation_status_t moving_average_f32_reset(moving_average_f32_t *filter);
foundation_status_t moving_average_f32_update(moving_average_f32_t *filter, float input,
    float *output);
foundation_status_t moving_average_f32_count(const moving_average_f32_t *filter, size_t *count);

#endif /* MOVING_AVERAGE_H */
