#pragma once
// ----------------------------------------------------------------------------
#include <Eigen/SparseCore>
#include <vector>
#include <span>
#include <limits>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <iterator>
#include <type_traits>
#include "PolyMesh/V3/DataStructs/Enablable_Queue.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {



template<typename DataT, typename IndexT, bool ReuseIdsT = true>
struct Vec_Eigen_Matrix {
public:
    using Index = IndexT;
    using Data  = DataT;
    static_assert(std::is_integral_v<IndexT>, "Index must be an integer type");
    static_assert(std::is_default_constructible_v<Data>, "Data must be default constructible");
 
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();
 
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
 
    // Reserve_Perz: spare capacity at the back (shared array and row table). Row_Reserve_*: per-row reserve, as in Append.
    Vec_Eigen_Matrix(float Reserve_Perz = 0.2f, float Row_Reserve_Perz = 0.34f, Index Row_Reserve_Min_Amount = 1)
        : _reserve_perz(Reserve_Perz), _row_reserve_perz(Row_Reserve_Perz), _row_reserve_min_amount(Row_Reserve_Min_Amount) {}


    Vec_Eigen_Matrix(const std::vector<Index>& Row_Sizes, float Reserve_Perz = 0.2f, float Row_Reserve_Perz = 0.34f, Index Row_Reserve_Min_Amount = 1)
        : Vec_Eigen_Matrix(Reserve_Perz, Row_Reserve_Perz, Row_Reserve_Min_Amount)
    {
        const Index n = static_cast<Index>(Row_Sizes.size());
        if (n == 0) return;

        _sp.resize(n, 1);
        _sp.uncompress();
        SIdx* o  = _outer();
        SIdx* nz = _nnz();
        SIdx pos = 0;
        for (Index i = 0; i < n; ++i) {
            o[i]  = pos;
            nz[i] = 0;
            pos  += static_cast<SIdx>(_initial_row_reserve(Row_Sizes[i]));
        }
        o[n] = pos;
        _sp.data().resize(static_cast<typename Sp::Index>(pos), static_cast<double>(_reserve_perz));

        _slots = n;
        _valid.assign(n, true);
        _valid_count = n;
    }
 

    // Copy must NOT go through Eigen::SparseMatrix::operator=: that treats the matrix as a real sparse matrix and
    // requires sorted inner indices, which this class never fills. The arrays are copied directly instead.
    Vec_Eigen_Matrix(const Vec_Eigen_Matrix& o)
        : _reserve_perz(o._reserve_perz), _row_reserve_perz(o._row_reserve_perz), _row_reserve_min_amount(o._row_reserve_min_amount),
          _sp(), _slots(o._slots), _valid(o._valid), _free_row_ids(o._free_row_ids), _valid_count(o._valid_count) {
        _copy_sp_from(o);
    }

    Vec_Eigen_Matrix& operator=(const Vec_Eigen_Matrix& o) {
        if (this == &o) return *this;
        _reserve_perz = o._reserve_perz; _row_reserve_perz = o._row_reserve_perz; _row_reserve_min_amount = o._row_reserve_min_amount;
        _slots = o._slots; _valid = o._valid; _free_row_ids = o._free_row_ids; _valid_count = o._valid_count;
        _copy_sp_from(o);
        return *this;
    }

    // Moves swap the raw buffers explicitly (older Eigen versions have no sparse move and would fall back to the broken copy).
    Vec_Eigen_Matrix(Vec_Eigen_Matrix&& o) noexcept
        : _reserve_perz(o._reserve_perz), _row_reserve_perz(o._row_reserve_perz), _row_reserve_min_amount(o._row_reserve_min_amount),
          _sp(), _slots(o._slots), _valid(std::move(o._valid)), _free_row_ids(std::move(o._free_row_ids)), _valid_count(o._valid_count) {
        _sp.swap(o._sp);
        o._slots = 0; o._valid_count = 0; o._valid.clear();
    }

    Vec_Eigen_Matrix& operator=(Vec_Eigen_Matrix&& o) noexcept {
        if (this == &o) return *this;
        _reserve_perz = o._reserve_perz; _row_reserve_perz = o._row_reserve_perz; _row_reserve_min_amount = o._row_reserve_min_amount;
        _sp.swap(o._sp);
        _slots = o._slots; _valid = std::move(o._valid); _free_row_ids = std::move(o._free_row_ids); _valid_count = o._valid_count;
        o._slots = 0; o._valid_count = 0; o._valid.clear();
        return *this;
    }
 
    
    // access
    [[nodiscard]] View       row(Index Idx) noexcept       { return View(_vals() + _outer()[Idx], static_cast<Index>(_nnz()[Idx]), Idx); }
    [[nodiscard]] Const_View row(Index Idx) const noexcept { return Const_View(_vals() + _outer()[Idx], static_cast<Index>(_nnz()[Idx]), Idx); }
 
