#ifndef BENCHMARK_RESULT_H
#define BENCHMARK_RESULT_H
// ----------------------------------------------------------------------------
#include <string>
// ----------------------------------------------------------------------------



struct Benchmark_Result
{
    std::string mesh_name;
    std::string test_name;
    size_t complexity_k;
    long long duration;   // time in ms  OR  mesh size in 
    size_t memoryUsage_after;

    std::string to_string() const;
};

#endif
