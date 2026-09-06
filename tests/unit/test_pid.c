/**
 * @file test_pid.c
 * @brief 验证 PID 初始化、计算、复位、限幅和异常输入。
 */
#include "test_support.h"

#include <math.h>

#include "pid.h"

/**
 * @brief 执行 PID 的确定性行为测试。
 * @return 所有断言通过时返回 0，否则返回 1。
 */
int main(void)
{
    pid_f32_t pid = {0};
    pid_f32_config_t config = {.kp = 2.0F,
        .ki = 1.0F,
        .kd = 0.5F,
        .output_min = -10.0F,
        .output_max = 10.0F,
        .integral_min = -5.0F,
        .integral_max = 5.0F,
        .tracking_gain = 0.5F,
        .derivative_mode = PID_DERIVATIVE_ON_MEASUREMENT,
        .anti_windup_mode = PID_ANTI_WINDUP_CONDITIONAL};
    float output = 99.0F;
    pid_f32_config_t invalid = config;
    TEST_ASSERT_STATUS(pid_f32_calculate(&pid, 1.0F, 0.0F, 1.0F, &output),
        FOUNDATION_STATUS_NOT_INITIALIZED);
    TEST_ASSERT_STATUS(pid_f32_init(&pid, &config), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(pid_f32_reset(&pid, 0.0F, 0.0F), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(pid_f32_calculate(&pid, 1.0F, 0.0F, 1.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 3.0F);
    TEST_ASSERT_STATUS(pid_f32_calculate(&pid, 1.0F, 0.0F, 0.0F, &output),
        FOUNDATION_STATUS_INVALID_DATA);
    TEST_ASSERT(output == 3.0F);
    output = 77.0F;
    TEST_ASSERT_STATUS(pid_f32_calculate(&pid, NAN, 0.0F, 1.0F, &output),
        FOUNDATION_STATUS_INVALID_DATA);
    TEST_ASSERT(output == 77.0F);
    invalid.output_min = 2.0F;
    invalid.output_max = 1.0F;
    TEST_ASSERT_STATUS(pid_f32_init(&pid, &invalid), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(pid_f32_calculate(&pid, 1.0F, 0.0F, 1.0F, &output), FOUNDATION_STATUS_OK);
    config.anti_windup_mode = PID_ANTI_WINDUP_BACK_CALCULATION;
    TEST_ASSERT_STATUS(pid_f32_init(&pid, &config), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(pid_f32_reset(&pid, 0.0F, 0.0F), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(pid_f32_calculate(&pid, 100.0F, 0.0F, 0.1F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 10.0F);
    TEST_ASSERT_STATUS(pid_f32_reset(&pid, 100.0F, 0.0F), FOUNDATION_STATUS_OK);
    config.derivative_mode = PID_DERIVATIVE_ON_ERROR;
    TEST_ASSERT_STATUS(pid_f32_init(&pid, &config), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(pid_f32_reset(&pid, 0.0F, 0.0F), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(pid_f32_calculate(&pid, 1.0F, 0.0F, 1.0F, &output), FOUNDATION_STATUS_OK);
    TEST_ASSERT(isfinite(output));
    return 0;
}
