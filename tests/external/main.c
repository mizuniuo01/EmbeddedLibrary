/**
 * @file main.c
 * @brief 验证组件在仓库外部 CMake 工程中的最小接入。
 */
#include <stddef.h>
#include <stdint.h>

#include "byte_fifo.h"
#include "deadband.h"
#include "first_order_filter.h"
#include "hysteresis.h"
#include "moving_average.h"
#include "rate_limiter.h"

/**
 * @brief 执行外部接入的最小 FIFO 用例。
 * @return 成功时返回 0，否则返回 1。
 */
int main(void)
{
    uint8_t storage[2];
    uint8_t value = 7U;
    uint8_t output = 0U;
    byte_fifo_t fifo = {0};
    first_order_filter_f32_t filter = {0};
    moving_average_f32_t average = {0};
    hysteresis_f32_t hysteresis = {0};
    deadband_f32_t deadband = {0};
    rate_limiter_f32_t limiter = {0};
    float samples[1];
    float result;
    if (byte_fifo_init(&fifo, storage, sizeof(storage)) != FOUNDATION_STATUS_OK) {
        return 1;
    }
    if (byte_fifo_push(&fifo, value) != FOUNDATION_STATUS_OK) {
        return 1;
    }
    if (byte_fifo_pop(&fifo, &output) != FOUNDATION_STATUS_OK) {
        return 1;
    }
    if ((output != value) ||
        (first_order_filter_f32_init(&filter, 1.0F, 0.0F) != FOUNDATION_STATUS_OK) ||
        (moving_average_f32_init(&average, samples, 1U) != FOUNDATION_STATUS_OK) ||
        (hysteresis_f32_init(&hysteresis, -1.0F, 1.0F, 0.0F, 1.0F, false) !=
            FOUNDATION_STATUS_OK) ||
        (deadband_f32_init(&deadband, 0.0F, 1.0F) != FOUNDATION_STATUS_OK) ||
        (rate_limiter_f32_init(&limiter, 1.0F, 1.0F, 0.0F) != FOUNDATION_STATUS_OK)) {
        return 1;
    }
    if ((first_order_filter_f32_update(&filter, 1.0F, &result) != FOUNDATION_STATUS_OK) ||
        (moving_average_f32_update(&average, 1.0F, &result) != FOUNDATION_STATUS_OK) ||
        (hysteresis_f32_update(&hysteresis, 1.0F, &result) != FOUNDATION_STATUS_OK) ||
        (deadband_f32_update(&deadband, 0.0F, &result) != FOUNDATION_STATUS_OK) ||
        (rate_limiter_f32_update(&limiter, 1.0F, 1.0F, &result) != FOUNDATION_STATUS_OK)) {
        return 1;
    }
    return 0;
}
