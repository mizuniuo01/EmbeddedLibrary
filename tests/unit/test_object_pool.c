/**
 * @file test_object_pool.c
 * @brief 验证有界容器正常路径、边界和失败原子性。
 */
#include "test_support.h"
#include <string.h>
#include <stdint.h>
#include "object_pool.h"

/**
 * @brief 执行容器确定性测试。
 * @return 全部断言成立时返回 0，否则返回 1。
 */
int main(void)
{
    object_pool_t pool = {0};
    uint32_t storage[3] = {1U, 2U, 3U};
    uint8_t states[3] = {9U, 9U, 9U};
    object_pool_config_t config = {.storage = storage,
        .storage_size = sizeof(storage),
        .states = states,
        .states_size = sizeof(states),
        .slot_size = sizeof(storage[0]),
        .capacity = 3U,
        .alignment = _Alignof(uint32_t)};
    void *first = NULL;
    void *second = NULL;
    void *third = NULL;
    void *unchanged;
    size_t index = 99U;
    size_t count;
    TEST_ASSERT_STATUS(object_pool_available(&pool, &index), FOUNDATION_STATUS_NOT_INITIALIZED);
    TEST_ASSERT(index == 99U);
    config.capacity = SIZE_MAX;
    TEST_ASSERT_STATUS(object_pool_init(&pool, &config), FOUNDATION_STATUS_OVERFLOW);
    TEST_ASSERT(states[0] == 9U);
    config.capacity = 3U;
    config.storage_size = 1U;
    TEST_ASSERT_STATUS(object_pool_init(&pool, &config), FOUNDATION_STATUS_BUFFER_TOO_SMALL);
    config.storage_size = sizeof(storage);
    config.storage = (uint8_t *)storage + 1U;
    TEST_ASSERT_STATUS(object_pool_init(&pool, &config), FOUNDATION_STATUS_INVALID_ARGUMENT);
    config.storage = storage;
    TEST_ASSERT_STATUS(object_pool_init(&pool, &config), FOUNDATION_STATUS_OK);
    TEST_ASSERT(storage[0] == 1U && states[0] == 0U);
    TEST_ASSERT_STATUS(object_pool_acquire(&pool, &first, &index), FOUNDATION_STATUS_OK);
    TEST_ASSERT(index == 0U && first == &storage[0]);
    TEST_ASSERT_STATUS(object_pool_init(&pool, &config), FOUNDATION_STATUS_BUSY);
    TEST_ASSERT_STATUS(object_pool_acquire(&pool, &second, &index), FOUNDATION_STATUS_OK);
    TEST_ASSERT(index == 1U);
    TEST_ASSERT_STATUS(object_pool_acquire(&pool, &third, &index), FOUNDATION_STATUS_OK);
    TEST_ASSERT(index == 2U);
    unchanged = first;
    index = 99U;
    TEST_ASSERT_STATUS(object_pool_acquire(&pool, &first, &index), FOUNDATION_STATUS_FULL);
    TEST_ASSERT(first == unchanged && index == 99U);
    TEST_ASSERT_STATUS(object_pool_lowest_free_index(&pool, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U);
    TEST_ASSERT_STATUS(object_pool_release(&pool, (uint8_t *)first + 1U),
        FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(object_pool_release(&pool, &count), FOUNDATION_STATUS_INVALID_ARGUMENT);
    TEST_ASSERT_STATUS(object_pool_release(&pool, second), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(object_pool_release(&pool, second), FOUNDATION_STATUS_INVALID_STATE);
    TEST_ASSERT_STATUS(object_pool_lowest_free_index(&pool, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 1U);
    TEST_ASSERT_STATUS(object_pool_acquire(&pool, &second, &index), FOUNDATION_STATUS_OK);
    TEST_ASSERT(index == 1U);
    pool.exhausted_count = SIZE_MAX;
    TEST_ASSERT_STATUS(object_pool_acquire(&pool, &unchanged, &index), FOUNDATION_STATUS_FULL);
    TEST_ASSERT(pool.exhausted_count == SIZE_MAX);
    TEST_ASSERT_STATUS(object_pool_release(&pool, first), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(object_pool_release(&pool, second), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(object_pool_release(&pool, third), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(object_pool_available(&pool, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U);
    TEST_ASSERT_STATUS(object_pool_capacity(&pool, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == 3U);
    TEST_ASSERT_STATUS(object_pool_exhausted_count(&pool, &count), FOUNDATION_STATUS_OK);
    TEST_ASSERT(count == SIZE_MAX);
    TEST_ASSERT_STATUS(object_pool_init(&pool, &config), FOUNDATION_STATUS_OK);
    TEST_ASSERT(pool.exhausted_count == 0U);
    config.capacity = 1U;
    TEST_ASSERT_STATUS(object_pool_init(&pool, &config), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(object_pool_acquire(&pool, &first, &index), FOUNDATION_STATUS_OK);
    TEST_ASSERT_STATUS(object_pool_acquire(&pool, &second, &index), FOUNDATION_STATUS_FULL);

    return 0;
}
