#ifndef BM_D_CSR_MATRIX_H
#define BM_D_CSR_MATRIX_H

#include <benchmark/benchmark.h>
#include <vector>
#include <span>
#include <iostream>
#include <cassert>
#include <Eigen/Sparse>
#include "benchmarks/structs/ibenchmark_matrix.h"
#include <PolyMesh/V3.h>




class Benchmark_D_CSR_Matrix : public IBenchmark_Matrix {
private:
    PolyMesh::V3::D_CSR_Matrix<DataT, IndexT> _matrix;

    // used to name unused cells which are not value checked ( this includes reserved space in the back )
    const DataT V = std::numeric_limits<DataT>::max();

    // validity base matrix is always build as 
    const std::vector<std::vector<DataT>> _base = {
        {},
        {0},
        {1, 0, 1},
        {2, 0, 1, 2, 3},
        {3, 0, 1, 2, 3, 4, 5},
        {4, 0, 1, 2, 3, 4, 5, 6, 7},
        {5, 0, 1, 2, 3, 4, 5, 6},
        {6, 0, 1, 2, 3, 4},
        {7, 0, 1, 2},
        {8, 0}
    };

    const std::vector<std::vector<DataT>> _instructions = {
        {},
        {0, 1, 9},
        {1, 2},
        {2, 2, 3, 9, 9, 9},
        {1, 3, 2, 2, 2, 9, 9}
    };

    const std::vector<std::vector<DataT>> _result_data = {
        {0, 1, 0, 1, 2, 0, 1, 2, 3, 3, 0, 1, 2, 3, 4, 5, 4, 0, 1, 2, 3, 4, 5, 6, 7, 5, 0, 1, 2, 3, 4, 5, 6, 6, 0, 1, 2, 3, 4, 7, 0, 1, 2, 8, 0, V, V, V, V, V, V, V, V, V},
        {0, 1, 0, 1, 2, 0, 1, 2, 3, 3, 0, 1, 2, 3, 4, 5, 4, 0, 1, 2, 3, 4, 5, 6, 7, 5, 0, 1, 2, 3, 4, 5, 6, 6, 0, 1, 2, 3, 4, 7, 0, 1, 2, 8, 0, 9, V, V, V, V, V, V, V, V},
        {0, V, V, V, 2, 0, 1, 2, 3, 3, 0, 1, 2, 3, 4, 5, 4, 0, 1, 2, 3, 4, 5, 6, 7, 5, 0, 1, 2, 3, 4, 5, 6, 6, 0, 1, 2, 3, 4, 7, 0, 1, 2, 8, 0, V, V, V, V, V, V, V, V, V},
        {0, V, V, V, 2, 0, 1, 2, 3, 3, 0, 1, 2, 3, 4, 5, 4, 0, 1, 2, 3, 4, 5, 6, 7, 5, 0, 1, 2, 3, 4, 5, 6, 6, 0, 1, 2, 3, 4, 7, 0, 1, 2, 8, 0, 1, 0, 1, 9, 9, 9, V, V, V},
        {0,1,0,1,9,9,V,V,V,3,0,1,2,3,4,5,4,0,1,2,3,4,5,6,7,5,0,1,2,3,4,5,6,6,0,1,2,3,4,7,0,1,2,8,0}
    
    };
    const std::vector<std::vector<PolyMesh::V3::D_CSR_Matrix<DataT, IndexT>::Block>> _result_blocks = {
        {{0,0}, {0,1}, {1,3}, {4,5}, {9,7}, {16,9}, {25,8}, {33,6}, {39,4}, {43,2}},
        {{0,0}, {0,1}, {1,3}, {4,5}, {9,7}, {16,9}, {25,8}, {33,6}, {39,4}, {43,2}, {45, 1}},
        {{0,0}, {0,1}, {43,2}, {4,5}, {9,7}, {16,9}, {25,8}, {33,6}, {39,4}},
        {{0,0}, {0,1}, {45, 6}, {4,5}, {9,7}, {16,9}, {25,8}, {33,6}, {39,4}, {43,2}},
        {{0,0}, {0,1}, {1,5}, {43,2}, {9,7}, {16,9}, {25,8}, {33,6}, {39,4}}
    };
    const std::vector<std::vector<IndexT>> _result_id_to_rawId = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
        {0, 1, V, 3, 4, 5, 6, 7, 8, 2},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        {0,1,2,V,4,5,6,7,8,3}
    };



