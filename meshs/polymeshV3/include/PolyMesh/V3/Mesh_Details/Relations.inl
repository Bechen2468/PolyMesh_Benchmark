#pragma once
// is never resolved true, just for intellicode linking
#ifndef POLY_MESH_V3_H
#include "PolyMesh/V3/Mesh.h"
#endif
// ----------------------------------------------------------------------------  



namespace PolyMesh::V3 {


PM_T
void PM::set_validate_modifications(bool flag) {
    _update_VV_HE = flag;
    _update_EF = flag;
    _update_VF = flag;
    _validate_modifications = flag;
    if(flag) { 
        VV_HE(); 
        EF(); 
        VF();
    }
}


PM_T
void PM::set_update_VV_HE(bool flag) {
    _update_VV_HE = flag;
    if(!flag) _validate_modifications = false;
    else VV_HE();
}


PM_T
void PM::set_update_EF(bool flag) {
    _update_EF = flag;
    if(!flag) _validate_modifications = false;
    else EF();
}


PM_T
void PM::set_update_VF(bool flag) {
    _update_VF = flag;
    if(!flag) _validate_modifications = false;
    else VF();
}




PM_T
const PM::T::VV_HE& PM::VV_HE() {
    if(!_relations.is_valid(CacheFlag::VV_HE)) {
       _VV_HE_build();
        _relations.set_valid(CacheFlag::VV_HE);
    }
    return _relations.VV_HE;
}




PM_T
const PM::T::VF& PM::VF() {
    if(!_relations.is_valid(CacheFlag::VF)) {
        _VF_build();
        _relations.set_valid(CacheFlag::VF);
    }
    return _relations.VF;
}




PM_T
const PM::T::EF& PM::EF() {
    if(!_relations.is_valid(CacheFlag::EF)) {
        _EF_build();
        _relations.set_valid(CacheFlag::EF);
    }
    return _relations.EF;
}





PM_T
void PM::_EF_build() {
    const Index nv = n_vertices();

    _relations.EF = typename T::EF(T::Reserve_Perz);
    auto& ef = _relations.EF;
    ef.reserve(nv * 6);
    ef.set_largest_vertex_idx(_v.capacity());

    for(auto f : _f) {
        _EF_add_face(f, f.idx());
    }
}




PM_T
void PM::_EF_add_face(std::span<const Index> face, Index face_idx) {
    Index n = static_cast<Index>(face.size());
    auto& ef = _relations.EF;

    for(Index j = 0; j < n; ++j) {
        Index v0 = face[j];
        Index v1 = face[(j + 1) % n];

        bool swapped = v0 > v1;
        if(swapped) std::swap(v0, v1);

        Index edge_slot = T::INDEX_INVALID;

        if(!ef.is_vertex_valid(v0)) ef.set_row_valid(v0);

        auto row = ef.get_edges(v0);
        for(Index k = 0; k < row.size(); ++k) {
            if(row[k].v1 == v1) { 
                edge_slot = k; 
                break; 
            }
        }
        

        if(edge_slot == T::INDEX_INVALID) {
            typename T::EF::_Edge_Data new_edge{v1, T::INDEX_INVALID, T::INDEX_INVALID};
            edge_slot = ef.edge_append(v0, new_edge);
            row = ef.get_edges(v0); // refetch: append may have relocated
        }

        
        if(swapped) row[edge_slot].f_right = face_idx;
        else        row[edge_slot].f_left  = face_idx;
    }
}



PM_T
void PM::_EF_remove_face(std::span<const Index> face, Index face_idx) {
    Index n = static_cast<Index>(face.size());
    auto& ef = _relations.EF;

    for(Index j = 0; j < n; ++j) {
        Index v0 = face[j];
        Index v1 = face[(j + 1) % n];

        bool swapped = v0 > v1;
        if(swapped) std::swap(v0, v1);

        auto row = ef.get_edges(v0);
        Index edge_slot = T::INDEX_INVALID;
        for(Index k = 0; k < row.size(); ++k) {
            if(row[k].v1 == v1) { edge_slot = k; break; }
        }

        if(edge_slot == T::INDEX_INVALID) continue;

        if(row[edge_slot].f_left == face_idx)  row[edge_slot].f_left = T::INDEX_INVALID;
        if(row[edge_slot].f_right == face_idx) row[edge_slot].f_right = T::INDEX_INVALID;

        if(row[edge_slot].f_left == T::INDEX_INVALID && row[edge_slot].f_right == T::INDEX_INVALID) {
            ef.edge_remove_swap(v0, edge_slot);
        }
    }
}








PM_T
void PM::_VV_HE_build() {
    const Index nv = n_vertices();

    _relations.VV_HE = typename T::VV_HE(T::Reserve_Perz);
    auto& vv_he = _relations.VV_HE;
    vv_he.reserve(nv * 6);
    vv_he.set_largest_vertex_idx(_v.capacity());

    _edge_count = 0;    // rebuilding

    for(auto f : _f) {
        _VV_HE_add_face(f, f.idx());
    }
}




PM_T
void PM::_VV_HE_add_face(std::span<const Index> face, Index face_idx) {
    Index n = static_cast<Index>(face.size());
    auto& vv_he = _relations.VV_HE;

    for(Index j = 0; j < n; ++j) {
        Index v0 = face[j];
        Index v1 = face[(j + 1) % n];

        if(!vv_he.is_vertex_valid(v0)) vv_he.set_row_valid(v0);
        auto row = vv_he.get_edges(v0);

        bool exists = false;
        for(Index k = 0; k < row.size(); ++k) {
            if(row[k] == v1) { exists = true; break; }
        }
        if(exists) continue;

        if(!vv_he.is_vertex_valid(v1)) vv_he.set_row_valid(v1);
        bool reverse_exists = vv_he.edge_exists(v1, v0);

        vv_he.edge_append(v0, v1);
        if(!reverse_exists) ++_edge_count;
    }
}




PM_T
void PM::_VV_HE_remove_face(std::span<const Index> face, Index face_idx) {
    Index n = static_cast<Index>(face.size());
    auto& vv_he = _relations.VV_HE;

    for(Index j = 0; j < n; ++j) {
        Index v0 = face[j];
        Index v1 = face[(j + 1) % n];

        auto row = vv_he.get_edges(v0);
        for(Index k = 0; k < row.size(); ++k) {
            if(row[k] == v1) {
                vv_he.edge_remove_swap(v0, k);
                if(!vv_he.edge_exists(v1, v0)) --_edge_count;
                break;
            }
        }
    }
}








PM_T
void PM::_VF_build() {
    const Index nf = n_faces();

    _relations.VF = typename T::VF(T::Reserve_Perz);
    auto& vf = _relations.VF;
    vf.reserve(nf * 3);
    vf.set_largest_vertex_idx(_v.capacity());

    for(auto f : _f) {
        _VF_add_face(f, f.idx());
    }
}




PM_T
void PM::_VF_add_face(std::span<const Index> face, Index face_idx) {
    Index n = static_cast<Index>(face.size());
    auto& vf = _relations.VF;

    for(Index j = 0; j < n; ++j) {
        Index v0 = face[j];

        if(!vf.is_vertex_valid(v0)) vf.set_row_valid(v0);
        if(vf.vertex_is_next_to_face(v0, face_idx)) continue;

        vf.face_append(v0, face_idx);
    }
}




PM_T
void PM::_VF_remove_face(std::span<const Index> face, Index face_idx) {
    Index n = static_cast<Index>(face.size());
    auto& vf = _relations.VF;

    for(Index j = 0; j < n; ++j) {
        Index v0 = face[j];

        auto row = vf.get_faces(v0);
        for(Index k = 0; k < row.size(); ++k) {
            if(row[k] == face_idx) {
                vf.face_remove_swap(v0, k);
                break;
            }
        }
    }
}

};