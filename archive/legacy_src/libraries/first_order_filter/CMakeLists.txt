add_library(first_order_filter STATIC src/first_order_filter.c)
target_include_directories(first_order_filter PUBLIC include)
target_link_libraries(first_order_filter PUBLIC foundation_status numeric
    PRIVATE embeddedlib::project_options)
