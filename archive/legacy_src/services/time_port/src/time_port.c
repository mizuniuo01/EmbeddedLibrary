/**
 * @file time_port.c
 * @brief 单调时间端口的安全包装和时间差计算。
 */

#include "time_port.h"

#include "tick32.h"

/**
 * @brief 校验 32 位时间端口的必需能力和 tick 频率。
 * @param port 时间端口。
 * @retval FOUNDATION_STATUS_OK 端口契约有效。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 端口、必需函数或 tick 频率无效。
 */
foundation_status_t time_port32_validate(const time_port32_t *port)
{
    if ((port == NULL) || (port->now == NULL) || (port->ticks_per_second == 0U)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 校验 64 位时间端口的必需能力和 tick 频率。
 * @param port 时间端口。
 * @retval FOUNDATION_STATUS_OK 端口契约有效。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 端口、必需函数或 tick 频率无效。
 */
foundation_status_t time_port64_validate(const time_port64_t *port)
{
    if ((port == NULL) || (port->now == NULL) || (port->ticks_per_second == 0U)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 读取 32 位单调时间。
 * @param port 时间端口。
 * @param value 用于接收时间值的输出地址。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或端口函数缺失。
 */
foundation_status_t time_port32_now(const time_port32_t *port, time_port_tick32_t *value)
{
    if ((time_port32_validate(port) != FOUNDATION_STATUS_OK) || (value == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    return port->now(port->context, value);
}

/**
 * @brief 休眠至 32 位绝对 deadline。
 * @param port 时间端口。
 * @param deadline 绝对 deadline。
 * @retval FOUNDATION_STATUS_OK 请求完成。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或端口函数缺失。
 */
foundation_status_t time_port32_sleep_until(const time_port32_t *port, time_port_tick32_t deadline)
{
    time_port_tick32_t now;
    bool is_reached;
    foundation_status_t status;

    status = time_port32_now(port, &now);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    status = tick32_deadline_reached(now, deadline, &is_reached);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (is_reached) {
        return FOUNDATION_STATUS_OK;
    }
    if (port->sleep_until == NULL) {
        return FOUNDATION_STATUS_UNSUPPORTED;
    }
    return port->sleep_until(port->context, deadline);
}

/**
 * @brief 计算 32 位 tick 的回绕安全经过时间。
 * @param now 当前时间。
 * @param start 起始时间。
 * @param elapsed 用于接收经过时间的输出地址。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 输出地址为空。
 */
foundation_status_t time_port32_elapsed(time_port_tick32_t now, time_port_tick32_t start,
    uint32_t *elapsed)
{
    return tick32_elapsed(now, start, elapsed);
}

/**
 * @brief 读取 64 位单调时间。
 * @param port 时间端口。
 * @param value 用于接收时间值的输出地址。
 * @retval FOUNDATION_STATUS_OK 读取成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或端口函数缺失。
 */
foundation_status_t time_port64_now(const time_port64_t *port, time_port_tick64_t *value)
{
    if ((time_port64_validate(port) != FOUNDATION_STATUS_OK) || (value == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    return port->now(port->context, value);
}

/**
 * @brief 休眠至 64 位绝对 deadline。
 * @param port 时间端口。
 * @param deadline 绝对 deadline。
 * @retval FOUNDATION_STATUS_OK 请求完成。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或端口函数缺失。
 */
foundation_status_t time_port64_sleep_until(const time_port64_t *port, time_port_tick64_t deadline)
{
    time_port_tick64_t now;
    foundation_status_t status;

    status = time_port64_now(port, &now);
    if (status != FOUNDATION_STATUS_OK) {
        return status;
    }
    if (now >= deadline) {
        return FOUNDATION_STATUS_OK;
    }
    if (port->sleep_until == NULL) {
        return FOUNDATION_STATUS_UNSUPPORTED;
    }
    return port->sleep_until(port->context, deadline);
}

/**
 * @brief 计算 64 位单调时间的经过时间。
 * @param now 当前时间。
 * @param start 起始时间。
 * @param elapsed 用于接收经过时间的输出地址。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 输出地址为空。
 */
foundation_status_t time_port64_elapsed(time_port_tick64_t now, time_port_tick64_t start,
    uint64_t *elapsed)
{
    if (elapsed == NULL) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (now < start) {
        return FOUNDATION_STATUS_OUT_OF_RANGE;
    }
    *elapsed = now - start;
    return FOUNDATION_STATUS_OK;
}
