add_library(time_port STATIC src/time_port.c)
target_include_directories(time_port PUBLIC include)
target_link_libraries(time_port PUBLIC foundation_status tick32 PRIVATE embeddedlib::project_options)
