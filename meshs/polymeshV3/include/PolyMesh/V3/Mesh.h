#ifndef POLY_MESH_V3_H
#define POLY_MESH_V3_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <span>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "PolyMesh/V3/Traits/Traits.h"
#include "PolyMesh/V3/DataStructs/ValidMask.h"
#include "PolyMesh/V3/Relation_Matrices/Cache.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {


template<TraitsC TraitsT = Traits<float, std::uint32_t, std::uint8_t>>
class PolyMesh {
public:
    using T = typename TraitsT::template Rebind<PolyMesh>;  // Binding Friend access

    template<typename T>
    using Vec = std::vector<T, Aligned_Allocator<T, 64>>;


    // unpacking base/atomic types for convinience
    using Index = T::Index;
    using Scalar = T::Scalar;



private:
    // options
    bool _validate_modifications = false;
    bool _update_VV_HE = false;
    bool _update_EF = false;
    bool _update_VF = false;

    T::Vertices _v;
    T::Faces _f;
    Index _edge_count = 0;
    RelationCache<T> _relations;



public:
    PolyMesh();
    ~PolyMesh();

    void read_file(const std::string& File_Path);

    inline Index n_vertices() const { return _v.valid_amount(); }
    inline Index n_faces() const    { return _f.valid_amount(); }
    inline Index n_edges() {
        if (!_relations.is_valid(CacheFlag::VV_HE)) VV_HE();   // forces a rebuild, which also recomputes _edge_count
        return _edge_count;
    }

    inline T::Vertices& vertices()  { return _v; }
    inline T::Faces& faces()        { return _f; }
    inline RelationCache<T>& relations() { return _relations; }

// ----------------------------------------------------------------------------  
// Modifiers
// ----------------------------------------------------------------------------  
public:
    // std::span<const Scalar, 3>, std::array, std::vector, raw arrays
    Index add_vertex(std::span<const Scalar, 3> Vert);

    // inline: mesh.add_vertex({1.0f, 2.0f, 3.0f});
    Index add_vertex(std::initializer_list<Scalar> Vert);

    

    void delete_vertex(Index Idx);

    Index add_face(std::span<const Index> Vertices);
    Index add_face(std::initializer_list<Index> Vertices);
    void delete_face(Index Idx);

    [[nodiscard]] bool is_face_addable(std::span<const Index> Face);

    [[nodiscard]] std::tuple<Index, Index> edge_bordering_faces(Index P0, Index P1);

    [[nodiscard]] std::tuple<Index, Index> try_get_flip_faces(Index P0, Index P1);

    std::tuple<Index, Index> flip(Index F0, Index F1);

    Index split(Index F, std::span<const Scalar, 3> P);

// ----------------------------------------------------------------------------  
// Helper Matrices
// ----------------------------------------------------------------------------  
public:
    void set_validate_modifications(bool flag);
    void set_update_VV_HE(bool flag);
    void set_update_EF(bool flag);
    void set_update_VF(bool flag);


    // always valid, builds if needed
    const T::VV_HE& VV_HE();
    const T::VF& VF();
    const T::EF& EF();

private:
    void _EF_build();
    void _EF_add_face(std::span<const Index> face, Index f_idx);
    void _EF_remove_face(std::span<const Index> face, Index face_idx);
    
    void _VV_HE_build();
    void _VV_HE_add_face(std::span<const Index> face, Index f_idx);
    void _VV_HE_remove_face(std::span<const Index> face, Index face_idx);
    
    void _VF_build();
    void _VF_add_face(std::span<const Index> face, Index f_idx);
    void _VF_remove_face(std::span<const Index> face, Index face_idx);
// ----------------------------------------------------------------------------  
};


#define PM_T template<TraitsC TraitsT>
#define PM PolyMesh<TraitsT>
};


#include "PolyMesh/V3/Mesh_Details/Core.inl"
#include "PolyMesh/V3/Mesh_Details/Modifiers.inl"
#include "PolyMesh/V3/Mesh_Details/Relations.inl"
#endif
