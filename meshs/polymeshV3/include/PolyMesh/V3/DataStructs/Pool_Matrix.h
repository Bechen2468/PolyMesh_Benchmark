#ifndef POLYMESH_V3_POOL_MATRIX_H
#define POLYMESH_V3_POOL_MATRIX_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include <vector>
#include <queue>
#include <span>
#include <Eigen/Dense>
#include "PolyMesh/V3/DataStructs/Aligned_Allocator.h"
// ----------------------------------------------------------------------------




namespace PolyMesh::V3 {


template<typename DataT, size_t RowSize, typename IndexT = std::uint32_t>
struct Pool_Matrix {
// ----------------------------------------------------------------------------
// Type defines
// ----------------------------------------------------------------------------
    template<typename T>
    //using Vec = std::vector<T>;
    using Vec = std::vector<T, Aligned_Allocator<T, 64>>;

    using EigenRowVector      = Eigen::Matrix<DataT, 1, RowSize, Eigen::RowMajor>;
    using EigenConstRowVector = const Eigen::Matrix<DataT, 1, RowSize, Eigen::RowMajor>;
    
    using EigenRowMap         = Eigen::Map<EigenRowVector>;
    using EigenConstRowMap    = Eigen::Map<EigenConstRowVector>;

    using EigenMatrixMap      = Eigen::Map<Eigen::Matrix<DataT, Eigen::Dynamic, RowSize, Eigen::RowMajor>>;
    using EigenConstMatrixMap = Eigen::Map<const Eigen::Matrix<DataT, Eigen::Dynamic, RowSize, Eigen::RowMajor>>;


// ----------------------------------------------------------------------------
// RowView
// ----------------------------------------------------------------------------
    struct RowView : public std::span<DataT, RowSize> {
        using Base = std::span<DataT, RowSize>;
    private:
        IndexT _idx = 0;

    public:
        RowView() noexcept = default;
        RowView(DataT* Data, IndexT Idx) noexcept : Base(Data, RowSize), _idx(Idx) {}

        [[nodiscard]] IndexT idx() const noexcept { return _idx; }

        // Zero copy wrapper to Eigen RowVector Map
        [[nodiscard]] EigenRowMap eigen() const noexcept {
            return EigenRowMap(this->data());
        }

        // Implicit wrapper to Eigen Map when passed to Eigen functions
        operator EigenRowMap() const noexcept {
            return EigenRowMap(this->data());
        }
    };

    struct Const_RowView : public std::span<const DataT, RowSize> {
        using Base = std::span<const DataT, RowSize>;
    private:
        IndexT _idx = 0;

    public:
        Const_RowView() noexcept = default;
        Const_RowView(DataT* Data, IndexT Idx) noexcept : Base(Data, RowSize), _idx(Idx) {}
        
        [[nodiscard]] IndexT idx() const noexcept { return _idx; }

        [[nodiscard]] EigenConstRowMap eigen() const noexcept {
            return EigenConstRowMap(this->data());
        }

        operator EigenConstRowMap() const noexcept {
            return EigenConstRowMap(this->data());
        }
    };



// ----------------------------------------------------------------------------
// Variables
// ----------------------------------------------------------------------------
private:
    float _reserve_Perz;

    Vec<DataT> _data;
    Vec<uint8_t> _valid;
    IndexT _valid_count;
    std::queue<IndexT> _free;


// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------
public:
    Pool_Matrix(IndexT Size = 0, float Reserve_Perz = 0.2);

    void reserve(IndexT TotalSize);


    [[nodiscard]] IndexT size() const;
    
    [[nodiscard]] IndexT valid_amount() const;

    [[nodiscard]] bool row_is_valid(IndexT Idx) const;

    [[nodiscard]] RowView row(IndexT Idx);
    [[nodiscard]] Const_RowView row(IndexT Idx) const;

    [[nodiscard]] EigenRowMap eigen_row(IndexT idx) noexcept;
    [[nodiscard]] EigenConstRowMap eigen_row(IndexT idx) const noexcept;

    IndexT insert(std::span<const DataT, RowSize> Data);
    IndexT insert(RowView Data);
    IndexT insert(Const_RowView Data);

