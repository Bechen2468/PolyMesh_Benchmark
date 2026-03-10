#ifndef BENCHMARK_OPENMESH_H
#define BENCHMARK_OPENMESH_H
// ----------------------------------------------------------------------------
#include <string>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <Eigen/Dense>
#include "benchmark/benchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_OpenMesh : public Benchmark {
    typedef OpenMesh::PolyMesh_ArrayKernelT<> Mesh;
private:
    Mesh mesh;

public:
    Benchmark_OpenMesh(std::string Off_File_Path, const size_t& Complexity);
    
    inline size_t count_V() const override { return mesh.n_vertices(); }
    inline size_t count_F() const override { return mesh.n_faces(); }
    inline size_t count_E() const override { return mesh.n_edges(); }

protected:
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
