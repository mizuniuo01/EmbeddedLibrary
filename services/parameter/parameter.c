/* Services 层的静态参数注册、校验和 getter/setter 路由。 */

#include "parameter.h"

#include <float.h>
#include <math.h>
#include <stdint.h>

#define MIZU_PARAMETER_MAX_GROUPS 16U
#define MIZU_PARAMETER_MAX_PARAMETERS 64U

static bool mizu_parameter_type_is_valid(mizu_parameter_value_type_t type)
{
    return (type == MIZU_PARAMETER_VALUE_FLOAT) || (type == MIZU_PARAMETER_VALUE_INT32) ||
           (type == MIZU_PARAMETER_VALUE_UINT32) || (type == MIZU_PARAMETER_VALUE_BOOL);
}

static bool mizu_parameter_value_is_valid(const mizu_parameter_value_t *value,
                                          mizu_parameter_value_type_t type)
{
    if ((value == NULL) || (value->type != type)) {
        return false;
    }
    return (type != MIZU_PARAMETER_VALUE_FLOAT) || isfinite(value->data.float_value);
}

static int mizu_parameter_compare(const mizu_parameter_value_t *left,
                                  const mizu_parameter_value_t *right)
{
    switch (left->type) {
        case MIZU_PARAMETER_VALUE_FLOAT:
            return (left->data.float_value > right->data.float_value) -
                   (left->data.float_value < right->data.float_value);
        case MIZU_PARAMETER_VALUE_INT32:
            return (left->data.int32_value > right->data.int32_value) -
                   (left->data.int32_value < right->data.int32_value);
        case MIZU_PARAMETER_VALUE_UINT32:
            return (left->data.uint32_value > right->data.uint32_value) -
                   (left->data.uint32_value < right->data.uint32_value);
        case MIZU_PARAMETER_VALUE_BOOL:
            return (left->data.bool_value > right->data.bool_value) -
                   (left->data.bool_value < right->data.bool_value);
        default:
            return 0;
    }
}

static bool mizu_parameter_value_in_range(const mizu_parameter_descriptor_t *descriptor,
                                          const mizu_parameter_value_t *value)
{
    return (mizu_parameter_compare(value, &descriptor->minimum) >= 0) &&
           (mizu_parameter_compare(value, &descriptor->maximum) <= 0);
}

static float mizu_parameter_minimum_float_step(uint8_t decimals)
{
    static const float minimum_steps[] = {1.0F, 0.1F, 0.01F, 0.001F, 0.0001F};

    return minimum_steps[decimals];
}

static mizu_status_t
mizu_parameter_validate_runtime_step(const mizu_parameter_descriptor_t *descriptor,
                                     const mizu_parameter_value_t *step)
{
    switch (descriptor->type) {
        case MIZU_PARAMETER_VALUE_FLOAT: {
            const float minimum = mizu_parameter_minimum_float_step(descriptor->decimals);
            const double range = (double)descriptor->maximum.data.float_value -
                                 (double)descriptor->minimum.data.float_value;
            const double maximum = range > (double)FLT_MAX ? (double)FLT_MAX : range;

            if (!isfinite(step->data.float_value)) {
                return MIZU_STATUS_INVALID_DATA;
            }
            if ((step->data.float_value <= 0.0F) || (step->data.float_value < minimum) ||
                ((double)step->data.float_value > maximum)) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            return MIZU_STATUS_OK;
        }
        case MIZU_PARAMETER_VALUE_INT32: {
            const int64_t range = (int64_t)descriptor->maximum.data.int32_value -
                                  (int64_t)descriptor->minimum.data.int32_value;

            if ((step->data.int32_value <= 0) || ((int64_t)step->data.int32_value > range)) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            return MIZU_STATUS_OK;
        }
        case MIZU_PARAMETER_VALUE_UINT32: {
            const uint32_t range =
                descriptor->maximum.data.uint32_value - descriptor->minimum.data.uint32_value;

            if ((step->data.uint32_value == 0U) || (step->data.uint32_value > range)) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            return MIZU_STATUS_OK;
        }
        case MIZU_PARAMETER_VALUE_BOOL:
        default:
            return MIZU_STATUS_INVALID_STATE;
    }
}

