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
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {


template<typename DataT, typename IndexT = std::uint32_t, typename FastBlockSizeT = uint8_t>
struct D_Ragged_Matrix {
// ----------------------------------------------------------------------------
// Core Structs
// ----------------------------------------------------------------------------
    using Index = IndexT;

    template<typename T>
    //using Vec = std::vector<T>;
    using Vec = std::vector<T, Aligned_Allocator<T, 64>>;

    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();
    static constexpr uint8_t FAST_BLOCK_SIZE = sizeof(FastBlockSizeT) * 8;

    
    struct Row {
        Index begin;
        Index size;
        Index reserve = 0;
    };

    struct FreeBlocks {
    private:
        struct FreeBlock {
            Index begin;
            Index size;
            
            // linked-list handles for the Size Bins
            Index bin_next = INDEX_INVALID;
            Index bin_prev = INDEX_INVALID;
            bool  in_bin   = false;
        };

        struct SizeBins {
            // Direct access bins for exact sizes (e.g., Bin 0 = size 1, Bin 1 = size 2, etc.)
            // Storing just the index of the first FreeBlock in _free_blocks
            std::array<Index, FAST_BLOCK_SIZE> small_bins; 

            // fallback map for large polygons (size > FastBlockSizeT)
            // Key: block size, Value: index of the first FreeBlock in _free_blocks
            std::map<Index, Index> large_bins;

            // bitmask tracking which small bins are populated (1 = has free blocks)
            FastBlockSizeT small_bins_bitmask = 0; 

            SizeBins() {
                small_bins.fill(INDEX_INVALID);
            }
        };

    private:
        // All blocks, ordered sequentially by their physical location in mem
        Vec<FreeBlock> _free_blocks;
        SizeBins _bins = SizeBins();
        Vec<Index> _dataIdx_to_free_block_idx;  // Holds a id to its free block in _freeblocks or INVALID for each "_blocks"

    public:
        // to resize parallel to the matrix blocks itself
        void resize(Index Size);

        Row try_get_row(Index Size);
        bool try_extend(Index Data_End, Index Extra);
        void free_row(const Row& row);

        void remove_from_size_bin(Index F_Block_Idx);
        void insert_into_size_bin(Index F_Block_Idx);
        void delete_free_block_element(Index F_Block_Idx);

        int debug_validate() const;
    };



// ----------------------------------------------------------------------------
// Iterators: RowView
// ----------------------------------------------------------------------------
    template<bool IsConst>
    struct RowViewImpl {
        using ElementType = std::conditional_t<IsConst, const DataT, DataT>;

        // cpp20 algo defines
        using pointer     = ElementType*;
        using reference   = ElementType&;
        // Element Iterator inside this row
        using iterator       = pointer; 
        using const_iterator = const DataT*;

    private:
        pointer _data = nullptr;
        Index _size = 0;
        Index _row_Idx = 0;

    public:
        RowViewImpl() = default;
        RowViewImpl(pointer Data, Index Size, Index Row_Idx) noexcept:
            _data(Data),
            _size(Size),
            _row_Idx(Row_Idx)
        {}

        [[nodiscard]] inline reference operator[](Index Idx) const noexcept { return _data[Idx]; }

        [[nodiscard]] inline Index  size() const noexcept { return _size; }
        [[nodiscard]] inline pointer data() const noexcept { return _data; }
        [[nodiscard]] inline Index  idx()  const noexcept { return _row_Idx; }


        [[nodiscard]] inline pointer begin() const noexcept { return _data; }
        [[nodiscard]] inline pointer end()   const noexcept { return _data + _size; }

        [[nodiscard]] inline std::span<ElementType> to_span() const noexcept { return {_data, static_cast<size_t>(_size)}; }


        // Zero copy Eigen vector mapping for faster math
        using EigenMap = Eigen::Map<
            std::conditional_t<IsConst, 
            const Eigen::Matrix<DataT, Eigen::Dynamic, 1>, 
            Eigen::Matrix<DataT, Eigen::Dynamic, 1>>
        >;

        [[nodiscard]] EigenMap to_eigen() const noexcept {
            return EigenMap(_data, static_cast<Eigen::Index>(_size));
        }
    };

    using RowView = RowViewImpl<false>;
    using Const_RowView = RowViewImpl<true>;
    


// ----------------------------------------------------------------------------
// Iterators: RowIterator
// ----------------------------------------------------------------------------
    template<bool IsConst>
    struct RowIteratorImpl {
    public:
        using MatrixPtr         = std::conditional_t<IsConst, const D_Ragged_Matrix*, D_Ragged_Matrix*>;

        // cpp20 algo defines
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept  = std::forward_iterator_tag;
        using value_type        = RowViewImpl<IsConst>;
        using reference         = RowViewImpl<IsConst>;

    private:
        MatrixPtr _matrix = nullptr;
        Index _row_idx = 0;

    public:
        RowIteratorImpl() = default;
        RowIteratorImpl(MatrixPtr matrix, Index row_idx) noexcept: 
            _matrix(matrix), 
            _row_idx(row_idx) 
        {}

        // Returns RowView directly (Zero copy)
        [[nodiscard]] reference operator*() const noexcept { return _matrix->row(_row_idx); }

        RowIteratorImpl& operator++() noexcept { 
            ++_row_idx;
            while(_row_idx < _matrix->row_count() && !_matrix->is_valid(_row_idx)) ++_row_idx; 
            return *this; 
        }
        RowIteratorImpl operator++(int) noexcept { 
            auto tmp = *this; 
            ++_row_idx;
            while(_row_idx < _matrix->row_count() && !_matrix->is_valid(_row_idx)) ++_row_idx; 
            return tmp; 
        }

        [[nodiscard]] bool operator==(const RowIteratorImpl& other) const noexcept { return _row_idx == other._row_idx; }

        [[nodiscard]] Index index() const noexcept { return _row_idx; }
    };

    using RowIter = RowIteratorImpl<false>;
    using Const_RowIter = RowIteratorImpl<true>;


// ----------------------------------------------------------------------------
// Variables
// ----------------------------------------------------------------------------
private:
    // Settings
    float _reserve_perz;
    float _row_reserve_perz = 0.34f;
    Index _row_reserve_min_amount = 1;

    // Data
    Vec<DataT> _data;
    Vec<Row> _rows;
    Vec<Index> _id_to_rawId;
    Vec<Index> _rawId_to_id;

    FreeBlocks _free_blocks_manager;

    
// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------
public:
    // Reserve_Size     = Amount of dataT to reserve
    // Reserve_Perz     = Perzentage to extra reserve when running out of space
    D_Ragged_Matrix(const Index Reserve_Size = 0, float Reserve_Perz = 0.2);
    D_Ragged_Matrix(std::vector<std::vector<DataT>> Data, float Reserve_Perz = 0.2);
    D_Ragged_Matrix(Vec<DataT>&& Data, Vec<Row>&& Rows, Vec<Index>&& Id_to_rawId, Vec<Index>&& RawId_to_id, float Reserve_Perz = 0.2);
    D_Ragged_Matrix(const D_Ragged_Matrix<DataT, Index, FastBlockSizeT>& other);
    D_Ragged_Matrix(D_Ragged_Matrix<DataT, Index, FastBlockSizeT>&& other) noexcept;
    
    void reserve(const Index Data_Size);
    void reserve_row(const Index row_reserve_size);
    inline Index row_reserve_size(const Index Row_Idx) const;
    void grow_row_count(const Index Row_Amount);

    
    inline void delete_row(const Index Row_Idx);           // shifts _id_to_rawId
    inline void reinit_row(const Index Row_Idx);
    inline Index append_row(std::span<const DataT> Data); 

    inline Index row_append(const Index Row_Idx, const DataT& Value);
    inline void row_remove_swap(const Index Row_Idx, const Index Slot_Idx);

    inline void row_resize(const Index Row_Idx, const Index Size);

    inline Index row_count() const;
    inline Index row_capacity() const { return _id_to_rawId.size(); };
    inline Index row_size(const Index Row_Idx) const;

    inline bool is_valid(const Index Row_Idx) const;
    
// ----------------------------------------------------------------------------
// Getter
// ----------------------------------------------------------------------------
    [[nodiscard]] DataT& get(const Index Row_RawIdx, const Index Data_Idx);
    [[nodiscard]] Const_RowView get_row(Index Idx) const;
    [[nodiscard]] Vec<DataT>& get_data() { return _data; };
    [[nodiscard]] Vec<Row>& get_rows() { return _rows; };
    [[nodiscard]] Vec<Index>& get_id_to_rawId() { return _id_to_rawId; };
    [[nodiscard]] Vec<Index>& get_rawId_to_id() { return _rawId_to_id; };


// ----------------------------------------------------------------------------
// Operator
// ----------------------------------------------------------------------------
    D_Ragged_Matrix& operator=(const D_Ragged_Matrix<DataT, Index, FastBlockSizeT>& other);
    D_Ragged_Matrix& operator=(D_Ragged_Matrix<DataT, Index, FastBlockSizeT>&& other) noexcept;



    
    [[nodiscard]] RowView row(Index Idx) noexcept { 
        Row b = _rows[_id_to_rawId[Idx]];
        return RowView(_data.data() + b.begin, b.size, Idx); 
    }

    [[nodiscard]] Const_RowView row(Index Idx) const noexcept { 
        Row b = _rows[_id_to_rawId[Idx]];
        return Const_RowView(_data.data() + b.begin, b.size, Idx); 
    }


    [[nodiscard]] RowIter begin() noexcept { 
        Index first_valid = 0;
        while (first_valid < row_count() && !is_valid(first_valid)) {
            ++first_valid;
        }
        return RowIter(this, first_valid);
    }

    [[nodiscard]] RowIter end() noexcept { return RowIter(this, row_count()); }


    [[nodiscard]] Const_RowIter begin() const noexcept { 
        Index first_valid = 0;
        while (first_valid < row_count() && !is_valid(first_valid)) {
            ++first_valid;
        }
        return Const_RowIter(this, first_valid);
    }

    [[nodiscard]] Const_RowIter end() const noexcept { return Const_RowIter(this, row_count()); }
};


};
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.inl"
#endif

