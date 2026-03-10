#include "benchmark/benchmark.h"



Benchmark::Benchmark(const std::string& Mesh_Name, size_t Complexity):
_mesh_name(Mesh_Name),
_complexity(Complexity),
_benchmark_sink(0.0)
{}


std::vector<Benchmark_Result> Benchmark::run_all(size_t seed) {
    std::vector<Benchmark_Result> results;

    results.push_back(this->run_iterate_V());
    results.push_back(this->run_iterate_F());
    results.push_back(this->run_iterate_E());
    

    std::mt19937 rng(seed);

    // creating random vertex input
    std::vector<size_t> rand_V(count_V());
    std::iota(rand_V.begin(), rand_V.end(), 0);
    std::shuffle(rand_V.begin(), rand_V.end(), rng);

    // creating random face input
    std::vector<size_t> rand_F(count_F());
    std::iota(rand_F.begin(), rand_F.end(), 0);
    std::shuffle(rand_F.begin(), rand_F.end(), rng);

    // creating random edge input
    std::vector<size_t> rand_E(count_E());
    std::iota(rand_E.begin(), rand_E.end(), 0);
    std::shuffle(rand_E.begin(), rand_E.end(), rng);

    results.push_back(this->run_iterate_random_V(rand_V));
    results.push_back(this->run_iterate_random_F(rand_F));
    results.push_back(this->run_iterate_random_E(rand_E));
    
    results.push_back(this->run_adjacent_VV());
    results.push_back(this->run_adjacent_VF());
    results.push_back(this->run_adjacent_FV());
    results.push_back(this->run_adjacent_EF());

    return results;
}
