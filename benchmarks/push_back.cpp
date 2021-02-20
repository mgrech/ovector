#include <benchmark/benchmark.h>

#include <vector>

#include "noopt.hpp"
#include <mgrech/ovector.hpp>

static
void push_back_std_vector(benchmark::State& state)
{
	auto n = state.range(0);

	for(auto _ : state)
	{
		std::vector<int> v;

		for(int i = 0; i != n; ++i)
			v.push_back(i);

		benchmark::DoNotOptimize(v.data());
	}
}

static
void push_back_std_vector_reserve(benchmark::State& state)
{
	auto n = state.range(0);

	for(auto _ : state)
	{
		std::vector<int> v;
		v.reserve(n);

		for(int i = 0; i != n; ++i)
			v.push_back(i);

		benchmark::DoNotOptimize(v.data());
	}
}

static
void push_back_ovector(benchmark::State& state)
{
	auto n = state.range(0);

	for(auto _ : state)
	{
		auto v = mgrech::ovector<int>::with_max_size_or_null(n);

		for(int i = 0; i != n; ++i)
			v.push_back(i);

		benchmark::DoNotOptimize(v.data());
	}
}

BENCHMARK(push_back_ovector)           ->RangeMultiplier(32)->Range(1, 1024*1024*1024)->Unit(benchmark::kMicrosecond);
BENCHMARK(push_back_std_vector)        ->RangeMultiplier(32)->Range(1, 1024*1024*1024)->Unit(benchmark::kMicrosecond);
BENCHMARK(push_back_std_vector_reserve)->RangeMultiplier(32)->Range(1, 1024*1024*1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_MAIN();
