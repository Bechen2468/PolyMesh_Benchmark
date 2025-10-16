#include "benchmark_result/benchmark_result_storage.h"


Benchmark_Result_Storage::Benchmark_Result_Storage() {

}


void Benchmark_Result_Storage::write_result(const Benchmark_Result& Result) {
    std::vector<Benchmark_Result> result_vec;
    result_vec.push_back(Result);
    this->write_results(result_vec);
}


void Benchmark_Result_Storage::write_results(const std::vector<Benchmark_Result>& Results) {
    std::ofstream storage;
    storage.open(this->_storage_path, std::ios::app);

    for(size_t i = 0; i < Results.size(); i++) {
        storage << (Results[i].to_string()) << "\n";
    }
    storage.flush();
    storage.close();
}
