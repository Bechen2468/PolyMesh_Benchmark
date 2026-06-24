#include "benchmarks/bm_openmesh.h"


void Benchmark_OpenMesh_V11::_iterate_V() {
    Mesh::VIter vi;                          // Vertex index
    Mesh::VIter vend = mesh.vertices_end();  // Vertex index bounds
    OpenMesh::Vec3d p(0,0,0);                // Barycenter point

    for(vi = mesh.vertices_begin(); vi != vend; ++vi) {
        p += mesh.point(*vi);
    }

    p /= mesh.n_vertices();
    _benchmark_sink += p[0] + p[1] + p[2];
    benchmark::DoNotOptimize(p);
}


void Benchmark_OpenMesh_V11::_iterate_F() {
    Mesh::FIter fi;
    Mesh::FIter fend = mesh.faces_end();
    size_t sink = 0;

    for(fi = mesh.faces_begin(); fi != fend; ++fi) {
        for(Mesh::FVIter fv = mesh.fv_iter(*fi); fv.is_valid(); ++fv) {
            sink += fv->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_OpenMesh_V11::_iterate_E() {
    Mesh::EIter ei;
    Mesh::EIter eend = mesh.edges_end();
    size_t sink = 0;

    for(ei = mesh.edges_begin(); ei != eend; ++ei) {
        for(Mesh::EVIter ev = mesh.ev_iter(*ei); ev.is_valid(); ++ev) {
            sink += ev->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}



void Benchmark_OpenMesh_V11::_iterate_random_V(const std::vector<size_t>& rand) {
    OpenMesh::Vec3d p(0,0,0);

    for(size_t vi : rand) {
        auto vh = Mesh::VertexHandle(vi);
        p += mesh.point(vh);
    }

    _benchmark_sink += (p[0] + p[1] + p[2]) * 1.0000001;
    benchmark::DoNotOptimize(p);
}


void Benchmark_OpenMesh_V11::_iterate_random_F(const std::vector<size_t>& rand) {
    size_t sink = 0;

    for(size_t idx : rand) {
        auto fh = Mesh::FaceHandle(idx);
        for(Mesh::FVIter fv = mesh.fv_iter(fh); fv.is_valid(); ++fv) {
            sink += fv->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_OpenMesh_V11::_iterate_random_E(const std::vector<size_t>& rand) {
    size_t sink = 0;

    for(size_t idx : rand) {
        auto eh = Mesh::EdgeHandle(idx);
        for(Mesh::EVIter ev = mesh.ev_iter(eh); ev.is_valid(); ++ev) {
            sink += ev->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}



void Benchmark_OpenMesh_V11::_adjacent_VV() {
    size_t sink = 0;

    for(auto v : mesh.vertices()) {
        for(Mesh::VVIter vv = mesh.vv_iter(v); vv.is_valid(); ++vv) {
            sink += vv->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_OpenMesh_V11::_adjacent_VF() {
    size_t sink = 0;

    for(auto v : mesh.vertices()) {
        for(Mesh::VFIter vf = mesh.vf_iter(v); vf.is_valid(); ++vf) {
            sink += vf->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_OpenMesh_V11::_adjacent_FV() {
    size_t sink = 0;

    for(auto f : mesh.faces()) {
        for(Mesh::FVIter fv = mesh.fv_iter(f); fv.is_valid(); ++fv) {
            sink += fv->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_OpenMesh_V11::_adjacent_EF() {
    size_t sink = 0;

    for(auto e : mesh.edges()) {
        for(Mesh::EFIter ef = mesh.ef_iter(e); ef.is_valid(); ++ef) {
            sink += ef->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}
