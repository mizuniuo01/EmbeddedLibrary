#ifndef CRITICAL_SECTION_H
#define CRITICAL_SECTION_H /* 头文件保护 */

#include <stdint.h>
#include <stddef.h>

#include "foundation_status.h"

typedef uintptr_t critical_section_token_t;

typedef foundation_status_t (*critical_section_enter_fn)(void *context,
    critical_section_token_t *token);
typedef foundation_status_t (*critical_section_exit_fn)(void *context,
    critical_section_token_t token);

typedef struct {
    critical_section_enter_fn enter;
    critical_section_exit_fn exit;
    void *context;
} critical_section_port_t;

foundation_status_t critical_section_enter(const critical_section_port_t *port,
    critical_section_token_t *token);
foundation_status_t critical_section_exit(const critical_section_port_t *port,
    critical_section_token_t token);

#endif /* CRITICAL_SECTION_H */
