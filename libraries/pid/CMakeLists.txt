add_library(pid STATIC src/pid.c)
target_include_directories(pid PUBLIC include)
target_link_libraries(pid PUBLIC foundation_status numeric PRIVATE embeddedlib::project_options)
target_link_libraries(pid PRIVATE m)
