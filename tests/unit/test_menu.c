/* 验证菜单静态描述、服务绑定和调用方步长缓存初始化。 */

#include "test_support.h"

#include <stddef.h>
#include <stdint.h>

#include "menu.h"

typedef struct {
    float value;
    int command_calls;
    mizu_status_t command_status;
    mizu_status_t get_status;
} test_menu_context_t;

static mizu_status_t test_menu_get(void *context, mizu_parameter_value_t *value)
{
    const test_menu_context_t *state = context;

    if ((state == NULL) || (value == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (state->get_status != MIZU_STATUS_OK) {
        return state->get_status;
    }
    *value = (mizu_parameter_value_t){
        .type = MIZU_PARAMETER_VALUE_FLOAT,
        .data.float_value = state->value,
    };
    return MIZU_STATUS_OK;
}

static mizu_status_t test_menu_set(void *context, const mizu_parameter_value_t *value)
{
    test_menu_context_t *state = context;

    if ((state == NULL) || (value == NULL) || (value->type != MIZU_PARAMETER_VALUE_FLOAT)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    state->value = value->data.float_value;
    return MIZU_STATUS_OK;
}

static mizu_status_t test_menu_command(void *context)
{
    test_menu_context_t *state = context;

    if (state == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    state->command_calls++;
    return state->command_status;
}

static int test_init_and_step_cache(void)
{
    test_menu_context_t state = {.value = 1.0F};
    const mizu_parameter_descriptor_t parameter = {
        .id = 1U,
        .name = "gain",
        .unit = "x",
        .type = MIZU_PARAMETER_VALUE_FLOAT,
        .minimum = {.type = MIZU_PARAMETER_VALUE_FLOAT, .data.float_value = 0.0F},
        .maximum = {.type = MIZU_PARAMETER_VALUE_FLOAT, .data.float_value = 10.0F},
        .step = {.type = MIZU_PARAMETER_VALUE_FLOAT, .data.float_value = 0.1F},
        .decimals = 1U,
        .writable = true,
        .get = test_menu_get,
        .set = test_menu_set,
        .context = &state,
    };
    const mizu_parameter_group_t parameter_group = {1U, "parameters", &parameter, 1U};
    const mizu_command_descriptor_t command_descriptors[] = {
        {2U, "confirmed", test_menu_command, &state},
        {3U, "direct", test_menu_command, &state},
    };
    const mizu_command_group_t command_group = {2U, "commands", command_descriptors, 2U};
    mizu_menu_item_t items[] = {
        {.kind = MIZU_MENU_ITEM_PARAMETER, .target.parameter_id = 1U},
        {.kind = MIZU_MENU_ITEM_COMMAND,
         .target.command = {.command_id = 2U, .requires_confirmation = true}},
        {.kind = MIZU_MENU_ITEM_COMMAND,
         .target.command = {.command_id = 3U, .requires_confirmation = false}},
    };
    const mizu_menu_group_t menu_group = {
        .id = 1U,
        .title = {"Control", 7U},
        .items = items,
        .item_count = 3U,
    };
    const mizu_menu_display_page_t page = {
        .id = 1U,
        .title = {"Live", 4U},
        .page_count = 2U,
    };
    const mizu_menu_root_item_t roots[] = {
        {.kind = MIZU_MENU_ROOT_GROUP, .target.group = &menu_group},
        {.kind = MIZU_MENU_ROOT_DISPLAY_PAGE, .target.display_page = &page},
    };
    mizu_parameter_service_t parameters = {0};
    mizu_command_service_t commands = {0};
    mizu_parameter_value_t step_cache[1] = {0};
    mizu_menu_service_t menu = {0};
    const mizu_menu_config_t config = {
        .root_title = {"Debug", 5U},
        .root_items = roots,
        .root_item_count = 2U,
        .parameters = &parameters,
        .commands = &commands,
        .step_cache = step_cache,
        .step_cache_count = 1U,
        .feedback_duration_ms = 1000U,
    };

    TEST_ASSERT(mizu_parameter_service_init(&parameters, &parameter_group, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_command_service_init(&commands, &command_group, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_init(&menu, &config) == MIZU_STATUS_OK);
    {
        mizu_parameter_value_t replacement_cache[1] = {0};

        TEST_ASSERT(mizu_menu_init(&menu, &(mizu_menu_config_t){
                                              .root_title = {"Replacement", 11U},
                                              .root_items = roots,
                                              .root_item_count = 2U,
                                              .parameters = &parameters,
                                              .commands = &commands,
                                              .step_cache = replacement_cache,
                                              .step_cache_count = 1U,
                                              .feedback_duration_ms = 1000U,
                                          }) == MIZU_STATUS_INVALID_STATE);
        TEST_ASSERT(menu.config.root_items == roots);
        TEST_ASSERT(menu.config.step_cache == step_cache);
    }
    TEST_ASSERT(menu.initialized);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_ROOT);
    TEST_ASSERT(step_cache[0].type == MIZU_PARAMETER_VALUE_FLOAT);
    TEST_ASSERT(step_cache[0].data.float_value == 0.1F);
    TEST_ASSERT(state.command_calls == 0);
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_SELECTION, -1},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.root_index == 1U);
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_SELECTION, 1},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.root_index == 0U);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_GROUP);
    {
        mizu_menu_component_t components[2] = {0};
        mizu_menu_view_t view = {
            .display_page_id = 99U,
            .display_subpage_index = 8U,
            .display_subpage_count = 9U,
        };

        TEST_ASSERT(mizu_menu_query_view(&menu, 1U, components, 2U, &view) == MIZU_STATUS_OK);
        TEST_ASSERT(view.total_count == 3U);
        TEST_ASSERT(view.component_count == 1U);
        TEST_ASSERT(view.display_page_id == 0U);
        TEST_ASSERT(view.display_subpage_index == 0U);
        TEST_ASSERT(view.display_subpage_count == 0U);
        TEST_ASSERT(components[0].kind == MIZU_MENU_COMPONENT_PARAMETER);
        TEST_ASSERT(mizu_menu_query_view(&menu, 2U, components, 1U, &view) ==
                    MIZU_STATUS_OUT_OF_RANGE);
        TEST_ASSERT(view.component_count == 2U);
    }
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_PARAMETER_EDIT);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ADJUST_VALUE, 2},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.edit_value.data.float_value == 1.2F);
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_OPEN_STEP_EDITOR, 0},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ADJUST_STEP, 2},
                                       0U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(step_cache[0].data.float_value == 0.1F);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ADJUST_STEP, 1},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(step_cache[0].data.float_value == 1.0F);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_BACK, 0}, 0U) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(step_cache[0].data.float_value == 0.1F);
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_OPEN_STEP_EDITOR, 0},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ADJUST_STEP, 1},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(step_cache[0].data.float_value == 1.0F);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_BACK, 0}, 0U) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_GROUP);
    TEST_ASSERT(state.value == 1.0F);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ADJUST_VALUE, 2},
                                       0U) == MIZU_STATUS_OK);
    state.value = 4.0F;
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       100U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_PARAMETER_EDIT);
    TEST_ASSERT(menu.edit_value.data.float_value == 4.0F);
    TEST_ASSERT(menu.feedback.active && (menu.feedback.reason == MIZU_MENU_FEEDBACK_CONFLICT));
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_ADJUST_VALUE, -1},
                                       100U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       200U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_GROUP);
    TEST_ASSERT(state.value == 3.0F);
    TEST_ASSERT(menu.feedback.active && (menu.feedback.reason == MIZU_MENU_FEEDBACK_APPLIED));
    TEST_ASSERT(mizu_menu_process(&menu, 1199U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.feedback.active);
    TEST_ASSERT(mizu_menu_process(&menu, 1200U) == MIZU_STATUS_OK);
    TEST_ASSERT(!menu.feedback.active);
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_SELECTION, 1},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_COMMAND_CONFIRM);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_BACK, 0}, 0U) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(state.command_calls == 0);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       300U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_GROUP);
    TEST_ASSERT(state.command_calls == 1);
    TEST_ASSERT(menu.feedback.active && (menu.feedback.kind == MIZU_MENU_FEEDBACK_COMMAND) &&
                (menu.feedback.status == MIZU_STATUS_OK));
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_SELECTION, 1},
                                       0U) == MIZU_STATUS_OK);
    state.command_status = MIZU_STATUS_BUSY;
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       400U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_GROUP);
    TEST_ASSERT(state.command_calls == 2);
    TEST_ASSERT(state.command_status == MIZU_STATUS_BUSY);
    TEST_ASSERT(menu.feedback.status == MIZU_STATUS_BUSY);
    state.command_status = MIZU_STATUS_INTERNAL_ERROR;
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       500U) == MIZU_STATUS_OK);
    TEST_ASSERT(state.command_calls == 3);
    TEST_ASSERT(state.command_status == MIZU_STATUS_INTERNAL_ERROR);
    TEST_ASSERT(menu.feedback.status == MIZU_STATUS_INTERNAL_ERROR);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_BACK, 0}, 0U) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_ROOT);
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_SELECTION, 1},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_DISPLAY_PAGE);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_PAGE, 1},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.display_subpage_index == 1U);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_PAGE, 1},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.display_subpage_index == 0U);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_PAGE, -1},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.display_subpage_index == 1U);
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_SELECTION, 1},
                                       0U) == MIZU_STATUS_INVALID_STATE);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_BACK, 0}, 0U) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_ROOT);
    TEST_ASSERT(menu.root_index == 1U);
    {
        mizu_menu_component_t components[2] = {0};
        mizu_menu_view_t view = {0};

        TEST_ASSERT(mizu_menu_query_view(&menu, 2U, components, 2U, &view) == MIZU_STATUS_OK);
        TEST_ASSERT(view.state == MIZU_MENU_STATE_ROOT);
        TEST_ASSERT(view.component_count == 2U);
        TEST_ASSERT(view.feedback.active);
        TEST_ASSERT(view.feedback.status == MIZU_STATUS_INTERNAL_ERROR);
        TEST_ASSERT(components[1].flags.selected);
        TEST_ASSERT(mizu_menu_query_view(&menu, SIZE_MAX, components, 2U, &view) == MIZU_STATUS_OK);
        TEST_ASSERT(view.page_count == 1U);
        TEST_ASSERT(view.component_count == 2U);
        TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                           0U) == MIZU_STATUS_OK);
        TEST_ASSERT(mizu_menu_query_view(&menu, 2U, NULL, 0U, &view) == MIZU_STATUS_OK);
        TEST_ASSERT(view.state == MIZU_MENU_STATE_DISPLAY_PAGE);
        TEST_ASSERT(view.component_count == 0U);
        TEST_ASSERT(view.display_subpage_count == 2U);
        TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_BACK, 0},
                                           0U) == MIZU_STATUS_OK);
    }
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_SELECTION, -1},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       0U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_handle_event(&menu,
                                       &(mizu_menu_event_t){MIZU_MENU_EVENT_MOVE_SELECTION, -1},
                                       0U) == MIZU_STATUS_OK);
    state.command_status = MIZU_STATUS_OK;
    TEST_ASSERT(mizu_menu_handle_event(&menu, &(mizu_menu_event_t){MIZU_MENU_EVENT_ACTIVATE, 0},
                                       UINT32_MAX - 499U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_process(&menu, 499U) == MIZU_STATUS_OK);
    TEST_ASSERT(menu.feedback.active);
    TEST_ASSERT(mizu_menu_process(&menu, 500U) == MIZU_STATUS_OK);
    TEST_ASSERT(!menu.feedback.active);
    {
        mizu_menu_component_t components[2] = {
            {.id = 91U},
            {.id = 92U},
        };
        mizu_menu_view_t view = {0};

        menu.item_index = 0U;
        state.get_status = MIZU_STATUS_INTERNAL_ERROR;
        TEST_ASSERT(mizu_menu_query_view(&menu, 1U, components, 2U, &view) == MIZU_STATUS_OK);
        TEST_ASSERT(!components[0].flags.value_valid);
        TEST_ASSERT(components[0].data.parameter.value_status == MIZU_STATUS_INTERNAL_ERROR);
        components[0] = (mizu_menu_component_t){.id = 91U};
        components[1] = (mizu_menu_component_t){.id = 92U};
        items[1].target.command.command_id = 99U;
        TEST_ASSERT(items[1].target.command.command_id == 99U);
        TEST_ASSERT(mizu_menu_query_view(&menu, 2U, components, 2U, &view) ==
                    MIZU_STATUS_OUT_OF_RANGE);
        TEST_ASSERT(components[0].id == 91U);
        TEST_ASSERT(components[1].id == 92U);
    }
    return 0;
}

