/* Services 层的硬件无关菜单装配和状态；不读取输入设备或执行显示渲染。 */

#include "menu.h"

#include <limits.h>
#include <string.h>

static bool mizu_menu_text_is_valid(mizu_menu_text_t text)
{
    return (text.data != NULL) && (text.length > 0U);
}

typedef struct {
    const mizu_menu_root_item_t *roots;
    mizu_menu_group_id_t id;
    size_t root_index;
} mizu_menu_group_search_t;

static bool mizu_menu_group_id_exists(const mizu_menu_group_search_t *search)
{
    for (size_t index = 0U; index < search->root_index; ++index) {
        if ((search->roots[index].kind == MIZU_MENU_ROOT_GROUP) &&
            (search->roots[index].target.group->id == search->id)) {
            return true;
        }
    }
    return false;
}

typedef struct {
    const mizu_menu_root_item_t *roots;
    mizu_menu_page_id_t id;
    size_t root_index;
} mizu_menu_page_search_t;

static bool mizu_menu_page_id_exists(const mizu_menu_page_search_t *search)
{
    for (size_t index = 0U; index < search->root_index; ++index) {
        if ((search->roots[index].kind == MIZU_MENU_ROOT_DISPLAY_PAGE) &&
            (search->roots[index].target.display_page->id == search->id)) {
            return true;
        }
    }
    return false;
}

typedef struct {
    const mizu_menu_root_item_t *roots;
    mizu_menu_item_kind_t kind;
    uint16_t id;
    size_t root_index;
    size_t item_index;
} mizu_menu_binding_search_t;

static bool mizu_menu_binding_exists(const mizu_menu_binding_search_t *search)
{
    for (size_t earlier_root = 0U; earlier_root <= search->root_index; ++earlier_root) {
        const mizu_menu_root_item_t *root = &search->roots[earlier_root];
        size_t limit;

        if (root->kind != MIZU_MENU_ROOT_GROUP) {
            continue;
        }
        limit = earlier_root == search->root_index ? search->item_index
                                                   : root->target.group->item_count;
        for (size_t earlier_item = 0U; earlier_item < limit; ++earlier_item) {
            const mizu_menu_item_t *item = &root->target.group->items[earlier_item];
            const uint16_t item_id = item->kind == MIZU_MENU_ITEM_PARAMETER
                                         ? item->target.parameter_id
                                         : item->target.command.command_id;

            if ((item->kind == search->kind) && (item_id == search->id)) {
                return true;
            }
        }
    }
    return false;
}

static mizu_status_t mizu_menu_validate_item(const mizu_menu_config_t *config, size_t root_index,
                                             size_t item_index)
{
    const mizu_menu_item_t *item = &config->root_items[root_index].target.group->items[item_index];

    switch (item->kind) {
        case MIZU_MENU_ITEM_PARAMETER: {
            const mizu_parameter_descriptor_t *descriptor;
            const mizu_status_t status =
                mizu_parameter_service_get_descriptor_by_id(config->parameters,
                                                            item->target.parameter_id, &descriptor);

            if (status != MIZU_STATUS_OK) {
                return status;
            }
            if (!descriptor->writable ||
                mizu_menu_binding_exists(&(mizu_menu_binding_search_t){config->root_items,
                                                                       item->kind,
                                                                       item->target.parameter_id,
                                                                       root_index, item_index})) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            break;
        }
        case MIZU_MENU_ITEM_COMMAND: {
            const mizu_command_descriptor_t *descriptor;
            const mizu_status_t status =
                mizu_command_service_get_descriptor_by_id(config->commands,
                                                          item->target.command.command_id,
                                                          &descriptor);

            if (status != MIZU_STATUS_OK) {
                return status;
            }
            if (mizu_menu_binding_exists(&(mizu_menu_binding_search_t){config->root_items,
                                                                       item->kind,
                                                                       item->target.command
                                                                           .command_id,
                                                                       root_index, item_index})) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            break;
        }
        default:
            return MIZU_STATUS_INVALID_ARGUMENT;
    }
    return MIZU_STATUS_OK;
}

