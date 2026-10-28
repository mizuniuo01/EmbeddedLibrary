add_library(numeric STATIC src/numeric.c)
target_include_directories(numeric PUBLIC include)
target_link_libraries(numeric PUBLIC foundation_status PRIVATE embeddedlib::project_options)
