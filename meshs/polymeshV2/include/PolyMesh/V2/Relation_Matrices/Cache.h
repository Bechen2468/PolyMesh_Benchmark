#ifndef RELATIONFLAG_V2_H
#define RELATIONFLAG_V2_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include "PolyMesh/V2/Relation_Matrices/CacheFlag.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V2 {
    
    template<typename IndexT = std::uint32_t>
    using VVMatrix = Eigen::SparseMatrix<uint8_t, Eigen::ColMajor, std::make_signed_t<IndexT>>;
    template<typename IndexT = std::uint32_t>
    using VFMatrix = Eigen::SparseMatrix<uint8_t, Eigen::RowMajor, std::make_signed_t<IndexT>>;
    template<typename IndexT = std::uint32_t>
    using EFMatrix = Eigen::SparseMatrix<uint8_t, Eigen::RowMajor, std::make_signed_t<IndexT>>;
    template<typename IndexT = std::uint32_t>
    using FEMatrix = Eigen::SparseMatrix<uint8_t, Eigen::RowMajor, std::make_signed_t<IndexT>>;
    template<typename IndexT = std::uint32_t>
    using EdgeMatrix = Eigen::Matrix<IndexT, Eigen::Dynamic, 2, Eigen::RowMajor>;

template<typename ScalarT, typename IndexT = std::uint32_t>
struct RelationCache {
    
public:
    VVMatrix<IndexT> VV;
    VFMatrix<IndexT> VF;
    EFMatrix<IndexT> EF;
    FEMatrix<IndexT> FE;
    EdgeMatrix<IndexT> edges;

    bool has_VV    = false;
    bool has_VF    = false;
    bool has_EF    = false;
    bool has_FE    = false;
    bool has_edges = false;

    void invalidate(CacheFlag flags) {
        if(flags & CacheFlag::VV)    { VV    = {}; has_VV    = false; }
        if(flags & CacheFlag::VF)    { VF    = {}; has_VF    = false; }
        if(flags & CacheFlag::EF)    { EF    = {}; has_EF    = false; }
        if(flags & CacheFlag::FE)    { FE    = {}; has_FE    = false; }
        if(flags & CacheFlag::Edges) { edges = {}; has_edges = false; }
    }
};

};
#endif