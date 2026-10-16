#ifndef POLYMESH_V3_TRAITS_FACES_RAGGED_MATRIX_H
#define POLYMESH_V3_TRAITS_FACES_RAGGED_MATRIX_H
// ----------------------------------------------------------------------------
#include <Eigen/Dense>
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
#include "PolyMesh/V3/DataStructs/Append_Ragged_Matrix.h"
#include "PolyMesh/V3/DataStructs/VecVec_Matrix.h"
#include "PolyMesh/V3/DataStructs/Vec_Eigen_Matrix.h"
// ----------------------------------------------------------------------------


namespace PolyMesh::V3 {

template<typename T>
concept Faces_ExternalC = requires(T& t, const T& ct, typename T::Index idx) {
    typename T::Index;
    { T::INDEX_INVALID } -> std::convertible_to<typename T::Index>;
    
    typename T::View;
    typename T::Const_View;
    typename T::Iter;
    typename T::Const_Iter;

    typename T::Face;

    { t[idx] }  -> std::same_as<typename T::View>;
    { ct[idx] } -> std::same_as<typename T::Const_View>;

    { ct.degree(idx) }    -> std::same_as<typename T::Index>;
    { ct.is_valid(idx) }  -> std::same_as<bool>;
    { ct.valid_amount() } -> std::same_as<typename T::Index>;
    { ct.capacity() }     -> std::same_as<typename T::Index>;

    { t.begin() }  -> std::same_as<typename T::Iter>;
    { t.end() }    -> std::same_as<typename T::Iter>;
    { ct.begin() } -> std::same_as<typename T::Const_Iter>;
    { ct.end() }   -> std::same_as<typename T::Const_Iter>;
};



template<typename T>
concept Faces_InternalC = requires(
    T& t, typename T::Index idx,
    std::span<const typename T::Index> verts,
    std::span<const typename T::Index> old_to_new, bool rebuild_reserve
) {
    { t.add(verts) } -> std::same_as<typename T::Index>;
    { t.add(std::declval<const typename T::Face&>()) } -> std::same_as<typename T::Index>;

    // bulk insert
    { t.add(std::declval<const std::vector<std::vector<typename T::Index>>&>()) }
        -> std::same_as<std::vector<typename T::Index>>;


    { t.compact(old_to_new, rebuild_reserve) } -> std::same_as<void>;

    // more efficient for initial build then bulk add !
    { t.load_off(std::declval<std::ifstream&>(), std::declval<typename T::Index>()) } -> std::same_as<void>;

    { t.remove(idx) } -> std::same_as<void>;
    { t.expand(idx, typename T::Index{}) } -> std::same_as<void>;
    { t.reserve(idx) } -> std::same_as<void>;
};
    











template<typename IndexT, typename MeshT = void>
struct Faces_D_Ragged_Matrix {
private:
    friend MeshT;
    using _Matrix = D_Ragged_Matrix<IndexT, IndexT, true>;
 
public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = _Matrix::INDEX_INVALID;
 
    using View       = typename _Matrix::View;
    using Const_View = typename _Matrix::Const_View;
    using Iter       = typename _Matrix::Iter;
    using Const_Iter = typename _Matrix::Const_Iter;
    using Face       = std::vector<Index>;
 
private:
    _Matrix _data;
 
public:
    Faces_D_Ragged_Matrix(float Reserve_Perz): _data(Reserve_Perz, 0.0f, 0) {}
 
    [[nodiscard]] View operator[](Index Idx) noexcept { return _data.row(Idx); }
    [[nodiscard]] Const_View operator[](Index Idx) const noexcept { return _data.row(Idx); }
    [[nodiscard]] Index degree(Index Idx) const noexcept { return _data.row_size(Idx); }
    [[nodiscard]] bool is_valid(Index Idx) const noexcept { return _data.is_valid(Idx); }
    [[nodiscard]] Index valid_amount() const noexcept { return _data.row_count(); }
    [[nodiscard]] Index capacity() const noexcept { return _data.row_capacity(); }
    [[nodiscard]] Iter begin() noexcept { return _data.begin(); }
    [[nodiscard]] Iter end() noexcept { return _data.end(); }
    [[nodiscard]] Const_Iter begin() const noexcept { return _data.begin(); }
    [[nodiscard]] Const_Iter end() const noexcept { return _data.end(); }
 
private:
    Index add(std::span<const Index> f) { return _data.add_row(f); }
    Index add(const Face& f) { return add(std::span<const Index>(f)); }
 
