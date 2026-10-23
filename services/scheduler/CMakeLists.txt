add_library(scheduler STATIC scheduler.c)
target_include_directories(scheduler PUBLIC .)
target_link_libraries(scheduler PUBLIC mizu_status PRIVATE embeddedlib::project_options)
