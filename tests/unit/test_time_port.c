/**
 * @file test_time_port.c
 * @brief 验证单调时间端口包装和 32/64 位时间差语义。
 */

#include "test_support.h"

#include "time_port.h"
#include "time_port_fake.h"

/**
 * @brief 返回测试用 32 位时间。
 * @param context 测试时钟地址。
 * @param value 时间输出地址。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 */
// NOLINTNEXTLINE(constParameterCallback)
// cppcheck-suppress constParameterCallback
static foundation_status_t test_now32(void *context, time_port_tick32_t *value)
{
    *value = *(const time_port_tick32_t *)context;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 写入测试用 32 位 deadline。
 * @param context 测试时钟地址。
 * @param deadline 绝对 deadline。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 */
static foundation_status_t test_sleep32(void *context, time_port_tick32_t deadline)
{
    *(time_port_tick32_t *)context = deadline;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 返回测试用 64 位时间。
 * @param context 测试时钟地址。
 * @param value 时间输出地址。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 */
// NOLINTNEXTLINE(constParameterCallback)
// cppcheck-suppress constParameterCallback
static foundation_status_t test_now64(void *context, time_port_tick64_t *value)
{
    *value = *(const time_port_tick64_t *)context;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 写入测试用 64 位 deadline。
 * @param context 测试时钟地址。
 * @param deadline 绝对 deadline。
 * @retval FOUNDATION_STATUS_OK 写入成功。
 */
static foundation_status_t test_sleep64(void *context, time_port_tick64_t deadline)
{
    *(time_port_tick64_t *)context = deadline;
    return FOUNDATION_STATUS_OK;
}

int main(void)
{
    time_port_tick32_t clock32 = UINT32_MAX - 2U;
    time_port_tick32_t value32 = 0U;
    time_port_tick64_t clock64 = 100U;
    time_port_tick64_t value64 = 0U;
    uint32_t elapsed32 = 0U;
    uint64_t elapsed64 = 0U;
    time_port32_t port32 = {test_now32, test_sleep32, &clock32, 1000U, true};
    time_port64_t port64 = {test_now64, test_sleep64, &clock64, 1000000U, false};
    time_port_fake_t fake;
    const time_port_fake_config_t fake_config = {10U, 20U};
    time_port32_t fake_port32;

    TEST_ASSERT(time_port32_now(&port32, &value32) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(value32 == UINT32_MAX - 2U);
    TEST_ASSERT(time_port32_sleep_until(&port32, 4U) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(clock32 == 4U);
    TEST_ASSERT(time_port32_elapsed(1U, UINT32_MAX - 2U, &elapsed32) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(elapsed32 == 4U);
    TEST_ASSERT(time_port32_now(NULL, &value32) == FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(time_port32_elapsed(1U, 0U, NULL) == FOUNDATION_STATUS_INVALID_ARGUMENT);

    TEST_ASSERT(time_port64_now(&port64, &value64) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(value64 == 100U);
    TEST_ASSERT(time_port64_sleep_until(&port64, 200U) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(clock64 == 200U);
    TEST_ASSERT(time_port64_elapsed(105U, 100U, &elapsed64) == FOUNDATION_STATUS_OK);
    TEST_ASSERT(elapsed64 == 5U);
    TEST_ASSERT(time_port64_sleep_until(NULL, 0U) == FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT(time_port64_elapsed(4U, 5U, &elapsed64) == FOUNDATION_STATUS_OUT_OF_RANGE);

    time_port_fake_init(&fake, &fake_config);
    fake_port32 = time_port_fake_port32(&fake);
    TEST_ASSERT(time_port32_sleep_until(&fake_port32, 11U) == FOUNDATION_STATUS_UNAVAILABLE);
    time_port_fake_advance32(&fake, 1U);
    TEST_ASSERT(time_port32_sleep_until(&fake_port32, 11U) == FOUNDATION_STATUS_OK);
    return 0;
}
