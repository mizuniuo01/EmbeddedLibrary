set(EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/support")

add_executable(embeddedlib_smoke unit/smoke.c)
target_link_libraries(embeddedlib_smoke PRIVATE embeddedlib::project_options)
target_include_directories(embeddedlib_smoke PRIVATE "${EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR}")
add_test(NAME repository.smoke COMMAND embeddedlib_smoke)

add_executable(test_tick32 unit/test_tick32.c)
target_link_libraries(test_tick32 PRIVATE tick32)
target_include_directories(test_tick32 PRIVATE "${EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR}")
add_test(NAME tick32.unit COMMAND test_tick32)

add_executable(test_numeric unit/test_numeric.c)
target_link_libraries(test_numeric PRIVATE numeric)
target_include_directories(test_numeric PRIVATE "${EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR}")
add_test(NAME numeric.unit COMMAND test_numeric)

add_executable(test_byte_order unit/test_byte_order.c)
target_link_libraries(test_byte_order PRIVATE byte_order)
target_include_directories(test_byte_order PRIVATE "${EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR}")
add_test(NAME byte_order.unit COMMAND test_byte_order)

add_executable(test_crc unit/test_crc.c)
target_link_libraries(test_crc PRIVATE crc)
target_include_directories(test_crc PRIVATE "${EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR}")
add_test(NAME crc.unit COMMAND test_crc)

add_executable(test_cobs unit/test_cobs.c)
target_link_libraries(test_cobs PRIVATE cobs)
target_include_directories(test_cobs PRIVATE "${EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR}")
add_test(NAME cobs.unit COMMAND test_cobs)

add_executable(test_ieee754_byte_order unit/test_ieee754_byte_order.c)
target_link_libraries(test_ieee754_byte_order PRIVATE ieee754_byte_order)
target_include_directories(test_ieee754_byte_order PRIVATE "${EMBEDDEDLIB_TEST_SUPPORT_INCLUDE_DIR}")
add_test(NAME ieee754_byte_order.unit COMMAND test_ieee754_byte_order)

if(EMBEDDEDLIB_ENABLE_SANITIZERS)
    add_executable(embeddedlib_sanitizer_probe tooling/sanitizer_probe.c)
    target_link_libraries(embeddedlib_sanitizer_probe PRIVATE embeddedlib::project_options)
    add_test(NAME repository.sanitizer_probe COMMAND embeddedlib_sanitizer_probe)
endif()