    std::vector<Index> add(const std::vector<std::vector<Index>>& f) {
        std::vector<Index> idx;
        idx.reserve(f.size());
        for (auto& fi : f) idx.push_back(add(std::span<const Index>(fi)));
        return idx;
    }
 
    void remove(Index Idx) noexcept { _data.set_invalid(Idx); }
    void expand(Index Idx, Index Deg) { _data.resize_row(Idx, Deg); }
    void reserve(Index Data_Size) { _data.reserve(Data_Size); }
    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }
 
    void load_off(std::ifstream& file, Index f_count) {
        reserve(f_count * 3);
        std::vector<Index> face_buf;
        for (Index fi = 0; fi < f_count; ++fi) {
            Index n; file >> n;
            face_buf.resize(n);
            for (Index j = 0; j < n; ++j) file >> face_buf[j];
            add(face_buf);
        }
    }
};





template<typename IndexT, typename MeshT = void>
struct Faces_Append_Ragged_Matrix {
private:
    friend MeshT;

    using _Matrix = Append_Ragged_Matrix<IndexT, IndexT, true>;


public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = _Matrix::INDEX_INVALID;

    using View       = typename _Matrix::View;
    using Const_View = typename _Matrix::Const_View;
    using Iter       = typename _Matrix::Iter;
    using Const_Iter = typename _Matrix::Const_Iter;
    
    using Face = std::vector<Index>;


private:
    _Matrix _data;

public:
    Faces_Append_Ragged_Matrix(float Reserve_Perz):
        _data(Reserve_Perz, 0.0f, 0) {}


    [[nodiscard]] View operator[](Index Idx) noexcept { return _data.row(Idx); }
    [[nodiscard]] Const_View operator[](Index Idx) const noexcept { return _data.row(Idx); }

    [[nodiscard]] Index degree(Index Idx) const noexcept { return _data.row_size(Idx); }
    [[nodiscard]] bool is_valid(Index Idx) const noexcept { return _data.is_valid(Idx); }
    

    [[nodiscard]] Index valid_amount() const noexcept { return _data.row_count(); }
    [[nodiscard]] Index capacity() const noexcept { return _data.row_capacity(); }
    

    [[nodiscard]] Iter begin() noexcept { return _data.begin(); }
    [[nodiscard]] Iter end() noexcept { return _data.end(); }
    [[nodiscard]] Const_Iter begin() const noexcept { return _data.begin(); }
    [[nodiscard]] Const_Iter end() const noexcept { return _data.end(); }


private:
    Index add(std::span<const Index> f) {
        return _data.add_row(f);
    }

    Index add(const Face& f) {
        return add(std::span<const Index>(f));
    }

    // TODO: make acctual bulk
    std::vector<Index> add(const std::vector<std::vector<Index>>& f) {
        std::vector<Index> idx;
        idx.reserve(f.size());

        for(auto& fi : f) {
            idx.push_back(add(std::span<const Index>(fi)));
        }

        return idx; 
    }

    void remove(Index Idx) noexcept { _data.set_invalid(Idx); }

    void expand(Index Idx, Index Deg) { _data.resize_row(Idx, Deg); }
    
    void reserve(Index Data_Size) { _data.reserve(Data_Size); }



    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }

    void load_off(std::ifstream& file, Index f_count) {
    reserve(f_count * 3);

    std::vector<Index> face_buf;
    for (Index fi = 0; fi < f_count; ++fi) {
        Index n;
        file >> n;

        face_buf.resize(n);
        for (Index j = 0; j < n; ++j) file >> face_buf[j];

        add(face_buf);
    }
}
};









template<typename IndexT, typename MeshT = void>
struct Faces_Vec_Vec_Matrix {
private:
    friend MeshT;
    using _Matrix = Vec_Vec_Matrix<IndexT, IndexT, true>;

public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = _Matrix::INDEX_INVALID;

    using View       = typename _Matrix::View;
    using Const_View = typename _Matrix::Const_View;
    using Iter       = typename _Matrix::Iter;
    using Const_Iter = typename _Matrix::Const_Iter;
    using Face       = std::vector<Index>;

private:
    _Matrix _data;

public:
    Faces_Vec_Vec_Matrix(float Reserve_Perz): 
        _data(Reserve_Perz, 0) {}

