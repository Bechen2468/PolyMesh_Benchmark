#pragma once
// is never resolved true, just for intellicode linking
#ifndef POLY_MESH_V3_H
#include "PolyMesh/V3/Mesh.h"
#endif
// ----------------------------------------------------------------------------  



namespace PolyMesh::V3 {

#define _COMPACT_RESERVE false


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
// OLD:
//    const Index nv = n_vertices();
//
//    // build sizes
//    std::vector<Index> row_sizes(nv, 0);
//    
//
//    _relations.EF = typename T::EF(T::Reserve_Perz);
//    auto& ef = _relations.EF;
//    ef.reserve(nv * 6);
//    ef.set_largest_vertex_idx(_v.capacity());
//
//    for(auto f : _f) {
//        _EF_add_face(f, f.idx());
//    }
//
//    ef.compact({}, _COMPACT_RESERVE);


    // Because 2 edges form one entry for EF, its acctual size is not as easy predictable as the other Matrices
    // ( As edges cant be expected to always have 2 Faces adjacent )

    // PART 1: Calculating acctual sizes
    const Index cap = _v.capacity();

    // 1. half-edges per smaller vertex (upper bound, duplicates included), shifted by one for the prefix sum
    std::vector<Index> off(cap + 1, 0);
    for(auto f : _f) {
        const Index n = f.size();
        for(Index i = 0; i < n; ++i) {
            const Index a = f[i];
            const Index b = f[(i + 1) % n];
            if(a != b) ++off[std::min(a, b) + 1];
        }
    }

    // 2. prefix sum: bucket of vertex v is other[off[v] .. off[v+1])
    for(Index v = 0; v < cap; ++v) off[v + 1] += off[v];

    // 3. write the larger vertex of every half-edge into the bucket of its smaller vertex
    std::vector<Index> other(off[cap]);
    {
        std::vector<Index> pos(off.begin(), off.end() - 1);
        for(auto f : _f) {
            const Index n = f.size();
            for(Index i = 0; i < n; ++i) {
                const Index a = f[i];
                const Index b = f[(i + 1) % n];
                if(a != b) other[pos[std::min(a, b)]++] = std::max(a, b);
            }
        }
    }

    // 4. distinct partners per bucket = edges per row (mark[b] == v: already seen while handling v)
    std::vector<Index> cnt(cap, 0);
    std::vector<Index> mark(cap, T::INDEX_INVALID);

    for(Index v = 0; v < cap; ++v) {
        for(Index k = off[v]; k < off[v + 1]; ++k) {
            const Index b = other[k];
            if(mark[b] != v) {
                mark[b] = v;
                ++cnt[v];
            }
        }
    }


    // PART 2: And now that we have the acctual sizes ( cnt ), we rebuild EF from that and add faces as before in OLD
    _relations.EF = typename T::EF(cnt, T::Reserve_Perz);
    auto& ef = _relations.EF;

    // the constructor makes every row valid; deleted vertices must stay invalid like in the old build
    for(Index v = 0; v < _v.capacity(); ++v) {
        if(!_v.is_valid(v)) ef.set_row_invalid(v);
    }

