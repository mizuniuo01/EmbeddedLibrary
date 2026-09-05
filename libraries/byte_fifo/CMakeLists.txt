add_library(byte_fifo STATIC src/byte_fifo.c)
target_include_directories(byte_fifo PUBLIC include)
target_link_libraries(byte_fifo PUBLIC foundation_status PRIVATE embeddedlib::project_options)
