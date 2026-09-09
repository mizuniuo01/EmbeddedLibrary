/**
 * @file test_critical_section.c
 * @brief 验证临界区端口的 token 传递和参数校验。
 */

#include "test_support.h"

#include "critical_section.h"

typedef struct {
    uint32_t depth;
    critical_section_token_t next_token;
    critical_section_token_t last_token;
} test_context_t;

static foundation_status_t test_enter(void *context, critical_section_token_t *token)
{
    test_context_t *state = context;
    state->depth++;
    state->last_token = state->next_token++;
    *token = state->last_token;
    return FOUNDATION_STATUS_OK;
}

static foundation_status_t test_exit(void *context, critical_section_token_t token)
{
    test_context_t *state = context;
    if ((state->depth == 0U) || (token != state->last_token)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    state->depth--;
    return FOUNDATION_STATUS_OK;
}

int main(void)
{
    test_context_t context = {0U, 1U, 0U};
    critical_section_token_t token = 0U;
    critical_section_port_t port = {test_enter, test_exit, &context};

    TEST_ASSERT(critical_section_enter(&port, &token) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(context.depth == 1U);
    TEST_ASSERT(critical_section_exit(&port, token) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(context.depth == 0U);
    TEST_ASSERT(critical_section_enter(NULL, &token) == FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(critical_section_enter(&port, NULL) == FOUNDATION_STATUS_INVALID_ARGUMENT);
    return 0;
}
