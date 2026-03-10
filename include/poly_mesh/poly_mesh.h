#ifndef POLY_MESH_H
#define POLY_MESH_H
// ----------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <Eigen/Dense>
#include <Eigen/Sparse>
// ----------------------------------------------------------------------------



class PolyMesh {
public:
    typedef Eigen::Vector3f Vertex;
    typedef uint8_t BoolType;

private:
    Eigen::Matrix<float, 3, Eigen::Dynamic, Eigen::RowMajor> _vertices;
    Eigen::SparseMatrix<BoolType> _adjacency;       // VxV adjacency
    Eigen::SparseMatrix<BoolType, Eigen::RowMajor> _face_vertices;   // FxV inclusion
    
    size_t _n_faces = 0;
    size_t _n_edges = 0;
    

public:
    PolyMesh();
    ~PolyMesh();

    void read_file(std::string File_Path);

    size_t add_vertex(const PolyMesh::Vertex& Vert);
    void replace_vertex(size_t Id, const PolyMesh::Vertex& Vert);
    void add_vertices(const std::vector<PolyMesh::Vertex>& Vertices);
    void add_edge(size_t I, size_t J);
    void add_face(const std::vector<int>& Face);

    std::vector<int> get_Neighbors(size_t Vertex_Id) const;
    bool edge_exists(size_t I, size_t J) const;

    inline size_t n_vertices() const { return _vertices.cols(); }
    inline size_t n_faces() const { return _n_faces; }
    inline size_t n_edges() const { return _n_edges; }
    

    inline Vertex vertex(size_t Index) { return _vertices.col(Index); }

    inline Eigen::Matrix<float, 3, Eigen::Dynamic, Eigen::RowMajor>& vertices() { return _vertices; }
    inline Eigen::SparseMatrix<BoolType>& adjacency() {  return _adjacency; }
    inline Eigen::SparseMatrix<BoolType, Eigen::RowMajor>& face_vertices() { return _face_vertices; }
};


#endif
