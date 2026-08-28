#pragma once
// is never resolved true, just for intellicode linking
#ifndef POLY_MESH_V2_H
#include "PolyMesh/V2/Mesh.h"
#endif


namespace PolyMesh::V2 {

template<typename ScalarT, typename IndexT>
IndexT PolyMesh<ScalarT, IndexT>::add_vertex(const PolyMesh<ScalarT, IndexT>::Vertex& Vert) {

    IndexT Idx = (IndexT)_v_valid.get_free();

    if(Idx >= _v.rows())
        _v.conservativeResize(Idx + 1, 3);

    _v.row(Idx) = Vert;
    _n_vertices++;
    return Idx;
    // no invalidation: isolated vertex has no relations yet
}


template<typename ScalarT, typename IndexT>
void PolyMesh<ScalarT, IndexT>::delete_vertex(IndexT Idx) {
    if(!_v_valid.is_valid(Idx)) return;
    _v_valid.set_invalid(Idx);
    _n_vertices--;

    _relations.invalidate(CacheFlag::VV | CacheFlag::VF |
        CacheFlag::EF | CacheFlag::FE |
        CacheFlag::Edges
    );
}

    
template<typename ScalarT, typename IndexT>
void PolyMesh<ScalarT, IndexT>::replace_vertex(IndexT Idx, const PolyMesh<ScalarT, IndexT>::Vertex& Vert) {
    _v.col(Idx) = Vert;
}


template<typename ScalarT, typename IndexT>
IndexT PolyMesh<ScalarT, IndexT>::add_face(const IndexT* Verts, uint8_t N) {
    IndexT Idx = _f.add_face(Verts, N);  // FaceMatrix owns the slot allocation
    _n_faces++;

    _relations.invalidate(
        CacheFlag::VV | CacheFlag::VF |
        CacheFlag::EF | CacheFlag::FE |
        CacheFlag::Edges
    );
    return Idx;
}


template<typename ScalarT, typename IndexT>
void PolyMesh<ScalarT, IndexT>::delete_face(IndexT Idx) {
    if(!_f.valid.is_valid(Idx)) return;
    _f.valid.set_invalid(Idx);
    _n_faces--;

    _relations.invalidate(
        CacheFlag::VV | CacheFlag::VF |
        CacheFlag::EF | CacheFlag::FE |
        CacheFlag::Edges
    );
}




template<typename ScalarT, typename IndexT>
IndexT PolyMesh<ScalarT, IndexT>::_find_opposite_vertex(IndexT Face, IndexT V0, IndexT V1) const {
    const IndexT* fv = _f.face_begin(Face);
    for(uint8_t i = 0; i < 3; i++)
        if(fv[i] != V0 && fv[i] != V1) return fv[i];
    return (IndexT) -1; // shouldnt happen for a valid triangle
}
};