    bool delete_row(IndexT Idx);


    [[nodiscard]] Vec<DataT>& data();
    [[nodiscard]] Vec<uint8_t>& valid_mask();
    [[nodiscard]] std::queue<IndexT>& free_queue();

    
    [[nodiscard]] EigenMatrixMap eigen_matrix() noexcept;
    [[nodiscard]] EigenConstMatrixMap eigen_matrix() const noexcept;


    

// ----------------------------------------------------------------------------
// Iterator
// ----------------------------------------------------------------------------   
    template<bool IsConst>
    struct RowIteratorImpl {
    public:
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept  = std::forward_iterator_tag;
        using value_type        = std::conditional_t<IsConst, Const_RowView, RowView>;
        using reference         = std::conditional_t<IsConst, Const_RowView, RowView>;
        using difference_type   = std::ptrdiff_t;

        using PointerT = std::conditional_t<IsConst, const DataT*, DataT*>;
        using MaskPtrT = const uint8_t*;

    private:
        PointerT _data{nullptr};
        PointerT _data_end{nullptr};
        MaskPtrT _valid{nullptr};
        bool     _all_valid{true};
        IndexT   _idx{0};

        void skip_invalid() noexcept {
            if (_all_valid || !_valid) return;
            while (_data < _data_end && !_valid[0]) {
                _data += RowSize;
                ++_valid;
                ++_idx;
            }
        }

    public:
        RowIteratorImpl() = default;

        RowIteratorImpl(PointerT data, PointerT data_end, MaskPtrT valid, bool all_valid, IndexT start_idx = 0) noexcept: 
            _data(data), _data_end(data_end), _valid(valid), _all_valid(all_valid), _idx(start_idx) {
            skip_invalid();
        }

        [[nodiscard]] reference operator*() const noexcept {
            return reference(_data, _idx);
        }

        RowIteratorImpl& operator++() noexcept {
            _data += RowSize;
            if (_valid) ++_valid;
            ++_idx;
            skip_invalid();
            return *this;
        }

        RowIteratorImpl operator++(int) noexcept {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        [[nodiscard]] bool operator==(const RowIteratorImpl& other) const noexcept { return _data == other._data; }

        [[nodiscard]] PointerT data() const noexcept { return _data; }
        [[nodiscard]] PointerT data_end() const noexcept { return _data_end; }
        [[nodiscard]] MaskPtrT valid_ptr() const noexcept { return _valid; }
        [[nodiscard]] bool all_valid() const noexcept { return _all_valid; }
    };

    using Iter = RowIteratorImpl<false>;
    using Const_Iter = RowIteratorImpl<true>;


// ----------------------------------------------------------------------------
// Iteration Interface
// ----------------------------------------------------------------------------
    [[nodiscard]] Iter begin() noexcept { 
        return Iter(_data.data(), _data.data() + _data.size(), _valid.data(), _free.empty(), 0); 
    }
    [[nodiscard]] Iter end() noexcept { 
        DataT* end_ptr = _data.data() + _data.size();
        IndexT end_idx = static_cast<IndexT>(_data.size() / RowSize);
        return Iter(end_ptr, end_ptr, _valid.data() + _valid.size(), _free.empty(), end_idx); 
    }


    [[nodiscard]] Const_Iter begin() const noexcept { 
        return Const_Iter(_data.data(), _data.data() + _data.size(), _valid.data(), _free.empty(), 0); 
    }
    [[nodiscard]] Const_Iter end() const noexcept { 
        const DataT* end_ptr = _data.data() + _data.size();
        IndexT end_idx = static_cast<IndexT>(_data.size() / RowSize);
        return Const_Iter(end_ptr, end_ptr, _valid.data() + _valid.size(), _free.empty(), end_idx); 
    }

    
    [[nodiscard]] Const_Iter cbegin() const noexcept { return begin(); }
    [[nodiscard]] Const_Iter cend() const noexcept { return end(); }


};
}

#include "PolyMesh/V3/DataStructs/Pool_Matrix.inl"
#endif

