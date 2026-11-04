#ifndef MIZU_COMMAND_SERVICE_H
#define MIZU_COMMAND_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mizu_status.h"

typedef uint16_t mizu_command_id_t;
typedef uint16_t mizu_command_group_id_t;

typedef mizu_status_t (*mizu_command_handler_t)(void *context);

typedef struct {
    mizu_command_id_t id;
    const char *name;
    mizu_command_handler_t handler;
    void *context;
} mizu_command_descriptor_t;

typedef struct {
    mizu_command_group_id_t id;
    const char *name;
    const mizu_command_descriptor_t *commands;
    size_t command_count;
} mizu_command_group_t;

typedef struct {
    mizu_command_id_t command_id;
    mizu_status_t status;
    uint32_t sequence;
    bool has_result;
} mizu_command_snapshot_t;

/* 调用方拥有的服务实例；注册表、名称和 context 只借用，不得直接修改。 */
typedef struct {
    const mizu_command_group_t *groups;
    size_t group_count;
    size_t command_count;
    mizu_command_snapshot_t snapshot;
    bool initialized;
} mizu_command_service_t;

/**
 * mizu_command_service_init - 验证并绑定静态命令注册表。
 * @service: 调用方拥有的服务对象，不要求预先清零。
 * @groups: 调用方长期持有的静态命令分组数组。
 * @group_count: @groups 的实际元素数，初始化后不可更换。
 *
 * 只在装配阶段调用一次；服务只借用注册表、名称、handler 和 context。
 * 只允许在主循环或普通任务上下文调用；服务不提供同步。返回值：成功返回 MIZU_STATUS_OK；
 * 结构参数或重复 ID 返回 MIZU_STATUS_INVALID_ARGUMENT；数量超出固定容量返回
 * MIZU_STATUS_OUT_OF_RANGE；失败时对象保持不变。
 */
mizu_status_t mizu_command_service_init(mizu_command_service_t *service,
                                        const mizu_command_group_t *groups, size_t group_count);

/**
 * mizu_command_service_get_group_count - 获取已注册命令分组数量。
 * @service: 已初始化的服务对象。
 * @count: 接收数量的输出地址。
 *
 * 返回值：成功返回 MIZU_STATUS_OK；空指针返回 MIZU_STATUS_INVALID_ARGUMENT；未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED。
 */
mizu_status_t mizu_command_service_get_group_count(const mizu_command_service_t *service,
                                                   size_t *count);

/**
 * mizu_command_service_get_group - 获取命令分组的只读借用视图。
 * @service: 已初始化的服务对象。
 * @group_index: 分组数组索引。
 * @group: 接收只读分组指针的输出地址。
 *
 * 返回值：成功返回 MIZU_STATUS_OK；空指针返回 MIZU_STATUS_INVALID_ARGUMENT；未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED；索引越界返回 MIZU_STATUS_OUT_OF_RANGE。
 */
mizu_status_t mizu_command_service_get_group(const mizu_command_service_t *service,
                                             size_t group_index,
                                             const mizu_command_group_t **group);

/**
 * mizu_command_service_get_descriptor_by_id - 获取命令的只读描述符。
 * @service: 已初始化的命令服务。
 * @command_id: 已注册的命令 ID。
 * @descriptor: 接收只读借用指针的输出地址。
 *
 * 描述符及其名称由注册表所有者持有，在服务生命周期内保持有效。返回值：成功返回
 * MIZU_STATUS_OK；空指针返回 MIZU_STATUS_INVALID_ARGUMENT；服务未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED；ID 未注册返回 MIZU_STATUS_OUT_OF_RANGE。
 */
mizu_status_t
mizu_command_service_get_descriptor_by_id(const mizu_command_service_t *service,
                                          mizu_command_id_t command_id,
                                          const mizu_command_descriptor_t **descriptor);

/**
 * mizu_command_service_execute - 按稳定 ID 同步执行一次命令。
 * @service: 已初始化的服务对象。
 * @command_id: 已注册或待查询的命令 ID。
 *
 * handler 只调用一次；BUSY 或其他状态原样返回并记录到最近结果快照。函数不阻塞，
 * 不自动轮询，也不依赖 parameter 或 menu 服务。只能串行调用，不支持并发或重入；handler
 * 不得重入同一 command service。
 * 返回值：空指针返回 MIZU_STATUS_INVALID_ARGUMENT；未初始化返回 MIZU_STATUS_NOT_INITIALIZED；
 * 未注册 ID 返回 MIZU_STATUS_OUT_OF_RANGE；已注册命令返回 handler 的原始状态。
 */
mizu_status_t mizu_command_service_execute(mizu_command_service_t *service,
                                           mizu_command_id_t command_id);

/**
 * mizu_command_service_get_snapshot - 获取最近一次命令执行结果。
 * @service: 已初始化的服务对象。
 * @snapshot: 接收结果快照的输出地址。
 *
 * 只能与 execute() 串行调用。返回值：成功返回 MIZU_STATUS_OK；空指针返回
 * MIZU_STATUS_INVALID_ARGUMENT；未初始化返回 MIZU_STATUS_NOT_INITIALIZED。
 */
mizu_status_t mizu_command_service_get_snapshot(const mizu_command_service_t *service,
                                                mizu_command_snapshot_t *snapshot);

#endif /* MIZU_COMMAND_SERVICE_H */