    [[nodiscard]] View operator[](Index Idx) noexcept { return _data.row(Idx); }
    [[nodiscard]] Const_View operator[](Index Idx) const noexcept { return _data.row(Idx); }
    [[nodiscard]] Index degree(Index Idx) const noexcept { return _data.row_size(Idx); }
    [[nodiscard]] bool is_valid(Index Idx) const noexcept { return _data.is_valid(Idx); }
    [[nodiscard]] Index valid_amount() const noexcept { return _data.row_count(); }
    [[nodiscard]] Index capacity() const noexcept { return _data.row_capacity(); }
    [[nodiscard]] Iter begin() noexcept { return _data.begin(); }
    [[nodiscard]] Iter end() noexcept { return _data.end(); }
    [[nodiscard]] Const_Iter begin() const noexcept { return _data.begin(); }
    [[nodiscard]] Const_Iter end() const noexcept { return _data.end(); }

private:
    Index add(std::span<const Index> f) { return _data.add_row(f); }
    Index add(const Face& f) { return add(std::span<const Index>(f)); }

    std::vector<Index> add(const std::vector<std::vector<Index>>& f) {
        std::vector<Index> idx;
        idx.reserve(f.size());
        for (auto& fi : f) idx.push_back(add(std::span<const Index>(fi)));
        return idx;
    }

    void remove(Index Idx) noexcept { _data.set_invalid(Idx); }
    void expand(Index Idx, Index Deg) { _data.resize_row(Idx, Deg); }
    void reserve(Index Data_Size) { _data.reserve(Data_Size); }
    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }

    void load_off(std::ifstream& file, Index f_count) {
        reserve(f_count * 3);
        std::vector<Index> face_buf;
        for (Index fi = 0; fi < f_count; ++fi) {
            Index n; file >> n;
            face_buf.resize(n);
            for (Index j = 0; j < n; ++j) file >> face_buf[j];
            add(face_buf);
        }
    }
};











template<typename IndexT, typename MeshT = void>
struct Faces_Vec_Eigen_Matrix {
private:
    friend MeshT;
    using _Matrix = Vec_Eigen_Matrix<IndexT, IndexT, true>;

public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = _Matrix::INDEX_INVALID;

    using View       = typename _Matrix::View;
    using Const_View = typename _Matrix::Const_View;
    using Iter       = typename _Matrix::Iter;
    using Const_Iter = typename _Matrix::Const_Iter;
    using Face       = std::vector<Index>;

private:
    _Matrix _data;

public:
    Faces_Vec_Eigen_Matrix(float Reserve_Perz): _data(Reserve_Perz, 0.0f, 0) {}

    [[nodiscard]] View operator[](Index Idx) noexcept { return _data.row(Idx); }
    [[nodiscard]] Const_View operator[](Index Idx) const noexcept { return _data.row(Idx); }
    [[nodiscard]] Index degree(Index Idx) const noexcept { return _data.row_size(Idx); }
    [[nodiscard]] bool is_valid(Index Idx) const noexcept { return _data.is_valid(Idx); }
    [[nodiscard]] Index valid_amount() const noexcept { return _data.row_count(); }
    [[nodiscard]] Index capacity() const noexcept { return _data.row_capacity(); }
    [[nodiscard]] Iter begin() noexcept { return _data.begin(); }
    [[nodiscard]] Iter end() noexcept { return _data.end(); }
    [[nodiscard]] Const_Iter begin() const noexcept { return _data.begin(); }
    [[nodiscard]] Const_Iter end() const noexcept { return _data.end(); }

private:
    Index add(std::span<const Index> f) { return _data.add_row(f); }
    Index add(const Face& f) { return add(std::span<const Index>(f)); }

    std::vector<Index> add(const std::vector<std::vector<Index>>& f) {
        std::vector<Index> idx;
        idx.reserve(f.size());
        for (auto& fi : f) idx.push_back(add(std::span<const Index>(fi)));
        return idx;
    }

    void remove(Index Idx) noexcept { _data.set_invalid(Idx); }
    void expand(Index Idx, Index Deg) { _data.resize_row(Idx, Deg); }
    void reserve(Index Data_Size) { _data.reserve(Data_Size); }
    void compact(std::span<const Index> new_order, bool rebuild_reserve) { _data.compact(new_order, rebuild_reserve); }

    void load_off(std::ifstream& file, Index f_count) {
        reserve(f_count * 3);
        std::vector<Index> face_buf;
        for (Index fi = 0; fi < f_count; ++fi) {
            Index n; file >> n;
            face_buf.resize(n);
            for (Index j = 0; j < n; ++j) file >> face_buf[j];
            add(face_buf);
        }
    }
};

};
#endif

