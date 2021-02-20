# need FetchContent module with FetchContent_MakeAvailable
cmake_minimum_required(VERSION 3.14)

project(ovector)
option(OVECTOR_BUILD_TESTS OFF)
option(OVECTOR_BUILD_BENCHMARKS OFF)

set(CMAKE_CXX_STANDARD 11)

add_library(ovector OBJECT source/ovector.cpp)
target_include_directories(ovector PRIVATE include/mgrech)
target_include_directories(ovector INTERFACE include)

if(OVECTOR_BUILD_TESTS)
	add_subdirectory(tests)
endif()

if(OVECTOR_BUILD_BENCHMARKS)
	add_subdirectory(benchmarks)
endif()
