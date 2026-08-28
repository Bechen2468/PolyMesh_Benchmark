#ifndef IBENCHMARK_STRUCT_H
#define IBENCHMARK_STRUCT_H
// Benchmark test meta-template
// ----------------------------------------------------------------------------
#include <fstream>
#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <span>
#include <benchmark/benchmark.h>
#include <Eigen/Sparse>
#include <PolyMesh/V3/DataStructs/D_Ragged_Matrix.h>
#include "my_utils.h"



#define BM_MATRIX_START 100000
#define BM_MATRIX_END   100000


#define REGISTER_TEMPLATE(FIXTURE, TEMPLATE_T, NAME, FUNC)                         \
BENCHMARK_TEMPLATE_DEFINE_F(FIXTURE, NAME, TEMPLATE_T)(benchmark::State& state) {  \
    FUNC(state);                                              \
}                                                             \
BENCHMARK_REGISTER_F(FIXTURE, NAME)                           \
    ->RangeMultiplier(RANGE_MULTIPLIER)                       \
    ->Range(BM_MATRIX_START, BM_MATRIX_END)                   \
    ->Unit(benchmark::kMicrosecond);
    //->Iterations(1)                                        \




// ----------------------------------------------------------------------------
struct Adapter_Eigen_Sparse {
    Eigen::SparseMatrix<size_t, Eigen::RowMajor> m;
    size_t m_activeRows = 0;
    float reserve_Perz = 0.2;
    size_t m_targetPerRowCapacity = 0;


    bool is_eigen() const {
        return true;
    }

    void reset(float Reserve_Perz) {
        m = Eigen::SparseMatrix<size_t, Eigen::RowMajor>();
        m_activeRows = 0;
        reserve_Perz = Reserve_Perz;
    }

    // Reserved Cols = Cols * (1 + Reserve_Perz), per row.
    void reserve(size_t Rows, size_t Cols, float Reserve_Perz) {
        reserve_Perz = Reserve_Perz;
        m.resize(static_cast<Eigen::Index>(Rows), static_cast<Eigen::Index>(Cols));

        m_targetPerRowCapacity = static_cast<size_t>(std::ceil(Cols * (1.0f + reserve_Perz)));
        Eigen::VectorXi reserveSizes = Eigen::VectorXi::Constant(static_cast<Eigen::Index>(Rows), static_cast<int>(m_targetPerRowCapacity));
        m.reserve(reserveSizes);
        m_activeRows = 0;
    }



    size_t row_capacity(size_t Idx) const {
        return static_cast<size_t>(m.outerIndexPtr()[Idx + 1] - m.outerIndexPtr()[Idx]);
    }



    // no worst case
    void make_worst_case() {}

    inline size_t get(size_t Row, size_t Col) {
        return (m.coeff(static_cast<Eigen::Index>(Row), static_cast<Eigen::Index>(Col)));
    }

    inline void set(size_t Row, size_t Col, size_t Data) {
        m.coeffRef(static_cast<Eigen::Index>(Row), static_cast<Eigen::Index>(Col)) = static_cast<size_t>(Data);
    }

    void add_Row() {
        ++m_activeRows;
    }

    void delete_Row(size_t Idx) {
        if (!m.isCompressed()) {
            m.innerNonZeroPtr()[Idx] = 0;
        }
    }

    void extend_Row(size_t Idx, size_t Data) {
        Eigen::Index row = static_cast<Eigen::Index>(Idx);
        if (row >= m.rows()) {
            m.conservativeResize(std::ceil(row * (1 + reserve_Perz)), m.cols());
        }

        Eigen::Index col = static_cast<Eigen::Index>(row_size(Idx));
        if (col >= m.cols()) {
            m.conservativeResize(m.rows(), std::ceil(col * (1 + reserve_Perz)));
        }


        Eigen::Index usedSize  = m.data().size();
        Eigen::Index allocSize = m.data().allocatedSize();
        if (allocSize - usedSize < 1) {
            Eigen::Index topUp = static_cast<Eigen::Index>(std::ceil(usedSize * (1 + reserve_Perz))) + 16;
            m.data().reserve(topUp);
        }

        m.insert(row, col) = Data;
    }

