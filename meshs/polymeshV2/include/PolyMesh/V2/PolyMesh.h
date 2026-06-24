#ifndef POLY_MESH_V2_H
#define POLY_MESH_V2_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include <cstddef>
// ----------------------------------------------------------------------------
//#ifndef(size_t)
//#if defined(POLYMESH_INDEX_64)
//    using size_t = std::uint64_t;
//#elif defined(POLYMESH_INDEX_SIZET)
//    using size_t = size_t;
//#else
//    // Default to 32-bit to match industry standard graphics libraries
//    using size_t = size_t; 
//#endif
//#endif
// ----------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "PolyMesh/V2/RelationCache.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V2 {

template<typename T>
class PolyMesh {
public:
    struct ValidMask {
        std::vector<uint8_t> valid;
        std::vector<size_t> next;
        std::vector<size_t> prev;
        size_t begin;
        size_t end;

        std::vector<size_t> _freeStack;

        ValidMask(size_t Size):
            valid(Size + 1, 1),   // +1 for end entry (invalid end)
            next(Size + 1),
            prev(Size + 1),
            begin(0),
            end(Size)             // invalid end at Size, always invalid
        {
            valid[Size] = 0;      // invalid start is always "invalid" but used as terminator    
    
            if (Size == 0) {
                next[0] = 0;
                prev[0] = 0;
                return; 
            }

            for(size_t i = 0; i < Size; i++) {
                next[i] = i + 1;
                prev[i] = i == 0 ? 0 : i - 1;
            }
            next[Size] = Size;    // invalid end points to itself
            prev[Size] = Size - 1;
        }

        inline bool is_valid(size_t Idx) const { return valid[Idx]; }

        inline void set_invalid(size_t Idx) {
            valid[Idx] = false;
            next[prev[Idx]] = next[Idx];
            prev[next[Idx]] = prev[Idx];
            _freeStack.push_back(Idx);
        }

        size_t get_free() {
            size_t i;
            if(!_freeStack.empty()) {
                i = _freeStack.back();
                _freeStack.pop_back();
            } else {
                i = valid.size() - 1;
                valid.insert(valid.end() - 1, 1);
                next.insert(next.end() - 1, 0);
                prev.insert(prev.end() - 1, 0);
                end = valid.size() - 1;
            }

            // relink into list after current end
            size_t last = prev[end];  // last valid element
            valid[i] = 1;
            next[last] = i;
            prev[i] = last;
            next[i] = end;
            prev[end]  = i;
            return i;
        }
    };

    struct FaceMatrix {
        struct FaceView {
            size_t* _data;
            uint8_t size;

            inline size_t& operator[](size_t i) { return _data[i]; }
            inline const size_t& operator[](size_t i) const { return _data[i]; }
        };

    public:
        std::vector<size_t>  vertex_idx;   // flat: f0v0, f0v1, f0v2, f1v0, f1v1, f1v2, f1v3...
        std::vector<size_t>  begin_idx;    // begin_idx[fi] = start in vertex_idx
        std::vector<uint8_t> size_idx;     // size_idx[fi] = vertex count (uint8_t = max 255)
        ValidMask valid;

        FaceMatrix() : valid(0) {}
        FaceMatrix(size_t n_faces) : valid(n_faces) {
            begin_idx.resize(n_faces, 0);
            size_idx.resize(n_faces, 0);
        }

        inline FaceView face(size_t idx) {
            return FaceView{vertex_idx.data() + begin_idx[idx], size_idx[idx]};
        }

        inline const size_t* face_begin(size_t fi) const {
            return vertex_idx.data() + begin_idx[fi];
        }
        inline size_t face_size(size_t fi) const {
            return size_idx[fi];
        }
        inline bool is_valid(size_t fi) const {
            return valid.is_valid(fi);
        }

        // add a face — returns raw index
        size_t add_face(const size_t* verts, size_t n) {
            size_t fi    = valid.get_free();
            size_t begin = vertex_idx.size();

            if(fi >= begin_idx.size()) {
                begin_idx.resize(fi + 1, 0);
                size_idx.resize(fi + 1, 0);
            }

            begin_idx[fi] = begin;
            size_idx[fi]  = (uint8_t)n;
            vertex_idx.insert(vertex_idx.end(), verts, verts + n);
            return fi;
        }

        void invalidate(size_t fi) {
            valid.set_invalid(fi);
        }

