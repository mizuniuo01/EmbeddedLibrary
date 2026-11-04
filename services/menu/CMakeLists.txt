add_library(menu STATIC menu.c)
target_include_directories(menu PUBLIC .)
target_link_libraries(menu PUBLIC parameter command mizu_status PRIVATE embeddedlib::project_options)
