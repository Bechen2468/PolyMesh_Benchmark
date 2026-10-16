



namespace PolyMesh::V3 {

ARM_T
ARM::Append_Ragged_Matrix(float Reserve_Perz, float Row_Reserve_Perz, Index Row_Reserve_Min_Amount):
_reserve_Perz(Reserve_Perz),
_row_reserve_perz(Row_Reserve_Perz),
_row_reserve_min_amount(Row_Reserve_Min_Amount)
{}


ARM_T
ARM::Append_Ragged_Matrix(const std::vector<Index>& Row_Sizes, float Reserve_Perz, float Row_Reserve_Perz, Index Row_Reserve_Min_Amount):
_reserve_Perz(Reserve_Perz),
_row_reserve_perz(Row_Reserve_Perz),
_row_reserve_min_amount(Row_Reserve_Min_Amount)
{
    _rows.resize(Row_Sizes.size());

    Index data_size = 0;
    for(Index i = 0; i < Row_Sizes.size(); ++i) {
        // set row dimensions
        const Index res = _initial_row_reserve(Row_Sizes[i]);
        _rows[i] = Row{data_size, 0, res};
        
        // increase data size
        data_size += res;
    }
    _data.reserve(data_size + static_cast<Index>(std::ceil(data_size * _reserve_Perz)));
    _data.resize(data_size);
    _valid_count = static_cast<Index>(Row_Sizes.size());
}



ARM_T
void ARM::_ensure_data_capacity(Index Needed_Size) {
    if(Needed_Size > static_cast<Index>(_data.capacity())) {
        Index padded = Needed_Size + static_cast<Index>(Needed_Size * _reserve_Perz);
        _data.reserve(padded);
    }
}


ARM_T
typename ARM::Index ARM::_initial_row_reserve(Index Requested_Size) const {
    return static_cast<Index>(std::ceil(
        std::max({
            static_cast<float>(Requested_Size) * (1.0f + _row_reserve_perz),
            static_cast<float>(_row_reserve_min_amount)
        })
    ));
}


ARM_T
typename ARM::Index ARM::_grown_row_reserve(Index Old_Reserve, Index Needed_Size) const {
    return static_cast<Index>(std::ceil(
        std::max({
            static_cast<float>(Needed_Size),
            static_cast<float>(Old_Reserve) * (1.0f + _row_reserve_perz),
            static_cast<float>(_row_reserve_min_amount)
        })
    ));
}


ARM_T
void ARM::reinit_row(Index Idx) {
    if(Idx >= static_cast<Index>(_rows.size())) {
        _rows.resize(Idx + 1, Row{INDEX_INVALID, 0, 0});
    }

    bool was_valid = (_rows[Idx].begin != INDEX_INVALID);

    Index res = _initial_row_reserve(0);   // same fresh-row sizing every other insertion path uses

    Index data_begin = static_cast<Index>(_data.size());
    _ensure_data_capacity(data_begin + res);
    _data.resize(data_begin + res);

    _rows[Idx] = Row{data_begin, 0, res};   // old span (if any) is abandoned, reclaimed on next compact()

    if(!was_valid) ++_valid_count;
}


ARM_T
typename ARM::Index ARM::add_row(std::span<const DataT> Row_Data) {
    Index sz  = static_cast<Index>(Row_Data.size());
    Index res = _initial_row_reserve(sz);

    Index data_begin = static_cast<Index>(_data.size());
    _ensure_data_capacity(data_begin + res);
    _data.resize(data_begin + res);
    std::copy(Row_Data.begin(), Row_Data.end(), _data.begin() + data_begin);

    Index id;
    if(!_free_row_ids.empty()) {
        id = _free_row_ids.front();
        _free_row_ids.pop();
        _rows[id] = Row{data_begin, sz, res};
    } else {
        id = static_cast<Index>(_rows.size());
        _rows.push_back(Row{data_begin, sz, res});
    }
    ++_valid_count;
    return id;
}


ARM_T
typename ARM::Index ARM::add_row(Index Size) {
    Index res = _initial_row_reserve(Size);

    Index data_begin = static_cast<Index>(_data.size());
    _ensure_data_capacity(data_begin + res);
    _data.resize(data_begin + res);

    Index id;
    if(!_free_row_ids.empty()) {
        id = _free_row_ids.front();
        _free_row_ids.pop();
        _rows[id] = Row{data_begin, Size, res};
    } else {
        id = static_cast<Index>(_rows.size());
        _rows.push_back(Row{data_begin, Size, res});
    }
    ++_valid_count;
    return id;
}


ARM_T
void ARM::resize_row(Index Idx, Index Size) {
    Row& r = _rows[Idx];

    if(Size <= r.reserve) {
        r.size = Size;
        return;
    }

    
    Index old_begin = r.begin;
    Index old_size  = r.size;
    Index new_reserve = _grown_row_reserve(r.reserve, Size);

    Index new_begin = static_cast<Index>(_data.size());
    _ensure_data_capacity(new_begin + new_reserve);
    _data.resize(new_begin + new_reserve);

    std::copy(_data.begin() + old_begin, _data.begin() + old_begin + old_size, _data.begin() + new_begin);

    r.begin   = new_begin;
    r.size    = Size;
    r.reserve = new_reserve;
}


ARM_T
void ARM::grow_row_count(Index Count) {
    if(Count <= static_cast<Index>(_rows.size())) return;
    _rows.resize(Count, Row{INDEX_INVALID, 0, 0});
}


ARM_T
void ARM::set_valid(Index Idx) {
    if(Idx >= static_cast<Index>(_rows.size())) {
        _rows.resize(Idx + 1, Row{INDEX_INVALID, 0, 0});
    }
    if(_rows[Idx].begin == INDEX_INVALID) ++_valid_count;
    _rows[Idx] = Row{0, 0, 0};
}


ARM_T
void ARM::set_invalid(Index Idx) {
    if(_rows[Idx].begin != INDEX_INVALID) {
        --_valid_count;
        _free_row_ids.push(Idx);
    }
    _rows[Idx] = Row{INDEX_INVALID, 0, 0};
}


ARM_T
void ARM::row_remove_swap(Index Idx, Index Slot_Idx) {
    Row& r = _rows[Idx];
    assert(Slot_Idx < r.size && "row_remove_swap: slot out of range");
    Index last = r.size - 1;
    if(Slot_Idx != last) {
        std::swap(_data[r.begin + Slot_Idx], _data[r.begin + last]);
    }
    r.size = last;
}


ARM_T
ARM::Index ARM::row_append(Index Idx, const DataT& Value) {
    Index slot = _rows[Idx].size;
    resize_row(Idx, slot + 1);
    _data[_rows[Idx].begin + slot] = Value;
    return slot;
}


ARM_T
void ARM::compact(std::span<const Index> Old_To_New, bool Rebuild_Reserve) {
    _free_row_ids = {};

    Vec<Data> new_data;
    new_data.reserve(_data.size());

    auto copy_row = [&](Row& dst_row, const Row& src_row) {
        Index new_begin = static_cast<Index>(new_data.size());
        new_data.insert(new_data.end(),
                         _data.begin() + src_row.begin,
                         _data.begin() + src_row.begin + src_row.size);

        Index new_reserve = src_row.size;
        if(Rebuild_Reserve) {
            new_reserve = _initial_row_reserve(src_row.size);
            new_data.resize(new_data.size() + (new_reserve - src_row.size));
        }

        dst_row = Row{new_begin, src_row.size, new_reserve};
    };

    if(Old_To_New.empty()) {
        Index valid = 0;
        for(auto& r : _rows) {
            if(r.begin == INDEX_INVALID) continue;
            copy_row(r, r);
            ++valid;
        }
        _valid_count = valid;
        _data = std::move(new_data);
        _data.shrink_to_fit();
        return;
    }

    assert(Old_To_New.size() == _rows.size() && "permutation size must match current row count");

    Index new_row_count = 0;
    for(Index new_id : Old_To_New)
        if(new_id != INDEX_INVALID) new_row_count = std::max<Index>(new_row_count, new_id + 1);

    Vec<Index> new_to_old(new_row_count, INDEX_INVALID);
    for(Index old_id = 0; old_id < static_cast<Index>(Old_To_New.size()); ++old_id) {
        Index new_id = Old_To_New[old_id];
        if(new_id != INDEX_INVALID) new_to_old[new_id] = old_id;
    }

    Vec<Row> new_rows(new_row_count, Row{INDEX_INVALID, 0, 0});

    Index valid = 0;
    for(Index new_id = 0; new_id < new_row_count; ++new_id) {
        Index old_id = new_to_old[new_id];
        if(old_id == INDEX_INVALID) continue;

        const Row& r = _rows[old_id];
        if(r.begin == INDEX_INVALID) continue;

        copy_row(new_rows[new_id], r);
        ++valid;
    }

    _rows = std::move(new_rows);
    _valid_count = valid;
    _data = std::move(new_data);
    _data.shrink_to_fit();
}


}