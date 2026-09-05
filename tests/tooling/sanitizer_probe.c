/**
 * @file sanitizer_probe.c
 * @brief 验证 Sanitizer Preset 确实启用了 ASan 和 UBSan 插桩。
 */
#if !defined(__clang__)
#    error "The sanitizer probe requires Clang"
#endif

#if !__has_feature(address_sanitizer)
#    error "AddressSanitizer instrumentation is not enabled"
#endif

#if !__has_feature(undefined_behavior_sanitizer)
#    error "UndefinedBehaviorSanitizer instrumentation is not enabled"
#endif

/**
 * @brief 验证当前编译目标已启用 AddressSanitizer 和 UBSan。
 * @return 探针成功运行时返回 0。
 */
int main(void)
{
    return 0;
}