    void remove_from_Row(size_t Idx, size_t Data) {
        Eigen::Index start = m.outerIndexPtr()[Idx];
        int count = m.innerNonZeroPtr()[Idx];
        size_t target = static_cast<size_t>(Data);
 
        Eigen::Index foundOffset = -1;
        for (int j = 0; j < count; ++j) {
            if (m.valuePtr()[start + j] == target) {
                foundOffset = start + j;
                break;
            }
        }
        if (foundOffset < 0) return; // value not present in this row
 
        // shift everything after the removed entry left by one to stay dense
        for (Eigen::Index k = foundOffset; k < start + count - 1; ++k) {
            m.valuePtr()[k] = m.valuePtr()[k + 1];
        }
        m.innerNonZeroPtr()[Idx] = count - 1;

    }

    size_t rows() const {
        return m_activeRows;
    }

    size_t row_size(size_t Idx) const {
        if (m.isCompressed()) {
            return m.outerIndexPtr()[Idx + 1] - m.outerIndexPtr()[Idx];
        }
        return static_cast<size_t>(m.innerNonZeroPtr()[Idx]);
    }
};




// ----------------------------------------------------------------------------
struct Adapter_D_Ragged_Matrix {
    PolyMesh::V3::D_Ragged_Matrix<size_t, size_t, uint8_t> m;
    float reserve_Perz = 0.2;
    std::vector<size_t> m_usedSize;


    bool is_eigen() const {
        return false;
    }

    void reset(float Reserve_Perz) {
        m = PolyMesh::V3::D_Ragged_Matrix<size_t, size_t, uint8_t>(0, Reserve_Perz);
        reserve_Perz = Reserve_Perz;
        m_usedSize.resize(0);
    }

    // Reserved Cols = Cols * (1 + Reserve_Perz), per row.
    inline void reserve(size_t Rows, size_t Cols, float Reserve_Perz) {
        reserve_Perz = Reserve_Perz;
        m.reserve(Rows * Cols * (1 + reserve_Perz));
        m_usedSize.resize(Rows * (1 + reserve_Perz), 0);
    }


    void make_worst_case() {
        auto& idx_to_raw = m.get_id_to_rawId();
        auto& raw_to_idx = m.get_rawId_to_id();

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(idx_to_raw.begin(), idx_to_raw.end(), g);

        for(size_t i = 0; i < idx_to_raw.size(); ++i) {
            raw_to_idx[idx_to_raw[i]] = i;
        }
    }

    inline size_t get(size_t Row, size_t Col) {
        size_t rawId = m.get_id_to_rawId()[Row];
        return m.get(rawId, Col);
    }

    inline void set(size_t Row, size_t Col, size_t Data) {
        size_t rawId = m.get_id_to_rawId()[Row];
        m.get(rawId, Col) = Data;
    }

    inline void add_Row() {
        m.append_row(std::span<const size_t>());

        if(m_usedSize.size() <= rows()) {
            m_usedSize.resize((rows() + 10) * (1 + reserve_Perz), 0);
        }
    }

    inline void delete_Row(size_t Idx) {
        m.delete_row(Idx);
        m_usedSize[Idx] = 0;
    }

    inline void extend_Row(size_t Idx, size_t Data) {
        // reserve
        if (m_usedSize[Idx] >= m.row_size(Idx)) {
            size_t newCap = static_cast<size_t>(std::ceil((m_usedSize[Idx] + 1) * (1 + reserve_Perz)));
            m.row_resize(Idx, newCap);
        }
        
        size_t rawId = m.get_id_to_rawId()[Idx];
        m.get(rawId, m_usedSize[Idx]) = Data;
        ++m_usedSize[Idx];
        if(m_usedSize[Idx] > 10) std::cout << m_usedSize[Idx] << std::endl;
    }

    inline void remove_from_Row(size_t Idx, size_t New_Size) {
        if (m_usedSize[Idx] == 0 || New_Size == 0) return; // nothing to remove
        --m_usedSize[Idx];
    }

    inline size_t rows() const {
        return m.row_count();
    }

    inline size_t row_size(size_t Idx) const {
        return m_usedSize[Idx];
    }
};






// ----------------------------------------------------------------------------
template<typename AdapterT>
class IBenchmark_Matrix : public benchmark::Fixture {
protected:
    volatile double _benchmark_sink = 0.0;


    AdapterT a;

    struct {
        size_t rows;
        size_t cols;
        float reserve_Perz;
    } cb;


private:
    bool _setUpIsFirst = true;

public:
    void SetUp(benchmark::State& state) override {
        cb.cols = 4;
        cb.rows = state.range(0) / cb.cols;
        cb.reserve_Perz = 0.34;
        _build();
    }


