add_library(mizu_fifo STATIC mizu_fifo.c)
target_include_directories(mizu_fifo PUBLIC .)
target_link_libraries(mizu_fifo PUBLIC mizu_status PRIVATE embeddedlib::project_options)