static mizu_status_t
mizu_parameter_validate_descriptor(const mizu_parameter_descriptor_t *descriptor)
{
    if ((descriptor == NULL) || (descriptor->name == NULL) || (descriptor->name[0] == '\0') ||
        (descriptor->unit == NULL) || (descriptor->get == NULL) ||
        (descriptor->writable != (descriptor->set != NULL)) ||
        !mizu_parameter_type_is_valid(descriptor->type) ||
        (descriptor->decimals > MIZU_PARAMETER_MAX_DECIMALS)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!mizu_parameter_value_is_valid(&descriptor->minimum, descriptor->type) ||
        !mizu_parameter_value_is_valid(&descriptor->maximum, descriptor->type) ||
        !mizu_parameter_value_is_valid(&descriptor->step, descriptor->type)) {
        return MIZU_STATUS_INVALID_DATA;
    }
    if (mizu_parameter_compare(&descriptor->minimum, &descriptor->maximum) > 0) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    switch (descriptor->type) {
        case MIZU_PARAMETER_VALUE_FLOAT:
            if ((descriptor->step.data.float_value <= 0.0F) ||
                (descriptor->step.data.float_value <
                 mizu_parameter_minimum_float_step(descriptor->decimals)) ||
                ((descriptor->minimum.data.float_value != descriptor->maximum.data.float_value) &&
                 (descriptor->step.data.float_value >
                  descriptor->maximum.data.float_value - descriptor->minimum.data.float_value))) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            break;
        case MIZU_PARAMETER_VALUE_INT32:
            if ((descriptor->decimals != 0U) || (descriptor->step.data.int32_value <= 0) ||
                ((descriptor->minimum.data.int32_value != descriptor->maximum.data.int32_value) &&
                 ((int64_t)descriptor->step.data.int32_value >
                  (int64_t)descriptor->maximum.data.int32_value -
                      (int64_t)descriptor->minimum.data.int32_value))) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            break;
        case MIZU_PARAMETER_VALUE_UINT32:
            if ((descriptor->decimals != 0U) || (descriptor->step.data.uint32_value == 0U) ||
                ((descriptor->minimum.data.uint32_value != descriptor->maximum.data.uint32_value) &&
                 (descriptor->step.data.uint32_value >
                  descriptor->maximum.data.uint32_value - descriptor->minimum.data.uint32_value))) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            break;
        case MIZU_PARAMETER_VALUE_BOOL:
            if ((descriptor->decimals != 0U) || !descriptor->step.data.bool_value) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            break;
        default:
            return MIZU_STATUS_INVALID_ARGUMENT;
    }
    return MIZU_STATUS_OK;
}

typedef struct {
    const mizu_parameter_group_t *groups;
    size_t group_index;
    size_t parameter_index;
    mizu_parameter_id_t id;
} mizu_parameter_id_search_t;

static bool mizu_parameter_id_exists(const mizu_parameter_id_search_t *search)
{
    for (size_t earlier_group = 0U; earlier_group <= search->group_index; ++earlier_group) {
        size_t limit = search->groups[earlier_group].parameter_count;

        if (earlier_group == search->group_index) {
            limit = search->parameter_index;
        }
        for (size_t earlier_parameter = 0U; earlier_parameter < limit; ++earlier_parameter) {
            if (search->groups[earlier_group].parameters[earlier_parameter].id == search->id) {
                return true;
            }
        }
    }
    return false;
}

