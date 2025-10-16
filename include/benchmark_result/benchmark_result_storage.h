#ifndef RESULT_STORAGE_H
#define RESULT_STORAGE_H
// ----------------------------------------------------------------------------
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
// ----------------------------------------------------------------------------
#include "benchmark_result/benchmark_result.h"
// ----------------------------------------------------------------------------



class Benchmark_Result_Storage {
private:
    const std::string _storage_path = "./result.csv";

public:
    Benchmark_Result_Storage();

    void write_result(const Benchmark_Result& Result);
    void write_results(const std::vector<Benchmark_Result>& Result);
};

#endif
