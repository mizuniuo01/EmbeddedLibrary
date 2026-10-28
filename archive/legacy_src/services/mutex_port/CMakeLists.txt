add_library(mutex_port STATIC src/mutex_port.c)
target_include_directories(mutex_port PUBLIC include)
target_link_libraries(mutex_port PUBLIC critical_section foundation_status waiter PRIVATE embeddedlib::project_options)