    for(auto f : _f) {
        _EF_add_face(f, f.idx());
    }
    // no compact needed: we precalculated the acctual sizes
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
        else row[edge_slot].f_left  = face_idx;
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
void PM::_EF_apply(const std::vector<std::tuple<Index,Index,Index>>& removes,
                    const std::vector<std::tuple<Index,Index,Index>>& adds,
                    const std::vector<std::tuple<Index,Index,Index>>& edits) {
    auto& ef = _relations.EF;

    auto find_slot = [&](Index v0, Index v1) -> std::pair<Index,Index> {   // returns {canon_v0, slot}
        Index a = v0, b = v1;
        if(a > b) std::swap(a, b);
        auto row = ef.get_edges(a);
        for(Index k = 0; k < row.size(); ++k) if(row[k].v1 == b) return {a, k};
        return {a, T::INDEX_INVALID};
    };

    for(auto& [v0, v1, fid] : removes) {
        auto [a, slot] = find_slot(v0, v1);
        if(slot == T::INDEX_INVALID) continue;
        auto row = ef.get_edges(a);
        if(row[slot].f_left == fid) row[slot].f_left = T::INDEX_INVALID;
        if(row[slot].f_right == fid) row[slot].f_right = T::INDEX_INVALID;
        if(row[slot].f_left == T::INDEX_INVALID && row[slot].f_right == T::INDEX_INVALID)
            ef.edge_remove_swap(a, slot);
    }

    for(auto& [v0, v1, fid] : edits) {
        auto [a, slot] = find_slot(v0, v1);
        if(slot == T::INDEX_INVALID) continue;   // shouldn't happen for a genuine edit -- edge must already exist
        auto row = ef.get_edges(a);
        bool swapped = (v0 > v1);
        if(swapped) row[slot].f_right = fid;
        else        row[slot].f_left  = fid;
    }

    for(auto& [v0, v1, fid] : adds) {
        bool swapped = (v0 > v1);
        auto [a, slot] = find_slot(v0, v1);
        if(slot == T::INDEX_INVALID) {
            Index b = swapped ? v0 : v1;
            typename T::EF::_Edge_Data new_edge{b, T::INDEX_INVALID, T::INDEX_INVALID};
            slot = ef.edge_append(a, new_edge);
        }
        auto row = ef.get_edges(a);
        if(swapped) row[slot].f_right = fid;
        else        row[slot].f_left  = fid;
    }
}







PM_T
void PM::_VV_HE_build() {
// OLD
//    const Index nv = n_vertices();
//    const INdex nf = n_faces();
//
//    _relations.VV_HE = typename T::VV_HE(T::Reserve_Perz);
//    auto& vv_he = _relations.VV_HE;
//    vv_he.reserve(nv * 6);
//    vv_he.set_largest_vertex_idx(_v.capacity());
//
//    _edge_count = 0;    // rebuilding
//
//    for(auto f : _f) {
//        _VV_HE_add_face(f, f.idx());
//    }
//
//    vv_he.compact({}, _COMPACT_RESERVE);



    const Index nv = n_vertices();

    // Collection acctual row sizes
    _edge_count = 0;    // rebuilding
    std::vector<Index> row_sizes(nv, 0);

    for(auto f : _f) {
        for(Index iv = 0; iv < f.size(); ++iv) {
            row_sizes[f[iv]]++;
            _edge_count++;
        }
    }

    // Building VV_HE
    _relations.VV_HE = typename T::VV_HE(row_sizes, T::Reserve_Perz);

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
            if(row[k] == v1) { 
                exists = true; 
                break; 
            }
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
void PM::_VV_HE_apply(const std::vector<std::tuple<Index,Index,Index>>& removes,
                       const std::vector<std::tuple<Index,Index,Index>>& adds) {
    auto& vv_he = _relations.VV_HE;

    for (auto& [v0, v1, fid] : removes) {
        auto row = vv_he.get_edges(v0);
        for (Index k = 0; k < row.size(); ++k) {
            if (row[k] == v1) { vv_he.edge_remove_swap(v0, k); break; }
        }
    }

    for (auto& [v0, v1, fid] : adds) {
        if (!vv_he.is_vertex_valid(v0)) vv_he.set_row_valid(v0);

        auto row = vv_he.get_edges(v0);
        bool exists = false;
        for (Index k = 0; k < row.size(); ++k) {
            if (row[k] == v1) { exists = true; break; }
        }
        if (exists) continue;

        vv_he.edge_append(v0, v1);
    }
}








PM_T
void PM::_VF_build() {
    const Index nv = n_vertices();

    // Collection acctual row sizes
    _edge_count = 0;    // rebuilding
    std::vector<Index> row_sizes(nv, 0);

    for(auto f : _f) {
        for(Index iv = 0; iv < f.size(); ++iv) {
            row_sizes[f[iv]]++;
            _edge_count++;
        }
    }

    // Building VF
    _relations.VF = typename T::VF(row_sizes, T::Reserve_Perz);

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



PM_T
void PM::_VF_apply(const std::vector<std::tuple<Index,Index>>& removes,
                    const std::vector<std::tuple<Index,Index>>& adds) {
    auto& vf = _relations.VF;

    for (auto& [v0, fid] : removes) {
        auto row = vf.get_faces(v0);
        for (Index k = 0; k < row.size(); ++k) {
            if (row[k] == fid) { vf.face_remove_swap(v0, k); break; }
        }
    }

    for (auto& [v0, fid] : adds) {
        if (!vf.is_vertex_valid(v0)) vf.set_row_valid(v0);
        if (vf.vertex_is_next_to_face(v0, fid)) continue;

        vf.face_append(v0, fid);
    }
}

};
