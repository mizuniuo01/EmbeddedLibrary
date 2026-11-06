/* 验证显式错误上报、来源槽位、清除和只读查询语义。 */

#include "test_support.h"

#include <stddef.h>

#include "mizu_status_service.h"

static int test_registration_and_report(void)
{
    mizu_status_service_t service = {0};
    mizu_status_record_t records[2] = {0};
    mizu_status_record_t record;
    size_t count;

    TEST_ASSERT(mizu_status_service_init(&service, records, 2U) == MIZU_STATUS_OK);
    TEST_ASSERT((records[0].source_id == 0U) && (records[1].source_id == 0U));
    TEST_ASSERT(mizu_status_service_count(&service, &count) == MIZU_STATUS_OK);
    TEST_ASSERT(count == 0U);
    TEST_ASSERT(mizu_status_service_report(&service, 1U, MIZU_STATUS_INTERNAL_ERROR) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(mizu_status_service_report(&service, 2U, MIZU_STATUS_INVALID_DATA) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(mizu_status_service_count(&service, &count) == MIZU_STATUS_OK);
    TEST_ASSERT(count == 2U);
    TEST_ASSERT(mizu_status_service_get_at(&service, 0U, &record) == MIZU_STATUS_OK);
    TEST_ASSERT(record.source_id == 1U && record.status == MIZU_STATUS_INTERNAL_ERROR);
    TEST_ASSERT(mizu_status_service_get_by_source(&service, 2U, &record) == MIZU_STATUS_OK);
    TEST_ASSERT(record.status == MIZU_STATUS_INVALID_DATA);
    return 0;
}

static int test_update_clear_and_full(void)
{
    mizu_status_service_t service = {0};
    mizu_status_record_t records[2] = {0};
    mizu_status_record_t record;
    size_t count;

    TEST_ASSERT(mizu_status_service_init(&service, records, 2U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_status_service_report(&service, 1U, MIZU_STATUS_INTERNAL_ERROR) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(mizu_status_service_report(&service, 1U, MIZU_STATUS_OVERFLOW) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_status_service_count(&service, &count) == MIZU_STATUS_OK);
    TEST_ASSERT(count == 1U);
    TEST_ASSERT(mizu_status_service_get_by_source(&service, 1U, &record) == MIZU_STATUS_OK);
    TEST_ASSERT(record.status == MIZU_STATUS_OVERFLOW);
    TEST_ASSERT(mizu_status_service_report(&service, 2U, MIZU_STATUS_INVALID_STATE) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(mizu_status_service_report(&service, 3U, MIZU_STATUS_INVALID_STATE) ==
                MIZU_STATUS_FULL);
    TEST_ASSERT(mizu_status_service_clear(&service, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_status_service_report(&service, 3U, MIZU_STATUS_INVALID_STATE) ==
                MIZU_STATUS_OK);
    TEST_ASSERT(mizu_status_service_clear(&service, 99U) == MIZU_STATUS_OUT_OF_RANGE);
    return 0;
}

static int test_invalid_and_non_consuming_queries(void)
{
    mizu_status_service_t service = {0};
    mizu_status_record_t records[1] = {0};
    mizu_status_record_t record;
    size_t count;

    TEST_ASSERT(mizu_status_service_init(NULL, records, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_init(&service, NULL, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_init(&service, records, 0U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_report(NULL, 1U, MIZU_STATUS_INTERNAL_ERROR) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_clear(NULL, 1U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_count(NULL, &count) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_get_at(NULL, 0U, &record) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_get_by_source(NULL, 1U, &record) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_count(&service, &count) == MIZU_STATUS_NOT_INITIALIZED);
    TEST_ASSERT(mizu_status_service_init(&service, records, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_status_service_count(&service, NULL) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_report(&service, 1U, MIZU_STATUS_INTERNAL_ERROR) ==
                MIZU_STATUS_OK);
    {
        mizu_status_record_t replacement_records[1] = {0};

        TEST_ASSERT(mizu_status_service_init(&service, replacement_records, 1U) ==
                    MIZU_STATUS_INVALID_STATE);
        TEST_ASSERT(mizu_status_service_get_by_source(&service, 1U, &record) == MIZU_STATUS_OK);
        TEST_ASSERT(record.status == MIZU_STATUS_INTERNAL_ERROR);
    }
    TEST_ASSERT(mizu_status_service_report(&service, 1U, MIZU_STATUS_OK) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_clear(&service, 0U) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_get_at(&service, 0U, NULL) == MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_get_by_source(&service, 1U, NULL) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_report(&service, 0U, MIZU_STATUS_INTERNAL_ERROR) ==
                MIZU_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(mizu_status_service_get_by_source(&service, 1U, &record) == MIZU_STATUS_OK);
    TEST_ASSERT(record.status == MIZU_STATUS_INTERNAL_ERROR);
    TEST_ASSERT(mizu_status_service_get_at(&service, 1U, &record) == MIZU_STATUS_OUT_OF_RANGE);
    TEST_ASSERT(mizu_status_service_clear(&service, 1U) == MIZU_STATUS_OK);
    TEST_ASSERT(mizu_status_service_get_at(&service, 0U, &record) == MIZU_STATUS_OUT_OF_RANGE);
    return 0;
}

int main(void)
{
    int (* const cases[])(void) = {test_registration_and_report, test_update_clear_and_full,
                                   test_invalid_and_non_consuming_queries};

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        const int result = cases[index]();

        if (result != 0) {
            return result;
        }
    }
    return 0;
}
