#ifndef POLY_MESH_H
#define POLY_MESH_H
// ----------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <Eigen/Dense>
#include <Eigen/Sparse>
// ----------------------------------------------------------------------------



class PolyMesh {
public:
    typedef Eigen::Vector3f Vertex;

private:
    Eigen::Matrix3Xf _v;
    Eigen::SparseMatrix<bool> _adjacency;

public:
    PolyMesh();
    ~PolyMesh();

    void read_file(std::string File_Path);

    int add_vertex(const PolyMesh::Vertex& Vert);
    void change_vertex(const int& Id, const PolyMesh::Vertex& Vert);
    void add_vertices(const std::vector<PolyMesh::Vertex>& Vertices);
    void add_edge(const int& I, const int& J);
    void add_face(const std::vector<int>& Face);

    PolyMesh::Vertex get_vertex(const int& Id) const;
    std::vector<int> get_Neighbors(const int& Vertex_Id) const;
    size_t get_vertex_count() const;
    
    bool edge_exists(const int& I, const int& J) const;

    Eigen::Matrix3Xf& get_v();
    Eigen::SparseMatrix<bool>& get_adjacency();
};


#endif
