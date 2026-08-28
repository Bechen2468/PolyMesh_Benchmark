#ifndef POLYMESH_V3_TRAITS_VERTICES_POOL_MATRIX_H
#define POLYMESH_V3_TRAITS_VERTICES_POOL_MATRIX_H
// ----------------------------------------------------------------------------
#include <Eigen/Dense>
#include "PolyMesh/V3/DataStructs/Pool_Matrix.h"
// ----------------------------------------------------------------------------


namespace PolyMesh::V3 {

template<typename T>
concept Vertices_ExternalC = requires(
    T& t, 
    const T& ct, 
    typename T::Index idx
) {
    typename T::Index;
    { T::INDEX_INVALID } -> std::convertible_to<typename T::Index>;
    
    typename T::Scalar;
    typename T::View;
    typename T::Const_View;
    typename T::Iter;
    typename T::Const_Iter;

    typename T::Vertex;

    { t[idx] }  -> std::same_as<typename T::View>;
    { ct[idx] } -> std::same_as<typename T::Const_View>;

    { ct.is_valid(idx) }  -> std::same_as<bool>;
    { ct.valid_amount() } -> std::same_as<typename T::Index>;
    { ct.capacity() }     -> std::same_as<typename T::Index>;

    { t.begin() }  -> std::same_as<typename T::Iter>;
    { t.end() }    -> std::same_as<typename T::Iter>;
    { ct.begin() } -> std::same_as<typename T::Const_Iter>;
    { ct.end() }   -> std::same_as<typename T::Const_Iter>;
};


template<typename T>
concept Vertices_InternalC = requires(
    T& t, 
    typename T::Index idx,
    std::span<const typename T::Scalar, 3> coords
) {
    // single insert
    { t.add(coords) } -> std::same_as<typename T::Index>;
    { t.add(std::declval<const typename T::Vertex&>()) } -> std::same_as<typename T::Index>;

    // bulk insert ( not for init building )
    { t.add(std::declval<const std::vector<typename T::Vertex>&>()) } -> std::same_as<std::vector<typename T::Index>>;


    // more efficient for initial build then bulk add !
    { t.load_off(std::declval<std::ifstream&>(), std::declval<typename T::Index>()) } -> std::same_as<void>;

    { t.remove(idx) }  -> std::same_as<bool>;
    { t.reserve(idx) } -> std::same_as<void>;
};








template<typename ScalarT = float, typename IndexT = std::uint32_t, typename MeshT = void>
struct Vertices_Pool_Matrix {
private:
    friend MeshT;

    // 1. Underlying Matrix and Type Aliases
    using _Matrix = Pool_Matrix<ScalarT, 3, IndexT>;


public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();
    using Scalar = ScalarT;

    using View = typename _Matrix::RowView;
    using Const_View = typename _Matrix::Const_RowView;

    using Iter = typename _Matrix::Iter;
    using Const_Iter = typename _Matrix::Const_Iter;

    using Vertex = std::array<Scalar, 3>;

    // Eigen Types
    using EigenRowMap      = typename _Matrix::EigenRowMap;
    using EigenConstRowMap = typename _Matrix::EigenConstRowMap;
    using EigenMatrixMap   = typename _Matrix::EigenMatrixMap;
    using EigenConstMatrixMap = typename _Matrix::EigenConstMatrixMap;

private:
    _Matrix _data;

public:
    Vertices_Pool_Matrix(float Reserve_Perz):
        _data(0, Reserve_Perz) 
    {}

    
// ----------------------------------------------------------------------------
// Accessors
// ----------------------------------------------------------------------------
    [[nodiscard]] View       operator[](Index idx) noexcept { return _data.row(idx); }
    [[nodiscard]] Const_View operator[](Index idx) const noexcept { return _data.row(idx); }

/*
    [[nodiscard]] EigenRowMap      eigen_vertex(Index idx) noexcept {  return _data.eigen_row(idx); }
    [[nodiscard]] EigenConstRowMap eigen_vertex(Index idx) const noexcept { return _data.eigen_row(idx); }

    /// Returns a zero copy Eigen Map over the raw array ( includes invalids !) => Idx are preserved this way
    [[nodiscard]] EigenMatrixMap      eigen_matrix() noexcept { return _data.eigen_matrix(); }
    [[nodiscard]] EigenConstMatrixMap eigen_matrix() const noexcept { return _data.eigen_matrix(); }
*/

    [[nodiscard]] bool is_valid(Index idx) const noexcept { return _data.row_is_valid(idx); }


// ----------------------------------------------------------------------------
// matrix data
// ----------------------------------------------------------------------------
    [[nodiscard]] Index valid_amount() const noexcept { return _data.valid_amount(); }
    [[nodiscard]] Index capacity() const noexcept { return _data.size(); }


// ----------------------------------------------------------------------------
// Iterator Accessors
// ----------------------------------------------------------------------------
    [[nodiscard]] Iter begin() noexcept { return _data.begin(); }
    [[nodiscard]] Iter end() noexcept { return _data.end(); }
    
    [[nodiscard]] Const_Iter begin() const noexcept { return _data.begin(); }
    [[nodiscard]] Const_Iter end() const noexcept { return _data.end(); }

    [[nodiscard]] Const_Iter cbegin() const noexcept { return _data.cbegin(); }
    [[nodiscard]] Const_Iter cend() const noexcept { return _data.cend(); }



private:
    // std::span<const ScalarT, 3>, std::array, std::vector, raw arrays
    Index add(std::span<const Scalar, 3> v) { return _data.insert(v); }
    Index add(const Vertex& v) { return _data.insert(std::span<const Scalar, 3>(v)); }

    // TODO: Make more efficient ( not real bulk insert rn ! )
    std::vector<Index> add(const std::vector<Vertex>& v) { 
        std::vector<Index> idx;
        idx.reserve(v.size());

        for(auto& vi : v) {
            idx.push_back(add(std::span<const Scalar, 3>(vi)));
        }

        return idx; 
    }

    bool remove(Index idx) noexcept { return _data.delete_row(idx); }

    void reserve(Index total_vertices) { _data.reserve(total_vertices); }


    void load_off(std::ifstream& file, Index v_count) {
        reserve(v_count);

        for (Index i = 0; i < v_count; ++i) {
            std::array<Scalar, 3> v;
            file >> v[0] >> v[1] >> v[2];
            _data.insert(std::span<const Scalar, 3>(v));
        }
    }
};
};


#endif
