#pragma once

#ifdef OVECTOR_BENCHMARK_DEBUG

// clang must be checked before msvc as clang-cl defines _MSC_VER but does not recognize msvc pragmas
#if defined(__clang__)
#pragma clang optimize off
#elif defined(_MSC_VER)
#pragma optimize("", off)
#else
#pragma GCC optimize ("O0")
#endif

#endif
