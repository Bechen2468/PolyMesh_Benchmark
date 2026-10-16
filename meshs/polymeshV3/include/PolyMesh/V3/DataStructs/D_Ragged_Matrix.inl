// ----------------------------------------------------------------------------
#ifndef POLYMESH_V3_D_RAGGED_MATRIX_H
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
#endif
// ----------------------------------------------------------------------------
#define DCSR_T template<typename DataT, typename IndexT, bool ReuseIdsT, typename FastBlockSizeT>
#define DCSR D_Ragged_Matrix<DataT, IndexT, ReuseIdsT, FastBlockSizeT>
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {
// ----------------------------------------------------------------------------
// Construction
// ----------------------------------------------------------------------------
#pragma region Construction

DCSR_T
DCSR::D_Ragged_Matrix(float Reserve_Perz, float Row_Reserve_Perz, Index Row_Reserve_Min_Amount):
_reserve_perz(Reserve_Perz),
_row_reserve_perz(Row_Reserve_Perz),
_row_reserve_min_amount(Row_Reserve_Min_Amount)
{
    _small_head.fill(INDEX_INVALID);
}


DCSR_T
DCSR::D_Ragged_Matrix(const std::vector<Index>& Row_Sizes, float Reserve_Perz, float Row_Reserve_Perz, Index Row_Reserve_Min_Amount):
D_Ragged_Matrix(Reserve_Perz, Row_Reserve_Perz, Row_Reserve_Min_Amount)
{
    _rows.resize(Row_Sizes.size());

    Index total = 0;
    for(std::size_t i = 0; i < Row_Sizes.size(); ++i) {
        const Index res = _initial_row_reserve(Row_Sizes[i]);
        _rows[i] = Row{total, 0, res};
        total += res;
    }
    _valid_count = static_cast<Index>(Row_Sizes.size());

    const Index spare = static_cast<Index>(std::ceil(static_cast<float>(total) * _reserve_perz));
    _resize_pool(total + spare);
    _free_span(total, spare);
}

#pragma endregion Construction


// ----------------------------------------------------------------------------
// Rows
// ----------------------------------------------------------------------------
#pragma region Rows

DCSR_T
void DCSR::reserve(Index Data_Size) {
    const Index n = _pool_size();
    if(Data_Size <= n) return;
    _resize_pool(Data_Size);
    _free_span(n, Data_Size - n);
}


DCSR_T
typename DCSR::Index DCSR::add_row(std::span<const DataT> Row_Data) {
    const Index sz  = static_cast<Index>(Row_Data.size());
    const Index res = _initial_row_reserve(sz);
    const Index b   = _take(res);
    std::copy(Row_Data.begin(), Row_Data.end(), _data.begin() + b);
    return _place_row(Row{b, sz, res});
}


DCSR_T
typename DCSR::Index DCSR::add_row(Index Size) {
    const Index res = _initial_row_reserve(Size);
    const Index b   = _take(res);
    std::fill_n(_data.begin() + b, Size, Data{});
    return _place_row(Row{b, Size, res});
}


DCSR_T
void DCSR::reinit_row(Index Idx) {
    if(Idx >= row_capacity()) _rows.resize(Idx + 1, Row{INDEX_INVALID, 0, 0});

    const bool was_valid = is_valid(Idx);
    if(was_valid) _free_span(_rows[Idx].begin, _rows[Idx].reserve);   // not like Append: the old span is reused

    const Index res = _initial_row_reserve(0);
    _rows[Idx] = Row{_take(res), 0, res};

    if(!was_valid) ++_valid_count;
}


DCSR_T
void DCSR::resize_row(Index Idx, Index Size) {
    Row& r = _rows[Idx];
    if(Size <= r.reserve) {
        if(Size > r.size) std::fill(_data.begin() + r.begin + r.size, _data.begin() + r.begin + Size, Data{});
        r.size = Size;
        return;
    }

    const Index old_size = r.size;
    _grow_row(Idx, Size);
    std::fill(_data.begin() + r.begin + old_size, _data.begin() + r.begin + Size, Data{});
    r.size = Size;
}


DCSR_T
void DCSR::grow_row_count(Index Count) {
    if(Count <= row_capacity()) return;
    _rows.resize(Count, Row{INDEX_INVALID, 0, 0});
}


DCSR_T
void DCSR::set_valid(Index Idx) {
    if(Idx >= row_capacity()) _rows.resize(Idx + 1, Row{INDEX_INVALID, 0, 0});

    if(is_valid(Idx)) _free_span(_rows[Idx].begin, _rows[Idx].reserve);
    else              ++_valid_count;

    _rows[Idx] = Row{0, 0, 0};
}


DCSR_T
void DCSR::set_invalid(Index Idx) {
    if(is_valid(Idx)) {
        _free_span(_rows[Idx].begin, _rows[Idx].reserve);
        --_valid_count;
        _free_row_ids.push(Idx);
    }
    _rows[Idx] = Row{INDEX_INVALID, 0, 0};
}


DCSR_T
void DCSR::row_remove_swap(Index Idx, Index Slot_Idx) {
    Row& r = _rows[Idx];
    assert(Slot_Idx < r.size && "row_remove_swap: slot out of range");

    const Index last = r.size - 1;
    if(Slot_Idx != last) std::swap(_data[r.begin + Slot_Idx], _data[r.begin + last]);
    r.size = last;
}


DCSR_T
typename DCSR::Index DCSR::row_append(Index Idx, const DataT& Value) {
    const Index slot = _rows[Idx].size;
    if(slot + 1 > _rows[Idx].reserve) _grow_row(Idx, slot + 1);

    Row& r = _rows[Idx];
    _data[r.begin + slot] = Value;
    r.size = slot + 1;
    return slot;
}


DCSR_T
void DCSR::compact(std::span<const Index> Old_To_New, bool Rebuild_Reserve) {
    _free_row_ids = {};

    // new slot -> old slot (INDEX_INVALID = empty slot)
    Vec<Index> new_to_old;
    if(Old_To_New.empty()) {
        new_to_old.resize(_rows.size());
        for(Index i = 0; i < row_capacity(); ++i) new_to_old[i] = i;
    } else {
        assert(Old_To_New.size() == _rows.size() && "permutation size must match current row count");
        Index new_row_count = 0;
        for(Index new_id : Old_To_New)
            if(new_id != INDEX_INVALID) new_row_count = std::max<Index>(new_row_count, new_id + 1);
        new_to_old.assign(new_row_count, INDEX_INVALID);
        for(Index old_id = 0; old_id < static_cast<Index>(Old_To_New.size()); ++old_id)
            if(Old_To_New[old_id] != INDEX_INVALID) new_to_old[Old_To_New[old_id]] = old_id;
    }

    Index total = 0;
    for(Index old : new_to_old)
        if(old != INDEX_INVALID && is_valid(old))
            total += Rebuild_Reserve ? _initial_row_reserve(_rows[old].size) : _rows[old].size;

    Vec<Data> new_data(total);
    Vec<Row>  new_rows(new_to_old.size(), Row{INDEX_INVALID, 0, 0});
    Index pos = 0, valid = 0;
    for(Index id = 0; id < static_cast<Index>(new_to_old.size()); ++id) {
        const Index old = new_to_old[id];
        if(old == INDEX_INVALID || !is_valid(old)) continue;

        const Row& r = _rows[old];
        std::copy_n(_data.begin() + r.begin, r.size, new_data.begin() + pos);

        const Index res = Rebuild_Reserve ? _initial_row_reserve(r.size) : r.size;
        new_rows[id] = Row{pos, r.size, res};
        pos += res;
        ++valid;
    }

    _data = std::move(new_data);
    _rows = std::move(new_rows);
    _valid_count = valid;
    _reset_free_space();
    _edge_bits.assign((static_cast<std::size_t>(_pool_size()) + 63) / 64, 0);
}

#pragma endregion Rows


// ----------------------------------------------------------------------------
// Statistics / Debug
// ----------------------------------------------------------------------------
#pragma region Statistics

DCSR_T
typename DCSR::Index DCSR::free_slot_count() const {
    Index s = 0;
    for(const FreeBlock& b : _fb) s += b.size;   // unused records have size 0
    return s;
}


DCSR_T
std::size_t DCSR::memory_bytes() const {
    return _data.capacity()      * sizeof(Data)
         + _rows.capacity()      * sizeof(Row)
         + _fb.capacity()        * sizeof(FreeBlock)
         + _fb_unused.capacity() * sizeof(Index)
         + _edge_bits.capacity() * sizeof(std::uint64_t)
         + _large_head.size()    * (sizeof(Index) * 2 + 32);   // rough node size of std::map
}


DCSR_T
int DCSR::debug_validate() const {
    const Index n = _pool_size();
    std::vector<std::uint8_t> owner(n, 0);           // 0 = nothing, 1 = row, 2 = free block

    // rows: inside the pool, no overlaps
    Index valid = 0;
    for(Index i = 0; i < row_capacity(); ++i) {
        const Row& r = _rows[i];
        if(r.begin == INDEX_INVALID) continue;
        ++valid;
        if(r.size > r.reserve) return 1;
        if(r.reserve == 0) continue;
        if(r.begin + r.reserve > n) return 2;
        for(Index k = r.begin; k < r.begin + r.reserve; ++k) { if(owner[k]) return 3; owner[k] = 1; }
    }
    if(valid != _valid_count) return 4;

    // free blocks: inside the pool, no overlaps, edge bits + tags, fully coalesced
    std::vector<bool> live(_fb.size(), true);
    for(Index u : _fb_unused) live[u] = false;
    for(Index id = 0; id < static_cast<Index>(_fb.size()); ++id) {
        if(!live[id]) continue;
        const FreeBlock& b = _fb[id];
        if(b.size == 0 || b.begin + b.size > n) return 5;
        for(Index k = b.begin; k < b.begin + b.size; ++k) { if(owner[k]) return 6; owner[k] = 2; }
        if(!_bit(b.begin) || !_bit(b.begin + b.size - 1)) return 7;
        if(_tag(b.begin) != id || _tag(b.begin + b.size - 1) != id) return 8;
        if(b.begin + b.size < n && _is_free_start(b.begin + b.size)) return 9;
    }

    // every slot owned, no stale edge bit on a used slot
    for(Index k = 0; k < n; ++k) {
        if(owner[k] == 0) return 10;
        if(owner[k] == 1 && _bit(k)) return 11;
    }

    // size bins hold exactly the live blocks
    Index binned = 0;
    for(Index s = 1; s <= SMALL_BINS; ++s) {
        const bool has = (_small_mask >> (s - 1)) & 1u;
        if(has != (_small_head[s - 1] != INDEX_INVALID)) return 12;
        for(Index id = _small_head[s - 1]; id != INDEX_INVALID; id = _fb[id].next) {
            if(_fb[id].size != s || !live[id]) return 13;
            ++binned;
        }
    }
    for(const auto& [s, head] : _large_head)
        for(Index id = head; id != INDEX_INVALID; id = _fb[id].next) {
            if(_fb[id].size != s || !live[id]) return 14;
            ++binned;
        }
    if(binned != free_block_count()) return 15;
    return 0;
}

#pragma endregion Statistics


// ----------------------------------------------------------------------------
// Internal: rows
// ----------------------------------------------------------------------------
#pragma region Internal_Rows

DCSR_T
typename DCSR::Index DCSR::_initial_row_reserve(Index Requested_Size) const {
    return static_cast<Index>(std::ceil(
        std::max({
            static_cast<float>(Requested_Size) * (1.0f + _row_reserve_perz),
            static_cast<float>(_row_reserve_min_amount)
        })
    ));
}


DCSR_T
typename DCSR::Index DCSR::_grown_row_reserve(Index Old_Reserve, Index Needed_Size) const {
    return static_cast<Index>(std::ceil(
        std::max({
            static_cast<float>(Needed_Size),
            static_cast<float>(Old_Reserve) * (1.0f + _row_reserve_perz),
            static_cast<float>(_row_reserve_min_amount)
        })
    ));
}


DCSR_T
typename DCSR::Index DCSR::_place_row(const Row& R) {
    // a freed id can be revived by reinit_row/set_valid while still queued: skip those, never overwrite a live row
    while(!_free_row_ids.empty() && is_valid(_free_row_ids.front())) _free_row_ids.pop();

    Index id;
    if(!_free_row_ids.empty()) {
        id = _free_row_ids.front();
        _free_row_ids.pop();
        _rows[id] = R;
    } else {
        id = row_capacity();
        _rows.push_back(R);
    }
    ++_valid_count;
    return id;
}


// give row Idx room for at least Needed entries: first try the free block right behind it, else move the row
DCSR_T
void DCSR::_grow_row(Index Idx, Index Needed) {
    Row& r = _rows[Idx];
    const Index new_res = _grown_row_reserve(r.reserve, Needed);

    if(_try_extend(r.begin + r.reserve, new_res - r.reserve)) {
        r.reserve = new_res;
        return;
    }

    const Index nb = _take(new_res);                 // may resize the pool; r stays valid (_rows is untouched)
    std::copy_n(_data.begin() + r.begin, r.size, _data.begin() + nb);
    _free_span(r.begin, r.reserve);
    r.begin   = nb;
    r.reserve = new_res;
}


DCSR_T
void DCSR::_resize_pool(Index N) {
    if(N > _data.capacity()) _data.reserve(N);
    _data.resize(N);
    _edge_bits.resize((static_cast<std::size_t>(N) + 63) / 64, 0);
}

#pragma endregion Internal_Rows


// ----------------------------------------------------------------------------
// Internal: free space
// ----------------------------------------------------------------------------
#pragma region Internal_FreeSpace

DCSR_T
typename DCSR::Index DCSR::_tag(Index x) const {
    Index id;
    std::memcpy(&id, &_data[x], sizeof(Index));
    return id;
}


DCSR_T
void DCSR::_write_tag(Index x, Index Id) {
    std::memcpy(&_data[x], &Id, sizeof(Index));
}


// write edge bits + tags of a free block
DCSR_T
void DCSR::_mark(Index Id) {
    const Index b = _fb[Id].begin;
    const Index e = b + _fb[Id].size - 1;
    _set_bit(b);
    _set_bit(e);
    _write_tag(b, Id);
    _write_tag(e, Id);
}


DCSR_T
void DCSR::_unmark(Index Id) {
    const Index b = _fb[Id].begin;
    const Index e = b + _fb[Id].size - 1;
    _clr_bit(b);
    _clr_bit(e);
}


DCSR_T
typename DCSR::Index DCSR::_new_record(Index Begin, Index Size) {
    Index id;
    if(!_fb_unused.empty()) {
        id = _fb_unused.back();
        _fb_unused.pop_back();
    } else {
        id = static_cast<Index>(_fb.size());
        _fb.push_back({});
    }
    _fb[id] = FreeBlock{Begin, Size, INDEX_INVALID, INDEX_INVALID};
    return id;
}


DCSR_T
void DCSR::_drop_record(Index Id) {
    _fb[Id].size = 0;
    _fb_unused.push_back(Id);
}


// size bins: doubl linked lists, new blocks at the head
DCSR_T
void DCSR::_bin_insert(Index Id) {
    FreeBlock& b = _fb[Id];
    b.prev = INDEX_INVALID;

    if(b.size <= SMALL_BINS) {
        Index& head = _small_head[b.size - 1];
        b.next = head;
        if(head != INDEX_INVALID) _fb[head].prev = Id;
        head = Id;
        _small_mask |= FastBlockSizeT(1) << (b.size - 1);
    } else {
        auto [it, inserted] = _large_head.try_emplace(b.size, Id);
        b.next = inserted ? INDEX_INVALID : it->second;
        if(!inserted) {
            _fb[it->second].prev = Id;
            it->second = Id;
        }
    }
}


DCSR_T
void DCSR::_bin_remove(Index Id) {
    FreeBlock& b = _fb[Id];
    if(b.prev != INDEX_INVALID) _fb[b.prev].next = b.next;
    if(b.next != INDEX_INVALID) _fb[b.next].prev = b.prev;

    if(b.prev == INDEX_INVALID) {                    // was the head
        if(b.size <= SMALL_BINS) {
            _small_head[b.size - 1] = b.next;
            if(b.next == INDEX_INVALID) _small_mask &= ~(FastBlockSizeT(1) << (b.size - 1));
        } else {
            if(b.next == INDEX_INVALID) _large_head.erase(b.size);
            else                        _large_head[b.size] = b.next;
        }
    }
    b.next = INDEX_INVALID;
    b.prev = INDEX_INVALID;
}


// smallest free block with at least N slots, or INDEX_INVALID
DCSR_T
typename DCSR::Index DCSR::_find_fit(Index N) const {
    if(N <= SMALL_BINS) {
        const FastBlockSizeT m = static_cast<FastBlockSizeT>(_small_mask >> (N - 1));
        if(m) return _small_head[N - 1 + static_cast<Index>(std::countr_zero(m))];
    }
    auto it = _large_head.lower_bound(std::max<Index>(N, SMALL_BINS + 1));
    return it == _large_head.end() ? INDEX_INVALID : it->second;
}


// take N slots from the free space (grows the pool if nothing fits), returns the begin
DCSR_T
typename DCSR::Index DCSR::_take(Index N) {
    if(N == 0) return 0;

    Index id = _find_fit(N);
    if(id == INDEX_INVALID) {
        const Index n      = _pool_size();
        const Index target = n + N + static_cast<Index>(std::ceil(static_cast<float>(n + N) * _reserve_perz));
        _resize_pool(target);
        _free_span(n, target - n);                   // coalesces with a free block at the old end
        id = _find_fit(N);
        assert(id != INDEX_INVALID);
    }

    _bin_remove(id);
    _unmark(id);
    const Index b = _fb[id].begin;
    if(_fb[id].size == N) {
        _drop_record(id);
    } else {
        _fb[id].begin += N;
        _fb[id].size  -= N;
        _mark(id);
        _bin_insert(id);
    }
    return b;
}


// take extra slots directly behind position end, if a large enough free block starts there
DCSR_T
bool DCSR::_try_extend(Index End, Index Extra) {
    if(Extra == 0) return true;
    if(End >= _pool_size() || !_bit(End)) return false;

    const Index id = _tag(End);
    if(_fb[id].begin != End || _fb[id].size < Extra) return false;

    _bin_remove(id);
    _unmark(id);
    if(_fb[id].size == Extra) {
        _drop_record(id);
    } else {
        _fb[id].begin += Extra;
        _fb[id].size  -= Extra;
        _mark(id);
        _bin_insert(id);
    }
    return true;
}


// give [Begin, Begin + Size) back, merging with free neighbours on both sides
DCSR_T
void DCSR::_free_span(Index Begin, Index Size) {
    if(Size == 0) return;
    Index b = Begin;
    Index s = Size;

    if(b > 0 && _bit(b - 1)) {
        const Index L = _tag(b - 1);
        if(_fb[L].begin + _fb[L].size == b) {
            _bin_remove(L);
            _unmark(L);
            b  = _fb[L].begin;
            s += _fb[L].size;
            _drop_record(L);
        }
    }

    const Index e = Begin + Size;
    if(e < _pool_size() && _bit(e)) {
        const Index R = _tag(e);
        if(_fb[R].begin == e) {
            _bin_remove(R);
            _unmark(R);
            s += _fb[R].size;
            _drop_record(R);
        }
    }

    const Index id = _new_record(b, s);
    _mark(id);
    _bin_insert(id);
}


DCSR_T
void DCSR::_reset_free_space() {
    _fb.clear();
    _fb_unused.clear();
    _small_head.fill(INDEX_INVALID);
    _small_mask = 0;
    _large_head.clear();
}

#pragma endregion Internal_FreeSpace



};

#undef DCSR
#undef DCSR_T
