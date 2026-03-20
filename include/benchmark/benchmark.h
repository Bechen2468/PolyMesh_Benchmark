#ifndef BENCHMARK_H
#define BENCHMARK_H
// Benchmark test meta-template
// ----------------------------------------------------------------------------
#include <random>
#include <algorithm>
#include <vector>
#include "my_utils.h"
#include "precision_timer.h"
#include "result/benchmark_result.h"
// ----------------------------------------------------------------------------


class Benchmark {
protected:
    const std::string _mesh_name;
    size_t _complexity;
    size_t _pre_mesh_mem_usage;

    // used to force unreused loop execution
    volatile double _benchmark_sink;

public:
    Benchmark(const std::string& Mesh_Name, size_t Complexity);


    inline virtual size_t count_V() const = 0;
    inline virtual size_t count_F() const = 0;
    inline virtual size_t count_E() const = 0;

    //Benchmark_Result run_barycenter() { return _run<&Benchmark::_barycenter>("barycenter"); }

    Benchmark_Result run_iterate_V() { return _run<&Benchmark::_iterate_V>("iterate_V"); }
    Benchmark_Result run_iterate_F() { return _run<&Benchmark::_iterate_F>("iterate_F"); }
    Benchmark_Result run_iterate_E() { return _run<&Benchmark::_iterate_E>("iterate_E"); }
    
    Benchmark_Result run_iterate_random_V(const std::vector<size_t>& rand) { return _run_random<&Benchmark::_iterate_random_V>("iterate_random_V", rand); }
    Benchmark_Result run_iterate_random_F(const std::vector<size_t>& rand) { return _run_random<&Benchmark::_iterate_random_F>("iterate_random_F", rand); }
    Benchmark_Result run_iterate_random_E(const std::vector<size_t>& rand) { return _run_random<&Benchmark::_iterate_random_E>("iterate_random_E", rand); }
    
    Benchmark_Result run_adjacent_VV() { return _run<&Benchmark::_adjacent_VV>("adjacent_VV"); }
    Benchmark_Result run_adjacent_VF() { return _run<&Benchmark::_adjacent_VF>("adjacent_VF"); }
    Benchmark_Result run_adjacent_FV() { return _run<&Benchmark::_adjacent_FV>("adjacent_FV"); }
    Benchmark_Result run_adjacent_EF() { return _run<&Benchmark::_adjacent_EF>("adjacent_EF"); }


    std::vector<Benchmark_Result> run_all(size_t seed = 1000);

protected:
    template<auto Func>
    Benchmark_Result _run(const std::string& name);
    
    template<auto FuncRand>
    Benchmark_Result _run_random(const std::string& name, const std::vector<size_t>& rand);

    //virtual void _barycenter() = 0;
    //virtual void _normal() = 0;
    //virtual void _smoothing() = 0;
    //virtual void _subdivision() = 0;
    //virtual void _collapse() = 0;
    
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



template<auto Func>
Benchmark_Result Benchmark::_run(const std::string& name) {
    Precision_Timer timer = Precision_Timer(); 

    timer.start();
    (this->*Func)();
    timer.stop();

    return {
        this->_mesh_name,
        name,
        this->_complexity,
        timer.result(),
        my_utils::system::get_memory_usage() - _pre_mesh_mem_usage
    };
}


template<auto FuncRand>
Benchmark_Result Benchmark::_run_random(const std::string& name, const std::vector<size_t>& rand) {
    Precision_Timer timer = Precision_Timer(); 

    timer.start();
    (this->*FuncRand)(rand);
    timer.stop();

    return {
        this->_mesh_name,
        name,
        this->_complexity,
        timer.result(),
        my_utils::system::get_memory_usage() - _pre_mesh_mem_usage
    };
}

#endif