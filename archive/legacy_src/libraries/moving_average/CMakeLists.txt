add_library(moving_average STATIC src/moving_average.c)
target_include_directories(moving_average PUBLIC include)
target_link_libraries(moving_average PUBLIC foundation_status numeric
    PRIVATE embeddedlib::project_options)
