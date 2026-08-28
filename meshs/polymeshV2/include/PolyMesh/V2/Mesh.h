#ifndef POLY_MESH_V2_H
#define POLY_MESH_V2_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "PolyMesh/V2/Relation_Matrices/Cache.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V2 {

template<typename ScalarT, typename IndexT = std::uint32_t>
class PolyMesh {
public:
    struct ValidMask {
        bool has_invalids;
        std::vector<uint8_t> valid;
        std::vector<IndexT> next;
        std::vector<IndexT> prev;
        IndexT begin;
        IndexT end;

        std::vector<IndexT> _freeStack;

        ValidMask(IndexT Size):
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

            for(IndexT i = 0; i < Size; i++) {
                next[i] = i + 1;
                prev[i] = i == 0 ? 0 : i - 1;
            }
            next[Size] = Size;    // invalid end points to itself
            prev[Size] = Size - 1;
        }

        inline bool is_valid(IndexT Idx) const { return valid[Idx]; }

        inline void set_invalid(IndexT Idx) {
            valid[Idx] = false;
            next[prev[Idx]] = next[Idx];
            prev[next[Idx]] = prev[Idx];
            _freeStack.push_back(Idx);
        }

        IndexT get_free() {
            IndexT i;
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
            IndexT last = prev[end];  // last valid element
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
            IndexT* _data;
            uint8_t size;

            inline IndexT& operator[](IndexT i) { return _data[i]; }
            inline const IndexT& operator[](IndexT i) const { return _data[i]; }
        };

    public:
        std::vector<IndexT>  vertex_idx;   // flat: f0v0, f0v1, f0v2, f1v0, f1v1, f1v2, f1v3...
        std::vector<IndexT>  begin_idx;    // begin_idx[fi] = start in vertex_idx
        std::vector<uint8_t> size_idx;     // size_idx[fi] = vertex count (uint8_t = max 255)
        ValidMask valid;

        FaceMatrix() : valid(0) {}
        FaceMatrix(IndexT n_faces) : valid(n_faces) {
            begin_idx.resize(n_faces, 0);
            size_idx.resize(n_faces, 0);
        }

        inline FaceView face(IndexT idx) {
            return FaceView{vertex_idx.data() + begin_idx[idx], size_idx[idx]};
        }

        inline const IndexT* face_begin(IndexT fi) const {
            return vertex_idx.data() + begin_idx[fi];
        }
        inline IndexT face_size(IndexT fi) const {
            return size_idx[fi];
        }
        inline bool is_valid(IndexT fi) const {
            return valid.is_valid(fi);
        }

        // add a face — returns raw index
        IndexT add_face(const IndexT* verts, IndexT n) {
            IndexT fi    = valid.get_free();
            IndexT begin = vertex_idx.size();

            if(fi >= begin_idx.size()) {
                begin_idx.resize(fi + 1, 0);
                size_idx.resize(fi + 1, 0);
            }

            begin_idx[fi] = begin;
            size_idx[fi]  = (uint8_t)n;
            vertex_idx.insert(vertex_idx.end(), verts, verts + n);
            return fi;
        }

        void invalidate(IndexT fi) {
            valid.set_invalid(fi);
        }

