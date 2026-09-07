add_library(rate_limiter STATIC src/rate_limiter.c)
target_include_directories(rate_limiter PUBLIC include)
target_link_libraries(rate_limiter PUBLIC foundation_status numeric
    PRIVATE embeddedlib::project_options)
