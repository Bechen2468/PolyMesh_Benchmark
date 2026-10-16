#ifndef POLYMESH_V3_APPEND_RAGGED_MATRIX
#define POLYMESH_V3_APPEND_RAGGED_MATRIX
// ----------------------------------------------------------------------------
#include <cstdint>
#include "PolyMesh/V3/DataStructs/Aligned_Allocator.h"
#include "PolyMesh/V3/DataStructs/Enablable_Queue.h"
// ----------------------------------------------------------------------------


namespace PolyMesh::V3 {




template<typename DataT, typename IndexT, bool ReuseIdsT = true>
struct Append_Ragged_Matrix {
// ----------------------------------------------------------------------------
// Type defines
// ----------------------------------------------------------------------------
public:
    using Index = IndexT;
    using Data = DataT;

    template<typename T>
    using Vec = std::vector<T, Aligned_Allocator<T, 64>>;
    //using Vec = std::vector<T>;

    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();

    struct Row {
        Index begin;
        Index size;
        Index reserve;
    };


// ----------------------------------------------------------------------------
// Data
// ----------------------------------------------------------------------------
private:
    float _reserve_Perz;
    float _row_reserve_perz;
    Index _row_reserve_min_amount;

    Vec<Data> _data;
    Vec<Row> _rows;

    [[no_unique_address]] Enablable_Queue<ReuseIdsT, IndexT> _free_row_ids;
    Index _valid_count = 0;

    void _ensure_data_capacity(Index Needed_Size);
    Index _initial_row_reserve(Index Requested_Size) const;
    Index _grown_row_reserve(Index Old_Reserve, Index Needed_Size) const;



// ----------------------------------------------------------------------------
// public functions
// ----------------------------------------------------------------------------
public:
    Append_Ragged_Matrix(float Reserve_Perz = 0.2f, float Row_Reserve_Perz = 0.34f, Index Row_Reserve_Min_Amount = 1);
    Append_Ragged_Matrix(const std::vector<Index>& Row_Sizes, float Reserve_Perz = 0.2f, float Row_Reserve_Perz = 0.34f, Index Row_Reserve_Min_Amount = 1);


    void reserve(Index Data_Size) { _ensure_data_capacity(Data_Size); }

    void reinit_row(Index Idx);
    Index add_row(std::span<const DataT> Row_Data);
    Index add_row(Index Size);

    void resize_row(Index Idx, Index Size);

    void grow_row_count(Index Count);

    [[nodiscard]] bool is_valid(Index Idx) const { return _rows[Idx].begin != INDEX_INVALID; }
    void set_valid(Index Idx);
    void set_invalid(Index Idx);

    void row_remove_swap(Index Idx, Index Slot_Idx);
    Index row_append(Index Idx, const DataT& Value);


    [[nodiscard]] Index row_capacity() const { return static_cast<Index>(_rows.size()); }
    [[nodiscard]] Index row_count() const { return _valid_count; }  
    [[nodiscard]] Index row_size(Index Idx) const { return _rows[Idx].size; }
    [[nodiscard]] Index row_reserve(Index Idx) const { return _rows[Idx].reserve; }

    
    // compacts the data array down 
    void compact(std::span<const Index> Old_To_New = {}, bool Rebuild_Reserve = true);




    
// ----------------------------------------------------------------------------
// Iterators
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Row_View
// ----------------------------------------------------------------------------
public:
    template<bool IsConstT>
    struct Row_View {
        using ElementType = std::conditional_t<IsConstT, const DataT, DataT>;
        using pointer = ElementType*;

    private:
        pointer _data = nullptr;
        Index   _size = 0;
        Index   _idx  = 0;

    public:
        Row_View() = default;
        Row_View(pointer Data, Index Size, Index Idx) noexcept: _data(Data), _size(Size), _idx(Idx) {}

        [[nodiscard]] ElementType& operator[](Index i) const noexcept { return _data[i]; }
        [[nodiscard]] Index   size() const noexcept { return _size; }
        [[nodiscard]] pointer data() const noexcept { return _data; }
        [[nodiscard]] Index   idx()  const noexcept { return _idx; }

        [[nodiscard]] pointer begin() const noexcept { return _data; }
        [[nodiscard]] pointer end()   const noexcept { return _data + _size; }

        [[nodiscard]] std::span<ElementType> to_span() const noexcept { return {_data, static_cast<size_t>(_size)}; }
    };

    using View       = Row_View<false>;
    using Const_View = Row_View<true>;


    [[nodiscard]] View row(Index Idx) {
        return View(_data.data() + _rows[Idx].begin, _rows[Idx].size, Idx);
    }

    [[nodiscard]] Const_View row(Index Idx) const {
        return Const_View(_data.data() + _rows[Idx].begin, _rows[Idx].size, Idx);
    }


// ----------------------------------------------------------------------------
// Row_Iter
// ----------------------------------------------------------------------------
    template<bool IsConstT>
    struct RowIter {
    public:
        using MatrixPtr = std::conditional_t<IsConstT, const Append_Ragged_Matrix*, Append_Ragged_Matrix*>;
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept  = std::forward_iterator_tag;
        using value_type = Row_View<IsConstT>;
        using reference  = Row_View<IsConstT>;

    private:
        MatrixPtr _matrix = nullptr;
        Index _idx = 0;

        void _skip_invalid() noexcept {
            while (_idx < _matrix->row_capacity() && !_matrix->is_valid(_idx)) ++_idx;
        }

    public:
        RowIter() = default;
        RowIter(MatrixPtr Matrix_Source, Index Idx) noexcept: _matrix(Matrix_Source), _idx(Idx) { _skip_invalid(); }

        [[nodiscard]] reference operator*() const noexcept {
            return _matrix->row(_idx);
        }

        RowIter& operator++() noexcept { ++_idx; _skip_invalid(); return *this; }
        RowIter operator++(int) noexcept { auto tmp = *this; ++(*this); return tmp; }

        [[nodiscard]] bool operator==(const RowIter& o) const noexcept { return _idx == o._idx; }
        [[nodiscard]] Index index() const noexcept { return _idx; }
    };

    using Iter = RowIter<false>;
    using Const_Iter = RowIter<true>;

    [[nodiscard]] Iter begin() noexcept { return Iter(this, 0); }
    [[nodiscard]] Iter end() noexcept { return Iter(this, row_capacity()); }
    [[nodiscard]] Const_Iter begin() const noexcept { return Const_Iter(this, 0); }
    [[nodiscard]] Const_Iter end() const noexcept { return Const_Iter(this, row_capacity()); }
};


#define ARM_T template<typename DataT, typename IndexT, bool ReuseIdsT>
#define ARM Append_Ragged_Matrix<DataT, IndexT, ReuseIdsT>

}

#include "PolyMesh/V3/DataStructs/Append_Ragged_Matrix.inl"

#endif
