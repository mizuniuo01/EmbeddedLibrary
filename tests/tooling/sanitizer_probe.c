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

int main(void)
{
    return 0;
}
