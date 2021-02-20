if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	message(SEND_ERROR "Attempted to build ovector benchmarks in debug configuration. See README.")
endif()

set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE INTERNAL "")
set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")

include(FetchContent)

FetchContent_Declare(googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.5.2)

FetchContent_MakeAvailable(googlebenchmark)

function(ov_add_benchmark_helper name suffix)
	add_executable(bench-${name}-${suffix} ${name}.cpp)
	target_link_libraries(bench-${name}-${suffix} ovector benchmark::benchmark)
endfunction()

function(ov_add_benchmark name)
	ov_add_benchmark_helper(${name} release)
	ov_add_benchmark_helper(${name} debug)
	target_compile_definitions(bench-${name}-debug PRIVATE
	                           OVECTOR_BENCHMARK_DEBUG
	                           _ITERATOR_DEBUG_LEVEL=0)
endfunction()

ov_add_benchmark(push_back)
ov_add_benchmark(sum)