    void TearDown(benchmark::State& state) override {
        a.reset(cb.reserve_Perz);
    }


    void run_iterate(benchmark::State& state) {
        for (auto _ : state) {
            _reset_struct(state);
            
            for(size_t i = 0; i < a.rows(); ++i) {
                size_t row_size = a.row_size(i);
                for(size_t j = 0; j < row_size; ++j) {
                    _benchmark_sink += a.get(i, j);
                }   
            }

            benchmark::DoNotOptimize(_benchmark_sink);
        }
    }


    void run_iterate_worst_case(benchmark::State& state) {
        for (auto _ : state) {
            _reset_struct(state, true);
            
            for(size_t i = 0; i < a.rows(); ++i) {
                size_t row_size = a.row_size(i);
                for(size_t j = 0; j < row_size; ++j) {
                    _benchmark_sink += a.get(i, j);
                }   
            }

            benchmark::DoNotOptimize(_benchmark_sink);
        }
    }

    
    void run_extend_InReserve(benchmark::State& state) {
        for (auto _ : state) {
            _reset_struct(state);
            _extend(state, cb.cols * cb.reserve_Perz);

            benchmark::DoNotOptimize(_benchmark_sink);
        }
    }

    
    void run_extend_OverReserve(benchmark::State& state) {
        for (auto _ : state) {
            _reset_struct(state);
            _extend(state, ceil(cb.cols * cb.reserve_Perz * 2));

            benchmark::DoNotOptimize(_benchmark_sink);
        }
    }


    void run_remove_from_Row(benchmark::State& state) {
        for (auto _ : state) {
            _reset_struct(state);
            
            for(size_t i = 0; i < a.rows(); ++i) {
                size_t row_size = a.row_size(i);
                for (size_t j = 0; j < row_size; ++j) {
                    a.remove_from_Row(i, row_size - 1 - j);
                }
            }

            benchmark::DoNotOptimize(_benchmark_sink);
        }
    }


    void run_add_rows(benchmark::State& state) {
        for (auto _ : state) {
            //a.reset(cb.reserve_Perz);
            _reset_struct(state);
            
            for(size_t i = 0; i < cb.rows; ++i) {
                a.add_Row();
                for(size_t j = 0; j < cb.cols; ++j) {
                    a.extend_Row(i, j);
                }
            }

            benchmark::DoNotOptimize(_benchmark_sink);
        }
    }


    void run_delete_rows(benchmark::State& state) {
        for (auto _ : state) {
            _reset_struct(state);
            
            size_t originalRows = a.rows();
            for(size_t i = 0; i < originalRows; ++i) {
                a.delete_Row(i);
            }

            benchmark::DoNotOptimize(_benchmark_sink);
        }
    }




protected:
    void _reset_struct(benchmark::State& state, bool make_worstcase = false) {
        state.PauseTiming();
        _build();
        if(make_worstcase) {
            a.make_worst_case();
        }
        state.ResumeTiming();
    }


    // builds the matrix with Random Col usage between Min and Max and an additional reserved space
    void _build() {
        bool d_csr_row_reserve = false;
        size_t row_reserve_len = ceil((cb.cols + 1) * (1 + cb.reserve_Perz));

        a.reset(cb.reserve_Perz);
        a.reserve(cb.rows, cb.cols, cb.reserve_Perz);
        
        for (size_t r = 0; r < cb.rows; ++r) {
            a.add_Row();
            for (size_t i = 0; i < cb.cols; ++i) {
                a.extend_Row(r, i);
            }

            //if(d_csr_row_reserve && !a.is_eigen()) {
            //    for (size_t i = cb.cols; i < row_reserve_len; ++i) {
            //        a.extend_Row(r, i);
            //    }
            //}
        }
        
        //if(d_csr_row_reserve && !a.is_eigen()) {
        //    for (size_t r = 0; r < cb.rows; ++r) {
        //        for (size_t i = cb.cols; i < row_reserve_len; ++i) {
        //            a.remove_from_Row(r, i);
        //        }
        //    }
        //}
    }


    
    
    void _extend(benchmark::State& state, size_t Amount) {
        for(size_t i = 0; i < a.rows(); ++i) {
            size_t row_size = cb.cols;
            for(size_t j = 0; j < Amount; ++j) {
                a.extend_Row(i, row_size + j);
            }   
        }
    }

};





#endif


