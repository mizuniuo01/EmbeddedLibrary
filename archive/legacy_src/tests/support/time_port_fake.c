/**
 * @file time_port_fake.c
 * @brief 测试专用的手动单调时间替身。
 */

#include "time_port_fake.h"

/**
 * @brief 读取 fake 的 32 位时间。
 * @param context fake 状态。
 * @param value 时间输出地址。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 */
static foundation_status_t fake_now32(void *context, time_port_tick32_t *value)
{
    const time_port_fake_t *fake = context;
    *value = fake->now32;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 尝试推进 fake 的 32 位 deadline。
 * @param context fake 状态。
 * @param deadline 绝对 deadline。
 * @retval FOUNDATION_STATUS_OK 已到期。
 * @retval FOUNDATION_STATUS_UNAVAILABLE 尚未到期，fake 不执行真实等待。
 */
static foundation_status_t fake_sleep32(void *context, time_port_tick32_t deadline)
{
    // NOLINTNEXTLINE(constParameterPointer)
    time_port_fake_t *fake = context;
    fake->sleep32_count++;
    if ((int32_t)(fake->now32 - deadline) < 0) {
        return FOUNDATION_STATUS_UNAVAILABLE;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 读取 fake 的 64 位时间。
 * @param context fake 状态。
 * @param value 时间输出地址。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 */
static foundation_status_t fake_now64(void *context, time_port_tick64_t *value)
{
    const time_port_fake_t *fake = context;
    *value = fake->now64;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 尝试推进 fake 的 64 位 deadline。
 * @param context fake 状态。
 * @param deadline 绝对 deadline。
 * @retval FOUNDATION_STATUS_OK 已到期。
 * @retval FOUNDATION_STATUS_UNAVAILABLE 尚未到期，fake 不执行真实等待。
 */
static foundation_status_t fake_sleep64(void *context, time_port_tick64_t deadline)
{
    time_port_fake_t *fake = context;
    fake->sleep64_count++;
    if (fake->now64 < deadline) {
        return FOUNDATION_STATUS_UNAVAILABLE;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 初始化手动时间 fake。
 * @param fake fake 状态。
 * @param config 初始时间配置。
 */
void time_port_fake_init(time_port_fake_t *fake, const time_port_fake_config_t *config)
{
    fake->now32 = config->initial32;
    fake->now64 = config->initial64;
    fake->sleep32_count = 0U;
    fake->sleep64_count = 0U;
}

/**
 * @brief 创建 fake 的 32 位端口视图。
 * @param fake fake 状态。
 * @return fake 的 32 位时间端口。
 */
time_port32_t time_port_fake_port32(time_port_fake_t *fake)
{
    time_port32_t port = {fake_now32, fake_sleep32, fake, 1000U, false};
    return port;
}

/**
 * @brief 创建 fake 的 64 位端口视图。
 * @param fake fake 状态。
 * @return fake 的 64 位时间端口。
 */
time_port64_t time_port_fake_port64(time_port_fake_t *fake)
{
    time_port64_t port = {fake_now64, fake_sleep64, fake, 1000000U, false};
    return port;
}

/**
 * @brief 手动推进 fake 的 32 位时间。
 * @param fake fake 状态。
 * @param duration 推进的 tick 数。
 */
void time_port_fake_advance32(time_port_fake_t *fake, uint32_t duration)
{
    fake->now32 += duration;
}

/**
 * @brief 手动推进 fake 的 64 位时间。
 * @param fake fake 状态。
 * @param duration 推进的 tick 数。
 */
void time_port_fake_advance64(time_port_fake_t *fake, uint64_t duration)
{
    fake->now64 += duration;
}
