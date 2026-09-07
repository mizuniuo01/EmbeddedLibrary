/**
 * @file test_cascaded_pid.c
 * @brief 验证两级串级 PID 的限幅、查询和事务回滚。
 */
#include "test_support.h"
#include "cascaded_pid.h"

/**
 * @brief 执行两级串级 PID 测试。
 * @return 测试通过时返回 0，否则返回 1。
 */
int main(void)
{
    pid_f32_t outer = {0};
    pid_f32_t inner = {0};
    pid_f32_config_t pid_config = {.kp = 1.0F,
        .ki = 0.0F,
        .kd = 0.0F,
        .output_min = -100.0F,
        .output_max = 100.0F,
        .integral_min = -10.0F,
        .integral_max = 10.0F,
        .tracking_gain = 0.0F,
        .derivative_mode = PID_DERIVATIVE_ON_MEASUREMENT,
        .anti_windup_mode = PID_ANTI_WINDUP_CONDITIONAL};
    cascaded_pid_f32_t controller = {0};
    cascaded_pid_f32_config_t config = {&outer, &inner, -2.0F, 2.0F};
    cascaded_pid_f32_input_t input = {10.0F, 0.0F, 0.0F, 1.0F};
    float output = 77.0F;
    float intermediate = 0.0F;

    TEST_ASSERT_STATUS(pid_f32_init(&outer, &pid_config), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(pid_f32_init(&inner, &pid_config), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(cascaded_pid_f32_init(&controller, &config), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(cascaded_pid_f32_calculate(&controller, &input, &output),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(output == 2.0F);
    TEST_ASSERT_STATUS(cascaded_pid_f32_get_intermediate(&controller, &intermediate),
        FOUNDATION_STATUS_OK);
    TEST_ASSERT(intermediate == 2.0F);
    input.dt = 0.0F;
    output = 77.0F;
    TEST_ASSERT_STATUS(cascaded_pid_f32_calculate(&controller, &input, &output),
        FOUNDATION_STATUS_INVALID_DATA);
    TEST_ASSERT(output == 77.0F);
    TEST_ASSERT_STATUS(cascaded_pid_f32_reset(&controller, 0.0F, 0.0F, 0.0F, 0.0F),
        FOUNDATION_STATUS_OK);
    return 0;
}
