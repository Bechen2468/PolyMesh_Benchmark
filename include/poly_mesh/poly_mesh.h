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
    std::vector<BoolType> _vertices_deleted;

    Eigen::Matrix<size_t, 3, Eigen::Dynamic> _faces;
    std::vector<BoolType> _faces_deleted;

    size_t _n_faces = 0;
    size_t _n_edges = 0;


    // fast translations
    // only generated when required first

    // edge list
    Eigen::Matrix<size_t, 2, Eigen::Dynamic> _edges;                // ExV
    
    bool _edges_build;

    Eigen::SparseMatrix<BoolType> _adjacency;                       // VxV adjacency
    Eigen::SparseMatrix<BoolType, Eigen::RowMajor> _face_vertices;  // FxV inclusion
    
    

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
    inline bool vertex_deleted(size_t Index) const { return _vertices_deleted[Index]; }
    inline bool face_deleted(size_t Index) const { return _faces_deleted[Index]; }


    inline Eigen::Matrix<float, 3, Eigen::Dynamic, Eigen::RowMajor>& vertices() { return _vertices; }
    inline Eigen::SparseMatrix<BoolType>& adjacency() {  return _adjacency; }
    inline Eigen::SparseMatrix<BoolType, Eigen::RowMajor>& face_vertices() { return _face_vertices; }


    Eigen::Matrix<size_t, 2, Eigen::Dynamic> edges();


private:
    void _build_edges();





// ----------------------------------------------------------------------------  
// Iterators
// ----------------------------------------------------------------------------  
public:
    struct VIter {
    private:
        const PolyMesh* _mesh;
        size_t id;

    public:
        VIter(const PolyMesh* Mesh, size_t Id);
        ~VIter();

        inline size_t operator*() const { return id; }
        inline bool operator!=(const VIter& Other) const { return id != Other.id; }

        inline VIter& operator++();
     
    private:
        void _skip_deleted();
    };

    VIter vertices_begin() { return VIter(this, 0); }
    VIter vertices_end() { return VIter(this, n_vertices()); }


    

    struct FIter {
    private:
        const PolyMesh* _mesh;
        size_t id;

    public:
        FIter(const PolyMesh* Mesh, size_t Id);
        ~FIter();

        inline size_t operator*() const { return id; }
        inline bool operator!=(const FIter& Other) const { return id != Other.id; }

        inline FIter& operator++();
     
    private:
        void _skip_deleted();
    };

    FIter faces_begin() { return FIter(this, 0); }
    FIter faces_end() { return FIter(this, n_vertices()); }
};


#endif
