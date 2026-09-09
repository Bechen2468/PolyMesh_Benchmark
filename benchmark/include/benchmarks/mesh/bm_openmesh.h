#ifndef BENCHMARK_OPENMESH_H
#define BENCHMARK_OPENMESH_H
// ----------------------------------------------------------------------------
#include <string>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <Eigen/Dense>
#include "benchmarks/mesh/ibenchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_OpenMesh_V11 : public IBenchmark {
protected:
    struct MyTraits : public OpenMesh::DefaultTraits {
        //typedef OpenMesh::Vec3d Point;
        //typedef OpenMesh::Vec3d Normal;
        typedef OpenMesh::Vec3f Point;
        typedef OpenMesh::Vec3f Normal;
    };

    
    //using Mesh = OpenMesh::PolyMesh_ArrayKernelT<MyTraits>;
    using Mesh = OpenMesh::TriMesh_ArrayKernelT<MyTraits>;

    Mesh mesh;
    Mesh mesh_storage;

protected:
    std::vector<size_t> load_mesh_storage(const std::string& FileName, int load_full = 0) override {
        mesh.clear();
        mesh_storage.clear();
        OpenMesh::IO::read_mesh(mesh_storage, FileName);
        
        if(load_full) {
            mesh_storage.request_vertex_status();
            mesh_storage.request_edge_status();
            mesh_storage.request_face_status();
            mesh_storage.request_vertex_normals();
            mesh_storage.request_face_normals();
        }

        //static_assert(sizeof(Mesh::Scalar) == 8, "OpenMesh is not using double!");
        return {mesh_storage.n_vertices(), mesh_storage.n_faces(), mesh_storage.n_edges()};
    }

    void load_mesh_from_storage(int load_full) override {
        mesh = mesh_storage;
        
        mesh.request_vertex_status();
        mesh.request_edge_status();
        mesh.request_face_status();
        mesh.request_vertex_normals();
        mesh.request_face_normals();
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
