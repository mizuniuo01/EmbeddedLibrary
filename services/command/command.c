/* Services 层的静态命令注册和同步动作路由；不解析协议、不依赖菜单。 */

#include "command.h"

#include <stddef.h>

#define MIZU_COMMAND_MAX_GROUPS 16U
#define MIZU_COMMAND_MAX_COMMANDS 64U

static const mizu_command_descriptor_t *mizu_command_find(const mizu_command_service_t *service,
                                                          mizu_command_id_t command_id)
{
    for (size_t group_index = 0U; group_index < service->group_count; ++group_index) {
        const mizu_command_group_t *group = &service->groups[group_index];

        for (size_t command_index = 0U; command_index < group->command_count; ++command_index) {
            if (group->commands[command_index].id == command_id) {
                return &group->commands[command_index];
            }
        }
    }
    return NULL;
}

typedef struct {
    const mizu_command_group_t *groups;
    size_t group_index;
    mizu_command_group_id_t id;
} mizu_command_group_search_t;

static bool mizu_command_group_id_exists(const mizu_command_group_search_t *search)
{
    for (size_t index = 0U; index < search->group_index; ++index) {
        if (search->groups[index].id == search->id) {
            return true;
        }
    }
    return false;
}

typedef struct {
    const mizu_command_group_t *groups;
    size_t group_index;
    size_t command_index;
    mizu_command_id_t id;
} mizu_command_search_t;

static bool mizu_command_id_exists(const mizu_command_search_t *search)
{
    for (size_t earlier_group = 0U; earlier_group <= search->group_index; ++earlier_group) {
        size_t limit = search->groups[earlier_group].command_count;

        if (earlier_group == search->group_index) {
            limit = search->command_index;
        }
        for (size_t earlier_command = 0U; earlier_command < limit; ++earlier_command) {
            if (search->groups[earlier_group].commands[earlier_command].id == search->id) {
                return true;
            }
        }
    }
    return false;
}

mizu_status_t mizu_command_service_init(mizu_command_service_t *service,
                                        const mizu_command_group_t *groups, size_t group_count)
{
    size_t total_count = 0U;

    if (service == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (service->initialized) {
        return MIZU_STATUS_INVALID_STATE;
    }
    if ((groups == NULL) || (group_count == 0U)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (group_count > MIZU_COMMAND_MAX_GROUPS) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    for (size_t group_index = 0U; group_index < group_count; ++group_index) {
        const mizu_command_group_t *group = &groups[group_index];

        if ((group->name == NULL) || (group->name[0] == '\0') || (group->commands == NULL) ||
            (group->command_count == 0U)) {
            return MIZU_STATUS_INVALID_ARGUMENT;
        }
        const mizu_command_group_search_t group_search = {groups, group_index, group->id};

        if (mizu_command_group_id_exists(&group_search)) {
            return MIZU_STATUS_INVALID_ARGUMENT;
        }
        if (group->command_count > (MIZU_COMMAND_MAX_COMMANDS - total_count)) {
            return MIZU_STATUS_OUT_OF_RANGE;
        }
        for (size_t command_index = 0U; command_index < group->command_count; ++command_index) {
            const mizu_command_descriptor_t *command = &group->commands[command_index];

            if ((command->name == NULL) || (command->name[0] == '\0') ||
                (command->handler == NULL)) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
            const mizu_command_search_t command_search = {groups, group_index, command_index,
                                                          command->id};

            if (mizu_command_id_exists(&command_search)) {
                return MIZU_STATUS_INVALID_ARGUMENT;
            }
        }
        total_count += group->command_count;
    }

    *service = (mizu_command_service_t){
        .groups = groups,
        .group_count = group_count,
        .command_count = total_count,
        .snapshot =
            {
                .command_id = 0U,
                .status = MIZU_STATUS_OK,
                .sequence = 0U,
                .has_result = false,
            },
        .initialized = true,
    };
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_command_service_get_group_count(const mizu_command_service_t *service,
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

mizu_status_t mizu_command_service_get_group(const mizu_command_service_t *service,
                                             size_t group_index, const mizu_command_group_t **group)
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
mizu_command_service_get_descriptor_by_id(const mizu_command_service_t *service,
                                          mizu_command_id_t command_id,
                                          const mizu_command_descriptor_t **descriptor)
{
    const mizu_command_descriptor_t *found;

    if ((service == NULL) || (descriptor == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!service->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    found = mizu_command_find(service, command_id);
    if (found == NULL) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    *descriptor = found;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_command_service_execute(mizu_command_service_t *service,
                                           mizu_command_id_t command_id)
{
    const mizu_command_descriptor_t *command;
    mizu_status_t status;

    if (service == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!service->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    command = mizu_command_find(service, command_id);
    if (command == NULL) {
        status = MIZU_STATUS_OUT_OF_RANGE;
    } else {
        status = command->handler(command->context);
    }
    service->snapshot.command_id = command_id;
    service->snapshot.status = status;
    service->snapshot.sequence++;
    service->snapshot.has_result = true;
    return status;
}

mizu_status_t mizu_command_service_get_snapshot(const mizu_command_service_t *service,
                                                mizu_command_snapshot_t *snapshot)
{
    if ((service == NULL) || (snapshot == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!service->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    *snapshot = service->snapshot;
    return MIZU_STATUS_OK;
}
