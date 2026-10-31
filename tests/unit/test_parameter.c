/* 验证参数服务的静态注册、回调路由、范围校验和步长调整。 */

#include "test_support.h"

#include <math.h>
#include <stddef.h>

#include "parameter.h"

typedef struct {
    float value;
    int32_t count;
    bool flag;
    mizu_status_t get_status;
    mizu_status_t set_status;
} test_parameter_context_t;

static mizu_status_t test_get(void *context, mizu_parameter_value_t *value)
{
    test_parameter_context_t *state = context;

    if ((state == NULL) || (value == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (state->get_status != MIZU_STATUS_OK) {
        return state->get_status;
    }
    value->type = MIZU_PARAMETER_VALUE_FLOAT;
    value->data.float_value = state->value;
    return MIZU_STATUS_OK;
}

static mizu_status_t test_set(void *context, const mizu_parameter_value_t *value)
{
    test_parameter_context_t *state = context;

    if ((state == NULL) || (value == NULL) || (value->type != MIZU_PARAMETER_VALUE_FLOAT)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (state->set_status != MIZU_STATUS_OK) {
        return state->set_status;
    }
    state->value = value->data.float_value;
    return MIZU_STATUS_OK;
}

static mizu_status_t test_get_count(void *context, mizu_parameter_value_t *value)
{
    test_parameter_context_t *state = context;

    if ((state == NULL) || (value == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    value->type = MIZU_PARAMETER_VALUE_INT32;
    value->data.int32_value = state->count;
    return MIZU_STATUS_OK;
}

static mizu_status_t test_set_count(void *context, const mizu_parameter_value_t *value)
{
    test_parameter_context_t *state = context;

    if ((state == NULL) || (value == NULL) || (value->type != MIZU_PARAMETER_VALUE_INT32)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    state->count = value->data.int32_value;
    return MIZU_STATUS_OK;
}

static mizu_status_t test_get_bool(void *context, mizu_parameter_value_t *value)
{
    test_parameter_context_t *state = context;

    if ((state == NULL) || (value == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    value->type = MIZU_PARAMETER_VALUE_BOOL;
    value->data.bool_value = state->flag;
    return MIZU_STATUS_OK;
}

static mizu_status_t test_set_bool(void *context, const mizu_parameter_value_t *value)
{
    test_parameter_context_t *state = context;

    if ((state == NULL) || (value == NULL) || (value->type != MIZU_PARAMETER_VALUE_BOOL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    state->flag = value->data.bool_value;
    return MIZU_STATUS_OK;
}

static mizu_parameter_descriptor_t test_descriptors(test_parameter_context_t *state)
{
    return (mizu_parameter_descriptor_t){
        .id = 1U,
        .name = "gain",
        .unit = "x",
        .type = MIZU_PARAMETER_VALUE_FLOAT,
        .minimum = {.type = MIZU_PARAMETER_VALUE_FLOAT, .data.float_value = -5.0F},
        .maximum = {.type = MIZU_PARAMETER_VALUE_FLOAT, .data.float_value = 5.0F},
        .step = {.type = MIZU_PARAMETER_VALUE_FLOAT, .data.float_value = 0.5F},
        .writable = true,
        .get = test_get,
        .set = test_set,
        .context = state,
    };
}

static int test_registration_and_access(void)
{
    test_parameter_context_t state = {.value = 1.0F};
    mizu_parameter_descriptor_t descriptor = test_descriptors(&state);
    const mizu_parameter_group_t group = {10U, "control", &descriptor, 1U};
    mizu_parameter_service_t service = {0};
    const mizu_parameter_group_t *view;
    mizu_parameter_value_t value;
    size_t group_count;

    TEST_ASSERT(mizu_parameter_service_init(&service, &group, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_parameter_service_get_group_count(&service, &group_count) == MIZU_STATUS_OK);
    TEST_ASSERT(group_count == 1U);
    TEST_ASSERT(mizu_parameter_service_get_group(&service, 0U, &view) == MIZU_STATUS_OK);
    TEST_ASSERT(view == &group);
    TEST_ASSERT(mizu_parameter_service_get_by_id(&service, 1U, &value) == MIZU_STATUS_OK);
    TEST_ASSERT(value.data.float_value == 1.0F);
    value.data.float_value = 2.0F;
    TEST_ASSERT(mizu_parameter_service_set_by_id(&service, 1U, &value) == MIZU_STATUS_OK);
    TEST_ASSERT(state.value == 2.0F);
    value.data.float_value = NAN;
    TEST_ASSERT(mizu_parameter_service_set_by_id(&service, 1U, &value) == MIZU_STATUS_INVALID_DATA);
    TEST_ASSERT(state.value == 2.0F);
    value.data.float_value = INFINITY;
    TEST_ASSERT(mizu_parameter_service_set_by_id(&service, 1U, &value) == MIZU_STATUS_INVALID_DATA);
    TEST_ASSERT(state.value == 2.0F);
    value.data.float_value = 20.0F;
    TEST_ASSERT(mizu_parameter_service_set_by_id(&service, 1U, &value) == MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(state.value == 2.0F);
    TEST_ASSERT(mizu_parameter_service_adjust_by_id(&service,
                                                    &(mizu_parameter_adjust_request_t){1U, 20}) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(state.value == 5.0F);
    TEST_ASSERT(mizu_parameter_service_adjust_by_id(&service,
                                                    &(mizu_parameter_adjust_request_t){1U, -20}) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(state.value == -5.0F);
    return 0;
}

static int test_types_and_errors(void)
{
    test_parameter_context_t state = {.count = 1, .flag = false};
    mizu_parameter_descriptor_t descriptors[2] = {
        {.id = 2U,
         .name = "count",
         .unit = "",
         .type = MIZU_PARAMETER_VALUE_INT32,
         .minimum = {.type = MIZU_PARAMETER_VALUE_INT32, .data.int32_value = 0},
         .maximum = {.type = MIZU_PARAMETER_VALUE_INT32, .data.int32_value = 10},
         .step = {.type = MIZU_PARAMETER_VALUE_INT32, .data.int32_value = 2},
         .writable = true,
         .get = test_get_count,
         .set = test_set_count,
         .context = &state},
        {.id = 3U,
         .name = "enabled",
         .unit = "",
         .type = MIZU_PARAMETER_VALUE_BOOL,
         .minimum = {.type = MIZU_PARAMETER_VALUE_BOOL, .data.bool_value = false},
         .maximum = {.type = MIZU_PARAMETER_VALUE_BOOL, .data.bool_value = true},
         .step = {.type = MIZU_PARAMETER_VALUE_BOOL, .data.bool_value = false},
         .writable = true,
         .get = test_get_bool,
         .set = test_set_bool,
         .context = &state},
    };
    const mizu_parameter_group_t group = {10U, "group", descriptors, 2U};
    mizu_parameter_service_t service = {0};
    mizu_parameter_value_t value = {MIZU_PARAMETER_VALUE_FLOAT, {.float_value = 1.0F}};
    size_t count = 0U;

    TEST_ASSERT(mizu_parameter_service_get_group_count(&service, NULL) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_parameter_service_get_group_count(&service, &count) ==
                MIZU_STATUS_NOT_INITIALIZED);
    TEST_ASSERT(mizu_parameter_service_init(&service, &group, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_parameter_service_set_by_id(&service, 3U, &value) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_parameter_service_adjust_by_id(&service,
                                                    &(mizu_parameter_adjust_request_t){3U, 1}) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(state.flag);
    value.type = MIZU_PARAMETER_VALUE_INT32;
    value.data.int32_value = 9;
    TEST_ASSERT(mizu_parameter_service_set_by_id(&service, 2U, &value) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_parameter_service_adjust_by_id(&service,
                                                    &(mizu_parameter_adjust_request_t){2U, -10}) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(state.count == 0);
    TEST_ASSERT(mizu_parameter_service_get_by_id(&service, 99U, &value) ==
                MIZU_STATUS_OUT_OF_RANGE);
    return 0;
}

static int test_registration_failure_and_callback_error(void)
{
    test_parameter_context_t state = {.value = 0.0F, .get_status = MIZU_STATUS_INTERNAL_ERROR};
    mizu_parameter_descriptor_t descriptor = test_descriptors(&state);
    const mizu_parameter_group_t group = {10U, "control", &descriptor, 1U};
    mizu_parameter_service_t service = {0};

    TEST_ASSERT(mizu_parameter_service_init(&service, &group, 1U) == MIZU_STATUS_INTERNAL_ERROR);
    state.get_status = MIZU_STATUS_OK;
    TEST_ASSERT(mizu_parameter_service_init(&service, &group, 1U) == MIZU_STATUS_OK);
    state.set_status = MIZU_STATUS_INTERNAL_ERROR;
    {
        const mizu_parameter_value_t value = {MIZU_PARAMETER_VALUE_FLOAT, {.float_value = 1.0F}};

        TEST_ASSERT(mizu_parameter_service_set_by_id(&service, 1U, &value) ==
                    MIZU_STATUS_INTERNAL_ERROR);
    }
    return 0;
}

int main(void)
{
    int (* const cases[])(void) = {test_registration_and_access, test_types_and_errors,
                                   test_registration_failure_and_callback_error};

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        const int result = cases[index]();

        if (result != 0) {
            return result;
        }
    }
    return 0;
}
