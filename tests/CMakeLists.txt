add_executable(embeddedlib_smoke unit/smoke.c)
target_link_libraries(embeddedlib_smoke PRIVATE embeddedlib::project_options)
add_test(NAME repository.smoke COMMAND embeddedlib_smoke)
