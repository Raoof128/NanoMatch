# The benchmarks print how they were built, so pass the build type through.
foreach(bench_name bench_throughput bench_latency bench_memory)
    add_executable(${bench_name} ${bench_name}.cpp)
    target_link_libraries(${bench_name} PRIVATE nanomatch_lib)
    target_compile_definitions(${bench_name} PRIVATE
        NANOMATCH_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
endforeach()
