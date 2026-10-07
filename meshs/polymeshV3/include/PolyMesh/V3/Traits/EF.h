#ifndef POLYMESH_V3_TRAITS_EF_RAGGED_MATRIX_H
#define POLYMESH_V3_TRAITS_EF_RAGGED_MATRIX_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
#include "PolyMesh/V3/DataStructs/Append_Ragged_Matrix.h"
#include "PolyMesh/V3/DataStructs/VecVec_Matrix.h"
// ----------------------------------------------------------------------------




namespace PolyMesh::V3 {
template<typename T>
concept EF_ExternalC = requires(const T& ct, typename T::Index v0, typename T::Index v1) {
    typename T::Index;
    typename T::Edge;
    typename T::Const_Group_View;

    { ct.get_edges(v0) }       -> std::same_as<typename T::Const_Group_View>;
    { ct.get_edge(v0, v1) }    -> std::same_as<typename T::Edge>;
    { ct.edge_exists(v0, v1) } -> std::same_as<bool>;
    { ct.edge_face_count(v0, v1) } -> std::same_as<typename T::Index>;
    { ct.is_vertex_valid(v0) } -> std::same_as<bool>;
};




template<typename T>
concept EF_InternalC = requires(
    T& t, typename T::Index idx, typename T::Index slot,
    const typename T::_Edge_Data& edge_data,
    std::span<const typename T::Index> old_to_new, bool rebuild_reserve
) {
    typename T::Group_View;
    typename T::_Edge_Data; // internal per-slot payload (e.g. {v1, f_left, f_right})

    { t.get_edges(idx) } -> std::same_as<typename T::Group_View>;

    { t.edge_append(idx, edge_data) } -> std::same_as<typename T::Index>; // returns slot
    { t.edge_remove_swap(idx, slot) } -> std::same_as<void>;              // swap-shrink removal
    { t.set_row_valid(idx) } -> std::same_as<void>;
    { t.set_row_invalid(idx) } -> std::same_as<void>;

    { t.reserve(idx) }               -> std::same_as<void>;
    { t.set_largest_vertex_idx(idx) } -> std::same_as<void>; // grows row count to cover new vertex ids

    { t.compact(old_to_new, rebuild_reserve) } -> std::same_as<void>;
};







template<typename IndexT, typename BinSizeT, typename MeshT = void>
struct EF_Ragged_Matrix {
private:
    friend MeshT;



public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();

private:
    struct _Edge_Data {
        Index v1;
        Index f_left, f_right;
    };




    using _Matrix = D_Ragged_Matrix<_Edge_Data, Index, BinSizeT>;
    _Matrix _data;

    using Group_View = _Matrix::RowView;


public:
    struct Edge {
        Index v0, v1;
        Index f_left, f_right;
    };

    using Const_Group_View = _Matrix::Const_RowView;



public:
    EF_Ragged_Matrix(float Reserve_Perz = 0.2):
        _data(0, Reserve_Perz)
    {}

    EF_Ragged_Matrix(const EF_Ragged_Matrix&) = default;
    EF_Ragged_Matrix(EF_Ragged_Matrix&&) noexcept = default;

    EF_Ragged_Matrix& operator=(const EF_Ragged_Matrix&) = default;
    EF_Ragged_Matrix& operator=(EF_Ragged_Matrix&&) noexcept = default;

    
    Const_Group_View get_edges(Index v0) const { return _data.row(v0); }
    

    Edge get_edge(Index v0, Index v1) const {
        if(v0 > v1) std::swap(v0, v1);

        for(auto _e : _data.row(v0)) {
            if(_e.v1 != v1) continue;

            return {v0, v1, _e.f_left, _e.f_right};
        }

        return {v0, v1, INDEX_INVALID, INDEX_INVALID};
    }


    bool edge_exists(Index v0, Index v1) const {
        Edge e = get_edge(v0, v1);
        return e.f_left != INDEX_INVALID || e.f_right != INDEX_INVALID;
    }

    Index edge_face_count(Index v0, Index v1) const {
        Index a = v0, b = v1;
        if (a > b) std::swap(a, b);
        
        if (!edge_exists(a, b)) return 0;
        auto e = get_edge(a, b);
        return (e.f_left != INDEX_INVALID) + (e.f_right != INDEX_INVALID);
    }
    
    bool is_vertex_valid(Index v0) const { return _data.is_valid(v0); }


private:
    Group_View get_edges(Index v0) { return _data.row(v0); }

    Index edge_append(Index row_idx, const _Edge_Data& value) { return _data.row_append(row_idx, value); }

    void edge_remove_swap(Index row_idx, Index edge_slot) { _data.row_remove_swap(row_idx, edge_slot); }


    void set_row_valid(Index row_idx) { _data.reinit_row(row_idx); }

    void set_row_invalid(Index row_idx) {  _data.delete_row(row_idx); }


    void reserve(Index size) { _data.reserve(size); }

    void set_largest_vertex_idx(Index v_idx) { _data.grow_row_count(v_idx); }


    // TODO
    void compact(std::span<const Index> new_order, bool rebuild_reserve) {}
};







template<typename IndexT, typename MeshT = void>
struct EF_Append_Ragged_Matrix {
private:
    friend MeshT;



public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();

private:
    struct _Edge_Data {
        Index v1;
        Index f_left, f_right;
    };


