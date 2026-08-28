#include "benchmarks/mesh/bm_polymesh_V1.h"




void Benchmark_PolyMesh_V01::_iterate_V_raw() {
    Mesh::Vertex p(0,0,0);                    // Barycenter point

    for (size_t i = 0; i < mesh.n_vertices(); ++i) {
        const auto& v = mesh.vertex(i);
        p[0] += v[0];       // DEVNOTE: Eigen::Vector.operator+=() is a templated call, so its sadly slow af ... 
        p[1] += v[1];
        p[2] += v[2];
        //p += v;
    }
    
    _benchmark_sink += p[0] + p[1] + p[2];  // volatile write — compiler must emit the loop
    benchmark::DoNotOptimize(p);
}


void Benchmark_PolyMesh_V01::_iterate_F_raw() {
    size_t sink = 0;

    for(size_t fi = 0; fi < mesh.n_faces(); ++fi) {
        const auto& f = mesh.face(fi);
        sink += f[0];
        sink += f[1];
        sink += f[2];
    }

    _benchmark_sink += sink;  // volatile write — compiler must emit the loop
    benchmark::DoNotOptimize(sink);
}



void Benchmark_PolyMesh_V01::_iterate_V() {
    Mesh::VIter vi;                          // Vertex index
    Mesh::VIter vend = mesh.vertices_end();  // Vertex index bounds
    Mesh::Vertex p(0,0,0);                // Barycenter point

    for(vi = mesh.vertices_begin(); vi != vend; ++vi) {
        const auto& v = vi.point();
        //auto v = mesh.vertex(*vi);    // *vi == rawindex, but slower with the extra vertex() call which does not return unaligned
        p[0] += v[0];       // DEVNOTE: Eigen::Vector.operator+=() is a templated call, so its sadly slow af ... 
        p[1] += v[1];
        p[2] += v[2];
    }

    p /= mesh.n_vertices();
    _benchmark_sink += p[0] + p[1] + p[2];
    benchmark::DoNotOptimize(p);
}


void Benchmark_PolyMesh_V01::_iterate_F() {
    Mesh::FIter fi;
    Mesh::FIter fend = mesh.faces_end();
    size_t sink = 0;

    for(fi = mesh.faces_begin(); fi != fend; ++fi) {
        const auto& f = fi.face();
        sink += f[0];
        sink += f[1];
        sink += f[2];
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_PolyMesh_V01::_iterate_E() {
    Mesh::EIter ei;
    Mesh::EIter eend = mesh.edges_end();
    size_t sink = 0;

    for(ei = mesh.edges_begin(); ei != eend; ++ei) {
        sink += ei.v0();
        sink += ei.v1();
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_PolyMesh_V01::_iterate_random_V(const std::vector<size_t>& rand) {
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


void Benchmark_PolyMesh_V01::_iterate_random_F(const std::vector<size_t>& rand) {
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


void Benchmark_PolyMesh_V01::_iterate_random_E(const std::vector<size_t>& rand) {
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




