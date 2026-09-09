#ifndef POLYMESH_V3_TRAITS_VV_HE_RAGGED_MATRIX_H
#define POLYMESH_V3_TRAITS_VV_HE_RAGGED_MATRIX_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
#include "PolyMesh/V3/DataStructs/Append_Ragged_Matrix.h"
// ----------------------------------------------------------------------------





namespace PolyMesh::V3 {
template<typename T>
concept VV_HE_ExternalC = requires(const T& ct, typename T::Index v0, typename T::Index v1) {
    typename T::Index;
    typename T::Const_Group_View;

    { ct.get_edges(v0) } -> std::same_as<typename T::Const_Group_View>;
    { ct.edge_exists(v0, v1) } -> std::same_as<bool>;
    { ct.is_vertex_valid(v0) } -> std::same_as<bool>;
};




template<typename T>
concept VV_HE_InternalC = requires(
    T& t, typename T::Index idx, typename T::Index slot, 
    typename T::Index v0, typename T::Index v1,
    std::span<const typename T::Index> old_to_new, bool rebuild_reserve
) {
    typename T::Group_View;

    { t.get_edges(idx) } -> std::same_as<typename T::Group_View>; // mutable overload

    { t.edge_append(v0, v1) } -> std::same_as<typename T::Index>; // returns slot
    { t.edge_remove_swap(v0, slot) } -> std::same_as<void>;              // swap-shrink removal
    { t.set_row_valid(idx) } -> std::same_as<void>;
    { t.set_row_invalid(idx) } -> std::same_as<void>;

    { t.reserve(idx) } -> std::same_as<void>;
    { t.set_largest_vertex_idx(idx) } -> std::same_as<void>; // grows row count to cover new vertex ids
    

    { t.compact(old_to_new, rebuild_reserve) } -> std::same_as<void>;
};






template<typename IndexT, typename BinSizeT, typename MeshT = void>
struct VV_HE_Ragged_Matrix {
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
    VV_HE_Ragged_Matrix(float Reserve_Perz = 0.2):
        _data(0, Reserve_Perz)
    {}

    
    Const_Group_View get_edges(Index v0) const { return _data.row(v0); }


    bool edge_exists(Index v0, Index v1) const {
        for(auto e : get_edges(v0)) {
            if(e == v1) return true;
        }
        return false;
    }

    bool is_vertex_valid(Index v0) const { return _data.is_valid(v0); }


private:
    Group_View get_edges(Index v0) { return _data.row(v0); }

    Index edge_append(Index v0, Index v1) { return _data.row_append(v0, v1); }

    void edge_remove_swap(Index v0, Index slot) { _data.row_remove_swap(v0, slot); }

    
    void set_row_valid(Index v0) { _data.reinit_row(v0); }

    void set_row_invalid(Index v0) {  _data.delete_row(v0); }


    void reserve(Index size) { _data.reserve(size); }

    void set_largest_vertex_idx(Index v_idx) { _data.grow_row_count(v_idx); }
    

    // TODO
    void compact(std::span<const Index> new_order, bool rebuild_reserve) {}
};






template<typename IndexT, typename MeshT = void>
struct VV_HE_Append_Ragged_Matrix {
private:
    friend MeshT;

public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();

private:
    using _Matrix = Append_Ragged_Matrix<Index, Index, false>;
    _Matrix _data;

    using Group_View = _Matrix::View;

public:
    using Const_Group_View = _Matrix::Const_View;


public:
    VV_HE_Append_Ragged_Matrix(float Reserve_Perz = 0.2):
        _data(0.2f, 0.4f, 6)
    {}

    
    Const_Group_View get_edges(Index v0) const { return _data.row(v0); }


    bool edge_exists(Index v0, Index v1) const {
        for(auto e : get_edges(v0)) {
            if(e == v1) return true;
        }
        return false;
    }

    bool is_vertex_valid(Index v0) const { return _data.is_valid(v0); }


private:
    Group_View get_edges(Index v0) { return _data.row(v0); }

    Index edge_append(Index v0, Index v1) { return _data.row_append(v0, v1); }

    void edge_remove_swap(Index v0, Index slot) { _data.row_remove_swap(v0, slot); }

    
    void set_row_valid(Index v0) { _data.reinit_row(v0); }

    void set_row_invalid(Index v0) {  _data.set_invalid(v0); }


    void reserve(Index size) { _data.reserve(size); }

    void set_largest_vertex_idx(Index v_idx) { _data.grow_row_count(v_idx); }
    

    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }
};



}
#endif