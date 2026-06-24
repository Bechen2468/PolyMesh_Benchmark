#include "benchmarks/ibenchmark.h"



void IBenchmark::build_mesh(const std::string& FileName, size_t Size) {
    std::ifstream f(FileName.c_str());
    if(f.good()) return;

    my_utils::off::build_random_sphere_off(Size, FileName);
    //my_utils::off::build_sphere_off(Size, FileName);
}


void IBenchmark::run_load(benchmark::State& state) {
    for (auto _ : state) {
        const std::string fileName = "./mesh_convex_" + std::to_string(state.range(0)) + ".off";
        load_mesh(fileName, false);
    }
}


void IBenchmark::run_iterate_V_raw(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_V_raw();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


void IBenchmark::run_iterate_F_raw(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_F_raw();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


void IBenchmark::run_iterate_V(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_V();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


void IBenchmark::run_iterate_F(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_F();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


void IBenchmark::run_iterate_E(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_E();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}




void IBenchmark::run_iterate_random_V(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_random_V(_rand_v);
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


void IBenchmark::run_iterate_random_F(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_random_F(_rand_f);
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


void IBenchmark::run_iterate_random_E(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_random_E(_rand_e);
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}




void IBenchmark::run_adjacent_VV(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_VV();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


void IBenchmark::run_adjacent_VF(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_VF();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


void IBenchmark::run_adjacent_FV(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_FV();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


void IBenchmark::run_adjacent_EF(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_EF();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}

