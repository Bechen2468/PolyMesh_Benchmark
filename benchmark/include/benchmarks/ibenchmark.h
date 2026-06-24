#ifndef IBENCHMARK_H
#define IBENCHMARK_H
// Benchmark test meta-template
// ----------------------------------------------------------------------------
#include <fstream>
#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <benchmark/benchmark.h>
#include "my_utils.h"



#define RANGE_START        100000
#define RANGE_END          100000
#define RANGE_MULTIPLIER         4



#define REGISTER(FIXTURE, NAME, FUNC)                         \
BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& state) {  \
    FUNC(state);                                              \
}                                                             \
BENCHMARK_REGISTER_F(FIXTURE, NAME)                           \
    ->RangeMultiplier(RANGE_MULTIPLIER)                       \
    ->Range(RANGE_START, RANGE_END);


    
static size_t _loadedN;

class IBenchmark : public benchmark::Fixture {
private:
    bool _setUpIsFirst = true;
    size_t _mem_base = 0;
    size_t _mem_before = 0;

public:
    void SetUp(benchmark::State& state) override {
        size_t n = state.range(0);  // parameterise mesh size

        // print seperation if first run
        if(n == RANGE_START) {
            if(_setUpIsFirst && (RANGE_START != RANGE_END || 
                (state.name().find("PolyMesh_V01") != std::string::npos || 
                (state.name().find("PolyMesh_V02/Adjacent_") != std::string::npos && state.name().find("Loaded") == std::string::npos)
                )
            )) {
                std::cout << "------------------------------------------------------------------------------------------------------" << std::endl;
            }
            _setUpIsFirst = false;
        } else {
            _setUpIsFirst = true;
        }
        
        std::string fileName = "./mesh_convex_" + std::to_string(n) + ".off";

        build_mesh(fileName, n);    // only if not already esists

        
        _mem_base = my_utils::system::get_resident_memory_kb();
        std::vector<size_t> mesh_Sizes = load_mesh(fileName, state.name().find("_loaded") != std::string::npos || state.name().find("_Loaded") != std::string::npos);

        _rand_v = my_utils::num::generate_random_indices(mesh_Sizes[0]);
        _rand_f = my_utils::num::generate_random_indices(mesh_Sizes[1]);
        _rand_e = my_utils::num::generate_random_indices(mesh_Sizes[2]);

        //MemoryManager::reset_counters();
        _mem_before = my_utils::system::get_resident_memory_kb() - _mem_base;
    }


    void TearDown(benchmark::State& state) override {
        size_t mem_after = my_utils::system::get_resident_memory_kb() - _mem_base;
        state.counters["RSS_mb_before"] = _mem_before / 1000;
        state.counters["RSS_mb_after"]  = mem_after / 1000;
        state.counters["RSS_mb_change"]  = ((double)mem_after - (double)_mem_before) / 1000;

        //state.counters["allocs"]     = MemoryManager::g_alloc_count.load();
        //state.counters["peak_bytes"] = MemoryManager::g_peak_bytes.load();
        //state.counters["net_bytes"]  = MemoryManager::g_current_bytes.load();
        
        destroy_mesh();
    }


    void run_load(benchmark::State& state);

    void run_iterate_V_raw(benchmark::State& state);
    void run_iterate_F_raw(benchmark::State& state);

    void run_iterate_V(benchmark::State& state);
    void run_iterate_F(benchmark::State& state);
    void run_iterate_E(benchmark::State& state);

    void run_iterate_random_V(benchmark::State& state);
    void run_iterate_random_F(benchmark::State& state);
    void run_iterate_random_E(benchmark::State& state);
    
    void run_adjacent_VV(benchmark::State& state);
    void run_adjacent_VF(benchmark::State& state);
    void run_adjacent_FV(benchmark::State& state);
    void run_adjacent_EF(benchmark::State& state);

protected:
    volatile double _benchmark_sink = 0.0;
    std::vector<size_t> _rand_v;
    std::vector<size_t> _rand_f;
    std::vector<size_t> _rand_e;

    void build_mesh(const std::string& FileName, size_t Size);

    virtual std::vector<size_t> load_mesh(const std::string& FileName, bool fully_loaded) = 0;
    virtual void destroy_mesh() {}


    virtual void _iterate_V_raw() = 0;
    virtual void _iterate_F_raw() = 0;

    virtual void _iterate_V() = 0;
    virtual void _iterate_F() = 0;
    virtual void _iterate_E() = 0;
    
    virtual void _iterate_random_V(const std::vector<size_t>& rand) = 0;
    virtual void _iterate_random_F(const std::vector<size_t>& rand) = 0;
    virtual void _iterate_random_E(const std::vector<size_t>& rand) = 0;

    virtual void _adjacent_VV() = 0;
    virtual void _adjacent_VF() = 0;
    virtual void _adjacent_FV() = 0;
    virtual void _adjacent_EF() = 0;
};

#endif
