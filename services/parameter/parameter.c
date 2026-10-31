/* Services 层的静态参数注册、校验和 getter/setter 路由。 */

#include "parameter.h"

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

static mizu_status_t
mizu_parameter_validate_descriptor(const mizu_parameter_descriptor_t *descriptor)
{
    if ((descriptor == NULL) || (descriptor->name == NULL) || (descriptor->name[0] == '\0') ||
        (descriptor->unit == NULL) || (descriptor->get == NULL) ||
        (descriptor->writable != (descriptor->set != NULL)) ||
        !mizu_parameter_type_is_valid(descriptor->type)) {
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
            if (descriptor->step.data.float_value <= 0.0F) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            break;
        case MIZU_PARAMETER_VALUE_INT32:
            if (descriptor->step.data.int32_value <= 0) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            break;
        case MIZU_PARAMETER_VALUE_UINT32:
            if (descriptor->step.data.uint32_value == 0U) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            break;
        case MIZU_PARAMETER_VALUE_BOOL:
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
    int64_t adjusted;
    double adjusted_float;
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
    switch (descriptor->type) {
        case MIZU_PARAMETER_VALUE_FLOAT:
            adjusted_float =
                (double)value.data.float_value +
                (double)descriptor->step.data.float_value * (double)request->step_count;
            if (adjusted_float < (double)descriptor->minimum.data.float_value) {
                value = descriptor->minimum;
            } else if (adjusted_float > (double)descriptor->maximum.data.float_value) {
                value = descriptor->maximum;
            } else {
                value.data.float_value = (float)adjusted_float;
            }
            break;
        case MIZU_PARAMETER_VALUE_INT32:
            adjusted = (int64_t)value.data.int32_value +
                       (int64_t)descriptor->step.data.int32_value * (int64_t)request->step_count;
            if (adjusted < (int64_t)descriptor->minimum.data.int32_value) {
                value = descriptor->minimum;
            } else if (adjusted > (int64_t)descriptor->maximum.data.int32_value) {
                value = descriptor->maximum;
            } else {
                value.data.int32_value = (int32_t)adjusted;
            }
            break;
        case MIZU_PARAMETER_VALUE_UINT32:
            adjusted = (int64_t)value.data.uint32_value +
                       (int64_t)descriptor->step.data.uint32_value * (int64_t)request->step_count;
            if (adjusted < (int64_t)descriptor->minimum.data.uint32_value) {
                value = descriptor->minimum;
            } else if (adjusted > (int64_t)descriptor->maximum.data.uint32_value) {
                value = descriptor->maximum;
            } else {
                value.data.uint32_value = (uint32_t)adjusted;
            }
            break;
        case MIZU_PARAMETER_VALUE_BOOL:
            value.data.bool_value = request->step_count > 0;
            break;
        default:
            return MIZU_STATUS_INVALID_ARGUMENT;
    }
    return mizu_parameter_service_set_by_id(service, request->id, &value);
}
