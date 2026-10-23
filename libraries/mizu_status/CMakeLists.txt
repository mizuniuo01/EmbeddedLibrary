add_library(mizu_status INTERFACE)
target_include_directories(mizu_status INTERFACE .)
target_link_libraries(mizu_status INTERFACE embeddedlib::project_options)
