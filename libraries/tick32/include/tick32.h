#ifndef TICK32_H
#define TICK32_H /* 头文件保护 */

#include <stdbool.h>
#include <stdint.h>

#include "foundation_status.h"

/* tick 回绕安全比较允许的最大时间间隔，单位为 tick。 */
#define TICK32_MAX_INTERVAL_TICKS ((uint32_t)INT32_MAX)

/* 时间差计算 */
foundation_status_t tick32_elapsed(uint32_t now_tick, uint32_t start_tick, uint32_t *elapsed_ticks);

/* 持续时间校验 */
bool tick32_duration_is_valid(uint32_t duration_ticks);

/* 到期判断 */
foundation_status_t tick32_has_elapsed(uint32_t now_tick, uint32_t start_tick,
    uint32_t duration_ticks, bool *has_elapsed);

foundation_status_t tick32_deadline_add(uint32_t now_tick, uint32_t delay_ticks,
    uint32_t *deadline_tick);

foundation_status_t tick32_deadline_reached(uint32_t now_tick, uint32_t deadline_tick,
    bool *is_reached);

#endif /* TICK32_H */
