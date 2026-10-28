add_library(cobs STATIC src/cobs.c)
target_include_directories(cobs PUBLIC include)
target_link_libraries(cobs PUBLIC foundation_status PRIVATE embeddedlib::project_options)
