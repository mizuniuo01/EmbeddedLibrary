/* 验证 mizu_pid 的初始化、微分、积分、限幅和抗饱和语义。 */

#include "test_support.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

#include "pid.h"

static mizu_pid_config_t test_config(mizu_pid_anti_windup_t anti_windup)
{
    return (mizu_pid_config_t){
        .kp = 2.0F,
        .ki = 1.0F,
        .kd = 0.5F,
        .output_min = -10.0F,
        .output_max = 10.0F,
        .integral_min = -5.0F,
        .integral_max = 5.0F,
        .tracking_gain = 1.0F,
        .anti_windup = anti_windup,
    };
}

static bool test_pid_equal(const mizu_pid_t *left, const mizu_pid_t *right)
{
    return (left->config.kp == right->config.kp) && (left->config.ki == right->config.ki) &&
           (left->config.kd == right->config.kd) &&
           (left->config.output_min == right->config.output_min) &&
           (left->config.output_max == right->config.output_max) &&
           (left->config.integral_min == right->config.integral_min) &&
           (left->config.integral_max == right->config.integral_max) &&
           (left->config.tracking_gain == right->config.tracking_gain) &&
           (left->config.anti_windup == right->config.anti_windup) &&
           (left->integral == right->integral) &&
           (left->previous_measurement == right->previous_measurement) &&
           (left->output == right->output) && (left->initialized == right->initialized) &&
           (left->history_valid == right->history_valid);
}

static int test_initialization_and_reset(void)
{
    mizu_pid_t pid = {0};
    mizu_pid_config_t config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    float output = 0.0F;

    TEST_ASSERT(mizu_pid_calculate(&pid, 1.0F, 0.0F, 1.0F, &output) == MIZU_STATUS_NOT_INITIALIZED);
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_pid_calculate(&pid, 1.0F, 0.0F, 1.0F, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == 3.0F);
    TEST_ASSERT(mizu_pid_reset(&pid, 2.0F) == MIZU_STATUS_OK);
    TEST_ASSERT(pid.integral == 0.0F);
    TEST_ASSERT(pid.output == 0.0F);
    TEST_ASSERT(pid.previous_measurement == 2.0F);

    config.kp = -1.0F;
    config.ki = 0.0F;
    config.kd = 0.0F;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_pid_calculate(&pid, 1.0F, 0.0F, 1.0F, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == -1.0F);
    return 0;
}

