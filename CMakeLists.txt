cmake_minimum_required(VERSION 3.20)
project(NanoMatch
    VERSION 1.0.0
    DESCRIPTION "High-performance limit order book and matching engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
add_compile_options(
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    -Wsign-conversion -Wnull-dereference
)

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -march=native -DNDEBUG)
endif()

# ── Library ──
add_library(nanomatch_lib
    src/book/order_book.cpp
)

target_include_directories(nanomatch_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ── Main executable ──
add_executable(nanomatch src/main.cpp)
target_link_libraries(nanomatch PRIVATE nanomatch_lib)

# ── Tests ──
option(BUILD_TESTING "Build test suite" ON)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Benchmarks ──
option(BUILD_BENCH "Build benchmarks" ON)

if(BUILD_BENCH)
    add_subdirectory(bench)
endif()
