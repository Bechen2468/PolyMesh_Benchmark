#include "benchmarks/bm_polymesh_V2.h"




void Benchmark_PolyMesh_V02::_iterate_V_raw() {
    Mesh::Vertex p(0,0,0);                    // Barycenter point

    for (size_t i = 0; i < mesh.n_vertices(); ++i) {
        //const auto& v = mesh.vertex(i);
        auto v = mesh.vertex(i);
        p[0] += v[0];       // DEVNOTE: Eigen::Vector.operator+=() is a templated call, so its sadly slow af ... 
        p[1] += v[1];
        p[2] += v[2];
        //p += mesh.vertex(i);
    }
    
    _benchmark_sink += p[0] + p[1] + p[2];  // volatile write — compiler must emit the loop
    benchmark::DoNotOptimize(p);
}


void Benchmark_PolyMesh_V02::_iterate_F_raw() {
    size_t sink = 0;

    for(size_t fi = 0; fi < mesh.n_faces(); ++fi) {
        const auto& f = mesh.face(fi);
        for(size_t v = 0; v < f.size; ++v) {
            sink += f[v];
        }
    }

    _benchmark_sink += sink;  // volatile write — compiler must emit the loop
    benchmark::DoNotOptimize(sink);
}



void Benchmark_PolyMesh_V02::_iterate_V() {
    Mesh::VIter vi;                          // Vertex index
    Mesh::VIter vend = mesh.vertices_end();  // Vertex index bounds
    Mesh::Vertex p(0,0,0);                // Barycenter point

    for(vi = mesh.vertices_begin(); vi != vend; ++vi) {
        auto v = vi.vertex();
        //auto v = mesh.vertex(*vi);    // *vi == rawindex, but slower with the extra vertex() call which does not return unaligned
        p[0] += v[0];       // DEVNOTE: Eigen::Vector.operator+=() is a templated call, so its sadly slow af ... 
        p[1] += v[1];
        p[2] += v[2];
    }

    p /= mesh.n_vertices();
    _benchmark_sink += p[0] + p[1] + p[2];
    benchmark::DoNotOptimize(p);
}


void Benchmark_PolyMesh_V02::_iterate_F() {
    Mesh::FIter fi;
    Mesh::FIter fend = mesh.faces_end();
    size_t sink = 0;

    for(fi = mesh.faces_begin(); fi != fend; ++fi) {
        const auto& f = mesh.face(*fi);
        for(size_t v = 0; v < f.size; ++v) {
            sink += f[v];
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_PolyMesh_V02::_iterate_E() {
    Mesh::EIter ei = mesh.edges_begin();
    Mesh::EIter eend = mesh.edges_end();
    size_t sink = 0;
    for(; ei != eend; ++ei) {
        sink += ei.v0();
        sink += ei.v1();
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_PolyMesh_V02::_iterate_random_V(const std::vector<size_t>& rand) {
    Mesh::Vertex p(0,0,0); 

    for(size_t idx : rand) {
        const auto& v = mesh.vertex(idx);
        p[0] += v[0];
        p[1] += v[1];
        p[2] += v[2];
    }

    _benchmark_sink += (p[0] + p[1] + p[2]) * 1.0000001;
    benchmark::DoNotOptimize(p);
}


void Benchmark_PolyMesh_V02::_iterate_random_F(const std::vector<size_t>& rand) {
    size_t sink = 0;

    for(size_t idx : rand) {
        const auto& f = mesh.face(idx);
        sink += f[0];
        sink += f[1];
        sink += f[2];
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_PolyMesh_V02::_iterate_random_E(const std::vector<size_t>& rand) {
    size_t sink = 0;
    
    for(size_t idx : rand) {
        size_t fi = idx / 3;
        uint8_t ei = idx % 3;
        const auto& f = mesh.face(fi);
        sink += f[ei] + f[(ei + 1) % 3];  // only half-edge possible without edgeiterationmatrix
    }
    
    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}



void Benchmark_PolyMesh_V02::_adjacent_VV() {
    size_t sink = 0;
    auto vend = mesh.vertices_end();
    
    for(auto vi = mesh.vertices_begin(); vi != vend; ++vi) {
        for(auto vv = mesh.vv_begin(vi.rawId); vv; ++vv) {
            sink += *vv;
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_PolyMesh_V02::_adjacent_VF() {
    size_t sink = 0;
    Mesh::VIter vi;
    Mesh::VIter vend = mesh.vertices_end();

    for(vi = mesh.vertices_begin(); vi != vend; ++vi) {
        for(auto vf = mesh.vf_begin(*vi); vf; ++vf) {
            sink += *vf;
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}



void Benchmark_PolyMesh_V02::_adjacent_FV() {
    size_t sink = 0;
    Mesh::FIter fi;
    Mesh::FIter fend = mesh.faces_end();

    for(fi = mesh.faces_begin(); fi != fend; ++fi) {
        for(auto fv = mesh.fv_begin(*fi); fv != mesh.fv_end(*fi); ++fv) {
            sink += *fv;
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_PolyMesh_V02::_adjacent_EF() {
    size_t sink = 0;
    Mesh::EIter ei = mesh.edges_begin();
    Mesh::EIter eend = mesh.edges_end();
    
    for(; ei != eend; ++ei) {
        for(auto ef = mesh.ef_begin(*ei); ef; ++ef) {
            sink += *ef;
        }
        sink += *ei;
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}




void Benchmark_PolyMesh_V02::run_load_all(benchmark::State& state) {
    for (auto _ : state) {
        const std::string fileName = "./mesh_convex_" + std::to_string(state.range(0)) + ".off";
        load_mesh(fileName, true);
    }
}


void Benchmark_PolyMesh_V02::run_iterate_loaded_E(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_E();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


void Benchmark_PolyMesh_V02::run_iterate_loaded_random_E(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_random_E(_rand_e);
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}



void Benchmark_PolyMesh_V02::run_adjacent_loaded_VV(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_VV();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}



void Benchmark_PolyMesh_V02::run_adjacent_loaded_VF(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_VF();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}



void Benchmark_PolyMesh_V02::run_adjacent_loaded_FV(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_FV();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}



void Benchmark_PolyMesh_V02::run_adjacent_loaded_EF(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_EF();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}