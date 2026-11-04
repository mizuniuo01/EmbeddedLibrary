/* 验证命令服务的静态注册、同步执行、BUSY 传播和结果快照。 */

#include "test_support.h"

#include <stddef.h>

#include "command.h"

typedef struct {
    int calls;
    mizu_status_t status;
} test_command_context_t;

static mizu_status_t test_handler(void *context)
{
    test_command_context_t *state = context;

    if (state == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    state->calls++;
    return state->status;
}

static int test_registration_and_execution(void)
{
    test_command_context_t state = {.status = MIZU_STATUS_OK};
    const mizu_command_descriptor_t commands[] = {{1U, "run", test_handler, &state}};
    const mizu_command_group_t groups[] = {{10U, "actions", commands, 1U}};
    mizu_command_service_t service = {0};
    const mizu_command_group_t *view;
    mizu_command_snapshot_t snapshot;
    size_t count;

    TEST_ASSERT(mizu_command_service_init(&service, groups, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_command_service_get_group_count(&service, &count) == MIZU_STATUS_OK);
    TEST_ASSERT(count == 1U);
    TEST_ASSERT(mizu_command_service_get_group(&service, 0U, &view) == MIZU_STATUS_OK);
    TEST_ASSERT(view == &groups[0]);
    TEST_ASSERT(mizu_command_service_get_snapshot(&service, &snapshot) == MIZU_STATUS_OK);
    TEST_ASSERT(!snapshot.has_result);
    TEST_ASSERT(mizu_command_service_execute(&service, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(state.calls == 1);
    TEST_ASSERT(mizu_command_service_get_snapshot(&service, &snapshot) == MIZU_STATUS_OK);
    TEST_ASSERT(snapshot.has_result && (snapshot.command_id == 1U) &&
                (snapshot.status == MIZU_STATUS_OK) && (snapshot.sequence == 1U));
    return 0;
}

static int test_busy_error_and_unknown(void)
{
    test_command_context_t state = {.status = MIZU_STATUS_BUSY};
    const mizu_command_descriptor_t commands[] = {{2U, "busy", test_handler, &state}};
    const mizu_command_group_t groups[] = {{10U, "actions", commands, 1U}};
    mizu_command_service_t service = {0};
    mizu_command_snapshot_t snapshot;

    TEST_ASSERT(mizu_command_service_init(&service, groups, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_command_service_execute(&service, 2U) == MIZU_STATUS_BUSY);
    TEST_ASSERT(state.calls == 1);
    state.status = MIZU_STATUS_INTERNAL_ERROR;
    TEST_ASSERT(mizu_command_service_execute(&service, 2U) == MIZU_STATUS_INTERNAL_ERROR);
    TEST_ASSERT(state.calls == 2);
    TEST_ASSERT(mizu_command_service_execute(&service, 99U) == MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(mizu_command_service_get_snapshot(&service, &snapshot) == MIZU_STATUS_OK);
    TEST_ASSERT(snapshot.command_id == 99U);
    TEST_ASSERT(snapshot.status == MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(snapshot.sequence == 3U);
    return 0;
}

static int test_registration_errors(void)
{
    test_command_context_t state = {.status = MIZU_STATUS_OK};
    const mizu_command_descriptor_t command = {1U, "run", test_handler, &state};
    const mizu_command_group_t group = {10U, "actions", &command, 1U};
    mizu_command_service_t service = {0};
    mizu_command_group_t duplicate_groups[] = {
        {10U, "one", &command, 1U},
        {10U, "two", &command, 1U},
    };
    mizu_command_descriptor_t duplicate_commands[] = {
        {1U, "one", test_handler, &state},
        {1U, "two", test_handler, &state},
    };
    const mizu_command_group_t duplicate_command_group = {11U, "commands", duplicate_commands, 2U};
    const mizu_command_group_t invalid_name_group = {12U, "", &command, 1U};
    const mizu_command_descriptor_t invalid_handler_command = {3U, "invalid", NULL, &state};
    const mizu_command_group_t invalid_handler_group = {13U, "invalid", &invalid_handler_command,
                                                        1U};
    mizu_command_service_t before = {0};

    TEST_ASSERT(mizu_command_service_get_group_count(&service, NULL) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_command_service_get_group_count(&service, &(size_t){0U}) ==
                MIZU_STATUS_NOT_INITIALIZED);
    TEST_ASSERT(mizu_command_service_init(&service, NULL, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_command_service_init(&service, &group, 1U) == MIZU_STATUS_OK);
    before = service;
    TEST_ASSERT(mizu_command_service_init(&service, duplicate_groups, 2U) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(service.groups == before.groups);
    TEST_ASSERT(service.initialized == before.initialized);
    TEST_ASSERT(mizu_command_service_init(&service, &duplicate_command_group, 1U) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_command_service_init(&service, &invalid_name_group, 1U) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_command_service_init(&service, &invalid_handler_group, 1U) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_command_service_get_group(&service, 1U, &(const mizu_command_group_t *){0}) ==
                MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(mizu_command_service_get_descriptor_by_id(&service, 1U,
                                                          &(const mizu_command_descriptor_t *){
                                                              0}) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_command_service_get_descriptor_by_id(&service, 99U,
                                                          &(const mizu_command_descriptor_t *){
                                                              0}) == MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(mizu_command_service_get_snapshot(&service, NULL) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_command_service_execute(NULL, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    return 0;
}

int main(void)
{
    int (* const cases[])(void) = {test_registration_and_execution, test_busy_error_and_unknown,
                                   test_registration_errors};

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        const int result = cases[index]();

        if (result != 0) {
            return result;
        }
    }
    return 0;
}