    [[nodiscard]] bool  is_valid(Index Idx) const { return _valid[Idx]; }
    [[nodiscard]] Index row_size(Index Idx) const { return static_cast<Index>(_nnz()[Idx]); }
    [[nodiscard]] Index row_reserve(Index Idx) const { return static_cast<Index>(_cap(Idx)); }
    [[nodiscard]] Index row_count() const    { return _valid_count; }   // valid rows
    [[nodiscard]] Index row_capacity() const { return _slots; }         // all row slots
 
    // Approximate heap use in bytes (shared array incl. Eigen's unused index array, outer table, sizes, flags)
    [[nodiscard]] std::size_t memory_bytes() const {
        return _sp.data().allocatedSize() * (sizeof(Data) + sizeof(SIdx))
             + static_cast<std::size_t>(_sp.outerSize() + 1) * 2 * sizeof(SIdx) + _valid.size() / 8;
    }
 
    // Data_Size: wanted number of entries in the shared array (with the usual slack)
    void reserve(Index Data_Size) {
        auto& d = _sp.data();
        if (static_cast<Index>(d.allocatedSize()) >= Data_Size) return;
        const Index padded = Data_Size + static_cast<Index>(static_cast<float>(Data_Size) * _reserve_perz);
        d.reserve(static_cast<typename Sp::Index>(padded) - d.size());
    }
 
    // rows
    Index add_row(std::span<const DataT> Row_Data) {
        const Index sz = static_cast<Index>(Row_Data.size());
        const Index id = _alloc_slot(sz);
        std::copy(Row_Data.begin(), Row_Data.end(), _vals() + _outer()[id]);
        _nnz()[id] = static_cast<SIdx>(sz);
        _valid[id] = true;
        ++_valid_count;
        return id;
    }
 
    Index add_row(Index Size) {
        const Index id = _alloc_slot(Size);
        std::fill_n(_vals() + _outer()[id], Size, Data{});
        _nnz()[id] = static_cast<SIdx>(Size);
        _valid[id] = true;
        ++_valid_count;
        return id;
    }
 
    void reinit_row(Index Idx) {
        if (Idx >= _slots) grow_row_count(Idx + 1);
        const Index res = _initial_row_reserve(0);
        if (_cap(Idx) < static_cast<SIdx>(res)) _make_room(Idx, res - static_cast<Index>(_cap(Idx)));
        const bool was_valid = _valid[Idx];
        _nnz()[Idx] = 0;
        _valid[Idx] = true;
        if (!was_valid) ++_valid_count;
    }
 
    void resize_row(Index Idx, Index Size) {
        const Index cap = static_cast<Index>(_cap(Idx));
        if (Size > cap) _make_room(Idx, _grown_row_reserve(cap, Size) - cap);
        const Index old = static_cast<Index>(_nnz()[Idx]);
        Data* p = _vals() + _outer()[Idx];
        if (Size > old) std::fill(p + old, p + Size, Data{});
        _nnz()[Idx] = static_cast<SIdx>(Size);
    }
 
    // Count = number of row slots that must exist afterwards. New slots are invalid but already hold a reserve.
    void grow_row_count(Index Count) {
        if (Count <= _slots) return;
        const Index res = _initial_row_reserve(0);
        _ensure_outer(Count);
        _grow_pool(static_cast<std::size_t>(Count - _slots) * res);
        SIdx* o = _outer(); SIdx* n = _nnz();
        for (Index id = _slots; id < Count; ++id) {
            o[id + 1] = o[id] + static_cast<SIdx>(res);
            n[id] = 0;
        }
        _valid.resize(Count, false);
        _slots = Count;
    }
 
    void set_valid(Index Idx) {
        if (Idx >= _slots) grow_row_count(Idx + 1);
        if (!_valid[Idx]) ++_valid_count;
        _valid[Idx] = true;
        _nnz()[Idx] = 0;
    }
 
    void set_invalid(Index Idx) {
        if (_valid[Idx]) {
            --_valid_count;
            _free_row_ids.push(Idx);
        }
        _valid[Idx] = false;
        _nnz()[Idx] = 0;
    }
 
    void row_remove_swap(Index Idx, Index Slot_Idx) {
        assert(Slot_Idx < static_cast<Index>(_nnz()[Idx]) && "row_remove_swap: slot out of range");
        Data* p = _vals() + _outer()[Idx];
        const Index last = static_cast<Index>(_nnz()[Idx]) - 1;
        p[Slot_Idx] = p[last];
        _nnz()[Idx] = static_cast<SIdx>(last);
    }
 
