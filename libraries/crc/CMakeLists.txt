add_library(crc STATIC src/crc.c)
target_include_directories(crc PUBLIC include)
target_link_libraries(crc PUBLIC foundation_status PRIVATE embeddedlib::project_options)
