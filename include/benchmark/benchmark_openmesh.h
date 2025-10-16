#ifndef BENCHMARK_OPENMESH_H
#define BENCHMARK_OPENMESH_H
// ----------------------------------------------------------------------------
#include <string>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include "./Benchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_OpenMesh : public Benchmark {
    typedef OpenMesh::PolyMesh_ArrayKernelT<>  Mesh;
private:
    Mesh mesh;

public:
    Benchmark_OpenMesh(std::string Off_File_Path, const size_t& Complexity);

protected:
    void barycenter() override;
};

#endif
