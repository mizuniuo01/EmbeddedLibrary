/**
 * @file smoke.c
 * @brief 验证 P0 主机构建和 CTest 链路的最小测试。
 */

/**
 * @brief 验证最小 C11 测试程序可以运行。
 * @return 测试通过时返回 0。
 */
int main(void)
{
    _Static_assert(1, "C11 smoke test must compile");
    return 0;
}
