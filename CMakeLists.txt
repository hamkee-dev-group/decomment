cmake_minimum_required(VERSION 3.14)
project(decomment VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Warning flags
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Main executable
set(SOURCES
    src/main.c
    src/cli.c
    src/buf.c
    src/log.c
    src/platform.c
    src/fs.c
    src/lang.c
    src/handlers/c_like.c
    src/handlers/python.c
    src/handlers/shell.c
    src/handlers/hash.c
    src/handlers/lua.c
)

add_executable(decomment ${SOURCES})
target_include_directories(decomment PRIVATE include src)

# Test executable
set(TEST_SOURCES
    tests/test_main.c
    src/buf.c
    src/handlers/c_like.c
    src/handlers/python.c
    src/handlers/shell.c
    src/handlers/hash.c
    src/handlers/lua.c
)

add_executable(decomment_tests ${TEST_SOURCES})
target_include_directories(decomment_tests PRIVATE include src)
target_compile_definitions(decomment_tests PRIVATE TESTING)

enable_testing()
add_test(NAME unit_tests COMMAND decomment_tests)

# Install
include(GNUInstallDirs)
install(TARGETS decomment RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Sanitizer build type
if(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
    if(NOT MSVC)
        target_compile_options(decomment PRIVATE -fsanitize=address,undefined -g)
        target_link_options(decomment PRIVATE -fsanitize=address,undefined)
        target_compile_options(decomment_tests PRIVATE -fsanitize=address,undefined -g)
        target_link_options(decomment_tests PRIVATE -fsanitize=address,undefined)
    endif()
endif()