static mizu_status_t mizu_parameter_find(const mizu_parameter_service_t *service,
                                         uint16_t parameter_id,
                                         const mizu_parameter_descriptor_t **descriptor)
{
    if ((service == NULL) || (descriptor == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!service->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    for (size_t group_index = 0U; group_index < service->group_count; ++group_index) {
        const mizu_parameter_group_t *group = &service->groups[group_index];

        for (size_t parameter_index = 0U; parameter_index < group->parameter_count;
             ++parameter_index) {
            if (group->parameters[parameter_index].id == parameter_id) {
                *descriptor = &group->parameters[parameter_index];
                return MIZU_STATUS_OK;
            }
        }
    }
    return MIZU_STATUS_OUT_OF_RANGE;
}

static mizu_status_t mizu_parameter_validate_current(const mizu_parameter_descriptor_t *descriptor)
{
    mizu_parameter_value_t value;
    mizu_status_t status = descriptor->get(descriptor->context, &value);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (!mizu_parameter_value_is_valid(&value, descriptor->type) ||
        !mizu_parameter_value_in_range(descriptor, &value)) {
        return MIZU_STATUS_INVALID_DATA;
    }
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_parameter_service_init(mizu_parameter_service_t *service,
                                          const mizu_parameter_group_t *groups, size_t group_count)
{
    size_t total_count = 0U;

    if ((service == NULL) || (groups == NULL) || (group_count == 0U)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (group_count > MIZU_PARAMETER_MAX_GROUPS) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    for (size_t group_index = 0U; group_index < group_count; ++group_index) {
        const mizu_parameter_group_t *group = &groups[group_index];

        if ((group->name == NULL) || (group->name[0] == '\0') || (group->parameters == NULL) ||
            (group->parameter_count == 0U)) {
            return MIZU_STATUS_INVALID_ARGUMENT;
        }
        for (size_t earlier = 0U; earlier < group_index; ++earlier) {
            if (groups[earlier].id == group->id) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
        }
        if (group->parameter_count > (MIZU_PARAMETER_MAX_PARAMETERS - total_count)) {
            return MIZU_STATUS_OUT_OF_RANGE;
        }
        for (size_t parameter_index = 0U; parameter_index < group->parameter_count;
             ++parameter_index) {
            const mizu_parameter_descriptor_t *descriptor = &group->parameters[parameter_index];
            mizu_status_t status = mizu_parameter_validate_descriptor(descriptor);

            if (status != MIZU_STATUS_OK) {
                return status;
            }
            const mizu_parameter_id_search_t search = {groups, group_index, parameter_index,
                                                       descriptor->id};

            if (mizu_parameter_id_exists(&search)) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            status = mizu_parameter_validate_current(descriptor);
            if (status != MIZU_STATUS_OK) {
                return status;
            }
        }
        total_count += group->parameter_count;
    }

    *service = (mizu_parameter_service_t){
        .groups = groups,
        .group_count = group_count,
        .parameter_count = total_count,
        .initialized = true,
    };
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_parameter_service_get_group_count(const mizu_parameter_service_t *service,
                                                     size_t *count)
{
    if ((service == NULL) || (count == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!service->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    *count = service->group_count;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_parameter_service_get_group(const mizu_parameter_service_t *service,
                                               size_t group_index,
                                               const mizu_parameter_group_t **group)
{
    if ((service == NULL) || (group == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!service->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    if (group_index >= service->group_count) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    *group = &service->groups[group_index];
    return MIZU_STATUS_OK;
}

mizu_status_t
mizu_parameter_service_get_descriptor_by_id(const mizu_parameter_service_t *service,
                                            mizu_parameter_id_t parameter_id,
                                            const mizu_parameter_descriptor_t **descriptor)
{
    return mizu_parameter_find(service, parameter_id, descriptor);
}

static mizu_status_t mizu_parameter_adjust_float(const mizu_parameter_descriptor_t *descriptor,
                                                 float value, float step,
                                                 mizu_parameter_step_count_t step_count,
                                                 float *adjusted)
{
    double result;

    if (step <= 0.0F) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    result = (double)value + (double)step * (double)step_count;
    if (result < (double)descriptor->minimum.data.float_value) {
        *adjusted = descriptor->minimum.data.float_value;
    } else if (result > (double)descriptor->maximum.data.float_value) {
        *adjusted = descriptor->maximum.data.float_value;
    } else {
        *adjusted = (float)result;
    }
    return MIZU_STATUS_OK;
}

static mizu_status_t mizu_parameter_adjust_int32(const mizu_parameter_descriptor_t *descriptor,
                                                 int32_t value, int32_t step,
                                                 mizu_parameter_step_count_t step_count,
                                                 int32_t *adjusted)
{
    int64_t result;

    if (step <= 0) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    result = (int64_t)value + (int64_t)step * (int64_t)step_count;
    if (result < (int64_t)descriptor->minimum.data.int32_value) {
        *adjusted = descriptor->minimum.data.int32_value;
    } else if (result > (int64_t)descriptor->maximum.data.int32_value) {
        *adjusted = descriptor->maximum.data.int32_value;
    } else {
        *adjusted = (int32_t)result;
    }
    return MIZU_STATUS_OK;
}

typedef struct {
    uint32_t value;
    uint32_t step;
    mizu_parameter_step_count_t step_count;
} mizu_parameter_uint32_adjustment_t;

static mizu_status_t mizu_parameter_adjust_uint32(const mizu_parameter_descriptor_t *descriptor,
                                                  const mizu_parameter_uint32_adjustment_t *input,
                                                  uint32_t *adjusted)
{
    uint64_t delta;
    uint32_t available;

    if (input->step == 0U) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (input->step_count < 0) {
        const uint64_t magnitude = (uint64_t)(-(int64_t)input->step_count);

        delta = (uint64_t)input->step * magnitude;
        available = input->value - descriptor->minimum.data.uint32_value;
        *adjusted = delta >= (uint64_t)available ? descriptor->minimum.data.uint32_value
                                                 : input->value - (uint32_t)delta;
    } else {
        delta = (uint64_t)input->step * (uint64_t)input->step_count;
        available = descriptor->maximum.data.uint32_value - input->value;
        *adjusted = delta >= (uint64_t)available ? descriptor->maximum.data.uint32_value
                                                 : input->value + (uint32_t)delta;
    }
    return MIZU_STATUS_OK;
}

static mizu_status_t mizu_parameter_adjust_bool(const mizu_parameter_descriptor_t *descriptor,
                                                bool value, bool step,
                                                mizu_parameter_step_count_t step_count,
                                                bool *adjusted)
{
    (void)descriptor;
    if (!step) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    *adjusted = step_count == 0 ? value : step_count > 0;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_parameter_adjust_value(const mizu_parameter_descriptor_t *descriptor,
                                          const mizu_parameter_value_t *value,
                                          const mizu_parameter_value_t *step,
                                          mizu_parameter_step_count_t step_count,
                                          mizu_parameter_value_t *adjusted)
{
    mizu_status_t status;
    mizu_parameter_value_t result;

    if ((descriptor == NULL) || (value == NULL) || (step == NULL) || (adjusted == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    status = mizu_parameter_validate_descriptor(descriptor);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if ((value->type != descriptor->type) || (step->type != descriptor->type)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if ((descriptor->type == MIZU_PARAMETER_VALUE_FLOAT) &&
        (!isfinite(value->data.float_value) || !isfinite(step->data.float_value))) {
        return MIZU_STATUS_INVALID_DATA;
    }
    if (!mizu_parameter_value_is_valid(value, descriptor->type)) {
        return MIZU_STATUS_INVALID_DATA;
    }
    if (!descriptor->writable) {
        return MIZU_STATUS_INVALID_STATE;
    }
    if (!mizu_parameter_value_in_range(descriptor, value)) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    result = *value;
    switch (descriptor->type) {
        case MIZU_PARAMETER_VALUE_FLOAT:
            status = mizu_parameter_adjust_float(descriptor, value->data.float_value,
                                                 step->data.float_value, step_count,
                                                 &result.data.float_value);
            break;
        case MIZU_PARAMETER_VALUE_INT32:
            status = mizu_parameter_adjust_int32(descriptor, value->data.int32_value,
                                                 step->data.int32_value, step_count,
                                                 &result.data.int32_value);
            break;
        case MIZU_PARAMETER_VALUE_UINT32:
            status = mizu_parameter_adjust_uint32(
                descriptor,
                &(mizu_parameter_uint32_adjustment_t){value->data.uint32_value,
                                                      step->data.uint32_value, step_count},
                &result.data.uint32_value);
            break;
        case MIZU_PARAMETER_VALUE_BOOL:
            status = mizu_parameter_adjust_bool(descriptor, value->data.bool_value,
                                                step->data.bool_value, step_count,
                                                &result.data.bool_value);
            break;
        default:
            return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    *adjusted = result;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_parameter_scale_step(const mizu_parameter_descriptor_t *descriptor,
                                        const mizu_parameter_value_t *step, int32_t direction,
                                        mizu_parameter_value_t *scaled)
{
    mizu_status_t status;
    mizu_parameter_value_t result;

    if ((descriptor == NULL) || (step == NULL) || (scaled == NULL) || (direction < -1) ||
        (direction > 1)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    status = mizu_parameter_validate_descriptor(descriptor);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (step->type != descriptor->type) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!descriptor->writable || (descriptor->type == MIZU_PARAMETER_VALUE_BOOL) ||
        (mizu_parameter_compare(&descriptor->minimum, &descriptor->maximum) == 0)) {
        return MIZU_STATUS_INVALID_STATE;
    }
    status = mizu_parameter_validate_runtime_step(descriptor, step);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    result = *step;
    switch (descriptor->type) {
        case MIZU_PARAMETER_VALUE_FLOAT: {
            const float minimum = mizu_parameter_minimum_float_step(descriptor->decimals);
            const double range = (double)descriptor->maximum.data.float_value -
                                 (double)descriptor->minimum.data.float_value;
            const double maximum = range > (double)FLT_MAX ? (double)FLT_MAX : range;

            if (direction < 0) {
                result.data.float_value = step->data.float_value / 10.0F;
                if (result.data.float_value < minimum) {
                    result.data.float_value = minimum;
                }
            } else if (direction > 0) {
                const double candidate = (double)step->data.float_value * 10.0;

                result.data.float_value = candidate > maximum ? (float)maximum : (float)candidate;
            }
            break;
        }
        case MIZU_PARAMETER_VALUE_INT32: {
            const int64_t range = (int64_t)descriptor->maximum.data.int32_value -
                                  (int64_t)descriptor->minimum.data.int32_value;
            const int32_t maximum = range > INT32_MAX ? INT32_MAX : (int32_t)range;

            if (direction < 0) {
                result.data.int32_value = step->data.int32_value / 10;
                if (result.data.int32_value < 1) {
                    result.data.int32_value = 1;
                }
            } else if (direction > 0) {
                const int64_t candidate = (int64_t)step->data.int32_value * 10;

                result.data.int32_value =
                    candidate > (int64_t)maximum ? maximum : (int32_t)candidate;
            }
            break;
        }
        case MIZU_PARAMETER_VALUE_UINT32: {
            const uint32_t maximum =
                descriptor->maximum.data.uint32_value - descriptor->minimum.data.uint32_value;

            if (direction < 0) {
                result.data.uint32_value = step->data.uint32_value / 10U;
                if (result.data.uint32_value == 0U) {
                    result.data.uint32_value = 1U;
                }
            } else if (direction > 0) {
                const uint64_t candidate = (uint64_t)step->data.uint32_value * UINT64_C(10);

                result.data.uint32_value =
                    candidate > (uint64_t)maximum ? maximum : (uint32_t)candidate;
            }
            break;
        }
        case MIZU_PARAMETER_VALUE_BOOL:
        default:
            return MIZU_STATUS_INVALID_STATE;
    }
    *scaled = result;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_parameter_service_get_by_id(const mizu_parameter_service_t *service,
                                               mizu_parameter_id_t parameter_id,
                                               mizu_parameter_value_t *value)
{
    const mizu_parameter_descriptor_t *descriptor;
    mizu_status_t status;

    if (value == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    status = mizu_parameter_find(service, parameter_id, &descriptor);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    status = descriptor->get(descriptor->context, value);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (!mizu_parameter_value_is_valid(value, descriptor->type) ||
        !mizu_parameter_value_in_range(descriptor, value)) {
        return MIZU_STATUS_INVALID_DATA;
    }
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_parameter_service_set_by_id(const mizu_parameter_service_t *service,
                                               mizu_parameter_id_t parameter_id,
                                               const mizu_parameter_value_t *value)
{
    const mizu_parameter_descriptor_t *descriptor;
    mizu_status_t status = mizu_parameter_find(service, parameter_id, &descriptor);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (!descriptor->writable) {
        return MIZU_STATUS_INVALID_STATE;
    }
    if (value == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (value->type != descriptor->type) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if ((descriptor->type == MIZU_PARAMETER_VALUE_FLOAT) && !isfinite(value->data.float_value)) {
        return MIZU_STATUS_INVALID_DATA;
    }
    if (!mizu_parameter_value_in_range(descriptor, value)) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    return descriptor->set(descriptor->context, value);
}

mizu_status_t mizu_parameter_service_adjust_by_id(const mizu_parameter_service_t *service,
                                                  const mizu_parameter_adjust_request_t *request)
{
    const mizu_parameter_descriptor_t *descriptor;
    mizu_parameter_value_t value;
    mizu_status_t status;

    if (request == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    status = mizu_parameter_find(service, request->id, &descriptor);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (!descriptor->writable) {
        return MIZU_STATUS_INVALID_STATE;
    }
    if (request->step_count == 0) {
        return MIZU_STATUS_OK;
    }
    status = mizu_parameter_service_get_by_id(service, request->id, &value);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    status = mizu_parameter_adjust_value(descriptor, &value, &descriptor->step, request->step_count,
                                         &value);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    return mizu_parameter_service_set_by_id(service, request->id, &value);
}
