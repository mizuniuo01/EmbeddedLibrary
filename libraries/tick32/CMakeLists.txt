add_library(tick32 STATIC src/tick32.c)
target_include_directories(tick32 PUBLIC include)
target_link_libraries(tick32 PUBLIC foundation_status PRIVATE embeddedlib::project_options)
