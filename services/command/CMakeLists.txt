add_library(command STATIC command.c)
target_include_directories(command PUBLIC .)
target_link_libraries(command PUBLIC mizu_status PRIVATE embeddedlib::project_options)
