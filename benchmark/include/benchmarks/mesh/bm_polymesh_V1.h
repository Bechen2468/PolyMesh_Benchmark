#ifndef BENCHMARK_POLYMESHV1_H
#define BENCHMARK_POLYMESHV1_H
// ----------------------------------------------------------------------------
#include <string>
#include <PolyMesh/V1.h>
#include "benchmarks/mesh/ibenchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_PolyMesh_V01 : public IBenchmark {
protected:
    using Mesh = PolyMesh::V1::PolyMesh<float>;
    Mesh mesh;

protected:
    std::vector<size_t> load_mesh_storage(const std::string& FileName, int full = 0) override {
        mesh.read_file(FileName);
        return {mesh.n_vertices(), mesh.n_faces(), mesh.n_edges()};
    }

    void load_mesh_from_storage(int load_full) override {}
    
    void destroy_mesh() override {
        mesh = Mesh();
    }

    
    void _iterate_V_raw() override;
    void _iterate_F_raw() override;

    void _iterate_V() override;
    void _iterate_F() override;
    void _iterate_E() override;
    
    void _iterate_random_V(const std::vector<size_t>& rand) override;
    void _iterate_random_F(const std::vector<size_t>& rand) override;
    void _iterate_random_E(const std::vector<size_t>& rand) override;

    void _adjacent_VV() override {}
    void _adjacent_VF() override {}
    void _adjacent_FV() override {}
    void _adjacent_EF() override {}
    
    void _convert_to_trimesh() override {}
};


#endif
