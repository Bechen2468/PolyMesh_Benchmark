#ifndef IBENCHMARK_H
#define IBENCHMARK_H
// Benchmark test meta-template

// ----------------------------------------------------------------------------
#if defined(__linux__)
#include <malloc.h>
#endif
// ----------------------------------------------------------------------------
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <algorithm>
#include <vector>
#include <span>
#include <random>
#include <benchmark/benchmark.h>
#include "my_utils.h"



#define RANGE_START        1000000
#define RANGE_END          1000000
#define RANGE_MULTIPLIER         4



#define REGISTER_00(DISPLAY_NAME, FIXTURE, NAME, FUNC)                      \
BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& state) {                \
    FUNC(state);                                                            \
}                                                                           \
BENCHMARK_REGISTER_F(FIXTURE, NAME)                                         \
    ->Name(DISPLAY_NAME)                                                    \
    ->ArgNames({"|V|", "Full_Load"})                                        \
    ->ArgsProduct({                                                         \
        benchmark::CreateRange(RANGE_START, RANGE_END, RANGE_MULTIPLIER),   \
        {0} /* FULL_LOAD FLAG: 0 = false, 1 = true */                       \
    })                                                                      \
    ->Unit(benchmark::kMicrosecond);
//    ->Iterations(20)                                                      \


#define REGISTER_01(DISPLAY_NAME, FIXTURE, NAME, FUNC)                      \
BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& state) {                \
    FUNC(state);                                                            \
}                                                                           \
BENCHMARK_REGISTER_F(FIXTURE, NAME)                                         \
    ->Name(DISPLAY_NAME)                                                    \
    ->ArgNames({"|V|", "Full_Load"})                                        \
    ->ArgsProduct({                                                         \
        benchmark::CreateRange(RANGE_START, RANGE_END, RANGE_MULTIPLIER),   \
        {1} /* FULL_LOAD FLAG: 0 = false, 1 = true */                       \
    })                                                                      \
    ->Unit(benchmark::kMicrosecond);
//    ->Iterations(20)                                                      \


#define REGISTER_0_1(DISPLAY_NAME, FIXTURE, NAME, FUNC)                      \
BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& state) {                \
    FUNC(state);                                                            \
}                                                                           \
BENCHMARK_REGISTER_F(FIXTURE, NAME)                                         \
    ->Name(DISPLAY_NAME)                                                    \
    ->ArgNames({"|V|", "Full_Load"})                                        \
    ->ArgsProduct({                                                         \
        benchmark::CreateRange(RANGE_START, RANGE_END, RANGE_MULTIPLIER),   \
        {0, 1} /* FULL_LOAD FLAG: 0 = false, 1 = true */                    \
    })                                                                      \
    ->Unit(benchmark::kMicrosecond);
//    ->Iterations(20)                                                      \


#define REGISTER_FULL(DISPLAY_NAME, FIXTURE, NAME, FUNC)                      \
BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& state) {                \
    FUNC(state);                                                            \
}                                                                           \
BENCHMARK_REGISTER_F(FIXTURE, NAME)                                         \
    ->Name(DISPLAY_NAME)                                                    \
    ->ArgNames({"|V|", "Full_Load"})                                        \
    ->ArgsProduct({                                                         \
        benchmark::CreateRange(RANGE_START, RANGE_END, RANGE_MULTIPLIER),   \
        {0, 1, 2, 3, 4}                   \
    })                                                                      \
    ->Unit(benchmark::kMicrosecond);
//    ->Iterations(20)                                                      \


    


struct Memory_Observer {
    size_t _base = 0;
    size_t _base_count = 0;
    size_t _pre_run = 0;
    size_t _pre_count = 0;
    size_t _post_run = 0;
    size_t _post_count = 0;

    void reset() {
        _base = 0;
        _base_count = 0;
        _pre_run = 0;
        _pre_count = 0;
        _post_run = 0;
        _post_count = 0;
    }

    void measure_base() { 
        _base += my_utils::system::get_current_memory_kb(); 
        _base_count++;
    }
    
    void measure_pre_run() { 
        _pre_run += my_utils::system::get_current_memory_kb(); 
        _pre_count++;
    }
    