    Index row_append(Index Idx, const DataT& Value) {
        const Index slot = static_cast<Index>(_nnz()[Idx]);
        const Index cap  = static_cast<Index>(_cap(Idx));
        if (slot + 1 > cap) _make_room(Idx, _grown_row_reserve(cap, slot + 1) - cap);
        _vals()[_outer()[Idx] + slot] = Value;
        _nnz()[Idx] = static_cast<SIdx>(slot + 1);
        return slot;
    }
 
    // Old_To_New empty : keep all rows and indices, rebuild the array tight (+ reserve if Rebuild_Reserve).
    // otherwise        : Old_To_New[old_id] = new_id (INDEX_INVALID = drop), size == row slots.
    // Invalid rows lose their reserve.
    void compact(std::span<const Index> Old_To_New = {}, bool Rebuild_Reserve = true) {
        _free_row_ids = {};
 
        auto cap_for = [&](Index size) { return Rebuild_Reserve ? _initial_row_reserve(size) : size; };
 
        std::vector<Index> new_to_old;      // new slot -> old slot (INDEX_INVALID = empty slot)
        if (Old_To_New.empty()) {
            new_to_old.resize(_slots);
            for (Index i = 0; i < _slots; ++i) new_to_old[i] = i;
        } else {
            assert(Old_To_New.size() == static_cast<std::size_t>(_slots) && "permutation size must match current row count");
            Index new_row_count = 0;
            for (Index new_id : Old_To_New)
                if (new_id != INDEX_INVALID) new_row_count = std::max<Index>(new_row_count, new_id + 1);
            new_to_old.assign(new_row_count, INDEX_INVALID);
            for (Index old_id = 0; old_id < static_cast<Index>(Old_To_New.size()); ++old_id)
                if (Old_To_New[old_id] != INDEX_INVALID) new_to_old[Old_To_New[old_id]] = old_id;
        }
 
        const Index new_slots = static_cast<Index>(new_to_old.size());
        std::size_t total = 0;
        for (Index id = 0; id < new_slots; ++id) {
            const Index old = new_to_old[id];
            if (old != INDEX_INVALID && _valid[old]) total += cap_for(static_cast<Index>(_nnz()[old]));
        }
 
        Sp n;
        std::vector<bool> new_valid(new_slots, false);
        Index valid = 0;
        if (new_slots > 0) {
            n.resize(new_slots, 1);
            n.uncompress();
            n.data().resize(static_cast<typename Sp::Index>(total), 0.0);
            SIdx* o = n.outerIndexPtr(); SIdx* nz = n.innerNonZeroPtr(); Data* v = n.valuePtr();
            SIdx pos = 0;
            for (Index id = 0; id < new_slots; ++id) {
                o[id] = pos;
                nz[id] = 0;
                const Index old = new_to_old[id];
                if (old == INDEX_INVALID || !_valid[old]) continue;
                const Index sz = static_cast<Index>(_nnz()[old]);
                std::copy_n(_vals() + _outer()[old], sz, v + pos);
                nz[id] = static_cast<SIdx>(sz);
                pos += static_cast<SIdx>(cap_for(sz));
                new_valid[id] = true;
                ++valid;
            }
            o[new_slots] = pos;
        }
        _sp.swap(n);
        _valid = std::move(new_valid);
        _slots = new_slots;
        _valid_count = valid;
    }
 
    // iteration (valid rows)
    template<bool IsConstT>
    struct RowIter {
        using MatrixPtr = std::conditional_t<IsConstT, const Vec_Eigen_Matrix*, Vec_Eigen_Matrix*>;
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
        RowIter(MatrixPtr M, Index Idx) noexcept : _matrix(M), _idx(Idx) { _skip_invalid(); }
        [[nodiscard]] reference operator*() const noexcept { return _matrix->row(_idx); }
        RowIter& operator++() noexcept { ++_idx; _skip_invalid(); return *this; }
        RowIter operator++(int) noexcept { auto t = *this; ++(*this); return t; }
        [[nodiscard]] bool operator==(const RowIter& o) const noexcept { return _idx == o._idx; }
        [[nodiscard]] Index index() const noexcept { return _idx; }
    };
    using Iter       = RowIter<false>;
    using Const_Iter = RowIter<true>;
 
    [[nodiscard]] Iter       begin() noexcept       { return Iter(this, 0); }
    [[nodiscard]] Iter       end() noexcept         { return Iter(this, row_capacity()); }
    [[nodiscard]] Const_Iter begin() const noexcept { return Const_Iter(this, 0); }
    [[nodiscard]] Const_Iter end() const noexcept   { return Const_Iter(this, row_capacity()); }
 
private:
    using SIdx = std::make_signed_t<IndexT>;                                    // Eigen needs a signed storage index
    using Sp   = Eigen::SparseMatrix<Data, Eigen::RowMajor, SIdx>;
 
