#ifndef BENCHMARK_PMP_H
#define BENCHMARK_PMP_H
// ----------------------------------------------------------------------------
#define PMP_SCALAR_TYPE_64  // double precision
//#define PMP_INDEX_TYPE_64   // 64 bit indexing
// ----------------------------------------------------------------------------
#include <string>
#include <pmp/surface_mesh.h>
#include <pmp/io/io.h>
#include "benchmarks/ibenchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_Pmp_Lib__V03 : public IBenchmark {
protected:
    using Mesh = pmp::SurfaceMesh;
    Mesh mesh;

protected:
    std::vector<size_t> load_mesh(const std::string& FileName, bool fully_loaded) override {
        pmp::read(mesh, FileName);
        //static_assert(sizeof(pmp::Scalar) == 8, "PMP is not using double!");
        return {mesh.n_vertices(), mesh.n_faces(), mesh.n_edges()};
    }

    void destroy_mesh() override {
        mesh = Mesh();
    }

    
    void _iterate_V_raw() override {}
    void _iterate_F_raw() override {}
    
    void _iterate_V() override;
    void _iterate_F() override;
    void _iterate_E() override;
    
    void _iterate_random_V(const std::vector<size_t>& rand) override;
    void _iterate_random_F(const std::vector<size_t>& rand) override;
    void _iterate_random_E(const std::vector<size_t>& rand) override;

    void _adjacent_VV() override;
    void _adjacent_VF() override;
    void _adjacent_FV() override;
    void _adjacent_EF() override;
};
#endif