static mizu_status_t mizu_menu_validate_roots(const mizu_menu_config_t *config)
{
    for (size_t root_index = 0U; root_index < config->root_item_count; ++root_index) {
        const mizu_menu_root_item_t *root = &config->root_items[root_index];

        if (root->kind == MIZU_MENU_ROOT_GROUP) {
            const mizu_menu_group_t *group = root->target.group;

            if ((group == NULL) || !mizu_menu_text_is_valid(group->title) ||
                (group->items == NULL) || (group->item_count == 0U) ||
                mizu_menu_group_id_exists(&(mizu_menu_group_search_t){config->root_items, group->id,
                                                                      root_index})) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            for (size_t item_index = 0U; item_index < group->item_count; ++item_index) {
                const mizu_status_t status =
                    mizu_menu_validate_item(config, root_index, item_index);

                if (status != MIZU_STATUS_OK) {
                    return status;
                }
            }
        } else if (root->kind == MIZU_MENU_ROOT_DISPLAY_PAGE) {
            const mizu_menu_display_page_t *page = root->target.display_page;

            if ((page == NULL) || !mizu_menu_text_is_valid(page->title) ||
                (page->page_count == 0U) ||
                mizu_menu_page_id_exists(&(mizu_menu_page_search_t){config->root_items, page->id,
                                                                    root_index})) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
        } else {
            return MIZU_STATUS_INVALID_ARGUMENT;
        }
    }
    return MIZU_STATUS_OK;
}

static mizu_status_t mizu_menu_parameter_count(const mizu_parameter_service_t *parameters,
                                               size_t *parameter_count)
{
    size_t group_count;
    mizu_status_t status = mizu_parameter_service_get_group_count(parameters, &group_count);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    *parameter_count = 0U;
    for (size_t group_index = 0U; group_index < group_count; ++group_index) {
        const mizu_parameter_group_t *group;

        status = mizu_parameter_service_get_group(parameters, group_index, &group);
        if (status != MIZU_STATUS_OK) {
            return status;
        }
        if (group->parameter_count > (SIZE_MAX - *parameter_count)) {
            return MIZU_STATUS_OVERFLOW;
        }
        *parameter_count += group->parameter_count;
    }
    return MIZU_STATUS_OK;
}

static void mizu_menu_initialize_steps(const mizu_parameter_service_t *parameters,
                                       mizu_parameter_value_t *step_cache)
{
    size_t cache_index = 0U;
    size_t group_count = 0U;

    (void)mizu_parameter_service_get_group_count(parameters, &group_count);
    for (size_t group_index = 0U; group_index < group_count; ++group_index) {
        const mizu_parameter_group_t *group = NULL;

        (void)mizu_parameter_service_get_group(parameters, group_index, &group);
        for (size_t item_index = 0U; item_index < group->parameter_count; ++item_index) {
            step_cache[cache_index] = group->parameters[item_index].step;
            cache_index++;
        }
    }
}

