add_library(cascaded_pid STATIC src/cascaded_pid.c)
target_include_directories(cascaded_pid PUBLIC include)
target_link_libraries(cascaded_pid PUBLIC foundation_status pid numeric
    PRIVATE embeddedlib::project_options)
