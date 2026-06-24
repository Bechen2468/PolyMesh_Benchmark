#include "benchmarks/bm_pmp.h"


void Benchmark_Pmp_Lib__V03::_iterate_V() {
    pmp::Point p(0.0, 0.0, 0.0);

    for (auto v : mesh.vertices()) {
        //p += mesh.position(v);    // causes heap buffer overflow with double precision
        auto pv = mesh.position(v);
        p[0] += pv[0];
        p[1] += pv[1];
        p[2] += pv[2];
    }

    p /= static_cast<pmp::Scalar>(mesh.n_vertices());

    _benchmark_sink += p[0] + p[1] + p[2];
    benchmark::DoNotOptimize(p);
}


void Benchmark_Pmp_Lib__V03::_iterate_F() {
    size_t sink = 0;

    for (auto f : mesh.faces()) {
        for (auto v : mesh.vertices(f)) {
            sink += static_cast<size_t>(v.idx());
        }
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_iterate_E() {
    size_t sink = 0;

    for (auto e : mesh.edges()) {
        auto h0 = mesh.halfedge(e, 0);
        auto h1 = mesh.halfedge(e, 1);

        sink += static_cast<size_t>(mesh.to_vertex(h0).idx());
        sink += static_cast<size_t>(mesh.to_vertex(h1).idx());
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_iterate_random_V(const std::vector<size_t>& rand) {
    pmp::Point p(0.0, 0.0, 0.0);

    for (size_t vi : rand) {
        auto vh = pmp::Vertex(static_cast<pmp::IndexType>(vi));
        p += mesh.position(vh);
    }

    _benchmark_sink += (p[0] + p[1] + p[2]) * 1.0000001;
    benchmark::DoNotOptimize(p);
}


void Benchmark_Pmp_Lib__V03::_iterate_random_F(const std::vector<size_t>& rand) {
    size_t sink = 0;

    for (size_t idx : rand) {
        auto fh = pmp::Face(static_cast<pmp::IndexType>(idx));
        for (auto v : mesh.vertices(fh)) {
            sink += static_cast<size_t>(v.idx());
        }
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_iterate_random_E(const std::vector<size_t>& rand) {
    size_t sink = 0;

    for (size_t idx : rand) {
        auto eh = pmp::Edge(static_cast<unsigned int>(idx));

        auto h0 = mesh.halfedge(eh, 0);
        auto h1 = mesh.halfedge(eh, 1);

        sink += static_cast<size_t>(mesh.to_vertex(h0).idx());
        sink += static_cast<size_t>(mesh.to_vertex(h1).idx());
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_adjacent_VV() {
    size_t sink = 0;

    for (auto v : mesh.vertices()) {
        for (auto vv : mesh.vertices(v)) {
            sink += static_cast<size_t>(vv.idx());
        }
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_adjacent_VF() {
    size_t sink = 0;

    for (auto v : mesh.vertices()) {
        for (auto f : mesh.faces(v)) {
            sink += static_cast<size_t>(f.idx());
        }
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_adjacent_FV() {
    size_t sink = 0;

    for (auto f : mesh.faces()) {
        for (auto v : mesh.vertices(f)) {
            sink += static_cast<size_t>(v.idx());
        }
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_adjacent_EF() {
    size_t sink = 0;

    for (auto e : mesh.edges()) {
        auto h0 = mesh.halfedge(e, 0);
        auto h1 = mesh.halfedge(e, 1);

        auto f0 = mesh.face(h0);
        auto f1 = mesh.face(h1);

        if (f0.is_valid()) sink += static_cast<size_t>(f0.idx());
        if (f1.is_valid()) sink += static_cast<size_t>(f1.idx());
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}