#ifndef BENCHMARK_PMP_H
#define BENCHMARK_PMP_H
// ----------------------------------------------------------------------------
// MOVED TO CMAKE ( not post compile compatable )
//#define PMP_SCALAR_TYPE_64  // double precision
//#define PMP_INDEX_TYPE_64   // 64 bit indexing
// ----------------------------------------------------------------------------
#include <string>
#include <pmp/surface_mesh.h>
#include <pmp/io/io.h>
#include "benchmarks/mesh/ibenchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_Pmp_Lib__V03 : public IBenchmark {
protected:
    using Mesh = pmp::SurfaceMesh;
    Mesh mesh;
    Mesh mesh_storage;
    
    pmp::VertexProperty<pmp::Point> points;
    pmp::VertexProperty<pmp::Point> vnormals;
    pmp::FaceProperty<pmp::Point>   fnormals;


protected:
    std::vector<size_t> load_mesh_storage(const std::string& FileName, int load_full = 0) override {
        mesh.clear();
        mesh_storage.clear();
        pmp::read(mesh_storage, FileName);
        points   = mesh.vertex_property<pmp::Point>("v:point");
        vnormals = mesh.vertex_property<pmp::Point>("v:normal");
        fnormals = mesh.face_property<pmp::Point>("f:normal");
        
        //static_assert(sizeof(pmp::Scalar) == 8, "PMP is not using double!");
        return {mesh.n_vertices(), mesh.n_faces(), mesh.n_edges()};
    }

    void load_mesh_from_storage(int load_full) override {
        mesh = mesh_storage;
        points   = mesh.vertex_property<pmp::Point>("v:point");
        vnormals = mesh.vertex_property<pmp::Point>("v:normal");
        fnormals = mesh.face_property<pmp::Point>("f:normal");
    }


    void destroy_mesh() override {
        mesh = Mesh();
        mesh_storage = Mesh();
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

    void _convert_to_trimesh() override;

    
    
    // real algorithms
    void _circular() override;
    void _barycenter() override;
    void _normal() override;
    void _smoothing() override;
    void _subdivision() override;
    void _collapse() override;
    void _render_export(std::vector<float>& out_positions, std::vector<int>& out_indices) override;
};
#endif
