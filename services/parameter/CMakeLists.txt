add_library(parameter STATIC parameter.c)
target_include_directories(parameter PUBLIC .)
target_link_libraries(parameter PUBLIC mizu_status PRIVATE embeddedlib::project_options)