        void compact_faces() {
            std::vector<IndexT> new_vertex_idx;
            new_vertex_idx.reserve(vertex_idx.size());
            
            IndexT fi = valid.begin;
            while(fi != valid.end) {
                IndexT new_begin = new_vertex_idx.size();
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
    
   /*
    struct FaceMatrix {
        struct FaceView {
            IndexT* _data;
            uint8_t size;
            inline IndexT& operator[](IndexT i) { return _data[i]; }
            inline const IndexT& operator[](IndexT i) const { return _data[i]; }
        };

        struct ConstFaceView {
            const IndexT* _data;
            uint8_t size;
            inline const IndexT& operator[](IndexT i) const { return _data[i]; }
        };

    public:
        std::vector<IndexT>  vertex_idx;   // Flat array: f0v0, f0v1, f0v2, f1v0...
        std::vector<IndexT>  begin_idx;    // Offset map into vertex_idx per face
        std::vector<uint8_t> size_idx;     // Face valency tracker
        ValidMask            valid;

        FaceMatrix() : valid(0) {}
        FaceMatrix(IndexT n_faces) : valid(n_faces) {
            begin_idx.resize(n_faces, 0);
            size_idx.resize(n_faces, 0);
        }

        inline FaceView face(IndexT idx) {
            return FaceView{vertex_idx.data() + begin_idx[idx], size_idx[idx]};
        }

        inline ConstFaceView face(IndexT idx) const {
            return ConstFaceView{vertex_idx.data() + begin_idx[idx], size_idx[idx]};
        }

        inline const IndexT* face_begin(IndexT fi) const {
            return vertex_idx.data() + begin_idx[fi];
        }
        
        inline IndexT face_size(IndexT fi) const {
            return size_idx[fi];
        }
        
        inline bool is_valid(IndexT fi) const {
            return valid.is_valid(fi);
        }

        IndexT add_face(const IndexT* verts, IndexT n) {
            IndexT fi    = valid.get_free();
            IndexT begin = vertex_idx.size();

            if (fi >= begin_idx.size()) {
                begin_idx.resize(fi + 1, 0);
                size_idx.resize(fi + 1, 0);
            }

            if constexpr (requires { valid.set_valid(fi); }) {
                valid.set_valid(fi);
            }

            begin_idx[fi] = begin;
            size_idx[fi]  = static_cast<uint8_t>(n);
            vertex_idx.insert(vertex_idx.end(), verts, verts + n);
            
            return fi;
        }

        void invalidate(IndexT fi) {
            valid.set_invalid(fi);
            size_idx[fi] = 0; 
            begin_idx[fi] = 0;
        }

        void compact_faces() {
            std::vector<IndexT> new_vertex_idx;
            new_vertex_idx.reserve(vertex_idx.size());
            
            IndexT fi = valid.begin;
            while (fi != valid.end) {
                IndexT new_begin = new_vertex_idx.size();
                
                if (size_idx[fi] > 0) {
                    new_vertex_idx.insert(
                        new_vertex_idx.end(),
                        face_begin(fi),
                        face_begin(fi) + size_idx[fi]
                    );
                    begin_idx[fi] = new_begin;
                } else {
                    begin_idx[fi] = 0;
                }
                
                fi = valid.next[fi];
            }
            vertex_idx = std::move(new_vertex_idx);
        }
    };
    */

    using VertexMatrix = Eigen::Matrix<ScalarT, Eigen::Dynamic, 3, Eigen::RowMajor>;
    using Vertex       = Eigen::Vector3<ScalarT>;
    struct VertexView {
        ScalarT* _data;

        inline ScalarT& operator[](IndexT i) { return _data[i]; }
        inline const ScalarT& operator[](IndexT i) const { return _data[i]; }
    };

    using Edge = Eigen::Vector2<IndexT>;

private:
    VertexMatrix _v;
    ValidMask _v_valid;
    FaceMatrix _f;
    RelationCache<ScalarT, IndexT> _relations;


    IndexT _n_vertices = 0;
    IndexT _n_faces = 0;
    

public:
    PolyMesh();
    ~PolyMesh();

    void read_file(const std::string& File_Path);

    inline IndexT n_vertices() const { return _n_vertices; }
    inline IndexT n_faces() const    { return _n_faces; }
    inline IndexT n_edges()          { return edges().rows(); }

    inline VertexView vertex(IndexT Index) { return VertexView{_v.data() + Index * 3}; }
    inline FaceMatrix::FaceView face(IndexT Index) { return _f.face(Index); }

    inline VertexMatrix& vertices()   { return _v; }
    inline FaceMatrix& faces()        { return _f; }
    inline RelationCache<ScalarT, IndexT>& relations() { return _relations; }

// ----------------------------------------------------------------------------  
// Modifiers
// ----------------------------------------------------------------------------  
public:
    IndexT add_vertex(const PolyMesh::Vertex& Vert);
    void delete_vertex(IndexT Idx);
    void replace_vertex(IndexT Idx, const PolyMesh::Vertex& Vert);

    IndexT add_face(const IndexT* Verts, uint8_t N);
    void delete_face(IndexT Idx);

private:
    IndexT _find_opposite_vertex(IndexT Face, IndexT V0, IndexT V1) const;

// ----------------------------------------------------------------------------  
// Helper Matrices
// ----------------------------------------------------------------------------  
public:
    // always valid, builds if needed
    const VVMatrix<IndexT>& VV();
    const VFMatrix<IndexT>& VF();
    const EFMatrix<IndexT>& EF();
    const FEMatrix<IndexT>& FE();
    const EdgeMatrix<IndexT>& edges();

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
        ScalarT* _vdata;
        IndexT _rawId;

        inline VertexView vertex() {
            return VertexView{_vdata};
        }

        inline VIter& operator++() {
            if(_mask->has_invalids) {
                IndexT n = _mask->next[_rawId];
                _vdata += 3 * (n - _rawId);
                _rawId = n;
                return *this;
            }
            else {
                _rawId++;
                _vdata += 3;
                return *this;
            }
        }


        inline bool operator!=(const VIter& o) const { return _rawId != o._rawId; }
        inline IndexT operator*() const { return _rawId; }
    };

    VIter vertices_begin() {
        return {&_v_valid, _v.data() + _v_valid.begin * 3, _v_valid.begin};
    }
    VIter vertices_end() {
        return {&_v_valid, nullptr, _v_valid.end};
    }


    
// ----------------------------------------------------------------------------  
    struct FIter {
        const ValidMask* _mask;
        IndexT* _vertex_data_base;      // Pointer to the face data ( indexes )
        const IndexT* _begin_ptr;       // Pointer into begin_idx vector
        const uint8_t* _size_ptr;       // Pointer into size_idx vector
        IndexT _rawId;

        inline typename FaceMatrix::FaceView face() {
            
            return typename FaceMatrix::FaceView{
                _vertex_data_base + _begin_ptr[_rawId], 
                _size_ptr[_rawId]
            };
        }

        inline FIter& operator++() {
            if(_mask->has_invalids) {
                _rawId = _mask->next[_rawId];
                return *this;
            }
            else {
                _rawId++;
                return *this;
            }
        }

        inline bool operator!=(const FIter& o) const { return _rawId != o._rawId; }
        inline IndexT operator*() const { return _rawId; }
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
        IndexT* _mat;   // row x 2
        IndexT _idx;

        EIter(IndexT* mat, IndexT idx = 0)
            : _mat(mat), _idx(idx)
        {}

        inline IndexT v0() const { return _mat[0]; }
        inline IndexT v1() const { return _mat[1]; }

        inline IndexT operator*() { return _idx; }

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
    struct VVIter {
        const VVMatrix<IndexT>* _mat;
        VVMatrix<IndexT>::InnerIterator _it;

        VVIter(const VVMatrix<IndexT>& mat, IndexT v_idx)
            : _mat(&mat), _it(*_mat, v_idx)
        {}

        ~VVIter() {
            _mat = nullptr;
        }

        inline IndexT operator*() const { return (IndexT)_it.row(); }
        inline VVIter& operator++() { ++_it; return *this; }

        inline operator bool() const { return _it; }
    };

    VVIter vv_begin(IndexT v_idx) {
        const VVMatrix<IndexT>& mat = VV();
        return VVIter(mat, v_idx);
    }


// ---------------------------------------------------------------------------- 
    struct VFIter {
        const VFMatrix<IndexT>* _mat;
        VFMatrix<IndexT>::InnerIterator _it;

        VFIter(const VFMatrix<IndexT>& mat, IndexT v)
            : _mat(&mat), _it(*_mat, (int)v)
        {}

        inline IndexT  operator*() const { return (IndexT)_it.col(); }  // col = face index
        inline VFIter& operator++() { ++_it; return *this; }
        inline operator bool() const { return (bool)_it; }
    };

    VFIter vf_begin(IndexT v) {
        const VFMatrix<IndexT>& m = VF();
        return VFIter(m, v);
    }


// ----------------------------------------------------------------------------
    struct FVIter {
        const IndexT* cur;
        const IndexT* end;

        inline FVIter& operator++()                    { ++cur; return *this; }
        inline bool operator!=(const FVIter& o)  const { return cur != o.cur; }
        inline IndexT operator*()                const { return *cur; }
        inline operator bool()                   const { return cur < end; }
    };

    FVIter fv_begin(IndexT f) { 
        const IndexT* b = _f.face_begin(f);
        return {b, b + _f.face_size(f)}; 
    }

    FVIter fv_end(IndexT f)   { 
        const IndexT* e = _f.face_begin(f) + _f.face_size(f);
        return {e, e}; 
    }

// ----------------------------------------------------------------------------
    struct EFIter {
        const EFMatrix<IndexT>* _mat;
        EFMatrix<IndexT>::InnerIterator _it;

        EFIter(const EFMatrix<IndexT>& mat, IndexT e)
            : _mat(&mat), _it(*_mat, e)
        {}

        inline IndexT  operator*()  const { return (IndexT)_it.col(); }  // row = face index
        inline EFIter& operator++()       { ++_it; return *this; }
        inline operator bool()      const { return (bool)_it; }
    };

    EFIter ef_begin(IndexT e) { 
        return EFIter(EF(), e);
    }

// ----------------------------------------------------------------------------
    struct FEIter {
        const FEMatrix<IndexT>* _mat;
        FEMatrix<IndexT>::InnerIterator _it;

        FEIter(const FEMatrix<IndexT>& mat, IndexT f)
            : _mat(&mat), _it(*_mat, (int)f)
        {}

        inline IndexT  operator*()  const { return (IndexT)_it.col(); }  // col = edge index
        inline FEIter& operator++()       { ++_it; return *this; }
        inline operator bool()      const { return (bool)_it; }
    };

    FEIter fe_begin(IndexT f) { 
        const FEMatrix<IndexT>& m = FE(); 
        return FEIter(m, f); 
    }


};
};


#include "PolyMesh/V2/Details/Core.inl"
#include "PolyMesh/V2/Details/Modifiers.inl"
#endif
