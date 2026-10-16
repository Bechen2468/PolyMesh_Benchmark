#ifndef POLYMESH_V3_TRAITS_VF_RAGGED_MATRIX_H
#define POLYMESH_V3_TRAITS_VF_RAGGED_MATRIX_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
#include "PolyMesh/V3/DataStructs/Append_Ragged_Matrix.h"
#include "PolyMesh/V3/DataStructs/VecVec_Matrix.h"
#include "PolyMesh/V3/DataStructs/Vec_Eigen_Matrix.h"
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
    typename T::Index v0, typename T::Index f0,
    std::span<const typename T::Index> old_to_new, bool rebuild_reserve
) {
    typename T::Group_View;

    { t.get_faces(v0) } -> std::same_as<typename T::Group_View>; // mutable overload

    { t.face_append(v0, f0) } -> std::same_as<typename T::Index>; // returns slot
    { t.face_remove_swap(v0, slot) } -> std::same_as<void>;       // swap-shrink removal

    { t.set_row_valid(idx) } -> std::same_as<void>;
    { t.set_row_invalid(idx) } -> std::same_as<void>;

    { t.reserve(idx) }               -> std::same_as<void>;
    { t.set_largest_vertex_idx(idx) } -> std::same_as<void>; // grows row count to cover new vertex ids
    

    { t.compact(old_to_new, rebuild_reserve) } -> std::same_as<void>;
};








template<typename IndexT, typename MeshT = void>
struct VF_D_Ragged_Matrix {
private:
    friend MeshT;
 
public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();
 
private:
    using _Matrix = D_Ragged_Matrix<Index, Index, false>;
    _Matrix _data;
    using Group_View = typename _Matrix::View;
 
public:
    using Const_Group_View = typename _Matrix::Const_View;
 
    VF_D_Ragged_Matrix(float Reserve_Perz = 0.2): _data(0.2f, 0.4f, 6) {}

    VF_D_Ragged_Matrix(const std::vector<Index>& Row_Sizes, float Reserve_Perz = 0.2f):
        _data(Row_Sizes, Reserve_Perz, Reserve_Perz, 6) {}
 
    Const_Group_View get_faces(Index v0) const { return _data.row(v0); }
 
    bool vertex_is_next_to_face(Index v0, Index f0) const {
        for (auto f : get_faces(v0)) if (f == f0) return true;
        return false;
    }
 
    bool is_vertex_valid(Index v0) const { return _data.is_valid(v0); }
 
private:
    Group_View get_faces(Index v0) { return _data.row(v0); }
    Index face_append(Index v0, Index f0) { return _data.row_append(v0, f0); }
    void face_remove_swap(Index v0, Index slot) { _data.row_remove_swap(v0, slot); }
    void set_row_valid(Index v0) { _data.reinit_row(v0); }
    void set_row_invalid(Index v0) { _data.set_invalid(v0); }
    void reserve(Index size) { _data.reserve(size); }
    void set_largest_vertex_idx(Index v_idx) { _data.grow_row_count(v_idx); }
    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }
};









template<typename IndexT, typename MeshT = void>
struct VF_Append_Ragged_Matrix {
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
    VF_Append_Ragged_Matrix(float Reserve_Perz = 0.2):
        _data(Reserve_Perz, Reserve_Perz, 6)
    {}
    
    VF_Append_Ragged_Matrix(const std::vector<Index>& Row_Sizes, float Reserve_Perz = 0.2):
        _data(Row_Sizes, Reserve_Perz, Reserve_Perz, 3) {}

    
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

    void set_row_invalid(Index v0) {  _data.set_invalid(v0); }


    void reserve(Index size) { _data.reserve(size); }

    void set_largest_vertex_idx(Index v_idx) { _data.grow_row_count(v_idx); }

    
    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }
};












template<typename IndexT, typename MeshT = void>
struct VF_Vec_Vec_Matrix {
private:
    friend MeshT;

public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();

private:
    using _Matrix = Vec_Vec_Matrix<Index, Index, false>;
    _Matrix _data;
    using Group_View = typename _Matrix::View;

public:
    using Const_Group_View = typename _Matrix::Const_View;

    VF_Vec_Vec_Matrix(float Reserve_Perz = 0.2): 
        _data(Reserve_Perz, 6) {}

    VF_Vec_Vec_Matrix(const std::vector<Index>& Row_Sizes, float Reserve_Perz = 0.2):
        _data(Row_Sizes, Reserve_Perz, 3) {}

    Const_Group_View get_faces(Index v0) const { return _data.row(v0); }

    bool vertex_is_next_to_face(Index v0, Index f0) const {
        for (auto f : get_faces(v0)) if (f == f0) return true;
        return false;
    }

    bool is_vertex_valid(Index v0) const { return _data.is_valid(v0); }

private:
    Group_View get_faces(Index v0) { return _data.row(v0); }
    Index face_append(Index v0, Index f0) { return _data.row_append(v0, f0); }
    void face_remove_swap(Index v0, Index slot) { _data.row_remove_swap(v0, slot); }
    void set_row_valid(Index v0) { _data.reinit_row(v0); }
    void set_row_invalid(Index v0) { _data.set_invalid(v0); }
    void reserve(Index size) { _data.reserve(size); }
    void set_largest_vertex_idx(Index v_idx) { _data.grow_row_count(v_idx); }
    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }
};











template<typename IndexT, typename MeshT = void>
struct VF_Vec_Eigen_Matrix {
private:
    friend MeshT;

public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();

private:
    using _Matrix = Vec_Eigen_Matrix<Index, Index, false>;
    _Matrix _data;
    using Group_View = typename _Matrix::View;

public:
    using Const_Group_View = typename _Matrix::Const_View;

    VF_Vec_Eigen_Matrix(float Reserve_Perz = 0.2): 
        _data(0.2f, 0.4f, 6) {}

    VF_Vec_Eigen_Matrix(const std::vector<Index>& Row_Sizes, float Reserve_Perz = 0.2):
        _data(Row_Sizes, Reserve_Perz, 3) {}

    Const_Group_View get_faces(Index v0) const { return _data.row(v0); }

    bool vertex_is_next_to_face(Index v0, Index f0) const {
        for (auto f : get_faces(v0)) if (f == f0) return true;
        return false;
    }

    bool is_vertex_valid(Index v0) const { return _data.is_valid(v0); }

private:
    Group_View get_faces(Index v0) { return _data.row(v0); }
    Index face_append(Index v0, Index f0) { return _data.row_append(v0, f0); }
    void face_remove_swap(Index v0, Index slot) { _data.row_remove_swap(v0, slot); }
    void set_row_valid(Index v0) { _data.reinit_row(v0); }
    void set_row_invalid(Index v0) { _data.set_invalid(v0); }
    void reserve(Index size) { _data.reserve(size); }
    void set_largest_vertex_idx(Index v_idx) { _data.grow_row_count(v_idx); }
    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }
};

}
#endif