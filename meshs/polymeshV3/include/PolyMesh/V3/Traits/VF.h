#ifndef POLYMESH_V3_TRAITS_VF_RAGGED_MATRIX_H
#define POLYMESH_V3_TRAITS_VF_RAGGED_MATRIX_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
// ----------------------------------------------------------------------------





namespace PolyMesh::V3 {
template<typename T>
concept VF_ExternalC = requires(const T& ct, typename T::Index v0, typename T::Index f0) {
    typename T::Index;
    typename T::Const_Group_View;

    { ct.get_faces(v0) }       -> std::same_as<typename T::Const_Group_View>;
    { ct.vertex_is_next_to_face(v0, f0) } -> std::same_as<bool>;
    { ct.is_vertex_valid(v0) } -> std::same_as<bool>;
};




template<typename T>
concept VF_InternalC = requires(
    T& t, typename T::Index idx, typename T::Index slot, 
    typename T::Index v0, typename T::Index f0
) {
    typename T::Group_View;

    { t.get_faces(v0) } -> std::same_as<typename T::Group_View>; // mutable overload

    { t.face_append(v0, f0) } -> std::same_as<typename T::Index>; // returns slot
    { t.face_remove_swap(v0, slot) } -> std::same_as<void>;       // swap-shrink removal

    { t.set_row_valid(idx) } -> std::same_as<void>;
    { t.set_row_invalid(idx) } -> std::same_as<void>;

    { t.reserve(idx) }               -> std::same_as<void>;
    { t.set_largest_vertex_idx(idx) } -> std::same_as<void>; // grows row count to cover new vertex ids
};






template<typename IndexT, typename BinSizeT, typename MeshT = void>
struct VF_Ragged_Matrix {
private:
    friend MeshT;

public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();

private:
    using _Matrix = D_Ragged_Matrix<Index, Index, BinSizeT>;
    _Matrix _data;

    using Group_View = _Matrix::RowView;

public:
    using Const_Group_View = _Matrix::Const_RowView;


public:
    VF_Ragged_Matrix(float Reserve_Perz = 0.2):
        _data(0, Reserve_Perz)
    {}

    
    Const_Group_View get_faces(Index v0) const { return _data.row(v0); }


    bool vertex_is_next_to_face(Index v0, Index f0) const {
        for(auto f : get_faces(v0)) {
            if(f == f0) return true;
        }
        return false;
    }

    bool is_vertex_valid(Index v0) const { return _data.is_valid(v0); }


private:
    Group_View get_faces(Index v0) { return _data.row(v0); }

    Index face_append(Index v0, Index f0) { return _data.row_append(v0, f0); }

    void face_remove_swap(Index v0, Index slot) { _data.row_remove_swap(v0, slot); }

    
    void set_row_valid(Index v0) { _data.reinit_row(v0); }

    void set_row_invalid(Index v0) {  _data.delete_row(v0); }


    void reserve(Index size) { _data.reserve(size); }

    void set_largest_vertex_idx(Index v_idx) { _data.grow_row_count(v_idx); }
};
}



#endif