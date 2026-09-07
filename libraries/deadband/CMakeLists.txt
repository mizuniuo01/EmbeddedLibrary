add_library(deadband STATIC src/deadband.c)
target_include_directories(deadband PUBLIC include)
target_link_libraries(deadband PUBLIC foundation_status numeric
    PRIVATE embeddedlib::project_options)
