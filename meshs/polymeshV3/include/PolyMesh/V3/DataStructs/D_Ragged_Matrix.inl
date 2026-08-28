// ----------------------------------------------------------------------------
#ifndef POLYMESH_V3_D_RAGGED_MATRIX_H
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
#endif
// ----------------------------------------------------------------------------
#define DCSR_T template<typename DataT, typename Index, typename FastBlockSizeT>
#define DCSR D_Ragged_Matrix<DataT, Index, FastBlockSizeT>
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {
// ----------------------------------------------------------------------------
// FreeBlocks
// ----------------------------------------------------------------------------
#pragma region FreeBlocks


DCSR_T
int DCSR::FreeBlocks::debug_validate() const {
    std::vector<bool> seen(_free_blocks.size(), false);
    Index reached = 0;
    int fail_code = 0;

    auto walk_chain = [&](Index head, Index expected_size) -> bool {
        Index cur = head;
        Index prev = INDEX_INVALID;
        while(cur != INDEX_INVALID) {
            if(cur >= _free_blocks.size()) { fail_code = 1; return false; }
            if(seen[cur]) { fail_code = 2; return false; }
            seen[cur] = true;
            ++reached;

            const FreeBlock& b = _free_blocks[cur];
            if(b.size != expected_size) { fail_code = 3; return false; }
            if(b.bin_prev != prev) { fail_code = 4; return false; }

            prev = cur;
            cur = b.bin_next;
        }
        return true;
    };

    for(Index i = 0; i < FAST_BLOCK_SIZE; ++i) {
        if(_bins.small_bins[i] == INDEX_INVALID) continue;
        if(!walk_chain(_bins.small_bins[i], i + 1)) return fail_code;
    }

    for(auto& [size, head] : _bins.large_bins) {
        if(!walk_chain(head, size)) return fail_code;
    }

    if(reached != _free_blocks.size()) return 7;

    for(Index i = 0; i < _free_blocks.size(); ++i) {
        const FreeBlock& b = _free_blocks[i];
        if(b.begin >= _dataIdx_to_free_block_idx.size()) return 8;
        if(_dataIdx_to_free_block_idx[b.begin] != i) return 9;
        Index last = b.begin + b.size - 1;
        if(last >= _dataIdx_to_free_block_idx.size()) return 10;
        if(_dataIdx_to_free_block_idx[last] != i) return 11;
    }

    return 0;
}





DCSR_T
void DCSR::FreeBlocks::resize(Index Size) {
    _dataIdx_to_free_block_idx.resize(Size, INDEX_INVALID);
}



DCSR_T
typename DCSR::Row DCSR::FreeBlocks::try_get_row(Index Size) {
    if (Size == 0) return Row{0, 0, 0};

    Index found_block_idx = INDEX_INVALID;

    if (Size <= FAST_BLOCK_SIZE) {
        // Bitmask witchcraft:
        // Setting all lower Size-1 bits to 0 by using AND on an inverted bitmask of Size-1 bit set minus a real 1 ( sets all lower bits to 1 ) 
        FastBlockSizeT usable = _bins.small_bins_bitmask & ~((FastBlockSizeT(1) << (Size - 1)) - 1);
        // if usage is 0, there are no bits set above equal to Size-1
        if (usable != 0) {
            // counter_zero just gets the index of the lowest 1 very fast
            Index bin_idx = static_cast<Index>(std::countr_zero(usable));
            found_block_idx = _bins.small_bins[bin_idx];
        }
    }

    if(found_block_idx == INDEX_INVALID) {
        auto it = _bins.large_bins.lower_bound(Size);
        if (it != _bins.large_bins.end())
            found_block_idx = it->second;
    }

    if (found_block_idx == INDEX_INVALID)
        return Row{0, 0, 0};

    Row full{_free_blocks[found_block_idx].begin, _free_blocks[found_block_idx].size};

    // unlink from bin, then reclaim the slot (swap-and-pop)
    this->remove_from_size_bin(found_block_idx);
    this->delete_free_block_element(found_block_idx);

    // mark claimed begin and end as occupied
    _dataIdx_to_free_block_idx[full.begin] = INDEX_INVALID;
    _dataIdx_to_free_block_idx[full.begin + Size - 1] = INDEX_INVALID;

    Row result{full.begin, Size, Size};

    // return leftover fragment (if any) to the pool
    if(full.size > Size) {
        Row fragment{full.begin + Size, full.size - Size};
        this->free_row(fragment);
    }

#ifdef D_RAGGED_MATRIX_DEBUG
    {
    int _dv = debug_validate();
    assert(_dv == 0);
    }
#endif

    return result;
}



DCSR_T
bool DCSR::FreeBlocks::try_extend(Index Data_End, Index Extra) {
    if (Extra == 0) return true;
    if (Data_End >= _dataIdx_to_free_block_idx.size()) return false;

    Index fb_idx = _dataIdx_to_free_block_idx[Data_End];
    if (fb_idx == INDEX_INVALID) return false;

    FreeBlock& fb = _free_blocks[fb_idx];
    if (fb.size < Extra) return false;

    Index new_occupied_last = Data_End + Extra - 1;

    this->remove_from_size_bin(fb_idx);

    if (fb.size == Extra) {
        // reclaim the free block slot entirely
        this->delete_free_block_element(fb_idx);
    } else {
        // shrink from the front. new leftmost boundary must be re-marked
        fb.begin += Extra;
        fb.size  -= Extra;
        this->insert_into_size_bin(fb_idx);
        _dataIdx_to_free_block_idx[fb.begin] = fb_idx;
    }

    // mark the newly occupied range boundaries
    _dataIdx_to_free_block_idx[Data_End] = INDEX_INVALID;
    _dataIdx_to_free_block_idx[new_occupied_last] = INDEX_INVALID;


#ifdef D_RAGGED_MATRIX_DEBUG
    {
    int _dv = debug_validate();
    assert(_dv == 0);
    }
#endif

    return true;
}



DCSR_T
void DCSR::FreeBlocks::free_row(const Row& row) {
    if(row.size == 0) return;

    Index left_fb_idx  = (row.begin > 0) ? _dataIdx_to_free_block_idx[row.begin - 1] : INDEX_INVALID;
    Index right_fb_idx = (row.begin + row.size < _dataIdx_to_free_block_idx.size()) ? _dataIdx_to_free_block_idx[row.begin + row.size] : INDEX_INVALID;

    Row merged = row;
    Index target_idx = INDEX_INVALID;

    if (left_fb_idx != INDEX_INVALID) {
        target_idx = left_fb_idx;
        merged.begin = _free_blocks[left_fb_idx].begin;
        merged.size += _free_blocks[left_fb_idx].size;
        this->remove_from_size_bin(left_fb_idx);
    }

    if (right_fb_idx != INDEX_INVALID) {
        merged.size += _free_blocks[right_fb_idx].size;
        this->remove_from_size_bin(right_fb_idx);

        Index last_idx = static_cast<Index>(_free_blocks.size()) - 1;
        this->delete_free_block_element(right_fb_idx); // swap-and-pop; moved block's
                                                         // _dataIdx_to_free_block_idx entries
                                                         // are re-pointed inside this call

        // if target_idx (left neighbor) was the tail element, the swap moved it into
        // right_fb_idx's now-vacated slot
        if (target_idx == last_idx) target_idx = right_fb_idx;
    }

    if (target_idx == INDEX_INVALID) {
        target_idx = static_cast<Index>(_free_blocks.size());
        _free_blocks.push_back(FreeBlock{merged.begin, merged.size});
    } else {
        _free_blocks[target_idx].begin = merged.begin;
        _free_blocks[target_idx].size  = merged.size;
    }




    this->insert_into_size_bin(target_idx);

    _dataIdx_to_free_block_idx[merged.begin] = target_idx;
    _dataIdx_to_free_block_idx[merged.begin + merged.size - 1] = target_idx;
    
#ifdef D_RAGGED_MATRIX_DEBUG
    {
    int _dv = debug_validate();
    assert(_dv == 0);
    }
#endif

}



DCSR_T
void DCSR::FreeBlocks::remove_from_size_bin(Index F_Block_Idx) {
    FreeBlock& block = _free_blocks[F_Block_Idx];
    Index size = block.size;

    if (size <= FAST_BLOCK_SIZE) {
        Index bin_idx = size - 1;
        if (_bins.small_bins[bin_idx] == F_Block_Idx) {
            _bins.small_bins[bin_idx] = block.bin_next;
            if (block.bin_next != INDEX_INVALID)
                _free_blocks[block.bin_next].bin_prev = INDEX_INVALID;
            if (_bins.small_bins[bin_idx] == INDEX_INVALID)
                _bins.small_bins_bitmask &= ~(FastBlockSizeT(1) << bin_idx);
        } else {
            if (block.bin_prev != INDEX_INVALID)
                _free_blocks[block.bin_prev].bin_next = block.bin_next;
            if (block.bin_next != INDEX_INVALID)
                _free_blocks[block.bin_next].bin_prev = block.bin_prev;
        }
    } else {
        auto it = _bins.large_bins.find(size);
        if (it != _bins.large_bins.end()) {
            if (it->second == F_Block_Idx) {
                if (block.bin_next != INDEX_INVALID) {
                    _free_blocks[block.bin_next].bin_prev = INDEX_INVALID;
                    it->second = block.bin_next;
                } else {
                    _bins.large_bins.erase(it);
                }
            } else {
                if (block.bin_prev != INDEX_INVALID)
                    _free_blocks[block.bin_prev].bin_next = block.bin_next;
                if (block.bin_next != INDEX_INVALID)
                    _free_blocks[block.bin_next].bin_prev = block.bin_prev;
            }
        }
    }

    block.bin_next = INDEX_INVALID;
    block.bin_prev = INDEX_INVALID;
    block.in_bin = false;   
    
#ifdef D_RAGGED_MATRIX_DEBUG
    {
    int _dv = debug_validate();
    assert(_dv == 0);
    }
#endif
}



DCSR_T
void DCSR::FreeBlocks::insert_into_size_bin(Index F_Block_Idx) {
    FreeBlock& block = _free_blocks[F_Block_Idx];
    Index size = block.size;



    if (size <= FAST_BLOCK_SIZE) {
        Index bin_idx = size - 1;
        Index old_head = _bins.small_bins[bin_idx];
        block.bin_next = old_head;
        block.bin_prev = INDEX_INVALID;
        block.in_bin = true;
        if (old_head != INDEX_INVALID)
            _free_blocks[old_head].bin_prev = F_Block_Idx;  //ERROR
        _bins.small_bins[bin_idx] = F_Block_Idx;
        _bins.small_bins_bitmask |= (FastBlockSizeT(1) << bin_idx);
    } else {
        auto it = _bins.large_bins.find(size);
        if (it != _bins.large_bins.end()) {
            Index old_head = it->second;
            block.bin_next = old_head;
            block.bin_prev = INDEX_INVALID;
            block.in_bin = true;
            _free_blocks[old_head].bin_prev = F_Block_Idx;
            it->second = F_Block_Idx;
        } else {
            _bins.large_bins[size] = F_Block_Idx;
            block.bin_next = INDEX_INVALID;
            block.bin_prev = INDEX_INVALID;
            block.in_bin = true;
        }
    }
    
    
#ifdef D_RAGGED_MATRIX_DEBUG
    {
    int _dv = debug_validate();
    assert(_dv == 0);
    }
#endif
}


DCSR_T
void DCSR::FreeBlocks::delete_free_block_element(Index F_Block_Idx) {
    Index last_idx = static_cast<Index>(_free_blocks.size()) - 1;

    if(F_Block_Idx != last_idx) {
        _free_blocks[F_Block_Idx] = _free_blocks[last_idx];
        FreeBlock& moved_block = _free_blocks[F_Block_Idx];

        if(moved_block.bin_prev != INDEX_INVALID) {
            _free_blocks[moved_block.bin_prev].bin_next = F_Block_Idx;
        } else if(moved_block.in_bin) {
            // moved block was a bin chain head — fix bin header
            if(moved_block.size <= FAST_BLOCK_SIZE)
                _bins.small_bins[moved_block.size - 1] = F_Block_Idx;
            else
                _bins.large_bins[moved_block.size] = F_Block_Idx;
        }
        if(moved_block.bin_next != INDEX_INVALID)
            _free_blocks[moved_block.bin_next].bin_prev = F_Block_Idx;

        // re-point begin and end address the moved block owns to its new slot index
        _dataIdx_to_free_block_idx[moved_block.begin] = F_Block_Idx;
        _dataIdx_to_free_block_idx[moved_block.begin + moved_block.size - 1] = F_Block_Idx;
    }

    _free_blocks.pop_back();


#ifdef D_RAGGED_MATRIX_DEBUG
    {
    int _dv = debug_validate();
    assert(_dv == 0);
    }
#endif
}



#pragma endregion FreeBlocks

// ----------------------------------------------------------------------------
// D_Ragged_Matrix
// ----------------------------------------------------------------------------
#pragma region Core

DCSR_T
DCSR::D_Ragged_Matrix(const Index Reserve_Size, float Reserve_Perz):
_reserve_perz(Reserve_Perz),
_data(0),
_rows(0),
_id_to_rawId(0)
{
    this->reserve(Reserve_Size);
}



DCSR_T
DCSR::D_Ragged_Matrix(std::vector<std::vector<DataT>> Data, float Reserve_Perz):
_reserve_perz(Reserve_Perz)
{
    Index size = 0;
    for(auto v : Data) size += v.size();

    _data.resize(size);
    _rows.reserve(Data.size());
    _id_to_rawId.reserve(Data.size());
    _rawId_to_id.reserve(Data.size());

    for(Index i = 0, realI = 0; i < Data.size(); ++i) {
        _data.insert(_data.begin() + realI, Data[i].begin(), Data[i].end());
        _rows.push_back({realI, Data[i].size()});
        _id_to_rawId[i] = i;
        _rawId_to_id[i] = i;

        realI += Data[i].size();
    }

    this->reserve(_data.size());
}
    


DCSR_T
DCSR::D_Ragged_Matrix(Vec<DataT>&& Data, Vec<Row>&& Rows, Vec<Index>&& Id_to_rawId, Vec<Index>&& RawId_to_id, float Reserve_Perz): 
_reserve_perz(static_cast<float>(Reserve_Perz)),
_data(std::move(Data)),
_rows(std::move(Rows)),
_id_to_rawId(std::move(Id_to_rawId)),
_rawId_to_id(std::move(RawId_to_id))
{
    _free_blocks_manager.resize(static_cast<Index>(_data.size()));
}



DCSR_T
DCSR::D_Ragged_Matrix(const D_Ragged_Matrix<DataT, Index, FastBlockSizeT>& other):
    _reserve_perz(other._reserve_perz),
    _data(other._data),
    _rows(other._rows),
    _id_to_rawId(other._id_to_rawId),
    _rawId_to_id(other._rawId_to_id),
    _free_blocks_manager(other._free_blocks_manager)
{}



DCSR_T
DCSR::D_Ragged_Matrix(D_Ragged_Matrix<DataT, Index, FastBlockSizeT>&& other) noexcept:
    _reserve_perz(other._reserve_perz),
    _data(std::move(other._data)),
    _rows(std::move(other._rows)),
    _id_to_rawId(std::move(other._id_to_rawId)),
    _rawId_to_id(std::move(other._rawId_to_id)),
    _free_blocks_manager(std::move(other._free_blocks_manager))
{}



DCSR_T
inline void DCSR::reserve(const Index Data_Size) {
    Index padding = static_cast<Index>(Data_Size * _reserve_perz);
    Index target_size = Data_Size + padding;
    Index old_size = static_cast<Index>(_data.size());

    if (target_size > old_size) {
        _data.resize(target_size);
        Index new_segment_size = target_size - old_size;

        Row new_tail_row;
        new_tail_row.begin = old_size;
        new_tail_row.size = new_segment_size;
        new_tail_row.reserve = new_segment_size;

        _free_blocks_manager.resize(target_size);
        _free_blocks_manager.free_row(new_tail_row);
    }
}



DCSR_T
inline Index DCSR::row_reserve_size(const Index Row_Idx) const {
    return _rows[_id_to_rawId[Row_Idx]].reserve;
}


DCSR_T
inline void DCSR::reserve_row(const Index Row_Reserve_Min_Amount) {
    _row_reserve_min_amount = Row_Reserve_Min_Amount;
}



DCSR_T
inline void DCSR::grow_row_count(const Index Row_Amount) {
    Index preSize = static_cast<Index>(_id_to_rawId.size());
    Index nSize = std::max<Index>(preSize, Row_Amount);
    if(nSize <= preSize) return;

    Index nAmount = nSize - preSize;
    Index preRaw = static_cast<Index>(_rows.size());

    _rows.resize(_rows.size() + nAmount, {0, 0, 0});
    _id_to_rawId.resize(_id_to_rawId.size() + nAmount);
    _rawId_to_id.resize(_rawId_to_id.size() + nAmount);
    for(Index i = 0; i < nAmount; ++i) {
        Index raw = preRaw + i;
        _id_to_rawId[preSize + i] = raw;
        _rawId_to_id[raw] = preSize + i;
    }
}



DCSR_T
inline void DCSR::row_resize(const Index Row_Idx, const Index Size) {
#ifdef D_RAGGED_MATRIX_DEBUG
    {
    int _dv = _free_blocks_manager.debug_validate();
    assert(_dv == 0);
    }
#endif

    Index raw_id = _id_to_rawId[Row_Idx];
    assert(raw_id != INDEX_INVALID && "Tried to resize a non existant row");
    Row& target = _rows[raw_id];

    if(Size <= target.reserve) {
        target.size = Size;
        return;
    }

    // need more physical capacity than currently reserved; grow with slack
    Index new_reserve = static_cast<Index>(std::ceil(
        std::max({
            static_cast<float>(Size),
            target.reserve * (1.0f + _row_reserve_perz),
            static_cast<float>(_row_reserve_min_amount)
        })
    ));

    Index extra = new_reserve - target.reserve;

    // fast path: extend into space adjacent to the RESERVED region, not the
    // used region;slack between size and reserve is already mine
    if(_free_blocks_manager.try_extend(target.begin + target.reserve, extra)) {
        target.size = Size;
        target.reserve = new_reserve;
        return;
    }

    // slow path: relocate the whole reserved region (not just the used part)
    Row new_space = _free_blocks_manager.try_get_row(new_reserve);
    if(new_space.size == 0) {
        this->reserve(_data.size() + new_reserve);
        new_space = _free_blocks_manager.try_get_row(new_reserve);
    }


    if(target.size != 0) {
        std::copy(_data.begin() + target.begin,
                  _data.begin() + target.begin + target.size,
                  _data.begin() + new_space.begin);
    }
    if(target.reserve != 0) {
        _free_blocks_manager.free_row({target.begin, target.reserve, target.reserve}); // free the FULL reserved span
    }

    target.begin = new_space.begin;
    target.size = Size;
    target.reserve = new_reserve;
    
#ifdef D_RAGGED_MATRIX_DEBUG
    {
    int _dv = _free_blocks_manager.debug_validate();
    assert(_dv == 0);
    }
#endif
}





DCSR_T
inline void DCSR::delete_row(const Index Row_Idx) {
    Index raw_id = _id_to_rawId[Row_Idx];
    if (raw_id == INDEX_INVALID) return;

    // free the RESERVED extent, not just the used part
    _free_blocks_manager.free_row({_rows[raw_id].begin, _rows[raw_id].reserve});

    Index last_raw = static_cast<Index>(_rows.size()) - 1;
    if (raw_id != last_raw) {
        _rows[raw_id] = _rows[last_raw];
        _rawId_to_id[raw_id] = _rawId_to_id[last_raw];
        _id_to_rawId[_rawId_to_id[raw_id]] = raw_id;
    }

    _rows.pop_back();
    _rawId_to_id.pop_back();
    _id_to_rawId[Row_Idx] = INDEX_INVALID;
}


DCSR_T
inline void DCSR::reinit_row(const Index Row_Idx) {
    if(Row_Idx >= row_capacity()) {
        grow_row_count(Row_Idx + 1);
    }

    Index initial_reserve = _row_reserve_min_amount;
    Row claim{0, 0, 0};
    if(initial_reserve > 0) {
        claim = _free_blocks_manager.try_get_row(initial_reserve);
        if(claim.size == 0) {
            this->reserve(_data.size() + initial_reserve);
            claim = _free_blocks_manager.try_get_row(initial_reserve);
        }
    }

    Index raw_id = _id_to_rawId[Row_Idx];

    if(raw_id != INDEX_INVALID) {
        Row& target = _rows[raw_id];
        if(target.reserve != 0) {
            _free_blocks_manager.free_row({target.begin, target.reserve, target.reserve});
        }
        target.begin = claim.begin;
        target.size = 0;
        target.reserve = initial_reserve;
    } else {
        Index new_raw_id = static_cast<Index>(_rows.size());
        _rows.push_back({claim.begin, 0, initial_reserve});
        _rawId_to_id.push_back(Row_Idx);
        _id_to_rawId[Row_Idx] = new_raw_id;
    }
}



DCSR_T
inline Index DCSR::append_row(std::span<const DataT> Data) {
    Index needed_size = static_cast<Index>(Data.size());

    // give fresh rows slack from the start too; not just exact-fits
    Index initial_reserve = static_cast<Index>(std::ceil(
        std::max(static_cast<float>(needed_size), static_cast<float>(_row_reserve_min_amount))
    ));

    Row claim = _free_blocks_manager.try_get_row(initial_reserve);
    if (claim.size == 0 && initial_reserve != 0) {
        this->reserve(_data.size() + initial_reserve);
        claim = _free_blocks_manager.try_get_row(initial_reserve);
    }

    std::copy(Data.begin(), Data.end(), _data.begin() + claim.begin);

    Index raw_id = static_cast<Index>(_rows.size());
    _rows.push_back({claim.begin, needed_size, initial_reserve});
    _rawId_to_id.push_back(INDEX_INVALID);

    Index logical_id = static_cast<Index>(_id_to_rawId.size());
    _id_to_rawId.push_back(raw_id);
    _rawId_to_id[raw_id] = logical_id;

    return logical_id;
}


DCSR_T
inline Index DCSR::row_append(const Index Row_Idx, const DataT& Value) {
    Index slot = row_size(Row_Idx);
    row_resize(Row_Idx, slot + 1); // O(1) whenever slack remains
    Index raw_id = _id_to_rawId[Row_Idx];
    _data[_rows[raw_id].begin + slot] = Value;
    return slot;
}


DCSR_T
inline void DCSR::row_remove_swap(const Index Row_Idx, const Index Slot_Idx) {
    Index raw_id = _id_to_rawId[Row_Idx];
    Row& r = _rows[raw_id];
    assert(r.size > 0 && "row_remove_swap on empty row");
    Index last = r.size - 1;
    if(Slot_Idx != last) {
        std::swap(_data[r.begin + Slot_Idx], _data[r.begin + last]);
    }
    r.size = last; // O(1) -- reserve untouched, no allocator call at all
}



DCSR_T
inline Index DCSR::row_count() const {
    // returning the total count of active rows currently tracked
    return static_cast<Index>(_rows.size());
}

DCSR_T
inline Index DCSR::row_size(const Index Row_Idx) const {
    // routing the logical user ID through our lookup layer to find the raw row sizeget_id_to_rawIds
    Index raw_id = _id_to_rawId[Row_Idx];
    return _rows[raw_id].size;
}


DCSR_T
inline bool DCSR::is_valid(const Index Row_Idx) const {
    return _id_to_rawId[Row_Idx] != INDEX_INVALID;
}



// ----------------------------------------------------------------------------
// Getter
// ----------------------------------------------------------------------------
DCSR_T
[[nodiscard]] DataT& DCSR::get(const Index Row_RawIdx, const Index Data_Idx) {
    // accessing the raw element directly using the physical row offset
    return _data[_rows[Row_RawIdx].begin + Data_Idx];
}



DCSR_T
DCSR&  DCSR::operator=(const DCSR& other) {
    if(this == &other) return *this;
    _data = other._data;
    _rows = other._rows;
    _id_to_rawId = other._id_to_rawId;
    _rawId_to_id = other._rawId_to_id;
    _reserve_perz = other._reserve_perz;
    _free_blocks_manager = other._free_blocks_manager;
    return *this;
}


DCSR_T
DCSR&  DCSR::operator=(DCSR&& other) noexcept {
    if(this == &other) return *this;
    _data = std::move(other._data);
    _rows = std::move(other._rows);
    _id_to_rawId = std::move(other._id_to_rawId);
    _rawId_to_id = std::move(other._rawId_to_id);
    _reserve_perz = other._reserve_perz;
    _free_blocks_manager = std::move(other._free_blocks_manager);
    return *this;
}


#pragma endregion Core
};