        void compact_faces() {
            std::vector<size_t> new_vertex_idx;
            new_vertex_idx.reserve(vertex_idx.size());
            
            size_t fi = valid.begin;
            while(fi != valid.end) {
                size_t new_begin = new_vertex_idx.size();
                new_vertex_idx.insert(
                    new_vertex_idx.end(),
                    face_begin(fi),
                    face_begin(fi) + face_size(fi)
                );
                begin_idx[fi] = new_begin;
                fi = valid.next[fi];
            }
            vertex_idx = std::move(new_vertex_idx);
        }
    };

    using VertexMatrix = Eigen::Matrix<T, Eigen::Dynamic, 3, Eigen::RowMajor>;
    using Vertex       = Eigen::Vector3<T>;
    struct VertexView {
        T* _data;

        inline T& operator[](size_t i) { return _data[i]; }
        inline const T& operator[](size_t i) const { return _data[i]; }
    };

    using Edge = Eigen::Vector2<size_t>;

private:
    VertexMatrix _v;
    ValidMask _v_valid;
    FaceMatrix _f;
    RelationCache _relations;


    size_t _n_vertices = 0;
    size_t _n_faces = 0;
    

public:
    PolyMesh();
    ~PolyMesh();

    void read_file(const std::string& File_Path);

    size_t add_vertex(const PolyMesh::Vertex& Vert);
    void replace_vertex(size_t Id, const PolyMesh::Vertex& Vert);
    void add_vertices(const std::vector<PolyMesh::Vertex>& Vertices);


    inline size_t n_vertices() const { return _n_vertices; }
    inline size_t n_faces() const    { return _n_faces; }
    inline size_t n_edges()          { return edges().rows(); }

    inline VertexView vertex(size_t Index) { return VertexView{_v.data() + Index * 3}; }
    inline FaceMatrix::FaceView face(size_t Index) { return _f.face(Index); }

    inline bool vertex_deleted(size_t Index) const { return _v_valid.is_valid(Index); }
    inline bool face_deleted(size_t Index) const   { return _f.is_valid(Index); }


    inline VertexMatrix& vertices()   { return _v; }
    inline FaceMatrix& faces()        { return _f; }
    inline RelationCache& relations() { return _relations; }

    // always valid,builds if needed
    const VVMatrix& VV();
    const VFMatrix& VF();
    const EFMatrix& EF();
    const FEMatrix& FE();
    const EdgeMatrix& edges();

private:
    void _build_VV();
    void _build_VF();
    void _build_EF();
    void _build_FE();
    void _build_edges();

// ----------------------------------------------------------------------------  
// Iterators
// ----------------------------------------------------------------------------  
public:
    struct VIter {
        const ValidMask* _mask;
        T* vdata;
        size_t rawId;

        inline VertexView vertex() {
            return VertexView{vdata};
        }

        inline VIter& operator++() {
            size_t n = _mask->next[rawId];
            vdata += 3 * (n - rawId);
            rawId = n;
            return *this;
        }


        inline bool operator!=(const VIter& o) const { return rawId != o.rawId; }
        inline size_t operator*() const { return rawId; }
    };

    VIter vertices_begin() {
        return {&_v_valid, _v.data() + _v_valid.begin * 3, _v_valid.begin};
    }
    VIter vertices_end() {
        return {&_v_valid, nullptr, _v_valid.end};
    }

    

// ----------------------------------------------------------------------------  
/*
    struct FIter {
        FaceMatrix* _fdata;
        size_t rawId;

        inline FaceMatrix::FaceView face() {
            return _fdata->face(rawId);
        }

        inline FIter& operator++() {
            rawId = _fdata->valid.next[rawId];
            return *this;
        }
        inline bool operator!=(const FIter& o) const { return rawId != o.rawId; }
        inline size_t operator*() const { return rawId; }
    };

    FIter faces_begin() { 
        return {&_f, _f.valid.begin}; 
    }
    FIter faces_end()   { 
        return {&_f, _f.valid.end};   
    }
*/
    
// ----------------------------------------------------------------------------  
    struct FIter {
        const ValidMask* mask;
        size_t* vertex_data_base; // Base pointer to vertex_idx.data()
        const size_t* begin_ptr;        // Pointer directly into begin_idx vector
        const uint8_t* size_ptr;        // Pointer directly into size_idx vector
        size_t rawId;

        inline typename FaceMatrix::FaceView face() {
            // Correct, independent lookup using the absolute begin offset
            return typename FaceMatrix::FaceView{
                vertex_data_base + begin_ptr[rawId], 
                size_ptr[rawId]
            };
        }

        inline FIter& operator++() {
            // Pure ID progression. Zero pointer-arithmetic dependency loops.
            rawId = mask->next[rawId];
            return *this;
        }

        inline bool operator!=(const FIter& o) const { return rawId != o.rawId; }
        inline size_t operator*() const { return rawId; }
    };

