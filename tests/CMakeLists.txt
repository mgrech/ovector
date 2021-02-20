set(BUILD_GMOCK OFF CACHE INTERNAL "")
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG release-1.10.0)

FetchContent_MakeAvailable(googletest)

# make it a target so IDEs don't get confused
add_executable(doctest doctest.cpp)
target_link_libraries(doctest ovector)

add_executable(tests tests.cpp)
target_link_libraries(tests ovector gtest gtest_main)
