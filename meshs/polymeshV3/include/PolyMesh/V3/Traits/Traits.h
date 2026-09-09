#ifndef POLYMESH_V3_TRAITS
#define POLYMESH_V3_TRAITS
// ----------------------------------------------------------------------------
#include <cstdint>
#include <vector>
#include <span>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "PolyMesh/V3/DataStructs/Pool_Matrix.h"
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
#include "PolyMesh/V3/Traits/Faces.h"
#include "PolyMesh/V3/Traits/Vertices.h"
#include "PolyMesh/V3/Traits/EF.h"
#include "PolyMesh/V3/Traits/VV_HE.h"
#include "PolyMesh/V3/Traits/VF.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {

// ----------------------------------------------------------------------------
// Vec_Vec_Matrix: naive baseline — one std::vector<Index> per row, no free-list,
// no slack machinery, no id indirection. For comparison purposes only.
// ----------------------------------------------------------------------------
template<typename IndexT, typename BinSizeT, typename MeshT = void>
struct Vec_Vec_Matrix {
private:
    friend MeshT;

public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();

    using Group_View       = std::span<Index>;
    using Const_Group_View = std::span<const Index>;

private:
    std::vector<std::vector<Index>> _rows;   // indexed directly by vertex id — no id_to_rawId at all
    std::vector<uint8_t> _valid;

public:
    Vec_Vec_Matrix(float /*Reserve_Perz*/ = 0.2f) {}

    Const_Group_View get_edges(Index v0) const { return Const_Group_View(_rows[v0]); }
    Const_Group_View get_faces(Index v0) const { return Const_Group_View(_rows[v0]); } // alias for VF use

    bool edge_exists(Index v0, Index v1) const {
        for (Index e : _rows[v0]) if (e == v1) return true;
        return false;
    }
    bool vertex_is_next_to_face(Index v0, Index f0) const { return edge_exists(v0, f0); } // alias for VF use

    bool is_row_valid(Index v0) const { return v0 < _valid.size() && _valid[v0] != 0; }
    bool is_vertex_valid(Index v0) const { return v0 < _rows.size(); }

private:
    Group_View get_edges(Index v0) { return Group_View(_rows[v0]); }
    Group_View get_faces(Index v0) { return Group_View(_rows[v0]); } // alias for VF use

    Index edge_append(Index v0, Index v1) {
        _rows[v0].push_back(v1);
        return static_cast<Index>(_rows[v0].size() - 1);
    }
    Index face_append(Index v0, Index f0) { return edge_append(v0, f0); } // alias for VF use

    void edge_remove_swap(Index v0, Index slot) {
        auto& row = _rows[v0];
        row[slot] = row.back();
        row.pop_back();
    }
    void face_remove_swap(Index v0, Index slot) { edge_remove_swap(v0, slot); } // alias for VF use

    // caller (PM::_VV_HE_add_face / _VF_add_face) already guards this with is_row_valid,
    // same as the fix applied to reinit_row — safe to unconditionally clear here.
    void set_row_valid(Index v0)   { _valid[v0] = 1; _rows[v0].clear(); }
    void set_row_invalid(Index v0) { _valid[v0] = 0; _rows[v0].clear(); _rows[v0].shrink_to_fit(); }

    void reserve(Index /*size*/) {} // no meaningful global reserve for per-row vectors

    void set_largest_vertex_idx(Index v_idx) {
        if (v_idx <= static_cast<Index>(_rows.size())) return;
        _rows.resize(v_idx);
        _valid.resize(v_idx, 0);
    }

    
    // TODO
    void compact(std::span<Index> new_order, bool rebuild_reserve) {}
};




// ----------------------------------------------------------------------------
// Faces_Vec_Vec_Matrix: naive baseline for Faces — one std::vector<Index> per
// face, no shared data blob, no free-list. For comparison purposes only.
// ----------------------------------------------------------------------------
template<typename IndexT = std::uint32_t, typename FastBlockSizeT = uint8_t, typename MeshT = void>
struct Faces_Vec_Vec_Matrix {
private:
    friend MeshT;

public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();
    using Face = std::vector<Index>;

    // ------------------------------------------------------------------
    // View
    // ------------------------------------------------------------------
    template<bool IsConst>
    struct FaceViewImpl {
        using ElementType = std::conditional_t<IsConst, const Index, Index>;
        using pointer   = ElementType*;
        using reference = ElementType&;
        using iterator       = pointer;
        using const_iterator = const Index*;

