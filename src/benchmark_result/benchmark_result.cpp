#include "benchmark_result/benchmark_result.h"


std::string Benchmark_Result::to_string() const{
    return mesh_name + ',' + test_name + ',' + std::to_string(complexity_k) + ',' + std::to_string(result);
}
