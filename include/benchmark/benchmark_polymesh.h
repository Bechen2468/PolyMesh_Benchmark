#ifndef BENCHMARK_POLYMESH_H
#define BENCHMARK_POLYMESH_H
// ----------------------------------------------------------------------------
#include <string>
#include "poly_mesh/poly_mesh.h"
#include "benchmark/benchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_PolyMesh : public Benchmark {
    typedef PolyMesh Mesh;
private:
    Mesh mesh;

public:
    Benchmark_PolyMesh(std::string Off_File_Path, const size_t& Complexity);
    
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
