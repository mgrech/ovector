# Performance
In general, `ovector` was written to provide good performance in both debug and release builds. Some debug overhead is unavoidable, but great care was taken to ensure that the performance is as good as possible. Most functions are annotated always-inline to ensure they get inlined in debug mode. Not only is this good for performance, it is also more convenient for debugging: If there is a crash, the debugger will point to the location in user code where it is overwhelmingly likely to be, not the `ovector` implementation.

## Methodology
Benchmarks should be built in release mode and the build will fail otherwise. For each benchmark two binaries are built: A `-debug` and a `-release` version.

Please note that debug benchmarking is imperfect: googlebench (the benchmark library used) depends on `std::vector`, so either googlebench *and* `std::vector` are compiled with optimizations, or neither is. The approach used is to build with optimizations on and disable them before including `ovector.hpp`, which is included last. The advantage is that timing code measures with the same accuracy in both builds, the disadvantage is that it may skew results in favor of `std::vector`. It doesn't seem like there is a good solution apart from abandoning googlebench. Other benchmarking libraries may suffer from the same problem.

`ovector` does not support debug iterators. In order to ensure that numbers are comparable, any such features are disabled for `std::vector`.

## Benchmarks

All benchmarks were compiled with MSVC and run on a Windows 10 system with a 9900KS CPU locked to 5 GHz. Numbers are taken from the output of googlebench.

### push_back

This first benchmark constructs the corresponding container and uses `push_back` to insert the given number of ints. In the case of `std::vector`, both with and without reserve is tested. For `ovector` the number of elements is used as the capacity.

#### Release

| Benchmark | Time | CPU | Iterations |
|:---|---:|---:|---:|
| push_back_ovector/1                     |    2.62 us |    2.61 us |   263529 |
| push_back_ovector/32                    |    2.58 us |    2.62 us |   280000 |
| push_back_ovector/1024                  |    2.88 us |    2.89 us |   248889 |
| push_back_ovector/32768                 |    35.5 us |    36.1 us |    20364 |
| push_back_ovector/1048576               |    1033 us |    1050 us |      640 |
| push_back_ovector/33554432              |   37244 us |   37829 us |       19 |
| push_back_ovector/1073741824            | 1180655 us | 1171875 us |        1 |
| push_back_std_vector/1                  |   0.047 us |   0.046 us | 14451613 |
| push_back_std_vector/32                 |   0.513 us |   0.516 us |  1120000 |
| push_back_std_vector/1024               |    1.77 us |    1.76 us |   407273 |
| push_back_std_vector/32768              |    57.6 us |    58.0 us |    13204 |
| push_back_std_vector/1048576            |    3444 us |    3446 us |      195 |
| push_back_std_vector/33554432           |  148069 us |  146875 us |        5 |
| push_back_std_vector/1073741824         | 5333700 us | 5343750 us |        1 |
| push_back_std_vector_reserve/1          |   0.046 us |   0.044 us | 14933333 |
| push_back_std_vector_reserve/32         |   0.083 us |   0.084 us |  8960000 |
| push_back_std_vector_reserve/1024       |    1.16 us |    1.14 us |   560000 |
| push_back_std_vector_reserve/32768      |    36.3 us |    36.1 us |    19478 |
| push_back_std_vector_reserve/1048576    |    1840 us |    1843 us |      407 |
| push_back_std_vector_reserve/33554432   |   63793 us |   63920 us |       11 |
| push_back_std_vector_reserve/1073741824 | 2049778 us | 2062500 us |        1 |

The constant overhead of `ovector` is clearly visible, as execution time for one element is roughly the same as for 1024 elements. However, once we reach n=32K, `ovector` takes the lead due to its faster `push_back` and at n=1M `ovector` is already ahead by a factor of 3.3x compared to `std::vector` without and by a factor of 1.8x with `reserve()`. The gap only increases in favor of `ovector` from there.

What follows is the same set of benchmarks, but in debug mode instead of release. One thing to point out is that `ovector` in debug mode wins against `std::vector` without `reserve()` in release mode. Even if `reserve()` is used, `ovector` is less than 2x slower for large n in this purposefully unfair comparison.

#### Debug

