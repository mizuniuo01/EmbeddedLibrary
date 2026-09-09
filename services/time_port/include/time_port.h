#ifndef TIME_PORT_H
#define TIME_PORT_H /* 头文件保护 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foundation_status.h"

typedef uint32_t time_port_tick32_t;
typedef uint64_t time_port_tick64_t;

typedef struct {
    foundation_status_t (*now)(void *context, time_port_tick32_t *value);
    foundation_status_t (*sleep_until)(void *context, time_port_tick32_t deadline);
    void *context;
    uint32_t ticks_per_second;
    bool callable_from_isr;
} time_port32_t;

typedef struct {
    foundation_status_t (*now)(void *context, time_port_tick64_t *value);
    foundation_status_t (*sleep_until)(void *context, time_port_tick64_t deadline);
    void *context;
    uint32_t ticks_per_second;
    bool callable_from_isr;
} time_port64_t;

/* 端口契约校验 */
foundation_status_t time_port32_validate(const time_port32_t *port);
foundation_status_t time_port64_validate(const time_port64_t *port);

/* 32 位单调时间操作 */
foundation_status_t time_port32_now(const time_port32_t *port, time_port_tick32_t *value);
foundation_status_t time_port32_sleep_until(const time_port32_t *port, time_port_tick32_t deadline);
foundation_status_t time_port32_elapsed(time_port_tick32_t now, time_port_tick32_t start,
    uint32_t *elapsed);

/* 64 位单调时间操作 */
foundation_status_t time_port64_now(const time_port64_t *port, time_port_tick64_t *value);
foundation_status_t time_port64_sleep_until(const time_port64_t *port, time_port_tick64_t deadline);
foundation_status_t time_port64_elapsed(time_port_tick64_t now, time_port_tick64_t start,
    uint64_t *elapsed);

#endif /* TIME_PORT_H */
