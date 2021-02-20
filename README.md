# `ovector` - overcommit vector
STL-like sequence container that pre-allocates memory to eliminate reallocations.

You might consider using `ovector` if...
- ... you can put an upper bound on the number of elements.
- ... you need pointer stability after `push_back`.
- ... you need true constant-time and deterministic `push_back`.
- ... you expect a large number of elements.
- ... you care about performance in debug builds.

## How it works
Modern operating systems allocate physical memory lazily for virtual allocations. `ovector` uses this fact to its advantage by allocating address space for the specified maximum size in advance, making reallocations unnecessary. The excess memory is not actually backed by physical memory until it is used.

Advantages:
- Since `ovector` never reallocates, `push_back` is always a constant time operation and in fact branch-free.
- Because there are no reallocations, `push_back` does not invalidate pointers.

Disadvantages:
- You need to be able to put an upper bound on the number of elements.
- You may run into issues in 32-bit applications due to limited address space.
- There is a large constant overhead because `ovector` needs to go to the system directly for allocating address space.

If the maximum size is exceeded, the behavior is undefined. `ovector` allocates a guard region at least the size of one element after the memory. This ensures a segfault if an attempt is made to `push_back` beyond the available space.

## Differences between `ovector` and `std::vector`
On the surface `ovector` may seem to be equivalent to a `std::vector` with `reserve()`, but note that `std::vector` does not provide a pointer stability guarantee and `ovector` was specifically designed for speed in this niche. In addition, the following differences apply:

- The default constructor of `ovector` yields an object where inserting elements is undefined behavior. This is because `ovector` only makes sense with a specified maximum size. A default-constructed `ovector` has no attached memory.
- `ovector` provides functions to grow and shrink without invoking constructors or destructors, making it suitable as an uninitialized buffer.
- `ovector` is neither copy-constructible nor copy-assignable. Besides being operations that should generally be avoided, the semantics are not obvious: How big should the allocated memory be for a copy?
- `ovector` does not provide `at()`.
- `ovector` does not provide member functions that invalidate pointers.
- `ovector` does not have a specialization for `bool`.

## Performance
See [performance](performance.md).

## Exceptions
`ovector` neither requires nor uses exceptions. If exceptions are enabled, `ovector` provides at least the strong exception guarantee and the nothrow guarantee where possible.

## Using `ovector` without CMake
Simply copy [ovector.hpp](include/mgrech/ovector.hpp) and [ovector.cpp](source/ovector.cpp) to your project.

## Using `ovector` with CMake (3.14 or newer)
Use the FetchContent module to declare and fetch the dependency:
```
include(FetchContent)

FetchContent_Declare(ovector
	GIT_REPOSITORY https://github.com/mgrech/ovector.git
	GIT_TAG <version>)

FetchContent_MakeAvailable(ovector)
```

The CMake script provides an object library you can link to:
```
target_link_libraries(<target> ovector::ovector)
```

CMake Options:
- `OVECTOR_BUILD_TESTS`: Include test executables in the build. Default: Off.
- `OVECTOR_BUILD_BENCHMARKS`: Include benchmark executables in the build. Default: Off.

## Supported Platforms
`ovector` requires C++11. If you want to build tests and/or benchmarks, you also need CMake 3.14 or newer.
