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



namespace PolyMesh::V1 {

template<typename T>
class PolyMesh {
public:
    using BoolType    = uint8_t;

    using VertexMatrix = Eigen::Matrix<T, Eigen::Dynamic, 3, Eigen::RowMajor>;
    using Vertex       = Eigen::Vector3<T>;
    using VertexRef    = VertexMatrix::ConstRowXpr; // for iteration
    using VertexMap    = VertexMatrix::RowXpr;      // for mutation
    
    using FaceMatrix = Eigen::Matrix<size_t, Eigen::Dynamic, 3, Eigen::RowMajor>;
    using Face       = Eigen::Vector3<size_t>;
    using FaceRef    = FaceMatrix::ConstRowXpr; // for iteration
    using FaceMap    = FaceMatrix::RowXpr;      // for mutation

    using Edge       = Eigen::Vector2<size_t>;

private:
    VertexMatrix _v;
    FaceMatrix _f;

    std::vector<BoolType> _v_valid;
    std::vector<BoolType> _f_valid;

    size_t _n_vertices = 0;
    size_t _n_faces = 0;
    size_t _n_edges = 0;
    

public:
    PolyMesh();
    ~PolyMesh();

    void read_file(const std::string& File_Path);

    size_t add_vertex(const PolyMesh::Vertex& Vert);
    void replace_vertex(size_t Id, const PolyMesh::Vertex& Vert);
    void add_vertices(const std::vector<PolyMesh::Vertex>& Vertices);

    std::vector<size_t> get_Neighbors(size_t Vertex_Id) const;

    inline size_t n_vertices() const { return _n_vertices; }
    inline size_t n_faces() const    { return _n_faces; }
    inline size_t n_edges() const    { return _n_edges; }

    inline VertexRef vertex(size_t Index) const { return _v.row(Index); }
    inline VertexMap vertex(size_t Index)       { return _v.row(Index); }
    inline FaceRef face(size_t Index) const     { return _f.row(Index); }
    inline FaceMap face(size_t Index)           { return _f.row(Index); }

    inline bool vertex_deleted(size_t Index) const { return _v_valid[Index] == 0; }
    inline bool face_deleted(size_t Index) const   { return _f_valid[Index] == 0; }


    inline VertexMatrix& vertices() { return _v; }
    inline FaceMatrix& faces()      { return _f; }



// ----------------------------------------------------------------------------  
// Iterators
// ----------------------------------------------------------------------------  
public:
    struct VIter {
        const uint8_t* valid;
        const uint8_t* valid_end;
        T* vdata;
        size_t rawId;


        inline size_t operator*() {
            return rawId;
        }


        inline VIter& operator++() {
            do { 
                ++valid; 
                vdata += 3; 
                ++rawId; 
            } 
            while(valid < valid_end && !*valid);
            return *this;
        }


        inline auto point() const {
            return Eigen::Map<const Vertex, Eigen::Unaligned>(vdata);
        }


        inline bool operator!=(const VIter& o) const { return valid != o.valid; }
        inline bool operator==(const VIter& o) const { return valid == o.valid; }
    };

    VIter vertices_begin() {
        const uint8_t* ptr = _v_valid.data();
        const uint8_t* end = ptr + _v_valid.size();
        T* vd = _v.data();
        while(ptr < end && !*ptr) { ++ptr; vd += 3; }
        return {ptr, end, vd, 0};
    }

    VIter vertices_end() {
        const uint8_t* end = _v_valid.data() + _v_valid.size();
        return {end, end, _v.data() + _v.size()};
    }



    

    struct FIter {
        const uint8_t* valid;
        const uint8_t* valid_end;
        size_t* fdata;
        size_t rawId;


        inline size_t operator*() {
            return rawId;
        }


        inline FIter& operator++() {
            do { ++valid; fdata += 3; } while(valid < valid_end && !*valid);
            return *this;
        }


        inline auto face() const {
            return Eigen::Map<const Eigen::Vector3<size_t>, Eigen::Unaligned>(fdata);
        }


        inline bool operator!=(const FIter& o) const { return valid != o.valid; }
        inline bool operator==(const FIter& o) const { return valid == o.valid; }
    };

    FIter faces_begin() {
        const uint8_t* ptr = _f_valid.data();
        const uint8_t* end = ptr + _f_valid.size();
        size_t* fd = _f.data();
        while(ptr < end && !*ptr) { ++ptr; fd += 3; }
        return {ptr, end, fd};
    }

    FIter faces_end() {
        const uint8_t* end = _f_valid.data() + _f_valid.size();
        return {end, end, _f.data() + _f.size()};
    }




    struct EIter {
        const uint8_t*  fvalid;
        const uint8_t*  fvalid_end;
        const size_t*   fdata;      // points to current face's v0
        uint8_t         feId;       // 0, 1, or 2

        inline bool operator!=(const EIter& o) const { 
            return fvalid != o.fvalid || feId != o.feId; 
        }

        // current edge as two vertex indices
        inline size_t v0() const { return fdata[feId]; }
        inline size_t v1() const { return fdata[NEXT[feId]]; }

        inline EIter& operator++() {
            _advance();
            return *this;
        }

    //private:
        static constexpr uint8_t NEXT[3] = {1, 2, 0};

        inline void _advance() {
            feId++;
            _skip_to_valid();
        }

        inline void _skip_to_valid() {
            while(true) {
                // scan remaining edges in current face
                while(feId < 3) {
                    if(fdata[feId] < fdata[NEXT[feId]]) return; // canonical edge
                    feId++;
                }
                // move to next valid face
                feId = 0;
                do {
                    ++fvalid; fdata += 3;
                    if(fvalid >= fvalid_end) return; // hit end
                } while(!*fvalid);
            }
        }
    };

    EIter edges_begin() {
        const uint8_t* fv  = _f_valid.data();
        const uint8_t* fve = fv + _f_valid.size();
        const size_t*  fd  = _f.data();

        // skip to first valid face
        while(fv < fve && !*fv) { ++fv; fd += 3; }

        EIter it{fv, fve, fd, 0};
        // skipping to first canonical edge in that face
        if(fv < fve) it._skip_to_valid();
        // reseting and searching properly
        it.feId = 0;
        it._skip_to_valid();
        return it;
    }

    EIter edges_end() {
        const uint8_t* end = _f_valid.data() + _f_valid.size();
        return {end, end, _f.data() + _f.size(), 0};
    }
};


};
#endif
