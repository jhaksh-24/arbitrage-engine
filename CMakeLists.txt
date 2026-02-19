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

add_library(arb_core STATIC
    # files to be added
    apps/main.cpp
)

target_include_directories(arb_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(arb_core PUBLIC cxx_std_20)

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
# Status Messages
# ==================================================================================================

message(STATUS "================== CELAE ================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler:   ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS "=========================================")