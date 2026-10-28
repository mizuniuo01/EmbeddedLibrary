set(EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/support")

add_executable(embeddedlib_smoke unit/smoke.c)
target_link_libraries(embeddedlib_smoke PRIVATE embeddedlib::project_options)
target_include_directories(embeddedlib_smoke PRIVATE "${EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR}")
add_test(NAME repository.smoke COMMAND embeddedlib_smoke)

add_executable(test_scheduler unit/test_scheduler.c)
target_link_libraries(test_scheduler PRIVATE scheduler embeddedlib::project_options)
target_include_directories(test_scheduler PRIVATE "${EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR}")
add_test(NAME scheduler.unit COMMAND test_scheduler)

add_executable(test_mizu_fifo unit/test_mizu_fifo.c)
target_link_libraries(test_mizu_fifo PRIVATE mizu_fifo embeddedlib::project_options)
target_include_directories(test_mizu_fifo PRIVATE "${EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR}")
add_test(NAME mizu_fifo.unit COMMAND test_mizu_fifo)

if(EMBEDDEDLIB_ENABLE_SANITIZERS)
    add_executable(embeddedlib_sanitizer_probe tooling/sanitizer_probe.c)
    target_link_libraries(embeddedlib_sanitizer_probe PRIVATE embeddedlib::project_options)
    add_test(NAME repository.sanitizer_probe COMMAND embeddedlib_sanitizer_probe)
endif()
