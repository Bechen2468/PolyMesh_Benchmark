#ifndef BENCHMARK_H
#define BENCHMARK_H
// Benchmark test meta-template
// ----------------------------------------------------------------------------
#include <vector>
#include "precision_timer.h"
#include "benchmark_result/benchmark_result.h"
// ----------------------------------------------------------------------------


class Benchmark {
protected:
    const std::string _mesh_name = "none";
    size_t _complexity;

public:
    Benchmark(const std::string& Mesh_Name, const size_t& Complexity);
    std::vector<Benchmark_Result> test_all();
    Benchmark_Result test_barycenter();

protected:
    virtual void barycenter() = 0;
    //virtual void circular() = 0;
    //virtual void normal() = 0;
    //virtual void smoothing() = 0;
    //virtual void subdivision() = 0;
    //virtual void collapse() = 0;
};

#endif