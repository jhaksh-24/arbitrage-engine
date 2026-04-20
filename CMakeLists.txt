# ==================================================================================================
# Project Settings
# ==================================================================================================

cmake_minimum_required(VERSION 3.20)
project(CELAE
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Cross Exchange Latency Arbitrage Engine"
)

# ==================================================================================================
# C++ Standard and Compiler Settings
# ==================================================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==================================================================================================
# Build Type Handling
# ==================================================================================================

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ==================================================================================================
# Core Library
# ==================================================================================================

add_library(arb_core INTERFACE)

target_include_directories(arb_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(arb_core INTERFACE cxx_std_20)

# ==================================================================================================
# Compiler Warnings
# ==================================================================================================

if(MSVC)
    add_compile_options(
        /W4 /permissive-
    )
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wsign-conversion
    )
endif()

# ==================================================================================================
# Executables + Linking
# ==================================================================================================

add_executable(celae apps/main.cpp)
target_link_libraries(celae PRIVATE arb_core)

# ==================================================================================================
# Testing (Google Test)
# ==================================================================================================

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_order_book tests/test_order_book.cpp)
target_link_libraries(test_order_book PRIVATE arb_core gtest_main)

include(GoogleTest)
gtest_discover_tests(test_order_book)

# ==================================================================================================
# Benchmarks (Google Benchmark)
# ==================================================================================================

FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/v1.8.3.zip
)
# Disable benchmark tests/install to speed up CMake
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(bench_order_book bench/bench_order_book.cpp)
target_link_libraries(bench_order_book PRIVATE arb_core benchmark::benchmark)

# ==================================================================================================
# Status Messages
# ==================================================================================================

message(STATUS "================== CELAE ================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler:   ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS "=========================================")