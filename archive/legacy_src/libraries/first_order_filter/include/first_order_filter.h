#ifndef FIRST_ORDER_FILTER_H
#define FIRST_ORDER_FILTER_H /* 头文件保护 */

#include <stdbool.h>

#include "foundation_status.h"

/* 一阶滤波器状态；初始化后禁止调用方直接修改或按值复制。 */
typedef struct {
    float alpha;
    float output;
    bool initialized;
} first_order_filter_f32_t;

/* 生命周期和滤波接口。 */
foundation_status_t first_order_filter_f32_init(first_order_filter_f32_t *filter, float alpha,
    float initial_output);
foundation_status_t first_order_filter_f32_reset(first_order_filter_f32_t *filter, float output);
foundation_status_t first_order_filter_f32_update(first_order_filter_f32_t *filter, float input,
    float *output);

#endif /* FIRST_ORDER_FILTER_H */
