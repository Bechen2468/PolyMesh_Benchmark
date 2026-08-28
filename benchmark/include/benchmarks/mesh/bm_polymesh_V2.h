#ifndef BENCHMARK_POLYMESHV2_H
#define BENCHMARK_POLYMESHV2_H
// ----------------------------------------------------------------------------
//#define POLYMESH_INDEX_64     // 64 bit indexing
//#define POLYMESH_INDEX_SIZET     // dyn 32/64 bit indexing
// ----------------------------------------------------------------------------
#include <cstdint>
#include <string>
#include <PolyMesh/V2.h>
#include "benchmarks/mesh/ibenchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_PolyMesh_V02 : public IBenchmark {
protected:
    using Mesh = PolyMesh::V2::PolyMesh<double, std::uint32_t>;
    Mesh mesh;

protected:
    std::vector<size_t> load_mesh_storage(const std::string& FileName, int full = 0) override {
        mesh.read_file(FileName);
        size_t n_edges = mesh.n_edges();
        if(full) {
            mesh.edges();
            mesh.VV();
            mesh.VF();
            mesh.EF();
            //mesh.FE();
        }
        else {
            mesh.relations().invalidate(PolyMesh::V2::CacheFlag::All);
        }

        return {mesh.n_vertices(), mesh.n_faces(), n_edges};
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

    void _adjacent_VV() override;
    void _adjacent_VF() override;
    void _adjacent_FV() override;
    void _adjacent_EF() override;


    void run_load_all(benchmark::State& state);

    void run_iterate_loaded_E(benchmark::State& state);
    void run_iterate_loaded_random_E(benchmark::State& state);
    void run_adjacent_loaded_VV(benchmark::State& state);
    void run_adjacent_loaded_VF(benchmark::State& state);
    void run_adjacent_loaded_FV(benchmark::State& state);
    void run_adjacent_loaded_EF(benchmark::State& state);

    void _convert_to_trimesh() override {}
};


#endif
