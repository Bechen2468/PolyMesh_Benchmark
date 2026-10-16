#ifndef BENCHMARK_POLYMESHV3_H
#define BENCHMARK_POLYMESHV3_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include <string>
#include <PolyMesh/V3.h>
#include "benchmarks/mesh/ibenchmark.h"
// ----------------------------------------------------------------------------


template<PolyMesh::V3::TraitsC TraitsT>
class Benchmark_PolyMesh_V03 : public IBenchmark {
protected:
    //using Mesh = PolyMesh::V3::PolyMesh<PolyMesh::V3::Traits<float, std::uint32_t, std::uint8_t>>;
    using Mesh = PolyMesh::V3::PolyMesh<TraitsT>;
    Mesh mesh;
    Mesh mesh_storage;
    bool VALIDATE_MESH = false;

    typename Mesh::Vec<typename Mesh::T::Vertices::Vertex> vnormals;
    typename Mesh::Vec<typename Mesh::T::Vertices::Vertex> fnormals;

protected:
    std::vector<size_t> load_mesh_storage(const std::string& FileName, int load_full = 0) override {
        mesh_storage = Mesh();
        mesh_storage.read_file(FileName);
        
        // 1 for load full,  10 for run validation checks
        if(load_full == 1 || load_full == 10 || load_full == 2) mesh_storage.VV_HE();
        if(load_full == 1 || load_full == 10 || load_full == 3) mesh_storage.VF();
        if(load_full == 1 || load_full == 10 || load_full == 4) mesh_storage.EF();
        if(load_full == 0) mesh_storage.relations().invalidate(PolyMesh::V3::CacheFlag::ALL);

        // setting validation check flag
        VALIDATE_MESH = load_full == 10;

        return {mesh_storage.n_vertices(), mesh_storage.n_faces(), 0};
    }


    void load_mesh_from_storage(int load_full) override {
        mesh = mesh_storage;
        //mesh.set_validate_modifications(false);

        // 1 for load full,  10 for run validation checks
        if(load_full == 1 || load_full == 10 || load_full == 2) mesh.VV_HE();
        if(load_full == 1 || load_full == 10 || load_full == 3) mesh.VF();
        if(load_full == 1 || load_full == 10 || load_full == 4) mesh.EF();
        if(load_full == 0) mesh.relations().invalidate(PolyMesh::V3::CacheFlag::ALL);
    }

    
    void destroy_mesh() override {
        mesh = Mesh();
        mesh_storage = Mesh();
    }


    void _iterate_V_raw() override;
    void _iterate_F_raw() override;

    void _iterate_V() override;
    void _iterate_F() override;
    void _iterate_E() override {}
    
    void _iterate_random_V(const std::vector<size_t>& rand) override {}
    void _iterate_random_F(const std::vector<size_t>& rand) override {}
    void _iterate_random_E(const std::vector<size_t>& rand) override {}

    void _adjacent_VV() override {}
    void _adjacent_VF() override {}
    void _adjacent_FV() override {}
    void _adjacent_EF() override {}

    void _convert_to_trimesh() override;



    
    // real algorithms
    void _circular() override;
    void _barycenter() override;
    void _normal() override;
    void _smoothing() override;
    void _subdivision() override;
    void _collapse() override;
    void _render_export(std::vector<float>& out_positions, std::vector<int>& out_indices) override;


    void _temp_collapse_vertex(Mesh::Index From, Mesh::Index To);
};


#include "benchmarks/mesh/bm_polymesh_V3.inl"
#endif