mizu_status_t mizu_menu_init(mizu_menu_service_t *menu, const mizu_menu_config_t *config)
{
    mizu_status_t status;
    size_t parameter_count;

    if ((menu == NULL) || (config == NULL) || !mizu_menu_text_is_valid(config->root_title) ||
        (config->root_items == NULL) || (config->root_item_count == 0U) ||
        (config->parameters == NULL) || (config->commands == NULL) ||
        (config->step_cache == NULL) || (config->feedback_duration_ms == 0U) ||
        (config->feedback_duration_ms > (uint32_t)INT32_MAX)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    status = mizu_menu_parameter_count(config->parameters, &parameter_count);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    status = mizu_command_service_get_group_count(config->commands, &(size_t){0U});
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (config->step_cache_count != parameter_count) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    status = mizu_menu_validate_roots(config);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    mizu_menu_initialize_steps(config->parameters, config->step_cache);
    *menu = (mizu_menu_service_t){
        .config = *config,
        .feedback = {.kind = MIZU_MENU_FEEDBACK_NONE},
        .state = MIZU_MENU_STATE_ROOT,
        .root_index = 0U,
        .item_index = 0U,
        .display_subpage_index = 0U,
        .step_cache_index = 0U,
        .feedback_deadline_ms = 0U,
        .initialized = true,
    };
    return MIZU_STATUS_OK;
}

static size_t mizu_menu_move_index(size_t index, size_t count, int32_t amount)
{
    size_t delta;

    if (amount >= 0) {
        delta = (size_t)(uint32_t)amount % count;
        return delta > (count - index - 1U) ? index - (count - delta) : index + delta;
    }
    delta = (size_t)(uint32_t)(-(int64_t)amount) % count;
    return delta > index ? count - (delta - index) : index - delta;
}

static bool mizu_menu_value_is_equal(const mizu_parameter_value_t *left,
                                     const mizu_parameter_value_t *right)
{
    if (left->type != right->type) {
        return false;
    }
    switch (left->type) {
        case MIZU_PARAMETER_VALUE_FLOAT:
            return left->data.float_value == right->data.float_value;
        case MIZU_PARAMETER_VALUE_INT32:
            return left->data.int32_value == right->data.int32_value;
        case MIZU_PARAMETER_VALUE_UINT32:
            return left->data.uint32_value == right->data.uint32_value;
        case MIZU_PARAMETER_VALUE_BOOL:
            return left->data.bool_value == right->data.bool_value;
        default:
            return false;
    }
}

static const mizu_menu_group_t *mizu_menu_current_group(const mizu_menu_service_t *menu)
{
    return menu->config.root_items[menu->root_index].target.group;
}

static const mizu_menu_item_t *mizu_menu_current_item(const mizu_menu_service_t *menu)
{
    return &mizu_menu_current_group(menu)->items[menu->item_index];
}

static mizu_status_t mizu_menu_find_step_index(const mizu_menu_service_t *menu,
                                               mizu_parameter_id_t parameter_id, size_t *step_index)
{
    size_t group_count;
    size_t linear_index = 0U;
    mizu_status_t status =
        mizu_parameter_service_get_group_count(menu->config.parameters, &group_count);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    for (size_t group_index = 0U; group_index < group_count; ++group_index) {
        const mizu_parameter_group_t *group;

        status = mizu_parameter_service_get_group(menu->config.parameters, group_index, &group);
        if (status != MIZU_STATUS_OK) {
            return status;
        }
        for (size_t parameter_index = 0U; parameter_index < group->parameter_count;
             ++parameter_index) {
            if (group->parameters[parameter_index].id == parameter_id) {
                *step_index = linear_index;
                return MIZU_STATUS_OK;
            }
            linear_index++;
        }
    }
    return MIZU_STATUS_OUT_OF_RANGE;
}

typedef struct {
    const mizu_parameter_descriptor_t *descriptor;
    mizu_menu_feedback_reason_t reason;
    mizu_status_t status;
    uint32_t now_ms;
} mizu_menu_parameter_feedback_t;

static void mizu_menu_set_parameter_feedback(mizu_menu_service_t *menu,
                                             const mizu_menu_parameter_feedback_t *feedback)
{
    menu->feedback = (mizu_menu_feedback_t){
        .kind = MIZU_MENU_FEEDBACK_PARAMETER,
        .reason = feedback->reason,
        .id = feedback->descriptor->id,
        .label = {feedback->descriptor->name, strlen(feedback->descriptor->name)},
        .status = feedback->status,
        .active = true,
    };
    menu->feedback_deadline_ms = feedback->now_ms + menu->config.feedback_duration_ms;
}

typedef struct {
    const mizu_command_descriptor_t *descriptor;
    mizu_status_t status;
    uint32_t now_ms;
} mizu_menu_command_feedback_t;

static void mizu_menu_set_command_feedback(mizu_menu_service_t *menu,
                                           const mizu_menu_command_feedback_t *feedback)
{
    menu->feedback = (mizu_menu_feedback_t){
        .kind = MIZU_MENU_FEEDBACK_COMMAND,
        .reason = MIZU_MENU_FEEDBACK_RESULT,
        .id = feedback->descriptor->id,
        .label = {feedback->descriptor->name, strlen(feedback->descriptor->name)},
        .status = feedback->status,
        .active = true,
    };
    menu->feedback_deadline_ms = feedback->now_ms + menu->config.feedback_duration_ms;
}

static mizu_status_t mizu_menu_execute_command(mizu_menu_service_t *menu, uint32_t now_ms)
{
    const mizu_menu_item_t *item = mizu_menu_current_item(menu);
    const mizu_command_descriptor_t *descriptor;
    mizu_status_t status =
        mizu_command_service_get_descriptor_by_id(menu->config.commands,
                                                  item->target.command.command_id, &descriptor);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    status = mizu_command_service_execute(menu->config.commands, descriptor->id);
    menu->state = MIZU_MENU_STATE_GROUP;
    mizu_menu_set_command_feedback(menu,
                                   &(mizu_menu_command_feedback_t){descriptor, status, now_ms});
    return MIZU_STATUS_OK;
}

static mizu_status_t mizu_menu_enter_parameter_edit(mizu_menu_service_t *menu, uint32_t now_ms)
{
    const mizu_menu_item_t *item = mizu_menu_current_item(menu);
    const mizu_parameter_descriptor_t *descriptor;
    mizu_parameter_value_t value;
    size_t step_index;
    mizu_status_t status =
        mizu_parameter_service_get_descriptor_by_id(menu->config.parameters,
                                                    item->target.parameter_id, &descriptor);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    status = mizu_parameter_service_get_by_id(menu->config.parameters, descriptor->id, &value);
    if (status != MIZU_STATUS_OK) {
        mizu_menu_set_parameter_feedback(
            menu, &(mizu_menu_parameter_feedback_t){descriptor, MIZU_MENU_FEEDBACK_READ_FAILED,
                                                    status, now_ms});
        return MIZU_STATUS_OK;
    }
    status = mizu_menu_find_step_index(menu, descriptor->id, &step_index);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    menu->edit_value = value;
    menu->original_value = value;
    menu->step_cache_index = step_index;
    menu->state = MIZU_MENU_STATE_PARAMETER_EDIT;
    return MIZU_STATUS_OK;
}

static mizu_status_t mizu_menu_handle_root(mizu_menu_service_t *menu,
                                           const mizu_menu_event_t *event)
{
    if (event->kind == MIZU_MENU_EVENT_MOVE_SELECTION) {
        menu->root_index =
            mizu_menu_move_index(menu->root_index, menu->config.root_item_count, event->amount);
        return MIZU_STATUS_OK;
    }
    if (event->kind == MIZU_MENU_EVENT_ACTIVATE) {
        if (menu->config.root_items[menu->root_index].kind == MIZU_MENU_ROOT_GROUP) {
            menu->item_index = 0U;
            menu->state = MIZU_MENU_STATE_GROUP;
        } else {
            menu->display_subpage_index = 0U;
            menu->state = MIZU_MENU_STATE_DISPLAY_PAGE;
        }
        return MIZU_STATUS_OK;
    }
    return MIZU_STATUS_INVALID_STATE;
}

static mizu_status_t mizu_menu_handle_group(mizu_menu_service_t *menu,
                                            const mizu_menu_event_t *event, uint32_t now_ms)
{
    const mizu_menu_group_t *group = mizu_menu_current_group(menu);

    if (event->kind == MIZU_MENU_EVENT_MOVE_SELECTION) {
        menu->item_index = mizu_menu_move_index(menu->item_index, group->item_count, event->amount);
        return MIZU_STATUS_OK;
    }
    if (event->kind == MIZU_MENU_EVENT_BACK) {
        menu->state = MIZU_MENU_STATE_ROOT;
        return MIZU_STATUS_OK;
    }
    if ((event->kind == MIZU_MENU_EVENT_ACTIVATE) &&
        (mizu_menu_current_item(menu)->kind == MIZU_MENU_ITEM_PARAMETER)) {
        return mizu_menu_enter_parameter_edit(menu, now_ms);
    }
    if ((event->kind == MIZU_MENU_EVENT_ACTIVATE) &&
        (mizu_menu_current_item(menu)->kind == MIZU_MENU_ITEM_COMMAND)) {
        if (mizu_menu_current_item(menu)->target.command.requires_confirmation) {
            menu->state = MIZU_MENU_STATE_COMMAND_CONFIRM;
            return MIZU_STATUS_OK;
        }
        return mizu_menu_execute_command(menu, now_ms);
    }
    return MIZU_STATUS_INVALID_STATE;
}

static mizu_status_t mizu_menu_handle_parameter_edit(mizu_menu_service_t *menu,
                                                     const mizu_menu_event_t *event,
                                                     uint32_t now_ms)
{
    const mizu_menu_item_t *item = mizu_menu_current_item(menu);
    const mizu_parameter_descriptor_t *descriptor;
    mizu_status_t status =
        mizu_parameter_service_get_descriptor_by_id(menu->config.parameters,
                                                    item->target.parameter_id, &descriptor);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (event->kind == MIZU_MENU_EVENT_ADJUST_VALUE) {
        status = mizu_parameter_adjust_value(descriptor, &menu->edit_value,
                                             &menu->config.step_cache[menu->step_cache_index],
                                             event->amount, &menu->edit_value);
        if (status != MIZU_STATUS_OK) {
            mizu_menu_set_parameter_feedback(
                menu, &(mizu_menu_parameter_feedback_t){descriptor, MIZU_MENU_FEEDBACK_WRITE_FAILED,
                                                        status, now_ms});
        }
        return MIZU_STATUS_OK;
    }
    if (event->kind == MIZU_MENU_EVENT_OPEN_STEP_EDITOR) {
        if ((descriptor->type == MIZU_PARAMETER_VALUE_BOOL) ||
            mizu_menu_value_is_equal(&descriptor->minimum, &descriptor->maximum)) {
            return MIZU_STATUS_INVALID_STATE;
        }
        menu->original_step = menu->config.step_cache[menu->step_cache_index];
        menu->state = MIZU_MENU_STATE_STEP_EDIT;
        return MIZU_STATUS_OK;
    }
    if (event->kind == MIZU_MENU_EVENT_BACK) {
        menu->state = MIZU_MENU_STATE_GROUP;
        return MIZU_STATUS_OK;
    }
    if (event->kind == MIZU_MENU_EVENT_ACTIVATE) {
        mizu_parameter_value_t current;

        status =
            mizu_parameter_service_get_by_id(menu->config.parameters, descriptor->id, &current);
        if (status != MIZU_STATUS_OK) {
            mizu_menu_set_parameter_feedback(
                menu, &(mizu_menu_parameter_feedback_t){descriptor, MIZU_MENU_FEEDBACK_READ_FAILED,
                                                        status, now_ms});
            return MIZU_STATUS_OK;
        }
        if (!mizu_menu_value_is_equal(&current, &menu->original_value)) {
            menu->edit_value = current;
            menu->original_value = current;
            mizu_menu_set_parameter_feedback(
                menu, &(mizu_menu_parameter_feedback_t){descriptor, MIZU_MENU_FEEDBACK_CONFLICT,
                                                        MIZU_STATUS_INVALID_STATE, now_ms});
            return MIZU_STATUS_OK;
        }
        status = mizu_parameter_service_set_by_id(menu->config.parameters, descriptor->id,
                                                  &menu->edit_value);
        if (status != MIZU_STATUS_OK) {
            mizu_menu_set_parameter_feedback(
                menu, &(mizu_menu_parameter_feedback_t){descriptor, MIZU_MENU_FEEDBACK_WRITE_FAILED,
                                                        status, now_ms});
            return MIZU_STATUS_OK;
        }
        menu->state = MIZU_MENU_STATE_GROUP;
        mizu_menu_set_parameter_feedback(
            menu, &(mizu_menu_parameter_feedback_t){descriptor, MIZU_MENU_FEEDBACK_APPLIED,
                                                    MIZU_STATUS_OK, now_ms});
        return MIZU_STATUS_OK;
    }
    return MIZU_STATUS_INVALID_STATE;
}

static mizu_status_t mizu_menu_handle_step_edit(mizu_menu_service_t *menu,
                                                const mizu_menu_event_t *event)
{
    const mizu_menu_item_t *item = mizu_menu_current_item(menu);
    const mizu_parameter_descriptor_t *descriptor;
    mizu_status_t status;

    if ((event->kind == MIZU_MENU_EVENT_ADJUST_STEP) &&
        ((event->amount < -1) || (event->amount > 1))) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    status = mizu_parameter_service_get_descriptor_by_id(menu->config.parameters,
                                                         item->target.parameter_id, &descriptor);
    if (status != MIZU_STATUS_OK) {
        return status;
    }
    if (event->kind == MIZU_MENU_EVENT_ADJUST_STEP) {
        return mizu_parameter_scale_step(descriptor,
                                         &menu->config.step_cache[menu->step_cache_index],
                                         event->amount,
                                         &menu->config.step_cache[menu->step_cache_index]);
    }
    if (event->kind == MIZU_MENU_EVENT_ACTIVATE) {
        menu->state = MIZU_MENU_STATE_PARAMETER_EDIT;
        return MIZU_STATUS_OK;
    }
    if (event->kind == MIZU_MENU_EVENT_BACK) {
        menu->config.step_cache[menu->step_cache_index] = menu->original_step;
        menu->state = MIZU_MENU_STATE_PARAMETER_EDIT;
        return MIZU_STATUS_OK;
    }
    return MIZU_STATUS_INVALID_STATE;
}

static mizu_status_t mizu_menu_handle_command_confirm(mizu_menu_service_t *menu,
                                                      const mizu_menu_event_t *event,
                                                      uint32_t now_ms)
{
    if (event->kind == MIZU_MENU_EVENT_ACTIVATE) {
        return mizu_menu_execute_command(menu, now_ms);
    }
    if (event->kind == MIZU_MENU_EVENT_BACK) {
        menu->state = MIZU_MENU_STATE_GROUP;
        return MIZU_STATUS_OK;
    }
    return MIZU_STATUS_INVALID_STATE;
}

static mizu_status_t mizu_menu_handle_display_page(mizu_menu_service_t *menu,
                                                   const mizu_menu_event_t *event)
{
    const mizu_menu_display_page_t *page =
        menu->config.root_items[menu->root_index].target.display_page;

    if (event->kind == MIZU_MENU_EVENT_MOVE_PAGE) {
        menu->display_subpage_index =
            mizu_menu_move_index(menu->display_subpage_index, page->page_count, event->amount);
        return MIZU_STATUS_OK;
    }
    if (event->kind == MIZU_MENU_EVENT_BACK) {
        menu->state = MIZU_MENU_STATE_ROOT;
        return MIZU_STATUS_OK;
    }
    return MIZU_STATUS_INVALID_STATE;
}

mizu_status_t mizu_menu_handle_event(mizu_menu_service_t *menu, const mizu_menu_event_t *event,
                                     uint32_t now_ms)
{
    if ((menu == NULL) || (event == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!menu->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    switch (menu->state) {
        case MIZU_MENU_STATE_ROOT:
            return mizu_menu_handle_root(menu, event);
        case MIZU_MENU_STATE_GROUP:
            return mizu_menu_handle_group(menu, event, now_ms);
        case MIZU_MENU_STATE_PARAMETER_EDIT:
            return mizu_menu_handle_parameter_edit(menu, event, now_ms);
        case MIZU_MENU_STATE_STEP_EDIT:
            return mizu_menu_handle_step_edit(menu, event);
        case MIZU_MENU_STATE_COMMAND_CONFIRM:
            return mizu_menu_handle_command_confirm(menu, event, now_ms);
        case MIZU_MENU_STATE_DISPLAY_PAGE:
            return mizu_menu_handle_display_page(menu, event);
        default:
            return MIZU_STATUS_INVALID_STATE;
    }
}

mizu_status_t mizu_menu_process(mizu_menu_service_t *menu, uint32_t now_ms)
{
    if (menu == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!menu->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    if (menu->feedback.active && ((int32_t)(now_ms - menu->feedback_deadline_ms) >= 0)) {
        menu->feedback = (mizu_menu_feedback_t){.kind = MIZU_MENU_FEEDBACK_NONE};
    }
    return MIZU_STATUS_OK;
}

static void mizu_menu_view_set_common(const mizu_menu_service_t *menu, mizu_menu_view_t *view)
{
    *view = (mizu_menu_view_t){
        .state = menu->state,
        .feedback = menu->feedback,
    };
}

static size_t mizu_menu_page_count(size_t total_count, size_t visible_count)
{
    return total_count / visible_count + ((total_count % visible_count) != 0U);
}

static void mizu_menu_fill_root_component(const mizu_menu_service_t *menu, size_t index,
                                          mizu_menu_component_t *component)
{
    const mizu_menu_root_item_t *root = &menu->config.root_items[index];

    *component = (mizu_menu_component_t){
        .kind = MIZU_MENU_COMPONENT_ROOT_ITEM,
        .flags = {.selected = index == menu->root_index},
    };
    if (root->kind == MIZU_MENU_ROOT_GROUP) {
        component->id = root->target.group->id;
        component->label = root->target.group->title;
    } else {
        component->id = root->target.display_page->id;
        component->label = root->target.display_page->title;
    }
}

static mizu_status_t mizu_menu_fill_group_component(const mizu_menu_service_t *menu, size_t index,
                                                    mizu_menu_component_t *component)
{
    const mizu_menu_group_t *group = mizu_menu_current_group(menu);
    const mizu_menu_item_t *item = &group->items[index];

    *component = (mizu_menu_component_t){
        .flags = {.selected = index == menu->item_index},
    };
    if (item->kind == MIZU_MENU_ITEM_PARAMETER) {
        const mizu_parameter_descriptor_t *descriptor;
        mizu_parameter_value_t value = {0};
        mizu_status_t status =
            mizu_parameter_service_get_descriptor_by_id(menu->config.parameters,
                                                        item->target.parameter_id, &descriptor);

        if (status != MIZU_STATUS_OK) {
            return status;
        }
        status = mizu_parameter_service_get_by_id(menu->config.parameters, descriptor->id, &value);
        *component = (mizu_menu_component_t){
            .kind = MIZU_MENU_COMPONENT_PARAMETER,
            .id = descriptor->id,
            .label = {descriptor->name, strlen(descriptor->name)},
            .flags = {.selected = index == menu->item_index,
                      .editable = descriptor->writable,
                      .value_valid = status == MIZU_STATUS_OK},
            .data.parameter =
                {
                    .value = value,
                    .minimum = descriptor->minimum,
                    .maximum = descriptor->maximum,
                    .default_step = descriptor->step,
                    .unit = {descriptor->unit, strlen(descriptor->unit)},
                    .decimals = descriptor->decimals,
                    .value_status = status,
                },
        };
        return MIZU_STATUS_OK;
    }
    {
        const mizu_command_descriptor_t *descriptor;
        const mizu_status_t status =
            mizu_command_service_get_descriptor_by_id(menu->config.commands,
                                                      item->target.command.command_id, &descriptor);

        if (status != MIZU_STATUS_OK) {
            return status;
        }
        *component = (mizu_menu_component_t){
            .kind = MIZU_MENU_COMPONENT_COMMAND,
            .id = descriptor->id,
            .label = {descriptor->name, strlen(descriptor->name)},
            .flags = {.selected = index == menu->item_index,
                      .requires_confirmation = item->target.command.requires_confirmation},
        };
    }
    return MIZU_STATUS_OK;
}

static mizu_status_t mizu_menu_validate_group_page(const mizu_menu_service_t *menu,
                                                   const mizu_menu_view_t *view, size_t first_index)
{
    const mizu_menu_group_t *group = mizu_menu_current_group(menu);

    for (size_t index = 0U; index < view->component_count; ++index) {
        const mizu_menu_item_t *item = &group->items[first_index + index];
        mizu_status_t status;

        if (item->kind == MIZU_MENU_ITEM_PARAMETER) {
            const mizu_parameter_descriptor_t *descriptor;

            status =
                mizu_parameter_service_get_descriptor_by_id(menu->config.parameters,
                                                            item->target.parameter_id, &descriptor);
        } else {
            const mizu_command_descriptor_t *descriptor;

            status = mizu_command_service_get_descriptor_by_id(menu->config.commands,
                                                               item->target.command.command_id,
                                                               &descriptor);
        }
        if (status != MIZU_STATUS_OK) {
            return status;
        }
    }
    return MIZU_STATUS_OK;
}

static mizu_status_t mizu_menu_fill_editor_component(const mizu_menu_service_t *menu,
                                                     mizu_menu_component_t *component)
{
    const mizu_menu_item_t *item = mizu_menu_current_item(menu);
    const mizu_parameter_descriptor_t *descriptor;
    mizu_status_t status =
        mizu_parameter_service_get_descriptor_by_id(menu->config.parameters,
                                                    item->target.parameter_id, &descriptor);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    *component = (mizu_menu_component_t){
        .kind = MIZU_MENU_COMPONENT_PARAMETER_EDITOR,
        .id = descriptor->id,
        .label = {descriptor->name, strlen(descriptor->name)},
        .flags = {.editable = true, .editing = true},
        .data.parameter =
            {
                .value = menu->edit_value,
                .minimum = descriptor->minimum,
                .maximum = descriptor->maximum,
                .default_step = descriptor->step,
                .current_step = menu->config.step_cache[menu->step_cache_index],
                .unit = {descriptor->unit, strlen(descriptor->unit)},
                .decimals = descriptor->decimals,
                .value_status = MIZU_STATUS_OK,
            },
    };
    return MIZU_STATUS_OK;
}

static mizu_status_t mizu_menu_fill_command_component(const mizu_menu_service_t *menu,
                                                      mizu_menu_component_t *component)
{
    const mizu_menu_item_t *item = mizu_menu_current_item(menu);
    const mizu_command_descriptor_t *descriptor;
    mizu_status_t status =
        mizu_command_service_get_descriptor_by_id(menu->config.commands,
                                                  item->target.command.command_id, &descriptor);

    if (status != MIZU_STATUS_OK) {
        return status;
    }
    *component = (mizu_menu_component_t){
        .kind = MIZU_MENU_COMPONENT_COMMAND_CONFIRMATION,
        .id = descriptor->id,
        .label = {descriptor->name, strlen(descriptor->name)},
        .flags = {.editing = true, .requires_confirmation = true},
    };
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_menu_query_view(const mizu_menu_service_t *menu, size_t visible_count,
                                   mizu_menu_component_t *components, size_t component_capacity,
                                   mizu_menu_view_t *view)
{
    size_t total_count = 0U;
    size_t first_index = 0U;
    size_t page_count = 0U;

    if ((menu == NULL) || (view == NULL) || (visible_count == 0U)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!menu->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    mizu_menu_view_set_common(menu, view);
    if ((menu->state == MIZU_MENU_STATE_DISPLAY_PAGE) ||
        (menu->state == MIZU_MENU_STATE_PARAMETER_EDIT) ||
        (menu->state == MIZU_MENU_STATE_STEP_EDIT) ||
        (menu->state == MIZU_MENU_STATE_COMMAND_CONFIRM)) {
        if (menu->state == MIZU_MENU_STATE_DISPLAY_PAGE) {
            const mizu_menu_display_page_t *page =
                menu->config.root_items[menu->root_index].target.display_page;

            view->title = page->title;
            view->display_page_id = page->id;
            view->display_subpage_index = menu->display_subpage_index;
            view->display_subpage_count = page->page_count;
            view->component_count = 0U;
            return MIZU_STATUS_OK;
        }
        view->component_count = 1U;
        view->total_count = 1U;
        view->page_count = 1U;
        if ((components == NULL) || (component_capacity < 1U)) {
            return MIZU_STATUS_OUT_OF_RANGE;
        }
        view->title = mizu_menu_current_group(menu)->title;
        if (menu->state == MIZU_MENU_STATE_COMMAND_CONFIRM) {
            return mizu_menu_fill_command_component(menu, &components[0]);
        }
        return mizu_menu_fill_editor_component(menu, &components[0]);
    }
    if (menu->state == MIZU_MENU_STATE_ROOT) {
        total_count = menu->config.root_item_count;
        view->title = menu->config.root_title;
    } else if (menu->state == MIZU_MENU_STATE_GROUP) {
        total_count = mizu_menu_current_group(menu)->item_count;
        view->title = mizu_menu_current_group(menu)->title;
    } else {
        return MIZU_STATUS_INVALID_STATE;
    }
    page_count = mizu_menu_page_count(total_count, visible_count);
    first_index = (menu->state == MIZU_MENU_STATE_ROOT ? menu->root_index : menu->item_index) /
                  visible_count * visible_count;
    view->total_count = total_count;
    view->page_count = page_count;
    view->page_index = first_index / visible_count;
    view->component_count =
        total_count - first_index < visible_count ? total_count - first_index : visible_count;
    if ((components == NULL) || (component_capacity < view->component_count)) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    if (menu->state == MIZU_MENU_STATE_GROUP) {
        mizu_status_t status = mizu_menu_validate_group_page(menu, view, first_index);

        if (status != MIZU_STATUS_OK) {
            return status;
        }
    }
    for (size_t index = 0U; index < view->component_count; ++index) {
        const size_t item_index = first_index + index;
        mizu_status_t status = MIZU_STATUS_OK;

        if (menu->state == MIZU_MENU_STATE_ROOT) {
            mizu_menu_fill_root_component(menu, item_index, &components[index]);
        } else {
            status = mizu_menu_fill_group_component(menu, item_index, &components[index]);
        }
        if (status != MIZU_STATUS_OK) {
            return status;
        }
    }
    return MIZU_STATUS_OK;
}
