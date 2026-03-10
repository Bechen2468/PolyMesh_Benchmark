#include "benchmark/benchmark_polymesh.h"



Benchmark_PolyMesh::Benchmark_PolyMesh(std::string Off_File_Path, const size_t& Complexity):
Benchmark("polymesh", Complexity)
{
    mesh.read_file(Off_File_Path);
}




void Benchmark_PolyMesh::_iterate_V() {
    PolyMesh::Vertex p(0,0,0);                    // Barycenter point

    const size_t cols = mesh.n_vertices();
    p[0] = mesh.vertices().row(0).sum() / cols;
    p[1] = mesh.vertices().row(1).sum() / cols;
    p[2] = mesh.vertices().row(2).sum() / cols;

    _benchmark_sink += (p[0] + p[1] + p[2]) * 1.0000001;


    // for (size_t i = 0; i < mesh.n_vertices(); ++i) {
    //     const auto& v = mesh.vertices().col(i);
    //     p[0] += v[0];
    //     p[1] += v[1];
    //     p[2] += v[2];
    // }
}


void Benchmark_PolyMesh::_iterate_F() {
    double sink = 0;

    for(size_t fi = 0; fi < mesh.n_faces(); ++fi) {
        for (Eigen::SparseMatrix<Mesh::BoolType, Eigen::RowMajor>::InnerIterator it(mesh.face_vertices(), fi); it; ++it) {
            sink += it.col();  // lightweight work, like fv->idx()
        }
    }

    _benchmark_sink += sink * 1.0000001;
}


void Benchmark_PolyMesh::_iterate_E() {
    double sink = 0;
    size_t nV = mesh.n_vertices();

    // loop over all edges (upper triangle to avoid double counting)
    for (size_t i = 0; i < nV; ++i) {
        for (Eigen::SparseMatrix<Mesh::BoolType>::InnerIterator it(mesh.adjacency(), i); it; ++it) {
            size_t j = it.row();  // neighbor vertex

            // upper triangle only
            if (i < j) {          
                sink += i + j;
            }
        }
    }

    _benchmark_sink += sink * 1.0000001;
}



void Benchmark_PolyMesh::_iterate_random_V(const std::vector<size_t>& rand) {

}


void Benchmark_PolyMesh::_iterate_random_F(const std::vector<size_t>& rand) {

}


void Benchmark_PolyMesh::_iterate_random_E(const std::vector<size_t>& rand) {

}



void Benchmark_PolyMesh::_adjacent_VV() {

}


void Benchmark_PolyMesh::_adjacent_VF() {

}


void Benchmark_PolyMesh::_adjacent_FV() {

}


void Benchmark_PolyMesh::_adjacent_EF() {

}

