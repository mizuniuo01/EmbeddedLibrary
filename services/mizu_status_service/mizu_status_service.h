#ifndef MIZU_STATUS_SERVICE_H
#define MIZU_STATUS_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mizu_status.h"

typedef uint16_t mizu_status_source_id_t;

typedef struct {
    mizu_status_source_id_t source_id;
    mizu_status_t status;
} mizu_status_record_t;

/* 调用方拥有的固定记录数组由服务借用；服务对象只能通过 API 操作。 */
typedef struct {
    mizu_status_record_t *records;
    size_t record_count;
    size_t active_count;
    bool initialized;
} mizu_status_service_t;

/**
 * mizu_status_service_init - 绑定并清空调用方提供的固定记录数组。
 * @service: 调用方拥有的服务对象，不要求预先清零。
 * @records: 调用方长期持有的记录数组。
 * @record_count: 记录数组的实际元素数，必须大于零。
 *
 * 只在装配阶段调用一次；服务不分配、不释放、不隐藏创建存储。
 * 服务使用 source_id 为零表示空槽；记录数组必须在服务生命周期内保持有效，且不得与
 * service 对象重叠。返回值：成功返回 MIZU_STATUS_OK；任一指针为空或 record_count 为零时
 * 返回 MIZU_STATUS_INVALID_ARGUMENT。失败时服务对象和记录数组保持不变。仅允许在装配阶段
 * 调用一次；所有接口均要求调用方在同一串行执行上下文中调用，不提供并发同步或 ISR 安全保证。
 */
mizu_status_t mizu_status_service_init(mizu_status_service_t *service,
                                       mizu_status_record_t *records, size_t record_count);

/**
 * mizu_status_service_report - 保存一个需要顶层输出的非成功状态。
 * @service: 已初始化的服务对象。
 * @source_id: 非零状态来源 ID。
 * @status: 非 MIZU_STATUS_OK 的状态值。
 *
 * 同一来源更新原有槽位；新来源占用第一个空槽；没有空槽时返回 MIZU_STATUS_FULL。
 * 返回值：服务为空返回 MIZU_STATUS_INVALID_ARGUMENT，未初始化返回 MIZU_STATUS_NOT_INITIALIZED，
 * source_id 为零或 status 为 MIZU_STATUS_OK 返回 MIZU_STATUS_INVALID_ARGUMENT，存储已满返回
 * MIZU_STATUS_FULL，成功返回 MIZU_STATUS_OK。函数只保存状态，不执行错误处理、恢复或回调；
 * 调用方负责保证串行调用，不得从 ISR 或并发上下文调用。
 */
mizu_status_t mizu_status_service_report(mizu_status_service_t *service,
                                         mizu_status_source_id_t source_id, mizu_status_t status);

/**
 * mizu_status_service_clear - 清除指定来源的当前错误记录。
 * @service: 已初始化的服务对象。
 * @source_id: 非零状态来源 ID。
 * 返回值：服务为空或 source_id 为零返回 MIZU_STATUS_INVALID_ARGUMENT，未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED，来源不存在返回 MIZU_STATUS_OUT_OF_RANGE，成功返回
 * MIZU_STATUS_OK。调用方负责保证串行调用，不得从 ISR 或并发上下文调用。
 */
mizu_status_t mizu_status_service_clear(mizu_status_service_t *service,
                                        mizu_status_source_id_t source_id);

/**
 * mizu_status_service_count - 获取当前有效错误记录数量。
 * @service: 已初始化的服务对象。
 * @count: 接收数量的输出地址。
 * 返回值：服务为空或 count 为空返回 MIZU_STATUS_INVALID_ARGUMENT，未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED，成功返回 MIZU_STATUS_OK。调用方负责保证串行调用，不得从 ISR
 * 或并发上下文调用。
 */
mizu_status_t mizu_status_service_count(const mizu_status_service_t *service, size_t *count);

/**
 * mizu_status_service_get_at - 按槽位顺序复制一条错误记录。
 * @service: 已初始化的服务对象。
 * @index: 记录数组索引。
 * @record: 接收记录副本的输出地址。
 *
 * 查询不消费记录；调用方不得保存内部数组地址。
 * 返回值：服务为空或 record 为空返回 MIZU_STATUS_INVALID_ARGUMENT，未初始化返回
 * MIZU_STATUS_NOT_INITIALIZED，索引越界或槽位为空返回 MIZU_STATUS_OUT_OF_RANGE，成功返回
 * MIZU_STATUS_OK。调用方负责保证串行调用，不得从 ISR 或并发上下文调用。
 */
mizu_status_t mizu_status_service_get_at(const mizu_status_service_t *service, size_t index,
                                         mizu_status_record_t *record);

/**
 * mizu_status_service_get_by_source - 按来源复制当前错误记录。
 * @service: 已初始化的服务对象。
 * @source_id: 非零状态来源 ID。
 * @record: 接收记录副本的输出地址。
 * 返回值：服务为空、record 为空或 source_id 为零返回 MIZU_STATUS_INVALID_ARGUMENT，未初始化
 * 返回 MIZU_STATUS_NOT_INITIALIZED，来源不存在返回 MIZU_STATUS_OUT_OF_RANGE，成功返回
 * MIZU_STATUS_OK。调用方负责保证串行调用，不得从 ISR 或并发上下文调用。
 */
mizu_status_t mizu_status_service_get_by_source(const mizu_status_service_t *service,
                                                mizu_status_source_id_t source_id,
                                                mizu_status_record_t *record);

#endif /* MIZU_STATUS_SERVICE_H */
