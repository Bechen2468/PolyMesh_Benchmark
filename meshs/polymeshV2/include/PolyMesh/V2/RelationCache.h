#ifndef RELATIONFLAG_H
#define RELATIONFLAG_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include <cstddef>
// ----------------------------------------------------------------------------
//#ifndef(std::uint32_t)
//#if defined(POLYMESH_INDEX_64)
//    using std::uint32_t = std::uint64_t;
//#elif defined(POLYMESH_INDEX_SIZET)
//    using std::uint32_t = size_t;
//#else
//    // Default to 32-bit to match industry standard graphics libraries
//    using std::uint32_t = std::uint32_t; 
//#endif
//#endif
// ----------------------------------------------------------------------------
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include "PolyMesh/V2/CacheFlag.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V2 {

using VVMatrix = Eigen::SparseMatrix<uint8_t, Eigen::ColMajor>;
using VFMatrix = Eigen::SparseMatrix<uint8_t, Eigen::RowMajor>;
using EFMatrix = Eigen::SparseMatrix<uint8_t, Eigen::RowMajor>;
using FEMatrix = Eigen::SparseMatrix<uint8_t, Eigen::RowMajor>;
using EdgeMatrix = Eigen::Matrix<size_t, Eigen::Dynamic, 2, Eigen::RowMajor>;

struct RelationCache {
    VVMatrix VV;
    VFMatrix VF;
    EFMatrix EF;
    FEMatrix FE;
    EdgeMatrix edges;

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