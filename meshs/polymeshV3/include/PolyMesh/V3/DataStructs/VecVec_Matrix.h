#pragma once
// ----------------------------------------------------------------------------
#include <vector>
#include <span>
#include <cassert>
#include <algorithm>
#include <limits>
#include <utility>
#include "PolyMesh/V3/DataStructs/Enablable_Queue.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {




template<typename T, typename IndexT, bool ReuseIdsT = true>
class Vec_Vec_Matrix {
public:
    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();
 
// ----------------------------------------------------------------------------
    template<typename ViewT, typename ViewIndexT>
    class Row_View {
    public:
        using Index = IndexT;
 
        Row_View() noexcept = default;
        Row_View(std::span<ViewT> data, Index idx) noexcept : _data(data), _idx(idx) {}
 
        [[nodiscard]] Index idx() const noexcept { return _idx; }
 
        [[nodiscard]] auto begin() const noexcept { return _data.begin(); }
        [[nodiscard]] auto end()   const noexcept { return _data.end(); }
        [[nodiscard]] auto size()  const noexcept { return _data.size(); }
        [[nodiscard]] bool empty() const noexcept { return _data.empty(); }
        [[nodiscard]] ViewT& operator[](std::size_t i) const noexcept { return _data[i]; }
 
        operator std::span<ViewT>() const noexcept { return _data; }
 
    private:
        std::span<ViewT> _data;
        Index        _idx{};
    };

    using View       = Row_View<T, IndexT>;
    using Const_View = Row_View<const T, IndexT>;
// ----------------------------------------------------------------------------
 
    Vec_Vec_Matrix(float Row_Reserve_Perz = 0.34f, Index Row_Reserve_Min_Amount = 1):
        _row_reserve_perz(Row_Reserve_Perz), _row_reserve_min_amount(Row_Reserve_Min_Amount) {}
 
    Vec_Vec_Matrix(const std::vector<Index>& Row_Sizes, float Row_Reserve_Perz = 0.34f, Index Row_Reserve_Min_Amount = 1):
        _row_reserve_perz(Row_Reserve_Perz), _row_reserve_min_amount(Row_Reserve_Min_Amount) 
    {
        const std::size_t n = Row_Sizes.size();
        _rows.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            _rows[i].reserve(_initial_row_reserve(Row_Sizes[i]));
        }

        _valid.assign(n, true);
        _valid_count = static_cast<Index>(n);
    }
 
    // -------- row access --------
    [[nodiscard]] View row(Index idx) noexcept {
        return View(std::span<T>(_rows[idx]), idx);
    }
    [[nodiscard]] Const_View row(Index idx) const noexcept {
        return Const_View(std::span<const T>(_rows[idx]), idx);
    }
 
    [[nodiscard]] Index row_size(Index idx) const noexcept { return static_cast<Index>(_rows[idx].size()); }
    [[nodiscard]] Index row_reserve(Index idx) const noexcept { return static_cast<Index>(_rows[idx].capacity()); }
    [[nodiscard]] bool  is_valid(Index idx) const noexcept { return _valid[idx]; }
 
    [[nodiscard]] Index row_count()    const noexcept { return _valid_count; }                       // valid rows
    [[nodiscard]] Index row_capacity() const noexcept { return static_cast<Index>(_rows.size()); }
 
    
    Index add_row(std::span<const T> data) {
        std::vector<T> r;
        r.reserve(_initial_row_reserve(static_cast<Index>(data.size())));
        r.insert(r.end(), data.begin(), data.end());
        return _place_row(std::move(r));
    }
 
    Index add_row(Index size) {
        std::vector<T> r;
        r.reserve(_initial_row_reserve(size));
        r.resize(size);
        return _place_row(std::move(r));
    }
 
    void reserve(Index size) { /* Not required/implimentable as this would require row vecs to be alloced in a way they can share space */ }
    
    void resize_row(Index idx, Index new_size) { _rows[idx].resize(new_size); }
 
    Index row_append(Index idx, const T& value) {
        auto& r = _rows[idx];
        r.push_back(value);
        return static_cast<Index>(r.size() - 1);
    }
 
    void row_remove_swap(Index idx, Index slot) {
        auto& r = _rows[idx];
        assert(slot < r.size() && "row_remove_swap: slot out of range");
        r[slot] = r.back();
        r.pop_back();
    }
 
    // Fresh, empty, valid row at idx. Grows the row table if needed.
    void reinit_row(Index idx) {
        if (idx >= _rows.size()) _grow_to(idx + 1);
        const bool was_valid = _valid[idx];
 
        std::vector<T> fresh;
        fresh.reserve(_initial_row_reserve(0));
        _rows[idx].swap(fresh);
 
        _valid[idx] = true;
        if (!was_valid) ++_valid_count;
    }
 
    void set_valid(Index idx) {
        if (idx >= _rows.size()) _grow_to(idx + 1);
        if (!_valid[idx]) ++_valid_count;
        _valid[idx] = true;
        std::vector<T>().swap(_rows[idx]);
    }
 
    void set_invalid(Index idx) {
        if (_valid[idx]) {
            --_valid_count;
            _free_row_ids.push(idx);
        }
        _valid[idx] = false;
        std::vector<T>().swap(_rows[idx]);
    }
 
 
    void grow_row_count(Index Count) {
        if (Count <= _rows.size()) return;
        _grow_to(Count);
    }
 
    
    void compact(std::span<const Index> Old_To_New = {}, bool Rebuild_Reserve = true) {
        _free_row_ids = {};
 
        auto rebuild_row = [&](std::vector<T>& r) {
            const Index size = static_cast<Index>(r.size());
            const Index cap  = Rebuild_Reserve ? _initial_row_reserve(size) : size;
            std::vector<T> n;
            n.reserve(cap);
            n.insert(n.end(), r.begin(), r.end());
            r.swap(n);
        };
 
        if (Old_To_New.empty()) {
            Index valid = 0;
            for (std::size_t i = 0; i < _rows.size(); ++i) {
                if (!_valid[i]) continue;
                rebuild_row(_rows[i]);
                ++valid;
            }
            _valid_count = valid;
            return;
        }
 
        assert(Old_To_New.size() == _rows.size() && "permutation size must match current row count");
 
        Index new_row_count = 0;
        for (Index new_id : Old_To_New)
            if (new_id != INDEX_INVALID) new_row_count = std::max<Index>(new_row_count, new_id + 1);
 
        std::vector<Index> new_to_old(new_row_count, INDEX_INVALID);
        for (Index old_id = 0; old_id < static_cast<Index>(Old_To_New.size()); ++old_id) {
            const Index new_id = Old_To_New[old_id];
            if (new_id != INDEX_INVALID) new_to_old[new_id] = old_id;
        }
 
        std::vector<std::vector<T>> new_rows(new_row_count);
        std::vector<bool>           new_valid(new_row_count, false);
 
        Index valid = 0;
        for (Index new_id = 0; new_id < new_row_count; ++new_id) {   // rows get re-allocated in new order
            const Index old_id = new_to_old[new_id];
            if (old_id == INDEX_INVALID || !_valid[old_id]) continue;
            new_rows[new_id] = std::move(_rows[old_id]);
            rebuild_row(new_rows[new_id]);
            new_valid[new_id] = true;
            ++valid;
        }
 
        _rows        = std::move(new_rows);
        _valid       = std::move(new_valid);
        _valid_count = valid;
    }
 
