#include <benchmark/benchmark.h>

#include <cstddef>
#include <vector>

#include "noopt.hpp"
#include <mgrech/ovector.hpp>

static
void sum_std_vector(benchmark::State& state)
{
	auto n = state.range(0);
	std::vector<int> v;

	for(int i = 0; i != n; ++i)
		v.push_back(i);

	for(auto _ : state)
	{
		std::size_t sum = 0;

		for(auto i : v)
			sum += i;

		benchmark::DoNotOptimize(sum);
	}
}

static
void sum_ovector(benchmark::State& state)
{
	auto n = state.range(0);
	auto v = mgrech::ovector<int>::with_max_size_or_null(n);

	for(int i = 0; i != n; ++i)
		v.push_back(i);

	for(auto _ : state)
	{
		std::size_t sum = 0;

		for(auto i : v)
			sum += i;

		benchmark::DoNotOptimize(sum);
	}
}

BENCHMARK(sum_ovector)   ->RangeMultiplier(32)->Range(1, 1024*1024*1024)->Unit(benchmark::kMicrosecond);
BENCHMARK(sum_std_vector)->RangeMultiplier(32)->Range(1, 1024*1024*1024)->Unit(benchmark::kMicrosecond);
BENCHMARK_MAIN();
