/**
 * @file main.c
 * @brief 验证组件在仓库外部 CMake 工程中的最小接入。
 */
#include <stdint.h>

#include "mizu_fifo.h"
#include "pid.h"

/**
 * @brief 执行外部接入的最小 FIFO 用例。
 * @return 成功时返回 0，否则返回 1。
 */
int main(void)
{
    uint8_t storage[2];
    uint8_t value = 7U;
    uint8_t output = 0U;
    mizu_fifo_t fifo = {0};
    mizu_pid_t pid = {0};
    const mizu_pid_config_t pid_config = {
        .kp = 1.0F,
        .ki = 0.0F,
        .kd = 0.0F,
        .output_min = -10.0F,
        .output_max = 10.0F,
        .integral_min = -10.0F,
        .integral_max = 10.0F,
        .tracking_gain = 1.0F,
        .anti_windup = MIZU_PID_ANTI_WINDUP_CONDITIONAL,
    };
    float pid_output = 0.0F;
    if (mizu_fifo_init(&fifo, storage, sizeof(storage)) != MIZU_STATUS_OK) {
        return 1;
    }
    if (mizu_fifo_push(&fifo, value) != MIZU_STATUS_OK) {
        return 1;
    }
    if (mizu_fifo_pop(&fifo, &output) != MIZU_STATUS_OK) {
        return 1;
    }
    if (output != value) {
        return 1;
    }
    if ((mizu_pid_init(&pid, &pid_config) != MIZU_STATUS_OK) ||
        (mizu_pid_calculate(&pid, 1.0F, 0.0F, 1.0F, &pid_output) != MIZU_STATUS_OK) ||
        (pid_output != 1.0F)) {
        return 1;
    }
    return 0;
}
