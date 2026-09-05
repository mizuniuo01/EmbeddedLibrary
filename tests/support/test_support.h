#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H /* 头文件保护 */

#include <stdio.h>

/* 测试失败报告使用固定格式和受控诊断文本。 */
// NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            (void)fprintf(stderr, "assertion failed: %s:%d: %s\n", __FILE__, __LINE__, \
                #condition); \
            return 1; \
        } \
    } while (0)

#define TEST_ASSERT_STATUS(actual, expected) TEST_ASSERT((actual) == (expected))
// NOLINTEND(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)

#endif /* TEST_SUPPORT_H */
