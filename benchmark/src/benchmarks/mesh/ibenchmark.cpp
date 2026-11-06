#include "benchmarks/mesh/ibenchmark.h"



void IBenchmark::load_meshlist() {
    const std::string path = std::string(ROOT_DIR) + "/off_files";
    if(mesh_paths.size()) return;

    std::filesystem::create_directory(path);
    for(const auto& file : std::filesystem::directory_iterator(path)) {
        if(file.is_regular_file() && file.path().extension() == ".off") {
            mesh_paths.push_back(file.path().string());
        }
    }
    std::sort(mesh_paths.begin(), mesh_paths.end());
    assert(mesh_paths.size() > 0 && "No off files found.");

    //std::cout << std::endl << "All loaded meshs:" << std::endl;
    //for(int i = 0; i < mesh_paths.size(); ++i) {
    //    std::cout << i+1 << ": " << mesh_paths[i] << std::endl;
    //}
    //std::cout << std::endl;
}



void IBenchmark::build_mesh(Mesh_Type Type, size_t Size) {
    const std::string path = std::string(ROOT_DIR) + "/off_files";
    std::filesystem::create_directory(path);
    std::ifstream f(file_path.c_str());
    if(f.good()) return;


    switch(Type) {
        case Rand_Sphere:
        my_utils::off::build_random_sphere_off(Size, file_path);
        break;
        case Quad:
        my_utils::off::build_quad_off(Size, file_path);
        break;
        case Dual:
        my_utils::off::build_dual_off(Size, file_path);
        break;
    }
}




void IBenchmark::run_load(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        mem.measure_pre_run();
        state.ResumeTiming();

        load_mesh_storage(file_path, state.range(1));

        state.PauseTiming();
        mem.measure_post_run();
        //std::cout << "Loading finished.." << std::endl;
        //getchar();
        state.ResumeTiming();
    }
}




void IBenchmark::run_memory_usage(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        mem.measure_pre_run();
        state.ResumeTiming();

        load_mesh_storage(file_path, state.range(1));

        state.PauseTiming();
        mem.measure_post_run();
        std::cout << "Loading finished, read memory usage now.." << std::endl;
        getchar();
        state.ResumeTiming();
    }
}



void IBenchmark::run_iterate_V_raw(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();
        
        _iterate_V_raw();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void IBenchmark::run_iterate_F_raw(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();
        
        _iterate_F_raw();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void IBenchmark::run_iterate_V(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _iterate_V();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void IBenchmark::run_iterate_F(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();
        
        _iterate_F();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void IBenchmark::run_iterate_E(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        state.ResumeTiming();
        
        _iterate_E();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}




void IBenchmark::run_iterate_random_V(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();
        
        _iterate_random_V(_rand_v);

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void IBenchmark::run_iterate_random_F(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();
        
        _iterate_random_F(_rand_f);

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void IBenchmark::run_iterate_random_E(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _iterate_random_E(_rand_e);

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}




void IBenchmark::run_adjacent_VV(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _adjacent_VV();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void IBenchmark::run_adjacent_VF(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _adjacent_VF();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void IBenchmark::run_adjacent_FV(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _adjacent_FV();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void IBenchmark::run_adjacent_EF(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();
        
        _adjacent_EF();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void IBenchmark::run_convert_to_trimesh(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _convert_to_trimesh();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}






void  IBenchmark::run_circular(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _circular();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void  IBenchmark::run_barycenter(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _barycenter();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void  IBenchmark::run_normal(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _normal();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void  IBenchmark::run_smoothing(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _smoothing();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void  IBenchmark::run_subdivision(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _subdivision();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void  IBenchmark::run_collapse(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _collapse();

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(_benchmark_sink);
        state.ResumeTiming();
    }
}


void  IBenchmark::run_render_export(benchmark::State& state) {
    std::vector<float> out_positions;
    std::vector<int> out_indices;

    for (auto _ : state) {
        state.PauseTiming();
        load_mesh_from_storage(state.range(1));
        mem.measure_pre_run();
        state.ResumeTiming();

        _render_export(out_positions, out_indices);

        state.PauseTiming();
        mem.measure_post_run();
        benchmark::DoNotOptimize(out_positions);
        benchmark::DoNotOptimize(out_indices);
        state.ResumeTiming();
    }
}

