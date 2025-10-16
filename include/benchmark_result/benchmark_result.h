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
    long long result;   // time in ms  OR  mesh size in 

    std::string to_string() const;
};

#endif
