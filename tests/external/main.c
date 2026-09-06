/**
 * @file main.c
 * @brief 验证组件在仓库外部 CMake 工程中的最小接入。
 */
#include <stddef.h>
#include <stdint.h>

#include "byte_fifo.h"

/**
 * @brief 执行外部接入的最小 FIFO 用例。
 * @return 成功时返回 0，否则返回 1。
 */
int main(void)
{
    uint8_t storage[2];
    uint8_t value = 7U;
    uint8_t output = 0U;
    byte_fifo_t fifo = {0};
    if (byte_fifo_init(&fifo, storage, sizeof(storage)) != FOUNDATION_STATUS_OK) {
        return 1;
    }
    if (byte_fifo_push(&fifo, value) != FOUNDATION_STATUS_OK) {
        return 1;
    }
    if (byte_fifo_pop(&fifo, &output) != FOUNDATION_STATUS_OK) {
        return 1;
    }
    return (output == value) ? 0 : 1;
}