static int test_init_errors_are_transactional(void)
{
    test_menu_context_t state = {.value = 1.0F};
    const mizu_parameter_descriptor_t parameter = {
        .id = 1U,
        .name = "gain",
        .unit = "x",
        .type = MIZU_PARAMETER_VALUE_FLOAT,
        .minimum = {.type = MIZU_PARAMETER_VALUE_FLOAT, .data.float_value = 0.0F},
        .maximum = {.type = MIZU_PARAMETER_VALUE_FLOAT, .data.float_value = 10.0F},
        .step = {.type = MIZU_PARAMETER_VALUE_FLOAT, .data.float_value = 0.1F},
        .decimals = 1U,
        .writable = true,
        .get = test_menu_get,
        .set = test_menu_set,
        .context = &state,
    };
    const mizu_parameter_group_t parameter_group = {1U, "parameters", &parameter, 1U};
    const mizu_command_descriptor_t command = {2U, "run", test_menu_command, &state};
    const mizu_command_group_t command_group = {2U, "commands", &command, 1U};
    const mizu_menu_item_t unknown_item = {
        .kind = MIZU_MENU_ITEM_PARAMETER,
        .target.parameter_id = 99U,
    };
    const mizu_menu_group_t invalid_group = {
        .id = 1U,
        .title = {"Invalid", 7U},
        .items = &unknown_item,
        .item_count = 1U,
    };
    const mizu_menu_root_item_t root = {
        .kind = MIZU_MENU_ROOT_GROUP,
        .target.group = &invalid_group,
    };
    mizu_parameter_service_t parameters = {0};
    mizu_command_service_t commands = {0};
    mizu_parameter_value_t step_cache[1] = {
        {.type = MIZU_PARAMETER_VALUE_UINT32, .data.uint32_value = 123U},
    };
    mizu_menu_service_t menu = {.state = MIZU_MENU_STATE_DISPLAY_PAGE};
    const mizu_menu_config_t config = {
        .root_title = {"Debug", 5U},
        .root_items = &root,
        .root_item_count = 1U,
        .parameters = &parameters,
        .commands = &commands,
        .step_cache = step_cache,
        .step_cache_count = 1U,
        .feedback_duration_ms = 1000U,
    };

    TEST_ASSERT(mizu_parameter_service_init(&parameters, &parameter_group, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_command_service_init(&commands, &command_group, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_menu_init(&menu, &config) == MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(menu.state == MIZU_MENU_STATE_DISPLAY_PAGE);
    TEST_ASSERT(step_cache[0].type == MIZU_PARAMETER_VALUE_UINT32);
    TEST_ASSERT(step_cache[0].data.uint32_value == 123U);
    return 0;
}

int main(void)
{
    int (* const cases[])(void) = {test_init_and_step_cache, test_init_errors_are_transactional};

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        const int result = cases[index]();

        if (result != 0) {
            return result;
        }
    }
    return 0;
}
