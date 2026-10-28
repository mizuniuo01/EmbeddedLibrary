/**
 * @file tick32.c
 * @brief 32 位单调 tick 的回绕安全时间计算实现。
 */

#include "tick32.h"

/**
 * @brief 计算当前 tick 与起始 tick 的无符号模差值。
 * @param now_tick 当前单调 tick。
 * @param start_tick 起始单调 tick。
 * @param elapsed_ticks 用于接收经过的 tick 数；成功时有效。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 输出参数为空。
 */
foundation_status_t tick32_elapsed(uint32_t now_tick, uint32_t start_tick, uint32_t *elapsed_ticks)
{
    if (!elapsed_ticks) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }

    *elapsed_ticks = now_tick - start_tick;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 检查持续时间是否满足 uint32_t tick 的半范围约束。
 * @param duration_ticks 待检查的 tick 数。
 * @return 不超过 INT32_MAX 时返回 true，否则返回 false。
 */
bool tick32_duration_is_valid(uint32_t duration_ticks)
{
    return duration_ticks <= TICK32_MAX_INTERVAL_TICKS;
}

/**
 * @brief 判断当前 tick 是否已经经过指定持续时间。
 * @param now_tick 当前单调 tick。
 * @param start_tick 起始单调 tick。
 * @param duration_ticks 持续时间，单位为 tick，不能超过 INT32_MAX。
 * @param has_elapsed 用于接收判断结果；成功时有效。
 * @retval FOUNDATION_STATUS_OK 判断成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 输出参数为空。
 * @retval FOUNDATION_STATUS_OUT_OF_RANGE 持续时间超过半范围。
 */
foundation_status_t tick32_has_elapsed(uint32_t now_tick, uint32_t start_tick,
    uint32_t duration_ticks, bool *has_elapsed)
{
    if (!has_elapsed) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!tick32_duration_is_valid(duration_ticks)) {
        return FOUNDATION_STATUS_OUT_OF_RANGE;
    }

    *has_elapsed = (now_tick - start_tick) >= duration_ticks;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 根据当前 tick 和延迟计算回绕安全的绝对 deadline。
 * @param now_tick 当前单调 tick。
 * @param delay_ticks 延迟时间，单位为 tick，不能超过 INT32_MAX。
 * @param deadline_tick 用于接收计算结果；成功时有效。
 * @retval FOUNDATION_STATUS_OK 计算成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 输出参数为空。
 * @retval FOUNDATION_STATUS_OUT_OF_RANGE 延迟超过半范围。
 */
foundation_status_t tick32_deadline_add(uint32_t now_tick, uint32_t delay_ticks,
    uint32_t *deadline_tick)
{
    if (!deadline_tick) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    if (!tick32_duration_is_valid(delay_ticks)) {
        return FOUNDATION_STATUS_OUT_OF_RANGE;
    }

    *deadline_tick = now_tick + delay_ticks;
    return FOUNDATION_STATUS_OK;
}

/**
 * @brief 按半范围规则判断当前 tick 是否到达 deadline。
 * @param now_tick 当前单调 tick。
 * @param deadline_tick 使用相同 tick 时间域计算出的 deadline。
 * @param is_reached 用于接收判断结果；成功时有效。
 * @retval FOUNDATION_STATUS_OK 判断成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 输出参数为空。
 * @retval FOUNDATION_STATUS_OUT_OF_RANGE 两个 tick 相隔恰好半个 uint32_t 范围。
 */
foundation_status_t tick32_deadline_reached(uint32_t now_tick, uint32_t deadline_tick,
    bool *is_reached)
{
    uint32_t deadline_delta;

    if (!is_reached) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }

    deadline_delta = now_tick - deadline_tick;
    if (deadline_delta == (TICK32_MAX_INTERVAL_TICKS + 1U)) {
        return FOUNDATION_STATUS_OUT_OF_RANGE;
    }

    *is_reached = deadline_delta <= TICK32_MAX_INTERVAL_TICKS;
    return FOUNDATION_STATUS_OK;
}
