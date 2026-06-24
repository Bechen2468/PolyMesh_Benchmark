#ifndef BENCHMARK_OPENMESH_H
#define BENCHMARK_OPENMESH_H
// ----------------------------------------------------------------------------
#include <string>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <Eigen/Dense>
#include "benchmarks/ibenchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_OpenMesh_V11 : public IBenchmark {
protected:
    struct MyTraits : public OpenMesh::DefaultTraits {
        typedef OpenMesh::Vec3d Point;
        typedef OpenMesh::Vec3d Normal;
        //typedef OpenMesh::Vec3f Point;
        //typedef OpenMesh::Vec3f Normal;
    };

    using Mesh = OpenMesh::PolyMesh_ArrayKernelT<MyTraits>;
    Mesh mesh;

protected:
    std::vector<size_t> load_mesh(const std::string& FileName, bool fully_loaded) override {
        OpenMesh::IO::read_mesh(mesh, FileName);
        //static_assert(sizeof(Mesh::Scalar) == 8, "OpenMesh is not using double!");
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
