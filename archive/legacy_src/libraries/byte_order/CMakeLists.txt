add_library(byte_order STATIC src/byte_order.c)
target_include_directories(byte_order PUBLIC include)
target_link_libraries(byte_order PUBLIC foundation_status PRIVATE embeddedlib::project_options)