    private:
        pointer _data = nullptr;
        Index   _size = 0;
        Index   _idx  = 0;

    public:
        FaceViewImpl() = default;
        FaceViewImpl(pointer Data, Index Size, Index Idx) noexcept:
            _data(Data), _size(Size), _idx(Idx) {}

        [[nodiscard]] reference operator[](Index i) const noexcept { return _data[i]; }
        [[nodiscard]] Index   size() const noexcept { return _size; }
        [[nodiscard]] pointer data() const noexcept { return _data; }
        [[nodiscard]] Index   idx()  const noexcept { return _idx; }

        [[nodiscard]] pointer begin() const noexcept { return _data; }
        [[nodiscard]] pointer end()   const noexcept { return _data + _size; }

        [[nodiscard]] std::span<ElementType> to_span() const noexcept { return {_data, static_cast<size_t>(_size)}; }
    };

    using View       = FaceViewImpl<false>;
    using Const_View = FaceViewImpl<true>;

    // ------------------------------------------------------------------
    // Iterator (skips invalid rows, same shape as D_Ragged_Matrix's)
    // ------------------------------------------------------------------
    template<bool IsConst>
    struct RowIteratorImpl {
        using MatrixPtr = std::conditional_t<IsConst, const Faces_Vec_Vec_Matrix*, Faces_Vec_Vec_Matrix*>;
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept  = std::forward_iterator_tag;
        using value_type = FaceViewImpl<IsConst>;
        using reference  = FaceViewImpl<IsConst>;

    private:
        MatrixPtr _matrix = nullptr;
        Index _idx = 0;

    public:
        RowIteratorImpl() = default;
        RowIteratorImpl(MatrixPtr m, Index idx) noexcept: _matrix(m), _idx(idx) {}

        [[nodiscard]] reference operator*() const noexcept { return (*_matrix)[_idx]; }

        RowIteratorImpl& operator++() noexcept {
            ++_idx;
            while(_idx < _matrix->capacity() && !_matrix->is_valid(_idx)) ++_idx;
            return *this;
        }
        RowIteratorImpl operator++(int) noexcept { auto tmp = *this; ++(*this); return tmp; }

        [[nodiscard]] bool operator==(const RowIteratorImpl& o) const noexcept { return _idx == o._idx; }
        [[nodiscard]] Index index() const noexcept { return _idx; }
    };

    using Iter       = RowIteratorImpl<false>;
    using Const_Iter = RowIteratorImpl<true>;

private:
    std::vector<Face>    _rows;
    std::vector<uint8_t> _valid;
    Index _valid_count = 0;

public:
    Faces_Vec_Vec_Matrix(float /*Reserve_Perz*/ = 0.2f) {}

    [[nodiscard]] View       operator[](Index Idx) noexcept       { return View(_rows[Idx].data(), static_cast<Index>(_rows[Idx].size()), Idx); }
    [[nodiscard]] Const_View operator[](Index Idx) const noexcept { return Const_View(_rows[Idx].data(), static_cast<Index>(_rows[Idx].size()), Idx); }

    [[nodiscard]] Index degree(Index Idx) const noexcept { return static_cast<Index>(_rows[Idx].size()); }
    [[nodiscard]] bool  is_valid(Index Idx) const noexcept { return Idx < _valid.size() && _valid[Idx] != 0; }

    [[nodiscard]] Index valid_amount() const noexcept { return _valid_count; }
    [[nodiscard]] Index capacity() const noexcept { return static_cast<Index>(_rows.size()); }

    [[nodiscard]] Iter begin() noexcept {
        Index first = 0;
        while(first < capacity() && !is_valid(first)) ++first;
        return Iter(this, first);
    }
    [[nodiscard]] Iter end() noexcept { return Iter(this, capacity()); }

    [[nodiscard]] Const_Iter begin() const noexcept {
        Index first = 0;
        while(first < capacity() && !is_valid(first)) ++first;
        return Const_Iter(this, first);
    }
    [[nodiscard]] Const_Iter end() const noexcept { return Const_Iter(this, capacity()); }

private:
    // NOTE: like D_Ragged_Matrix::append_row, this never reuses a freed logical id —
    // same leak-on-churn behavior as the real Faces_Ragged_Matrix, kept intentionally
    // so this is a fair baseline rather than an apples-to-oranges one.
    Index add(std::span<const Index> f) {
        Index idx = static_cast<Index>(_rows.size());
        _rows.emplace_back(f.begin(), f.end());
        _valid.push_back(1);
        ++_valid_count;
        return idx;
    }
    Index add(const Face& f) { return add(std::span<const Index>(f)); }

