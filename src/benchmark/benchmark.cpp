#include "benchmark/benchmark.h"



Benchmark::Benchmark(const std::string& Mesh_Name, const size_t& Complexity):
_mesh_name(Mesh_Name),
_complexity(Complexity)
{
    
}


std::vector<Benchmark_Result> Benchmark::test_all() {
    return std::vector<Benchmark_Result>();
}



Benchmark_Result Benchmark::test_barycenter() {
    Precision_Timer timer = Precision_Timer(); 

    timer.start();
    barycenter();
    timer.stop();

    return {
        this->_mesh_name,
        "barycenter",
        this->_complexity,
        timer.result()
    };
}
