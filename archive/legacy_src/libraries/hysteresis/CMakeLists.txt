add_library(hysteresis STATIC src/hysteresis.c)
target_include_directories(hysteresis PUBLIC include)
target_link_libraries(hysteresis PUBLIC foundation_status numeric
    PRIVATE embeddedlib::project_options)
