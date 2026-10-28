add_library(object_pool STATIC src/object_pool.c)
target_include_directories(object_pool PUBLIC include)
target_link_libraries(object_pool PUBLIC foundation_status PRIVATE embeddedlib::project_options)