    FIter faces_begin() { 
        return {
            &_f.valid, 
            _f.vertex_idx.data(), 
            _f.begin_idx.data(), 
            _f.size_idx.data(), 
            _f.valid.begin
        }; 
    }

    FIter faces_end() { 
        return {
            &_f.valid, 
            nullptr, 
            nullptr, 
            nullptr, 
            _f.valid.end
        };   
    }



// ----------------------------------------------------------------------------  
    struct EIter {
        size_t* _mat;   // row x 2
        size_t _idx;

        EIter(size_t* mat, size_t idx = 0)
            : _mat(mat), _idx(idx)
        {}

        inline size_t v0() const { return _mat[0]; }
        inline size_t v1() const { return _mat[1]; }

        inline size_t operator*() { return _idx; }

        inline EIter&  operator++() { 
            ++_idx; 
            _mat += 2;
            return *this; 
        }

        inline bool operator!=(const EIter& o) const { return _idx != o._idx; }
    };

    EIter edges_begin() { 
        if(!_relations.has_edges) _build_edges();
        return EIter(_relations.edges.data(), 0); 
    }

    EIter edges_end() { 
        if(!_relations.has_edges) _build_edges();
        return EIter(nullptr, n_edges());
    }



// ----------------------------------------------------------------------------  
    // usage
    // for(auto vv = mesh.vv_begin(v); vv; ++vv) {
    //     size_t neighbor = *vv;
    // }
    struct VVIter {
        const VVMatrix* _mat;
        VVMatrix::InnerIterator _it;

        VVIter(const VVMatrix& mat, size_t v_idx)
            : _mat(&mat), _it(*_mat, (VVMatrix::Index)v_idx)
        {}

        ~VVIter() {
            _mat = nullptr;
        }

        inline size_t operator*() const { return (size_t)_it.row(); }
        inline VVIter& operator++() { ++_it; return *this; }

        inline operator bool() const { return _it; }
    };

    VVIter vv_begin(size_t v_idx) {
        const VVMatrix& mat = VV();
        return VVIter(mat, v_idx);
    }


// ----------------------------------------------------------------------------  
    // usage
    // for(auto vf = mesh.vf_begin(v); vf; ++vf) {
    //     size_t face = *vf;
    // }
    struct VFIter {
        const VFMatrix* _mat;
        VFMatrix::InnerIterator _it;

        VFIter(const VFMatrix& mat, size_t v)
            : _mat(&mat), _it(*_mat, (int)v)
        {}

        inline size_t  operator*()  const { return (size_t)_it.col(); }  // col = face index
        inline VFIter& operator++()       { ++_it; return *this; }
        inline operator bool()      const { return (bool)_it; }
    };

    VFIter vf_begin(size_t v) {
        const VFMatrix& m = VF();
        return VFIter(m, v);
    }


// ----------------------------------------------------------------------------
    struct FVIter {
        const size_t* cur;
        const size_t* end;

        inline FVIter& operator++()                    { ++cur; return *this; }
        inline bool operator!=(const FVIter& o)  const { return cur != o.cur; }
        inline size_t operator*()                const { return *cur; }
        inline operator bool()                   const { return cur < end; }
    };

    FVIter fv_begin(size_t f) { 
        const size_t* b = _f.face_begin(f);
        return {b, b + _f.face_size(f)}; 
    }

    FVIter fv_end(size_t f)   { 
        const size_t* e = _f.face_begin(f) + _f.face_size(f);
        return {e, e}; 
    }

// ----------------------------------------------------------------------------
    struct EFIter {
        const EFMatrix* _mat;
        EFMatrix::InnerIterator _it;

        EFIter(const EFMatrix& mat, size_t e)
            : _mat(&mat), _it(*_mat, e)
        {}

        inline size_t  operator*()  const { return (size_t)_it.col(); }  // row = face index
        inline EFIter& operator++()       { ++_it; return *this; }
        inline operator bool()      const { return (bool)_it; }
    };

    EFIter ef_begin(size_t e) { 
        return EFIter(EF(), e);
    }

// ----------------------------------------------------------------------------
    struct FEIter {
        const FEMatrix* _mat;
        FEMatrix::InnerIterator _it;

        FEIter(const FEMatrix& mat, size_t f)
            : _mat(&mat), _it(*_mat, (int)f)
        {}

        inline size_t  operator*()  const { return (size_t)_it.col(); }  // col = edge index
        inline FEIter& operator++()       { ++_it; return *this; }
        inline operator bool()      const { return (bool)_it; }
    };

    FEIter fe_begin(size_t f) { 
        const FEMatrix& m = FE(); 
        return FEIter(m, f); 
    }


};
};

#endif
