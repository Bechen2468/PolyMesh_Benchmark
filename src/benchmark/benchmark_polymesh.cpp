#include "benchmark/benchmark_polymesh.h"



Benchmark_PolyMesh::Benchmark_PolyMesh(std::string Off_File_Path, const size_t& Complexity):
Benchmark("polymesh", Complexity)
{
    mesh.read_file(Off_File_Path);
}



void Benchmark_PolyMesh::barycenter() {
    PolyMesh::Vertex p(0,0,0);                    // Barycenter point

    int cols = mesh.get_vertex_count();
    p[0] = mesh.get_v().row(0).sum() / cols;
    p[1] = mesh.get_v().row(1).sum() / cols;
    p[2] = mesh.get_v().row(2).sum() / cols;
    

    for(int i = 0; i < mesh.get_vertex_count(); ++i) {
        mesh.get_v().col(i) -= p;
    }
}
