#ifndef POLYMESH_V3_D_RAGGED_MATRIX_H
#define POLYMESH_V3_D_RAGGED_MATRIX_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include <vector>
#include <span>
#include <map>
#include <cassert>
#include <Eigen/Dense>
#include "PolyMesh/V3/DataStructs/Aligned_Allocator.h"
#include "PolyMesh/V3/DataStructs/Enablable_Queue.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {


 template<typename DataT, typename IndexT = std::uint32_t, bool ReuseIdsT = true, typename FastBlockSizeT = std::uint32_t>
struct D_Ragged_Matrix {
// ----------------------------------------------------------------------------
// Core Structs
// ----------------------------------------------------------------------------
public:
    using Index = IndexT;
    using Data  = DataT;
 
    template<typename T>
    using Vec = std::vector<T, Aligned_Allocator<T, 64>>;
 
    static_assert(std::is_integral_v<IndexT> && std::is_unsigned_v<IndexT>, "Index must be an unsigned integer type");
    static_assert(std::is_trivially_copyable_v<DataT> && sizeof(DataT) >= sizeof(IndexT), "free blocks store their record id inside the free slots");
    static_assert(std::is_unsigned_v<FastBlockSizeT>, "FastBlockSizeT is used as a bitmask");
 
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();
    static constexpr Index SMALL_BINS    = static_cast<Index>(sizeof(FastBlockSizeT) * 8);   // exact-size bins 1..SMALL_BINS
 

    struct Row {
        Index begin;
        Index size;
        Index reserve;
    };
 
private:
    struct FreeBlock {
        Index begin;
        Index size;
        Index next;
        Index prev;
    };
 
 
// ----------------------------------------------------------------------------
// Iterators: Row_View
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
 
        [[nodiscard]] std::span<ElementType> to_span() const noexcept { return {_data, static_cast<std::size_t>(_size)}; }
    };
 
    using View       = Row_View<false>;
    using Const_View = Row_View<true>;
 
 
// ----------------------------------------------------------------------------
// Iterators: RowIter (valid rows only)
// ----------------------------------------------------------------------------
    template<bool IsConstT>
    struct RowIter {
        using MatrixPtr = std::conditional_t<IsConstT, const D_Ragged_Matrix*, D_Ragged_Matrix*>;
 
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept  = std::forward_iterator_tag;
        using value_type        = Row_View<IsConstT>;
        using reference         = Row_View<IsConstT>;
 
    private:
        MatrixPtr _matrix = nullptr;
        Index     _idx    = 0;
 
        void _skip_invalid() noexcept {
            while (_idx < _matrix->row_capacity() && !_matrix->is_valid(_idx)) ++_idx;
        }
 
    public:
        RowIter() = default;
        RowIter(MatrixPtr M, Index Idx) noexcept: _matrix(M), _idx(Idx) { _skip_invalid(); }
 
        [[nodiscard]] reference operator*() const noexcept { return _matrix->row(_idx); }
        RowIter& operator++() noexcept { ++_idx; _skip_invalid(); return *this; }
        RowIter  operator++(int) noexcept { auto t = *this; ++(*this); return t; }
 
        [[nodiscard]] bool  operator==(const RowIter& o) const noexcept { return _idx == o._idx; }
        [[nodiscard]] Index index() const noexcept { return _idx; }
    };
 
    using Iter       = RowIter<false>;
    using Const_Iter = RowIter<true>;
 
 
// ----------------------------------------------------------------------------
// Variables
// ----------------------------------------------------------------------------
private:
    // Settings
    float _reserve_perz;
    float _row_reserve_perz;
    Index _row_reserve_min_amount;
 
    // Data
    Vec<Data> _data;
    Vec<Row>  _rows;
    Index     _valid_count = 0;
    Enablable_Queue<ReuseIdsT, IndexT> _free_row_ids;
 
    // Free space
    Vec<FreeBlock>                                _fb;                // free block records
    std::vector<Index>                            _fb_unused;         // recycled record ids
    std::array<Index, sizeof(FastBlockSizeT) * 8> _small_head;        // exact-size bins 1..SMALL_BINS
    FastBlockSizeT                                _small_mask = 0;    // bit s-1 set: bin s not empty
    std::map<Index, Index>                        _large_head;        // size -> first block (sizes > SMALL_BINS)
    std::vector<std::uint64_t>                    _edge_bits;         // bit x: slot x is the first or last slot of a free block
 
 
// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------
public:
    D_Ragged_Matrix(float Reserve_Perz = 0.2f, float Row_Reserve_Perz = 0.34f, Index Row_Reserve_Min_Amount = 1);
 
    // Row_Sizes.size() valid, EMPTY rows laid out back to back; a free block of Reserve_Perz * total sits behind them
    explicit D_Ragged_Matrix(const std::vector<Index>& Row_Sizes, float Reserve_Perz = 0.2f,
                             float Row_Reserve_Perz = 0.34f, Index Row_Reserve_Min_Amount = 1);
 
    // Data_Size: wanted number of slots in the pool; the new part becomes free space
    void reserve(Index Data_Size);
 
    Index add_row(std::span<const DataT> Row_Data);
    Index add_row(Index Size);
    void  reinit_row(Index Idx);
    void  resize_row(Index Idx, Index Size);
 
    // Count = number of row slots that must exist afterwards; new slots are invalid
    void grow_row_count(Index Count);
 
    void set_valid(Index Idx);
    void set_invalid(Index Idx);
 
    void  row_remove_swap(Index Idx, Index Slot_Idx);
    Index row_append(Index Idx, const DataT& Value);
 
    
    void compact(std::span<const Index> Old_To_New = {}, bool Rebuild_Reserve = true);
 
    [[nodiscard]] bool  is_valid(Index Idx) const     { return _rows[Idx].begin != INDEX_INVALID; }
    [[nodiscard]] Index row_capacity() const          { return static_cast<Index>(_rows.size()); }
    [[nodiscard]] Index row_count() const             { return _valid_count; }
    [[nodiscard]] Index row_size(Index Idx) const     { return _rows[Idx].size; }
    [[nodiscard]] Index row_reserve(Index Idx) const  { return _rows[Idx].reserve; }
 
    // Free-space stats
    [[nodiscard]] Index       pool_size() const        { return _pool_size(); }
    [[nodiscard]] Index       free_block_count() const { return static_cast<Index>(_fb.size() - _fb_unused.size()); }
    [[nodiscard]] Index       free_slot_count() const;
    [[nodiscard]] std::size_t memory_bytes() const;
 
    
    [[nodiscard]] int debug_validate() const;
 
 
// ----------------------------------------------------------------------------
// Old D_Ragged names (for existing calls)
// ----------------------------------------------------------------------------
    void  delete_row(Index Idx)                     { set_invalid(Idx); }
    Index append_row(std::span<const DataT> Data)   { return add_row(Data); }
    void  row_resize(Index Idx, Index Size)         { resize_row(Idx, Size); }
    void  reserve_row(Index Row_Reserve_Min_Amount) { _row_reserve_min_amount = Row_Reserve_Min_Amount; }
    [[nodiscard]] Index      row_reserve_size(Index Idx) const { return row_reserve(Idx); }
    [[nodiscard]] Const_View get_row(Index Idx) const          { return row(Idx); }
 
 
// ----------------------------------------------------------------------------
// Getter / Iteration
// ----------------------------------------------------------------------------
    [[nodiscard]] View row(Index Idx) noexcept {
        const Row& r = _rows[Idx];
        return View(_data.data() + r.begin, r.size, Idx);
    }
    [[nodiscard]] Const_View row(Index Idx) const noexcept {
        const Row& r = _rows[Idx];
        return Const_View(_data.data() + r.begin, r.size, Idx);
    }
 
    [[nodiscard]] Iter       begin() noexcept       { return Iter(this, 0); }
    [[nodiscard]] Iter       end() noexcept         { return Iter(this, row_capacity()); }
    [[nodiscard]] Const_Iter begin() const noexcept { return Const_Iter(this, 0); }
    [[nodiscard]] Const_Iter end() const noexcept   { return Const_Iter(this, row_capacity()); }
 
 
// ----------------------------------------------------------------------------
// Internal
// ----------------------------------------------------------------------------
private:
    [[nodiscard]] Index _pool_size() const { return static_cast<Index>(_data.size()); }
    [[nodiscard]] Index _initial_row_reserve(Index Requested_Size) const;
    [[nodiscard]] Index _grown_row_reserve(Index Old_Reserve, Index Needed_Size) const;
 
    Index _place_row(const Row& R);
    void  _grow_row(Index Idx, Index Needed);
    void  _resize_pool(Index N);
 
    // edge bits + in-place tags
    [[nodiscard]] bool _bit(Index x) const { return (_edge_bits[x >> 6] >> (x & 63)) & 1u; }
    void _set_bit(Index x) { _edge_bits[x >> 6] |=  (std::uint64_t(1) << (x & 63)); }
    void _clr_bit(Index x) { _edge_bits[x >> 6] &= ~(std::uint64_t(1) << (x & 63)); }
    void _write_tag(Index x, Index Id);
    void _mark(Index Id);
    void _unmark(Index Id);
    [[nodiscard]] Index _tag(Index x) const;
    [[nodiscard]] bool  _is_free_start(Index Pos) const { return _bit(Pos) && _fb[_tag(Pos)].begin == Pos; }
 
    // free block records + size bins
    Index _new_record(Index Begin, Index Size);
    void  _drop_record(Index Id);
    void  _bin_insert(Index Id);
    void  _bin_remove(Index Id);
 
    // free space operations
    [[nodiscard]] Index _find_fit(Index N) const;
    Index _take(Index N);
    bool  _try_extend(Index End, Index Extra);
    void  _free_span(Index Begin, Index Size);
    void  _reset_free_space();
};
 
};
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.inl"
#endif

