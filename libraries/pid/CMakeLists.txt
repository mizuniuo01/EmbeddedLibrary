add_library(pid STATIC pid.c)
target_include_directories(pid PUBLIC .)
target_link_libraries(pid PUBLIC mizu_status PRIVATE embeddedlib::project_options)
target_link_libraries(pid PRIVATE m)