    SIdx*       _outer()       noexcept { return _sp.outerIndexPtr(); }
    const SIdx* _outer() const noexcept { return _sp.outerIndexPtr(); }
    SIdx*       _nnz()         noexcept { return _sp.innerNonZeroPtr(); }
    const SIdx* _nnz()   const noexcept { return _sp.innerNonZeroPtr(); }
    Data*       _vals()        noexcept { return _sp.valuePtr(); }
    const Data* _vals()  const noexcept { return _sp.valuePtr(); }
    SIdx        _cap(Index i) const noexcept { return _outer()[i + 1] - _outer()[i]; }
 
    // same formulas as Append_Ragged_Matrix
    [[nodiscard]] Index _initial_row_reserve(Index Requested_Size) const {
        return static_cast<Index>(std::ceil(std::max({ static_cast<float>(Requested_Size) * (1.0f + _row_reserve_perz),
                                                       static_cast<float>(_row_reserve_min_amount) })));
    }
    [[nodiscard]] Index _grown_row_reserve(Index Old_Reserve, Index Needed_Size) const {
        return static_cast<Index>(std::ceil(std::max({ static_cast<float>(Needed_Size),
                                                       static_cast<float>(Old_Reserve) * (1.0f + _row_reserve_perz),
                                                       static_cast<float>(_row_reserve_min_amount) })));
    }
 
    // row table: geometric growth (outer + innerNonZeros are realloc'ed, the data array is not touched)
    void _ensure_outer(Index Count) {
        if (static_cast<typename Sp::Index>(Count) <= _sp.outerSize()) return;
        const typename Sp::Index cur = _sp.outerSize();
        const typename Sp::Index grown = static_cast<typename Sp::Index>(std::ceil(static_cast<double>(cur) * (1.0 + _reserve_perz)));
        const typename Sp::Index n = std::max<typename Sp::Index>(Count, grown);
        if (cur == 0) { _sp.resize(n, 1); _sp.uncompress(); }
        else          { _sp.conservativeResize(n, 1); }
    }
 
    // shared array: grows by Delta at the back, with spare capacity (Eigen's CompressedStorage factor)
    void _grow_pool(std::size_t Delta) {
        auto& d = _sp.data();
        d.resize(d.size() + static_cast<typename Sp::Index>(Delta), static_cast<double>(_reserve_perz));
    }
 
    // Give row i Delta more reserve.
    void _make_room(Index i, Index Delta) {
        if (Delta == 0) return;
        const SIdx delta = static_cast<SIdx>(Delta);
 
        const SIdx end = _outer()[_slots];
        _grow_pool(static_cast<std::size_t>(Delta));      // may reallocate the data array
        SIdx* o = _outer(); Data* v = _vals();
        std::move_backward(v + o[i + 1], v + end, v + end + delta);
        for (Index k = i + 1; k <= _slots; ++k) o[k] += delta;
    }
 
    
    Index _alloc_slot(Index Size) {
        if (!_free_row_ids.empty()) {
            const Index id = _free_row_ids.front();
            _free_row_ids.pop();
            const Index cap = static_cast<Index>(_cap(id));
            if (cap < Size) _make_room(id, _initial_row_reserve(Size) - cap);
            return id;
        }
        const Index id  = _slots;
        const Index res = _initial_row_reserve(Size);
        _ensure_outer(id + 1);
        _grow_pool(res);
        SIdx* o = _outer();
        o[id + 1] = o[id] + static_cast<SIdx>(res);
        _nnz()[id] = 0;
        _valid.push_back(false);
        ++_slots;
        return id;
    }
 
    
    void _copy_sp_from(const Vec_Eigen_Matrix& o) {
        Sp n;
        if (o._slots > 0) {
            n.resize(o._slots, 1);
            n.uncompress();
            const SIdx end = o._outer()[o._slots];
            n.data().resize(end, 0.0);
            std::copy_n(o._outer(), static_cast<std::size_t>(o._slots) + 1, n.outerIndexPtr());
            std::copy_n(o._nnz(),   static_cast<std::size_t>(o._slots),     n.innerNonZeroPtr());
            std::copy_n(o._vals(),  static_cast<std::size_t>(end),          n.valuePtr());
        }
        _sp.swap(n);
    }
 
    float _reserve_perz;
    float _row_reserve_perz;
    Index _row_reserve_min_amount;
 
    Sp                _sp;
    Index             _slots = 0;
    std::vector<bool> _valid;
    [[no_unique_address]] Enablable_Queue<ReuseIdsT, IndexT> _free_row_ids;
    Index _valid_count = 0;
};

}