    using _Matrix = Append_Ragged_Matrix<_Edge_Data, Index, false>;
    _Matrix _data;

    using Group_View = _Matrix::View;


public:
    struct Edge {
        Index v0, v1;
        Index f_left, f_right;
    };

    using Const_Group_View = _Matrix::Const_View;



public:
    EF_Append_Ragged_Matrix(float Reserve_Perz = 0.2):
        _data(0.2f, 0.4f, 3) {}


    EF_Append_Ragged_Matrix(const EF_Append_Ragged_Matrix&) = default;
    EF_Append_Ragged_Matrix(EF_Append_Ragged_Matrix&&) noexcept = default;

    EF_Append_Ragged_Matrix& operator=(const EF_Append_Ragged_Matrix&) = default;
    EF_Append_Ragged_Matrix& operator=(EF_Append_Ragged_Matrix&&) noexcept = default;

    
    Const_Group_View get_edges(Index v0) const { return _data.row(v0); }
    

    Edge get_edge(Index v0, Index v1) const {
        if(v0 > v1) std::swap(v0, v1);

        for(auto _e : _data.row(v0)) {
            if(_e.v1 != v1) continue;

            return {v0, v1, _e.f_left, _e.f_right};
        }

        return {v0, v1, INDEX_INVALID, INDEX_INVALID};
    }


    bool edge_exists(Index v0, Index v1) const {
        Edge e = get_edge(v0, v1);
        return e.f_left != INDEX_INVALID || e.f_right != INDEX_INVALID;
    }

    Index edge_face_count(Index v0, Index v1) const {
        Index a = v0, b = v1;
        if (a > b) std::swap(a, b);
        
        if (!edge_exists(a, b)) return 0;
        auto e = get_edge(a, b);
        return (e.f_left != INDEX_INVALID) + (e.f_right != INDEX_INVALID);
    }
    
    bool is_vertex_valid(Index v0) const { return _data.is_valid(v0); }


private:
    Group_View get_edges(Index v0) { return _data.row(v0); }

    Index edge_append(Index row_idx, const _Edge_Data& value) { return _data.row_append(row_idx, value); }

    void edge_remove_swap(Index row_idx, Index edge_slot) { _data.row_remove_swap(row_idx, edge_slot); }


    void set_row_valid(Index row_idx) { _data.reinit_row(row_idx); }

    void set_row_invalid(Index row_idx) {  _data.set_invalid(row_idx); }


    void reserve(Index size) { _data.reserve(size); }

    void set_largest_vertex_idx(Index v_idx) { _data.grow_row_count(v_idx); }

    
    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }
};








template<typename IndexT, typename MeshT = void>
struct EF_Vec_Vec_Matrix {
private:
    friend MeshT;

public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();

private:
    struct _Edge_Data { Index v1; Index f_left, f_right; };
    using _Matrix = Vec_Vec_Matrix<_Edge_Data, Index, false>;
    _Matrix _data;
    using Group_View = typename _Matrix::View;

public:
    struct Edge { Index v0, v1; Index f_left, f_right; };
    using Const_Group_View = typename _Matrix::Const_View;

    EF_Vec_Vec_Matrix(float Reserve_Perz = 0.2): _data(0.2f, 0.4f, 3) {}

    EF_Vec_Vec_Matrix(const EF_Vec_Vec_Matrix&) = default;
    EF_Vec_Vec_Matrix(EF_Vec_Vec_Matrix&&) noexcept = default;
    EF_Vec_Vec_Matrix& operator=(const EF_Vec_Vec_Matrix&) = default;
    EF_Vec_Vec_Matrix& operator=(EF_Vec_Vec_Matrix&&) noexcept = default;

    Const_Group_View get_edges(Index v0) const { return _data.row(v0); }

    Edge get_edge(Index v0, Index v1) const {
        if (v0 > v1) std::swap(v0, v1);
        for (auto _e : _data.row(v0)) {
            if (_e.v1 != v1) continue;
            return {v0, v1, _e.f_left, _e.f_right};
        }
        return {v0, v1, INDEX_INVALID, INDEX_INVALID};
    }

    bool edge_exists(Index v0, Index v1) const {
        Edge e = get_edge(v0, v1);
        return e.f_left != INDEX_INVALID || e.f_right != INDEX_INVALID;
    }

    Index edge_face_count(Index v0, Index v1) const {
        Index a = v0, b = v1;
        if (a > b) std::swap(a, b);
        if (!edge_exists(a, b)) return 0;
        auto e = get_edge(a, b);
        return (e.f_left != INDEX_INVALID) + (e.f_right != INDEX_INVALID);
    }

    bool is_vertex_valid(Index v0) const { return _data.is_valid(v0); }

private:
    Group_View get_edges(Index v0) { return _data.row(v0); }
    Index edge_append(Index row_idx, const _Edge_Data& value) { return _data.row_append(row_idx, value); }
    void edge_remove_swap(Index row_idx, Index edge_slot) { _data.row_remove_swap(row_idx, edge_slot); }
    void set_row_valid(Index row_idx) { _data.reinit_row(row_idx); }
    void set_row_invalid(Index row_idx) { _data.set_invalid(row_idx); }
    void reserve(Index size) { _data.reserve(size); }
    void set_largest_vertex_idx(Index v_idx) { _data.grow_row_count(v_idx); }
    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }
};

}
#endif