// ----------------------------------------------------------------------------
    class Iter {
        Vec_Vec_Matrix* _m; Index _i;
        void skip() { while (_i < static_cast<Index>(_m->_rows.size()) && !_m->_valid[_i]) ++_i; }
    public:
        Iter(Vec_Vec_Matrix* m, Index i) : _m(m), _i(i) { skip(); }
        View operator*() const { return _m->row(_i); }
        Iter& operator++() { ++_i; skip(); return *this; }
        bool operator!=(const Iter& o) const { return _i != o._i; }
    };
// ----------------------------------------------------------------------------
 
    class Const_Iter {
        const Vec_Vec_Matrix* _m; Index _i;
        void skip() { while (_i < static_cast<Index>(_m->_rows.size()) && !_m->_valid[_i]) ++_i; }
    public:
        Const_Iter(const Vec_Vec_Matrix* m, Index i) : _m(m), _i(i) { skip(); }
        Const_View operator*() const { return _m->row(_i); }
        Const_Iter& operator++() { ++_i; skip(); return *this; }
        bool operator!=(const Const_Iter& o) const { return _i != o._i; }
    };
// ----------------------------------------------------------------------------
 
    [[nodiscard]] Iter       begin()       noexcept { return Iter(this, 0); }
    [[nodiscard]] Iter       end()         noexcept { return Iter(this, static_cast<Index>(_rows.size())); }
    [[nodiscard]] Const_Iter begin() const noexcept { return Const_Iter(this, 0); }
    [[nodiscard]] Const_Iter end()   const noexcept { return Const_Iter(this, static_cast<Index>(_rows.size())); }
// ----------------------------------------------------------------------------
 
private:

    [[nodiscard]] Index _initial_row_reserve(Index Requested_Size) const {
        return static_cast<Index>(std::ceil(
            std::max({ static_cast<float>(Requested_Size) * (1.0f + _row_reserve_perz),
                       static_cast<float>(_row_reserve_min_amount) })));
    }
 
    void _grow_to(Index count) {
        _rows.resize(count);
        _valid.resize(count, false);
    }
 
    Index _place_row(std::vector<T>&& r) {
        Index id;
        if (!_free_row_ids.empty()) {
            id = _free_row_ids.front();
            _free_row_ids.pop();
            _rows[id]  = std::move(r);
            _valid[id] = true;
        } else {
            id = static_cast<Index>(_rows.size());
            _rows.push_back(std::move(r));
            _valid.push_back(true);
        }
        ++_valid_count;
        return id;
    }

// ----------------------------------------------------------------------------
    float _row_reserve_perz;
    Index _row_reserve_min_amount;
 
    std::vector<std::vector<T>> _rows;
    std::vector<bool>           _valid;
    [[no_unique_address]] Enablable_Queue<ReuseIdsT, IndexT> _free_row_ids;
    Index _valid_count = 0;
};



}