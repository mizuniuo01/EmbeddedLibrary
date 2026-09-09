/**
 * @file critical_section.c
 * @brief 临界区端口的参数和函数表校验包装。
 */

#include "critical_section.h"

/**
 * @brief 进入注入的临界区并保存恢复 token。
 * @param port 临界区端口。
 * @param token 用于接收恢复 token 的输出地址。
 * @retval FOUNDATION_STATUS_OK 进入成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 参数为空或函数缺失。
 */
foundation_status_t critical_section_enter(const critical_section_port_t *port,
    critical_section_token_t *token)
{
    if ((port == NULL) || (token == NULL) || (port->enter == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    return port->enter(port->context, token);
}

/**
 * @brief 使用 token 退出临界区并恢复进入前状态。
 * @param port 临界区端口。
 * @param token 进入操作返回的恢复 token。
 * @retval FOUNDATION_STATUS_OK 退出成功。
 * @retval FOUNDATION_STATUS_INVALID_ARGUMENT 端口为空或函数缺失。
 */
foundation_status_t critical_section_exit(const critical_section_port_t *port,
    critical_section_token_t token)
{
    if ((port == NULL) || (port->exit == NULL)) {
        return FOUNDATION_STATUS_INVALID_ARGUMENT;
    }
    return port->exit(port->context, token);
}