    std::vector<Index> add(const std::vector<std::vector<Index>>& f) {
        std::vector<Index> idx;
        idx.reserve(f.size());
        for(auto& fi : f) idx.push_back(add(std::span<const Index>(fi)));
        return idx;
    }

    void remove(Index Idx) noexcept {
        if(Idx >= _valid.size() || !_valid[Idx]) return;
        _valid[Idx] = 0;
        _rows[Idx].clear();
        _rows[Idx].shrink_to_fit();   // honest naive-equivalent of freeing the row's storage
        --_valid_count;
    }

    void expand(Index Idx, Index Deg) { _rows[Idx].resize(Deg, INDEX_INVALID); }

    void reserve(Index Data_Size) { _rows.reserve(Data_Size); } // approximation: reserves row COUNT, not total vertex-index count — Faces_Ragged_Matrix::reserve sizes the flat data blob instead, so this isn't a 1:1 match; flagging rather than pretending equivalence.

    void load_off(std::ifstream& file, Index f_count) {
        _rows.clear();  _rows.reserve(f_count);
        _valid.clear(); _valid.reserve(f_count);
        _valid_count = 0;

        for(Index fi = 0; fi < f_count; ++fi) {
            Index n;
            file >> n;
            Face face(n);
            for(Index j = 0; j < n; ++j) file >> face[j];
            _rows.push_back(std::move(face));
            _valid.push_back(1);
            ++_valid_count;
        }
    }

    
    // TODO
    void compact(std::span<Index> new_order, bool rebuild_reserve) {}
};



























// checks private friend accessable function requirements
struct _Traits_InternalC {
    template<typename T>
    static constexpr bool check_internal_requirements() {
        return requires {
            requires Vertices_InternalC<typename T::Vertices>;
            requires Faces_InternalC<typename T::Faces>;
            requires EF_InternalC<typename T::EF>;
            requires VV_HE_InternalC<typename T::VV_HE>;
            requires VF_InternalC<typename T::VF>;
        };
    }
};


template<typename T>
concept TraitsC = requires {
    typename T::Scalar;
    typename T::Index;

    typename T::Vertex;
    typename T::Face;
    typename T::Edge;

    typename T::Vertices;
    typename T::Faces;

    typename T::VV_HE;
    typename T::EF;
    typename T::VF;
    
    { T::INDEX_INVALID } -> std::convertible_to<typename T::Index>;

    { T::Reserve_Perz } -> std::convertible_to<float>;
    requires std::same_as<std::decay_t<decltype(T::Reserve_Perz)>, float>;

    typename T::template Rebind<_Traits_InternalC>;
}
&& Vertices_ExternalC<typename T::Vertices>
&& Faces_ExternalC<typename T::Faces>
&& EF_ExternalC<typename T::EF>
&& VV_HE_ExternalC<typename T::VV_HE>
&& VF_ExternalC<typename T::VF>
&& _Traits_InternalC::check_internal_requirements<typename T::template Rebind<_Traits_InternalC>>();












template<typename ScalarT, typename IndexT = std::uint32_t, typename BinSizeT = std::uint8_t, typename MeshT = void>
struct Traits {
    template<typename NewMeshT>
    using Rebind = Traits<ScalarT, IndexT, BinSizeT, NewMeshT>;   // for rebinding the friend object types
protected:
    friend MeshT;


public:
    using Scalar = ScalarT;
    using SparseScalar = std::uint8_t;

    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();
    using Signed_Index = typename std::make_signed<Index>;

    using Vertices = Vertices_Pool_Matrix<ScalarT, IndexT, MeshT>;
    using Faces = Faces_Ragged_Matrix<IndexT, BinSizeT, MeshT>;
    //using Faces = Faces_Vec_Vec_Matrix<IndexT, BinSizeT, MeshT>;

    using Vertex = std::array<Scalar, 3>;
    using Face = std::vector<Index>;
    
    using VV_HE = VV_HE_Ragged_Matrix<IndexT, BinSizeT, MeshT>;
    using VF = VF_Ragged_Matrix<IndexT, BinSizeT, MeshT>;

    using EF = EF_Ragged_Matrix<IndexT, BinSizeT, MeshT>;
    using Edge = EF::Edge;


    static constexpr float Reserve_Perz = 0.25f;
};

};

#endif