    void measure_post_run() { 
        _post_run += my_utils::system::get_current_memory_kb(); 
        _post_count++;
    }

    int get_base() {
        return static_cast<int>(_base / _base_count) / 1000;
    }
    int get_pre_run() {
        return ( static_cast<int>(_pre_run/_pre_count) - static_cast<int>(_base/_base_count) ) / 1000;
    }
    int get_post_run() {
        return ( static_cast<int>(_post_run/_post_count) - static_cast<int>(_base/_base_count) ) / 1000;
    }
};



class IBenchmark : public benchmark::Fixture {
    enum Mesh_Type : size_t {
        Rand_Sphere = 0,
        Quad = 1,
        Dual = 2
    };

protected:
    bool _setUpIsFirst = true;
    Memory_Observer mem;

    size_t V, F, E;

    std::string file_path;

public:
    void SetUp(benchmark::State& state) override {
#if defined(__linux__)
        malloc_trim(0); // Forces glibc to release free arenas back to OS
#endif
        size_t n = state.range(0);  // mesh size
        size_t full_load = state.range(1);  // pre load full

        mem.reset();

        // print seperation if first run
        if(n == RANGE_START) {
            if(_setUpIsFirst && (RANGE_START != RANGE_END || 
                (
                    state.name().find("OpenMesh") != std::string::npos && state.name().find("Load") == std::string::npos
                )
            )) {
                std::cout << "-----------------------------------------------------------------------------------------------------" << std::endl;
            }
            _setUpIsFirst = false;
        } else {
            _setUpIsFirst = true;
        }
        
        // build mesh
        Mesh_Type mesh_type = Mesh_Type::Rand_Sphere;
        if(state.name().find("Quad") != std::string::npos) mesh_type = Mesh_Type::Quad;
        if(state.name().find("Dual") != std::string::npos) mesh_type = Mesh_Type::Dual;
        
        file_path = "./off_files/" 
            + (std::array<std::string, 3>{"convex", "quad", "dual"}[mesh_type]) 
            + std::to_string(n) + ".off";
            
        build_mesh(file_path, Rand_Sphere, n);

        

        if(file_path, state.name().find("Load") == std::string::npos) {
            std::vector<size_t> mesh_Sizes = load_mesh_storage(file_path);

            V = mesh_Sizes[0];
            F = mesh_Sizes[1];
            E = mesh_Sizes[2];

            if(state.name().find("Rand") != std::string::npos) {
                _rand_v = my_utils::num::generate_random_indices(mesh_Sizes[0]);
                _rand_f = my_utils::num::generate_random_indices(mesh_Sizes[1]);
                _rand_e = my_utils::num::generate_random_indices(mesh_Sizes[2]);
            }
        }
        mem.measure_base();
    }


    void TearDown(benchmark::State& state) override {
        // memory print
        state.counters["mb_pre_run"]  = mem.get_pre_run();
        state.counters["mb_post_run"] = mem.get_post_run();
        
        destroy_mesh();
#if defined(__linux__)
        malloc_trim(0); // Forces glibc to release free arenas back to OS
#endif
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

    void run_convert_to_trimesh(benchmark::State& state);


    
    // real algorithms
    void run_circular(benchmark::State& state);
    void run_barycenter(benchmark::State& state);
    void run_normal(benchmark::State& state);
    void run_smoothing(benchmark::State& state);
    void run_subdivision(benchmark::State& state);
    void run_collapse(benchmark::State& state);

protected:
    volatile double _benchmark_sink = 0.0;
    std::vector<size_t> _rand_v;
    std::vector<size_t> _rand_f;
    std::vector<size_t> _rand_e;


    void build_mesh(const std::string& file_path, Mesh_Type Type, size_t Size);


    
    virtual std::vector<size_t> load_mesh_storage(const std::string& file_path, int load_full = 0) = 0;
    virtual void load_mesh_from_storage(int load_full) = 0;
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
    
    virtual void _convert_to_trimesh() = 0;


    // real algorithms
    virtual void _circular() = 0;
    virtual void _barycenter() = 0;
    virtual void _normal() = 0;
    virtual void _smoothing() = 0;
    virtual void _subdivision() = 0;
    virtual void _collapse() = 0;

};

#endif
