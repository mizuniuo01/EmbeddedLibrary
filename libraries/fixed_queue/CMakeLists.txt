add_library(fixed_queue STATIC src/fixed_queue.c)
target_include_directories(fixed_queue PUBLIC include)
target_link_libraries(fixed_queue PUBLIC foundation_status PRIVATE embeddedlib::project_options)
