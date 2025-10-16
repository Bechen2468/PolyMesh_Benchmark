#include "benchmark/benchmark_openmesh.h"


Benchmark_OpenMesh::Benchmark_OpenMesh(std::string Off_File_Path, const size_t& Complexity):
Benchmark("openmesh", Complexity)
{
    OpenMesh::IO::read_mesh(mesh, Off_File_Path);
}


void Benchmark_OpenMesh::barycenter() {
    Mesh::VIter vi;                          // Vertex index
    Mesh::VIter vend = mesh.vertices_end();  // Vertex index bounds
    OpenMesh::Vec3d p(0,0,0);                    // Barycenter point

    for (vi = mesh.vertices_begin(); vi != vend; ++vi)
        p += mesh.point(vi);

    p /= mesh.n_vertices();

    for (vi = mesh.vertices_begin(); vi != vend; ++vi)
        mesh.point(vi) -= p;
}
