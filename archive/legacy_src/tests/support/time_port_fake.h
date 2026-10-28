#ifndef TIME_PORT_FAKE_H
#define TIME_PORT_FAKE_H /* 头文件保护 */

#include "time_port.h"

typedef struct {
    time_port_tick32_t now32;
    time_port_tick64_t now64;
    size_t sleep32_count;
    size_t sleep64_count;
} time_port_fake_t;

typedef struct {
    time_port_tick32_t initial32;
    time_port_tick64_t initial64;
} time_port_fake_config_t;

void time_port_fake_init(time_port_fake_t *fake, const time_port_fake_config_t *config);
time_port32_t time_port_fake_port32(time_port_fake_t *fake);
time_port64_t time_port_fake_port64(time_port_fake_t *fake);
void time_port_fake_advance32(time_port_fake_t *fake, uint32_t duration);
void time_port_fake_advance64(time_port_fake_t *fake, uint64_t duration);

#endif /* TIME_PORT_FAKE_H */
