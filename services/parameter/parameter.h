#ifndef MIZU_PARAMETER_SERVICE_H
#define MIZU_PARAMETER_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mizu_status.h"

#define MIZU_PARAMETER_MAX_DECIMALS 4U

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
    uint8_t decimals;
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
 * @service: 调用方拥有且首次调用前已零初始化的服务对象。
 * @groups: 调用方长期持有的静态分组数组。
 * @group_count: @groups 的实际元素数，初始化后不可更换。
 *
 * 只在装配阶段调用一次；函数读取每个 getter 的当前值并验证注册表。
 * 已初始化对象再次调用返回 MIZU_STATUS_INVALID_STATE；成功返回 MIZU_STATUS_OK；结构参数或重复 ID 错误返回
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
 * mizu_parameter_service_get_descriptor_by_id - 获取参数的只读描述符。
 * @service: 已初始化的参数服务。
 * @parameter_id: 已注册的参数 ID。
 * @descriptor: 接收只读借用指针的输出地址。
 *
 * 描述符及其文本由注册表所有者持有，在服务生命周期内保持有效。返回值：成功返回
 * MIZU_STATUS_OK；空指针返回 MIZU_STATUS_INVALID_ARGUMENT；服务未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED；ID 未注册返回 MIZU_STATUS_OUT_OF_RANGE。
 */
mizu_status_t
mizu_parameter_service_get_descriptor_by_id(const mizu_parameter_service_t *service,
                                            mizu_parameter_id_t parameter_id,
                                            const mizu_parameter_descriptor_t **descriptor);

/**
 * mizu_parameter_adjust_value - 按指定步长计算一个新的参数值。
 * @descriptor: 已通过参数服务验证的参数描述符。
 * @value: 当前强类型值。
 * @step: 正的强类型运行期步长。
 * @step_count: 有符号调整次数；bool 使用正负值选择 true 或 false。
 * @adjusted: 接收钳制后新值的输出地址，允许与 value 指向同一对象。
 *
 * 本函数不读取或写入参数所有者，不保存菜单草稿。中间计算超出类型或参数范围时安全钳制。
 * 返回值：成功返回 MIZU_STATUS_OK；空指针、类型或步长非法返回
 * MIZU_STATUS_INVALID_ARGUMENT；描述符为只读返回 MIZU_STATUS_INVALID_STATE；当前值越界返回
 * MIZU_STATUS_OUT_OF_RANGE；数值非有限返回 MIZU_STATUS_INVALID_DATA；失败时 adjusted 保持不变。
 */
mizu_status_t mizu_parameter_adjust_value(const mizu_parameter_descriptor_t *descriptor,
                                          const mizu_parameter_value_t *value,
                                          const mizu_parameter_value_t *step,
                                          mizu_parameter_step_count_t step_count,
                                          mizu_parameter_value_t *adjusted);

/**
 * mizu_parameter_scale_step - 将运行期步长缩放一个十倍档。
 * @descriptor: 已通过参数服务验证的参数描述符。
 * @step: 当前强类型运行期步长。
 * @direction: -1 缩小，0 保持，1 放大。
 * @scaled: 接收边界内新步长的输出地址，允许与 step 指向同一对象。
 *
 * 浮点下限由 decimals 推导，整数下限为 1，上限为参数量程；传入步长必须处于该合法范围。
 * bool 和零量程参数不支持缩放。
 * 返回值：成功返回 MIZU_STATUS_OK；空指针、类型、步长或 direction 非法返回
 * MIZU_STATUS_INVALID_ARGUMENT；参数不可缩放返回 MIZU_STATUS_INVALID_STATE；数值非有限返回
 * MIZU_STATUS_INVALID_DATA；失败时 scaled 保持不变。
 */
mizu_status_t mizu_parameter_scale_step(const mizu_parameter_descriptor_t *descriptor,
                                        const mizu_parameter_value_t *step, int32_t direction,
                                        mizu_parameter_value_t *scaled);

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
 * @request: 包含已注册参数 ID 和有符号调整步数的请求；bool 使用正/负表示 true/false。
 *
 * 返回值：成功返回 MIZU_STATUS_OK；空指针或参数组合非法返回 MIZU_STATUS_INVALID_ARGUMENT；
 * 服务未初始化返回 MIZU_STATUS_NOT_INITIALIZED；参数不可写返回 MIZU_STATUS_INVALID_STATE；
 * 当前值越界或参数 ID 未注册时返回相应错误。
 */
mizu_status_t mizu_parameter_service_adjust_by_id(const mizu_parameter_service_t *service,
                                                  const mizu_parameter_adjust_request_t *request);

#endif /* MIZU_PARAMETER_SERVICE_H */