public:
    void run_verification(benchmark::State& state) {
        for (auto _ : state) {
            _run_verify_test(state.range(0));
            benchmark::DoNotOptimize(_benchmark_sink);
        }
    }


private:
    void _build_var_struct(size_t Size) override {
        _matrix = PolyMesh::V3::D_CSR_Matrix<DataT, IndexT>(Size);
        assert(_matrix.block_count() == 0);

        DataT len = 0;
        for(IndexT i = 0; i < Size; ++i) {
            assert(_matrix.block_count() == i);
            
            std::vector<DataT> data;
            for(DataT j = 0; j < len; ++j) {
                data.push_back(j);
            }

            assert(_matrix.append_block(std::span<const DataT>{data}) == i);
            assert(_matrix.block_count() == i + 1);
            assert(_matrix.block_size(i) == len);

            len = (len + 1) % 12;
        }
    }


    void _run_verify_test(size_t TestId) {
        _build_base_struct();
        _exec_instructions(_instructions[TestId]);
        _verify(TestId);
    }

    void _build_base_struct() {
        _matrix = PolyMesh::V3::D_CSR_Matrix<DataT, IndexT>(45, 0.2);

        for(size_t i = 0; i < _base.size(); ++i) {
            _matrix.append_block(std::span<const DataT> {_base[i]});
        }
        
    }


    void _exec_instructions(const std::vector<DataT>& inst) override {
        std::span<const DataT> insts(inst);
        for (size_t i = 0; i < insts.size(); ) {
            DataT id = 0, len = 0;
            switch (insts[i] % 3) {
                case InstructionType::add_block:
                    len = insts[i + 1] % 10;
                    _matrix.append_block(insts.subspan(i + 2, len));
                    i += len + 2;
                    break;
                case InstructionType::delete_block:
                    _matrix.delete_block(insts[i + 1]);
                    i += 2;
                    break;
                case InstructionType::add_data:
                    id = insts[i + 1];
                    len = insts[i + 2] % 10;
                    _matrix.block_add_data(id, insts.subspan(i + 3, len));
                    i += len + 3;
                    break;
            }
        }
    }

    
    void _verify(size_t ResultIdx) {
        // 1. DATA
        const auto& matrix_data = _matrix.get_data();
        const auto& expected_data = _result_data[ResultIdx];


        for(size_t i = 0; i < expected_data.size(); ++i) {
            if (expected_data[i] == V) continue; 
            assert(matrix_data[i] == expected_data[i] && "Data content mismatch!");
        }

        // 2. METADATA
        const auto& matrix_blocks = _matrix.get_blocks();
        const auto& expected_blocks = _result_blocks[ResultIdx];

        assert(matrix_blocks.size() == expected_blocks.size() && "Blocks count size mismatch!");

        for(size_t i = 0; i < matrix_blocks.size(); ++i) {
            assert(matrix_blocks[i].begin == expected_blocks[i].begin && "Block begin offset mismatch!");
            assert(matrix_blocks[i].size  == expected_blocks[i].size  && "Block internal size mismatch!");
        }
        
        // 3. IDS
        const auto& matrix_raw_ids = _matrix.get_id_to_rawId();
        const auto& expected_raw_ids = _result_id_to_rawId[ResultIdx];

        assert(matrix_raw_ids.size() == expected_raw_ids.size() && "ID mapping array size mismatch!");

        for(size_t i = 0; i < matrix_raw_ids.size(); ++i) {
            assert(matrix_raw_ids[i] == expected_raw_ids[i] && "Logical ID route translation breakdown!");
        }
    }

};


#endif