static int test_derivative_and_limits(void)
{
    mizu_pid_t pid;
    mizu_pid_config_t config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    float output = 0.0F;

    config.kp = 0.0F;
    config.ki = 0.0F;
    config.kd = 1.0F;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_pid_calculate(&pid, 0.0F, 1.0F, 1.0F, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == 0.0F);
    TEST_ASSERT(mizu_pid_calculate(&pid, 0.0F, 2.0F, 1.0F, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == -1.0F);

    config.kp = 10.0F;
    config.ki = 0.0F;
    config.kd = 0.0F;
    config.output_min = -1.0F;
    config.output_max = 1.0F;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_pid_calculate(&pid, 1.0F, 0.0F, 1.0F, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == 1.0F);
    return 0;
}

static int test_conditional_anti_windup(void)
{
    mizu_pid_t pid;
    mizu_pid_config_t config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    float output = 0.0F;

    config.kp = 10.0F;
    config.ki = 1.0F;
    config.kd = 0.0F;
    config.output_min = -1.0F;
    config.output_max = 1.0F;
    config.integral_min = -100.0F;
    config.integral_max = 100.0F;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_pid_calculate(&pid, 1.0F, 0.0F, 1.0F, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == 1.0F);
    TEST_ASSERT(pid.integral == 0.0F);
    TEST_ASSERT(mizu_pid_calculate(&pid, -1.0F, 0.0F, 1.0F, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == -1.0F);
    return 0;
}

static int test_back_calculation(void)
{
    mizu_pid_t pid;
    mizu_pid_config_t config = test_config(MIZU_PID_ANTI_WINDUP_BACK_CALCULATION);
    float output = 0.0F;

    config.kp = 10.0F;
    config.ki = 1.0F;
    config.kd = 0.0F;
    config.output_min = -1.0F;
    config.output_max = 1.0F;
    config.integral_min = -100.0F;
    config.integral_max = 100.0F;
    config.tracking_gain = 1.0F;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_pid_calculate(&pid, 1.0F, 0.0F, 1.0F, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == 1.0F);
    TEST_ASSERT(pid.integral == -9.0F);
    return 0;
}

static int test_invalid_input_is_transactional(void)
{
    mizu_pid_t pid;
    mizu_pid_t before;
    mizu_pid_config_t config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    mizu_pid_config_t invalid = config;
    float output = 7.0F;
    float before_output;

    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_pid_calculate(&pid, 1.0F, 0.0F, 1.0F, &output) == MIZU_STATUS_OK);
    before = pid;
    before_output = output;
    TEST_ASSERT(mizu_pid_calculate(&pid, 1.0F, 0.0F, 0.0F, &output) == MIZU_STATUS_INVALID_DATA);
    TEST_ASSERT(test_pid_equal(&pid, &before));
    TEST_ASSERT(output == before_output);

    invalid.output_min = 2.0F;
    invalid.output_max = 1.0F;
    TEST_ASSERT(mizu_pid_init(&pid, &invalid) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(test_pid_equal(&pid, &before));
    TEST_ASSERT(mizu_pid_reset(&pid, NAN) == MIZU_STATUS_INVALID_DATA);
    TEST_ASSERT(test_pid_equal(&pid, &before));

    config.kp = FLT_MAX;
    config.ki = 0.0F;
    config.kd = 0.0F;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_pid_calculate(&pid, FLT_MAX, -FLT_MAX, 1.0F, &output) == MIZU_STATUS_OVERFLOW);
    return 0;
}

static int test_configuration_errors(void)
{
    mizu_pid_t pid = {0};
    mizu_pid_config_t config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);

    TEST_ASSERT(mizu_pid_init(NULL, &config) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_pid_init(&pid, NULL) == MIZU_STATUS_INVALID_ARGUMENT);

    config.kp = NAN;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_INVALID_DATA);
    config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    config.ki = INFINITY;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_INVALID_DATA);
    config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    config.output_min = INFINITY;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_INVALID_DATA);
    config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    config.integral_max = NAN;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_INVALID_DATA);
    config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    config.tracking_gain = INFINITY;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_INVALID_DATA);

    config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    config.output_min = 1.0F;
    config.output_max = -1.0F;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_INVALID_ARGUMENT);
    config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    config.integral_min = 1.0F;
    config.integral_max = -1.0F;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_INVALID_ARGUMENT);
    config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    config.tracking_gain = -1.0F;
    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_INVALID_ARGUMENT);
    config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    TEST_ASSERT(mizu_pid_reset(&pid, 0.0F) == MIZU_STATUS_NOT_INITIALIZED);
    return 0;
}

static int test_calculate_arguments(void)
{
    mizu_pid_t pid = {0};
    mizu_pid_config_t config = test_config(MIZU_PID_ANTI_WINDUP_CONDITIONAL);
    float output = 3.0F;

    TEST_ASSERT(mizu_pid_init(&pid, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_pid_calculate(&pid, 0.0F, 0.0F, 1.0F, NULL) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_pid_calculate(&pid, NAN, 0.0F, 1.0F, &output) == MIZU_STATUS_INVALID_DATA);
    TEST_ASSERT(mizu_pid_calculate(&pid, 0.0F, INFINITY, 1.0F, &output) ==
                MIZU_STATUS_INVALID_DATA);
    TEST_ASSERT(mizu_pid_calculate(&pid, 0.0F, 0.0F, NAN, &output) == MIZU_STATUS_INVALID_DATA);
    TEST_ASSERT(mizu_pid_calculate(&pid, 0.0F, 0.0F, -1.0F, &output) == MIZU_STATUS_INVALID_DATA);
    TEST_ASSERT(output == 3.0F);
    return 0;
}

int main(void)
{
    int (* const cases[])(void) = {test_initialization_and_reset,
                                   test_derivative_and_limits,
                                   test_conditional_anti_windup,
                                   test_back_calculation,
                                   test_invalid_input_is_transactional,
                                   test_configuration_errors,
                                   test_calculate_arguments};

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        const int result = cases[index]();

        if (result != 0) {
            return result;
        }
    }
    return 0;
}
