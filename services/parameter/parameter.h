#ifndef MIZU_PARAMETER_SERVICE_H
#define MIZU_PARAMETER_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mizu_status.h"

typedef uint16_t mizu_parameter_id_t;
typedef int32_t mizu_parameter_step_count_t;

typedef struct {
    mizu_parameter_id_t id;
    mizu_parameter_step_count_t step_count;
} mizu_parameter_adjust_request_t;

typedef enum {
    MIZU_PARAMETER_VALUE_FLOAT = 0,
    MIZU_PARAMETER_VALUE_INT32,
    MIZU_PARAMETER_VALUE_UINT32,
    MIZU_PARAMETER_VALUE_BOOL
} mizu_parameter_value_type_t;

typedef struct {
    mizu_parameter_value_type_t type;
    union {
        float float_value;
        int32_t int32_value;
        uint32_t uint32_value;
        bool bool_value;
    } data;
} mizu_parameter_value_t;

typedef mizu_status_t (*mizu_parameter_get_t)(void *context, mizu_parameter_value_t *value);
typedef mizu_status_t (*mizu_parameter_set_t)(void *context, const mizu_parameter_value_t *value);

typedef struct {
    mizu_parameter_id_t id;
    const char *name;
    const char *unit;
    mizu_parameter_value_type_t type;
    mizu_parameter_value_t minimum;
    mizu_parameter_value_t maximum;
    mizu_parameter_value_t step;
    bool writable;
    mizu_parameter_get_t get;
    mizu_parameter_set_t set;
    void *context;
} mizu_parameter_descriptor_t;

typedef struct {
    uint16_t id;
    const char *name;
    const mizu_parameter_descriptor_t *parameters;
    size_t parameter_count;
} mizu_parameter_group_t;

/* 调用方拥有的服务实例；注册表和回调上下文只借用，不得直接修改。 */
typedef struct {
    const mizu_parameter_group_t *groups;
    size_t group_count;
    size_t parameter_count;
    bool initialized;
} mizu_parameter_service_t;

/**
 * mizu_parameter_service_init - 验证并绑定静态参数注册表。
 * @service: 调用方拥有的服务对象，不要求预先清零。
 * @groups: 调用方长期持有的静态分组数组。
 * @group_count: @groups 的实际元素数，初始化后不可更换。
 *
 * 只在装配阶段调用一次；函数读取每个 getter 的当前值并验证注册表。
 * 返回值：成功返回 MIZU_STATUS_OK；结构参数或重复 ID 错误返回
 * MIZU_STATUS_INVALID_ARGUMENT；当前值或数值元数据非法返回 MIZU_STATUS_INVALID_DATA；
 * 回调错误原样传播。失败时服务对象保持不变。
 */
mizu_status_t mizu_parameter_service_init(mizu_parameter_service_t *service,
                                          const mizu_parameter_group_t *groups, size_t group_count);

/**
 * mizu_parameter_service_get_group_count - 获取已注册分组数量。
 * @service: 已初始化的服务对象。
 * @count: 接收数量的输出地址。
 */
mizu_status_t mizu_parameter_service_get_group_count(const mizu_parameter_service_t *service,
                                                     size_t *count);

/**
 * mizu_parameter_service_get_group - 获取分组的只读借用视图。
 * @service: 已初始化的服务对象。
 * @group_index: 分组数组索引。
 * @group: 接收只读分组指针的输出地址。
 */
mizu_status_t mizu_parameter_service_get_group(const mizu_parameter_service_t *service,
                                               size_t group_index,
                                               const mizu_parameter_group_t **group);

/**
 * mizu_parameter_service_get_by_id - 按稳定 ID 读取当前参数值。
 * @service: 已初始化的服务对象。
 * @parameter_id: 已注册的参数 ID。
 * @value: 接收值的输出地址。
 */
mizu_status_t mizu_parameter_service_get_by_id(const mizu_parameter_service_t *service,
                                               mizu_parameter_id_t parameter_id,
                                               mizu_parameter_value_t *value);

/**
 * mizu_parameter_service_set_by_id - 按稳定 ID 校验并提交参数值。
 * @service: 已初始化的服务对象。
 * @parameter_id: 已注册的参数 ID。
 * @value: 待提交的参数值。
 */
mizu_status_t mizu_parameter_service_set_by_id(const mizu_parameter_service_t *service,
                                               mizu_parameter_id_t parameter_id,
                                               const mizu_parameter_value_t *value);

/**
 * mizu_parameter_service_adjust_by_id - 按注册步长调整参数并在边界钳制。
 * @service: 已初始化的服务对象。
 * @parameter_id: 已注册的参数 ID。
 * @step_count: 有符号调整步数；bool 使用正/负表示 true/false。
 */
mizu_status_t mizu_parameter_service_adjust_by_id(const mizu_parameter_service_t *service,
                                                  const mizu_parameter_adjust_request_t *request);

#endif /* MIZU_PARAMETER_SERVICE_H */
