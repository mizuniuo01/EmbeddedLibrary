add_executable(embeddedlib_smoke unit/smoke.c)
target_link_libraries(embeddedlib_smoke PRIVATE embeddedlib::project_options)
add_test(NAME repository.smoke COMMAND embeddedlib_smoke)

if(EMBEDDEDLIB_ENABLE_SANITIZERS)
    add_executable(embeddedlib_sanitizer_probe tooling/sanitizer_probe.c)
    target_link_libraries(embeddedlib_sanitizer_probe PRIVATE embeddedlib::project_options)
    add_test(NAME repository.sanitizer_probe COMMAND embeddedlib_sanitizer_probe)
endif()
