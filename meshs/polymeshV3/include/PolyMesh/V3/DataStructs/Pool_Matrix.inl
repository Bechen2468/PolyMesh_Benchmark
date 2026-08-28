// ----------------------------------------------------------------------------
#ifndef POLYMESH_V3_POOL_MATRIX_H
#include "PolyMesh/V3/DataStructs/Pool_Matrix.h"
#endif
// ----------------------------------------------------------------------------
#define PoolM_T template<typename DataT, size_t RowSize, typename IndexT>
#define PoolM Pool_Matrix<DataT, RowSize, IndexT>
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {

PoolM_T
PoolM::Pool_Matrix(IndexT Size, float Reserve_Perz):
_reserve_Perz(Reserve_Perz),
_valid_count(0)
{
    reserve(std::max<IndexT>(10, static_cast<IndexT>(Size * (1.0 + _reserve_Perz))));
}


PoolM_T
void PoolM::reserve(IndexT TotalSize) {
    _data.reserve(TotalSize * RowSize);
    _valid.reserve(TotalSize);
}

PoolM_T
[[nodiscard]] IndexT PoolM::size() const {
    return _data.size() / RowSize;
}

PoolM_T
[[nodiscard]] IndexT PoolM::valid_amount() const {
    return _valid_count;
}


PoolM_T
[[nodiscard]] bool PoolM::row_is_valid(IndexT Idx) const {
    return _valid[Idx] != 0;
}


PoolM_T
[[nodiscard]] PoolM::RowView PoolM::row(IndexT Idx) {
    return RowView(_data.data() + Idx * RowSize, Idx);
}


PoolM_T
[[nodiscard]] PoolM::Const_RowView PoolM::row(IndexT Idx) const {
    return Const_RowView(_data.data() + Idx * RowSize, Idx);
}



PoolM_T
[[nodiscard]] PoolM::EigenRowMap PoolM::eigen_row(IndexT idx) noexcept {
    return EigenRowMap(_data.data() + idx * RowSize);
}

PoolM_T
[[nodiscard]] PoolM::EigenConstRowMap PoolM::eigen_row(IndexT idx) const noexcept {
    return EigenConstRowMap(_data.data() + idx * RowSize);
}



PoolM_T
IndexT PoolM::insert(std::span<const DataT, RowSize> Data) {   // DEVNOTE(performance): in vec override: std::copy > insert; end append: insert > std:copy
    _valid_count++;
    if (_free.size()) {
        IndexT idx = _free.front();
        _free.pop();
        std::copy(Data.begin(), Data.end(), _data.begin() + idx * RowSize);
        _valid[idx] = 1;
        return idx;
    }

    if (_data.capacity() == _data.size()) {
        reserve(std::max<IndexT>(10, static_cast<IndexT>(size() * (1 + _reserve_Perz))));
    }
    IndexT idx = size();
    _data.insert(_data.end(), Data.begin(), Data.end());
    _valid.push_back(1);
    return idx;
}


PoolM_T
IndexT PoolM::insert(RowView Data) {
    return insert(Const_Row_View(Data));
}


PoolM_T
IndexT PoolM::insert(Const_RowView Data) {   // DEVNOTE(performance): in vec override: std::copy > insert; end append: insert > std:copy
    _valid_count++;
    if (_free.size()) {
        IndexT idx = _free.front();
        _free.pop();
        std::copy(Data.begin(), Data.end(), _data.begin() + idx * RowSize);
        _valid[idx] = 1;
        return idx;
    }

    if (_data.capacity() == _data.size()) {
        reserve(std::max<IndexT>(10, static_cast<IndexT>(size() * (1 + _reserve_Perz))));
    }
    IndexT idx = size();
    _data.insert(_data.end(), Data.begin(), Data.end());
    _valid.push_back(1);
    return idx;
}


PoolM_T
bool PoolM::delete_row(IndexT Idx) {
    if(_valid[Idx] == 0) return false;
    _valid_count--;
    _valid[Idx] = 0;
    _free.push(Idx);
    return true;
}


PoolM_T
[[nodiscard]] PoolM::Vec<DataT>& PoolM::data() {
    return _data;
}


PoolM_T
[[nodiscard]] PoolM::Vec<uint8_t>& PoolM::valid_mask() {
    return _valid;
}


PoolM_T
[[nodiscard]] std::queue<IndexT>& PoolM::free_queue() {
    return _free;
}



PoolM_T
[[nodiscard]] PoolM::EigenMatrixMap PoolM::eigen_matrix() noexcept {
    return EigenMatrixMap(_data.data(), _data.size() / RowSize, RowSize);
}

PoolM_T
[[nodiscard]] PoolM::EigenConstMatrixMap PoolM::eigen_matrix() const noexcept {
    return EigenConstMatrixMap(_data.data(), _data.size() / RowSize, RowSize);
}
}

