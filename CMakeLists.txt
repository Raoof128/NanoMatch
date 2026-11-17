cmake_minimum_required(VERSION 3.20)
project(NanoMatch
    VERSION 1.0.0
    DESCRIPTION "Limit order book and matching engine (a study project)"
    HOMEPAGE_URL "https://github.com/Raoof128/NanoMatch"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Compiler warnings ──
add_compile_options(
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    -Wsign-conversion -Wnull-dereference -Wformat=2
    -Wdouble-promotion -Wimplicit-fallthrough
)

# Treat warnings as errors in CI
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if(WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
endif()

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -march=native -DNDEBUG)
endif()

# ── Library ──
add_library(nanomatch_lib
    src/book/order_book.cpp
)

target_include_directories(nanomatch_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(nanomatch_lib PROPERTIES
    OUTPUT_NAME nanomatch
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# ── Main executable ──
add_executable(nanomatch_bin src/main.cpp)
target_link_libraries(nanomatch_bin PRIVATE nanomatch_lib)
set_target_properties(nanomatch_bin PROPERTIES OUTPUT_NAME nanomatch)

# ── Install targets ──
include(GNUInstallDirs)

install(TARGETS nanomatch_lib nanomatch_bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/nanomatch
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

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
