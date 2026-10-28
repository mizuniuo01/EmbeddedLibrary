add_library(queue_port STATIC src/queue_port.c)
target_include_directories(queue_port PUBLIC include)
target_link_libraries(queue_port PUBLIC critical_section foundation_status waiter PRIVATE embeddedlib::project_options)
