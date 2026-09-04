cmake_minimum_required(VERSION 3.20)
project(embedded_library VERSION 0.1.0 LANGUAGES C)

include(CTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(EMBEDDEDLIB_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(embeddedlib_project_options INTERFACE)
target_compile_features(embeddedlib_project_options INTERFACE c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(embeddedlib_project_options INTERFACE
        -Wall
        -Wextra
        -Wconversion
        -Wshadow
        -Wformat=2
        -Wundef
        -Wstrict-prototypes
        -Wmissing-prototypes
        -Werror
    )
endif()

if(EMBEDDEDLIB_ENABLE_SANITIZERS)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "EMBEDDEDLIB_ENABLE_SANITIZERS requires Clang")
    endif()

    target_compile_options(embeddedlib_project_options INTERFACE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )
    target_link_options(embeddedlib_project_options INTERFACE
        -fsanitize=address,undefined
    )
endif()

add_library(embeddedlib::project_options ALIAS embeddedlib_project_options)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
