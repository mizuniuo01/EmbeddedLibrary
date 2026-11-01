/* Services 层的显式错误状态记录；不自动接收下层状态或执行错误处理。 */

#include "mizu_status_service.h"

static size_t mizu_status_service_find_slot(const mizu_status_service_t *service,
                                            mizu_status_source_id_t source_id)
{
    for (size_t index = 0U; index < service->record_count; ++index) {
        if (service->records[index].source_id == source_id) {
            return index;
        }
    }
    return service->record_count;
}

static size_t mizu_status_service_find_empty(const mizu_status_service_t *service)
{
    for (size_t index = 0U; index < service->record_count; ++index) {
        if (service->records[index].source_id == 0U) {
            return index;
        }
    }
    return service->record_count;
}

static mizu_status_t mizu_status_service_check(const mizu_status_service_t *service)
{
    if (service == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if (!service->initialized) {
        return MIZU_STATUS_NOT_INITIALIZED;
    }
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_status_service_init(mizu_status_service_t *service,
                                       mizu_status_record_t *records, size_t record_count)
{
    if ((service == NULL) || (records == NULL) || (record_count == 0U)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    for (size_t index = 0U; index < record_count; ++index) {
        records[index] = (mizu_status_record_t){0};
    }
    *service = (mizu_status_service_t){
        .records = records,
        .record_count = record_count,
        .active_count = 0U,
        .initialized = true,
    };
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_status_service_report(mizu_status_service_t *service,
                                         mizu_status_source_id_t source_id, mizu_status_t status)
{
    const mizu_status_t service_status = mizu_status_service_check(service);
    size_t slot;

    if (service_status != MIZU_STATUS_OK) {
        return service_status;
    }
    if ((source_id == 0U) || (status == MIZU_STATUS_OK)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    slot = mizu_status_service_find_slot(service, source_id);
    if (slot == service->record_count) {
        slot = mizu_status_service_find_empty(service);
        if (slot == service->record_count) {
            return MIZU_STATUS_FULL;
        }
        service->active_count++;
    }
    service->records[slot] = (mizu_status_record_t){
        .source_id = source_id,
        .status = status,
    };
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_status_service_clear(mizu_status_service_t *service,
                                        mizu_status_source_id_t source_id)
{
    const mizu_status_t service_status = mizu_status_service_check(service);
    size_t slot;

    if (service_status != MIZU_STATUS_OK) {
        return service_status;
    }
    if (source_id == 0U) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    slot = mizu_status_service_find_slot(service, source_id);
    if (slot == service->record_count) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    service->records[slot] = (mizu_status_record_t){0};
    service->active_count--;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_status_service_count(const mizu_status_service_t *service, size_t *count)
{
    const mizu_status_t service_status = mizu_status_service_check(service);

    if (service_status != MIZU_STATUS_OK) {
        return service_status;
    }
    if (count == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    *count = service->active_count;
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_status_service_get_at(const mizu_status_service_t *service, size_t index,
                                         mizu_status_record_t *record)
{
    const mizu_status_t service_status = mizu_status_service_check(service);

    if (service_status != MIZU_STATUS_OK) {
        return service_status;
    }
    if (record == NULL) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    if ((index >= service->record_count) || (service->records[index].source_id == 0U)) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    *record = service->records[index];
    return MIZU_STATUS_OK;
}

mizu_status_t mizu_status_service_get_by_source(const mizu_status_service_t *service,
                                                mizu_status_source_id_t source_id,
                                                mizu_status_record_t *record)
{
    const mizu_status_t service_status = mizu_status_service_check(service);
    size_t slot;

    if (service_status != MIZU_STATUS_OK) {
        return service_status;
    }
    if ((source_id == 0U) || (record == NULL)) {
        return MIZU_STATUS_INVALID_ARGUMENT;
    }
    slot = mizu_status_service_find_slot(service, source_id);
    if (slot == service->record_count) {
        return MIZU_STATUS_OUT_OF_RANGE;
    }
    *record = service->records[slot];
    return MIZU_STATUS_OK;
}