| Benchmark | Time | CPU | Iterations |
|:---|---:|---:|---:|
| push_back_ovector/1                     |     2.53 us |     2.57 us |  280000 |
| push_back_ovector/32                    |     2.59 us |     2.61 us |  263529 |
| push_back_ovector/1024                  |     5.04 us |     5.16 us |  100000 |
| push_back_ovector/32768                 |      104 us |      105 us |    6400 |
| push_back_ovector/1048576               |     3227 us |     3228 us |     213 |
| push_back_ovector/33554432              |   108586 us |   109375 us |       6 |
| push_back_ovector/1073741824            |  3466728 us |  3468750 us |       1 |
| push_back_std_vector/1                  |    0.088 us |    0.088 us | 7466667 |
| push_back_std_vector/32                 |     1.04 us |     1.05 us |  746667 |
| push_back_std_vector/1024               |     7.89 us |     7.85 us |   89600 |
| push_back_std_vector/32768              |      241 us |      239 us |    2489 |
| push_back_std_vector/1048576            |     9017 us |     8958 us |      75 |
| push_back_std_vector/33554432           |   326052 us |   328125 us |       2 |
| push_back_std_vector/1073741824         | 10934870 us | 10937500 us |       1 |
| push_back_std_vector_reserve/1          |    0.079 us |    0.080 us | 8960000 |
| push_back_std_vector_reserve/32         |    0.267 us |    0.267 us | 2635294 |
| push_back_std_vector_reserve/1024       |     6.17 us |     6.14 us |  112000 |
| push_back_std_vector_reserve/32768      |      195 us |      197 us |    3733 |
| push_back_std_vector_reserve/1048576    |     6990 us |     6975 us |     112 |
| push_back_std_vector_reserve/33554432   |   227142 us |   223958 us |       3 |
| push_back_std_vector_reserve/1073741824 |  7294065 us |  7281250 us |       1 |

### Iteration

Then next benchmark compares iteration performance by filling a vector with integers and computing the sum of all elements with a range-based for loop. The time to insert elements is not included in these numbers, therefore there is only one `std::vector` configuration.

The result is as expected: Both containers are equally as fast in both debug and release builds.

#### Debug

| Benchmark | Time | CPU | Iterations |
|:---|---:|---:|---:|
| sum_ovector/1             |   0.004 us |   0.004 us | 203636364 |
| sum_ovector/32            |   0.048 us |   0.048 us |  14451613 |
| sum_ovector/1024          |    1.48 us |    1.48 us |    497778 |
| sum_ovector/32768         |    47.3 us |    47.1 us |     14933 |
| sum_ovector/1048576       |    1524 us |    1535 us |       448 |
| sum_ovector/33554432      |   48844 us |   49107 us |        14 |
| sum_ovector/1073741824    | 1572742 us | 1562500 us |         1 |
| sum_std_vector/1          |   0.003 us |   0.003 us | 263529412 |
| sum_std_vector/32         |   0.047 us |   0.047 us |  14933333 |
| sum_std_vector/1024       |    1.47 us |    1.48 us |    497778 |
| sum_std_vector/32768      |    47.1 us |    47.1 us |     14933 |
| sum_std_vector/1048576    |    1521 us |    1500 us |       448 |
| sum_std_vector/33554432   |   48957 us |   50000 us |        10 |
| sum_std_vector/1073741824 | 1563200 us | 1578125 us |         1 |

#### Release

| Benchmark | Time | CPU | Iterations |
|:---|---:|---:|---:|
| sum_ovector/1             |  0.001 us |  0.001 us | 640000000 |
| sum_ovector/32            |  0.009 us |  0.009 us |  89600000 |
| sum_ovector/1024          |  0.271 us |  0.273 us |   2635294 |
| sum_ovector/32768         |   8.50 us |   8.54 us |     89600 |
| sum_ovector/1048576       |    274 us |    276 us |      2489 |
| sum_ovector/33554432      |   9567 us |   9521 us |        64 |
| sum_ovector/1073741824    | 312881 us | 312500 us |         2 |
| sum_std_vector/1          |  0.001 us |  0.001 us | 746666667 |
| sum_std_vector/32         |  0.009 us |  0.009 us |  74666667 |
| sum_std_vector/1024       |  0.269 us |  0.273 us |   2635294 |
| sum_std_vector/32768      |   8.49 us |   8.37 us |     74667 |
| sum_std_vector/1048576    |    274 us |    270 us |      2489 |
| sum_std_vector/33554432   |   9533 us |   9583 us |        75 |
| sum_std_vector/1073741824 | 309617 us | 312500 us |         2 |