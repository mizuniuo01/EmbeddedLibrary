/* 验证固定两级串级 PID 的级联、重置、错误传播和事务语义。 */

#include "test_support.h"

#include <math.h>
#include <stddef.h>

#include "cascaded_pid.h"

static mizu_pid_config_t test_pid_config(float kp, float kd, float output_min, float output_max)
{
    return (mizu_pid_config_t){
        .kp = kp,
        .ki = 0.0F,
        .kd = kd,
        .output_min = output_min,
        .output_max = output_max,
        .integral_min = -10.0F,
        .integral_max = 10.0F,
        .tracking_gain = 1.0F,
        .anti_windup = MIZU_PID_ANTI_WINDUP_CONDITIONAL,
    };
}

static mizu_cascaded_pid_config_t test_config(void)
{
    return (mizu_cascaded_pid_config_t){
        .outer_config = test_pid_config(2.0F, 0.0F, -10.0F, 10.0F),
        .inner_config = test_pid_config(1.0F, 0.0F, -10.0F, 10.0F),
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

static bool test_controller_equal(const mizu_cascaded_pid_t *left, const mizu_cascaded_pid_t *right)
{
    return test_pid_equal(&left->outer_pid, &right->outer_pid) &&
           test_pid_equal(&left->inner_pid, &right->inner_pid) &&
           (left->initialized == right->initialized);
}

static int test_initialization_and_calculation(void)
{
    mizu_cascaded_pid_t controller = {0};
    const mizu_cascaded_pid_config_t config = test_config();
    const mizu_cascaded_pid_input_t input = {2.0F, 0.0F, 0.0F, 1.0F};
    float output = 0.0F;

    TEST_ASSERT(mizu_cascaded_pid_calculate(&controller, &input, &output) ==
                MIZU_STATUS_NOT_INITIALIZED);
    TEST_ASSERT(mizu_cascaded_pid_init(&controller, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_cascaded_pid_calculate(&controller, &input, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == 4.0F);

    controller = (mizu_cascaded_pid_t){0};
    {
        mizu_cascaded_pid_config_t limited_config = config;

        limited_config.inner_config.output_max = 1.0F;
        limited_config.inner_config.output_min = -1.0F;
        TEST_ASSERT(mizu_cascaded_pid_init(&controller, &limited_config) == MIZU_STATUS_OK);
    }
    TEST_ASSERT(mizu_cascaded_pid_calculate(&controller, &input, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == 1.0F);
    return 0;
}

static int test_reset_and_reinitialization(void)
{
    mizu_cascaded_pid_t controller = {0};
    mizu_cascaded_pid_config_t config = test_config();
    mizu_cascaded_pid_input_t input = {0.0F, 1.0F, 0.0F, 1.0F};
    float output = 0.0F;

    config.outer_config.kd = 1.0F;
    config.inner_config.kd = 1.0F;
    TEST_ASSERT(mizu_cascaded_pid_init(&controller, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_cascaded_pid_reset(&controller, 1.0F, 2.0F) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_cascaded_pid_calculate(&controller, &input, &output) == MIZU_STATUS_OK);
    TEST_ASSERT(output == 0.0F);

    config.outer_config.kp = 3.0F;
    TEST_ASSERT(mizu_cascaded_pid_init(&controller, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(controller.outer_pid.integral == 0.0F);
    TEST_ASSERT(controller.inner_pid.integral == 0.0F);
    return 0;
}

static int test_transaction_and_errors(void)
{
    mizu_cascaded_pid_t controller = {0};
    mizu_cascaded_pid_t before;
    mizu_cascaded_pid_config_t config = test_config();
    mizu_cascaded_pid_input_t input = {2.0F, 0.0F, 0.0F, 1.0F};
    float output = 9.0F;

    TEST_ASSERT(mizu_cascaded_pid_init(&controller, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_cascaded_pid_calculate(&controller, &input, &output) == MIZU_STATUS_OK);
    controller.inner_pid.initialized = false;
    before = controller;
    output = 9.0F;
    TEST_ASSERT(mizu_cascaded_pid_calculate(&controller, &input, &output) ==
                MIZU_STATUS_NOT_INITIALIZED);
    TEST_ASSERT(test_controller_equal(&controller, &before));
    TEST_ASSERT(output == 9.0F);
    controller.inner_pid.initialized = true;
    TEST_ASSERT(mizu_cascaded_pid_calculate(&controller, NULL, &output) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_cascaded_pid_calculate(&controller, &input, NULL) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    input.dt = 0.0F;
    TEST_ASSERT(mizu_cascaded_pid_calculate(&controller, &input, &output) ==
                MIZU_STATUS_INVALID_DATA);

    config.inner_config.output_min = 1.0F;
    config.inner_config.output_max = -1.0F;
    TEST_ASSERT(mizu_cascaded_pid_init(&controller, &config) == MIZU_STATUS_INVALID_ARGUMENT);
    return 0;
}

static int test_reset_transaction_and_instances(void)
{
    mizu_cascaded_pid_t first = {0};
    mizu_cascaded_pid_t second = {0};
    mizu_cascaded_pid_t before;
    const mizu_cascaded_pid_config_t config = test_config();
    const mizu_cascaded_pid_input_t input = {2.0F, 0.0F, 0.0F, 1.0F};
    float first_output = 0.0F;
    float second_output = 0.0F;

    TEST_ASSERT(mizu_cascaded_pid_init(&first, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_cascaded_pid_init(&second, &config) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_cascaded_pid_calculate(&first, &input, &first_output) == MIZU_STATUS_OK);
    before = first;
    TEST_ASSERT(mizu_cascaded_pid_reset(&first, 1.0F, NAN) == MIZU_STATUS_INVALID_DATA);
    TEST_ASSERT(test_controller_equal(&first, &before));
    TEST_ASSERT(mizu_cascaded_pid_calculate(&second, &input, &second_output) == MIZU_STATUS_OK);
    TEST_ASSERT(first_output == second_output);
    return 0;
}

int main(void)
{
    int (* const cases[])(void) = {test_initialization_and_calculation,
                                   test_reset_and_reinitialization, test_transaction_and_errors,
                                   test_reset_transaction_and_instances};

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        const int result = cases[index]();

        if (result != 0) {
            return result;
        }
    }
    return 0;
}
