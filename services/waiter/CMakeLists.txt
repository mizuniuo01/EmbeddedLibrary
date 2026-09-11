add_library(waiter STATIC src/waiter.c)
target_include_directories(waiter PUBLIC include)
target_link_libraries(waiter PUBLIC foundation_status PRIVATE embeddedlib::project_options)
