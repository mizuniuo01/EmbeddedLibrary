add_library(cascaded_pid STATIC cascaded_pid.c)
target_include_directories(cascaded_pid PUBLIC .)
target_link_libraries(cascaded_pid PUBLIC pid PRIVATE embeddedlib::project_options)
