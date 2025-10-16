#ifndef BENCHMARK_POLYMESH_H
#define BENCHMARK_POLYMESH_H
// ----------------------------------------------------------------------------
#include <string>
#include "poly_mesh/poly_mesh.h"
#include "./Benchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_PolyMesh : public Benchmark {
    typedef PolyMesh  Mesh;
private:
    Mesh mesh;

public:
    Benchmark_PolyMesh(std::string Off_File_Path, const size_t& Complexity);

protected:
    void barycenter() override;
};

#endif
