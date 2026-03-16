# Production-Grade CMake Build System for System Programming Portfolio
cmake_minimum_required(VERSION 3.14)

project(
    LinuxSystemProgramming
    VERSION 1.0.0
    LANGUAGES C
    DESCRIPTION "Production-grade Linux system programming demonstration"
)

# Project metadata
set(PROJECT_HOMEPAGE_URL "https://github.com/odeliyach/Linux-System-Programming")
set(PROJECT_MAINTAINER "Odeliya Chitayat")

# Modern C standards and compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Export compile commands for tooling (clang-tidy, IDEs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Compiler flags following industry best practices
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow -Wformat=2 -Wunused")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

# Options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

# Thread support (required for queue implementation)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Common library for shared utilities
add_library(common_utils STATIC
    src/common/logger.c
)
target_include_directories(common_utils PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(common_utils PRIVATE Threads::Threads)

# Shell library
add_library(shell_lib STATIC
    src/shell/shell_core.c
)
target_include_directories(shell_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(shell_lib PRIVATE common_utils)

# Shell executable
add_executable(myshell
    src/shell/main.c
)
target_link_libraries(myshell PRIVATE shell_lib common_utils)

# Queue library
add_library(queue_lib STATIC
    src/queue/queue.c
)
target_include_directories(queue_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(queue_lib PRIVATE common_utils Threads::Threads)

# Sanitizers for development
if(ENABLE_SANITIZERS)
    set(SANITIZER_FLAGS "-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

# Code coverage
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Testing
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation rules
install(TARGETS myshell
    RUNTIME DESTINATION bin
    COMPONENT applications
)

install(DIRECTORY include/
    DESTINATION include
    COMPONENT development
    FILES_MATCHING PATTERN "*.h"
)

install(TARGETS shell_lib queue_lib common_utils
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    COMPONENT development
)

# Package configuration
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR ${PROJECT_MAINTAINER})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
include(CPack)

# Print build configuration
message(STATUS "=== Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Tests: ${BUILD_TESTS}")
message(STATUS "Benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "Sanitizers: ${ENABLE_SANITIZERS}")
message(STATUS "Coverage: ${ENABLE_COVERAGE}")
message(STATUS "========================")
