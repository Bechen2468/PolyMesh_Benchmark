// is never resolved true, just for intellicode linking
#ifndef POLY_MESH_V3_H
#include "PolyMesh/V3/Mesh.h"
#endif


namespace PolyMesh::V3 {

PM_T
void PM::compact(bool Index_Compaction, bool With_Reserve) { 
    if(Index_Compaction) {
        // TODO
        return;
    }

    _f.compact({}, With_Reserve);
    if(_relations.is_valid(CacheFlag::VV_HE)) _relations.VV_HE.compact({}, With_Reserve);
    if(_relations.is_valid(CacheFlag::VF)) _relations.VF.compact({}, With_Reserve);
    if(_relations.is_valid(CacheFlag::EF)) _relations.EF.compact({}, With_Reserve);
}



PM_T
PM::Index PM::add_vertex(std::span<const Scalar, 3> Vert) { 
    Index idx = _v.add(Vert);

    // update relationmatrices
    if(_validate_modifications) {
        _relations.EF.set_largest_vertex_idx(idx + 1);
        _relations.VV_HE.set_largest_vertex_idx(idx + 1);
        _relations.VF.set_largest_vertex_idx(idx + 1);
    }
    else {
        if(_update_EF) _relations.EF.set_largest_vertex_idx(idx + 1);
        else _relations.invalidate(CacheFlag::EF);

        if(_update_VV_HE) _relations.VV_HE.set_largest_vertex_idx(idx + 1);
        else _relations.invalidate(CacheFlag::VV_HE);

        if(_update_VF) _relations.VF.set_largest_vertex_idx(idx + 1);
        else _relations.invalidate(CacheFlag::VF);
    }

    return idx; 
}



PM_T
PM::Index PM::add_vertex(std::initializer_list<Scalar> Vert) { 
    return add_vertex(std::span<const Scalar, 3>(Vert.begin(), 3)); 
}




PM_T
void PM::delete_vertex(Index Idx) {
    // check if any faces still reference it
    if(_validate_modifications) {
        auto incident_view = VF().get_faces(Idx);
        std::vector<Index> incident(incident_view.begin(), incident_view.end());

        for (Index f_idx : incident) delete_face(f_idx);
    }

    _v.remove(Idx);
}



PM_T
PM::Index PM::add_face(std::span<const Index> Face) {
    if(_validate_modifications && !is_face_addable(Face)) return T::INDEX_INVALID;

    Index f_idx = _f.add(Face);

    if(_validate_modifications) {
        _EF_add_face(Face, f_idx);
        _VV_HE_add_face(Face, f_idx);
        _VF_add_face(Face, f_idx);
    }
    else {
        if(_update_EF) _EF_add_face(Face, f_idx);
        else _relations.invalidate(CacheFlag::EF);

        if(_update_VV_HE) _VV_HE_add_face(Face, f_idx);
        else _relations.invalidate(CacheFlag::VV_HE);

        if(_update_VF) _VF_add_face(Face, f_idx);
        else _relations.invalidate(CacheFlag::VF);
    }

    return f_idx;
}


PM_T
PM::Index PM::add_face(std::initializer_list<Index> Face) {
    return add_face(std::span<const Index>{Face.begin(), Face.size()});
}


PM_T
void PM::delete_face(Index Idx) {
    auto face = faces()[Idx];

    if(_validate_modifications) {
        _EF_remove_face(face, Idx);
        _VV_HE_remove_face(face, Idx);
        _VF_remove_face(face, Idx);
    }
    else {
        if(_update_EF) _EF_remove_face(face, Idx);
        else _relations.invalidate(CacheFlag::EF);

        if(_update_VV_HE) _VV_HE_remove_face(face, Idx);
        else _relations.invalidate(CacheFlag::VV_HE);

        if(_update_VF) _VF_remove_face(face, Idx);
        else _relations.invalidate(CacheFlag::VF);
    }

    _f.remove(Idx);
}



PM_T
[[nodiscard]] bool PM::is_face_addable(std::span<const Index> Face) {
    // -------------------------------------------------------------------------
    // Init and helper
    // -------------------------------------------------------------------------
    const typename T::VV_HE& vv_he = VV_HE();
    const size_t n = Face.size();
    if(n < 3) return false; // Minimal face must be a triangle

    // Helper: Returns true if directed edge (from -> to) exists in the mesh
    auto has_edge = [&](Index from, Index to) -> bool {
        if(from == T::INDEX_INVALID || to == T::INDEX_INVALID) return false;
        for(auto v : vv_he.get_edges(from)) if (v == to) return true;
        return false;
    };

    // -------------------------------------------------------------------------
    // CHECK 1: Duplicate vertices
    // -------------------------------------------------------------------------
    for(size_t i = 0; i < n; ++i) {
        if (Face[i] == T::INDEX_INVALID) return false;

        for(size_t j = i + 1; j < n; ++j) {
            if (Face[i] == Face[j]) return false;
        }
    }

    // -------------------------------------------------------------------------
    // CHECK 2: Directed Edge Saturation
    // Each Directed Edge can only be used by one face
    // Cant add already existing edge
    // Note: Passing guarantees that every directed edge in Face is currently unused.
    // -------------------------------------------------------------------------
    for(size_t i = 0; i < n; ++i) {
        Index va = Face[i];
        Index vb = Face[(i + 1) % n];

        if(has_edge(va, vb)) return false;
    }

    // -------------------------------------------------------------------------
    // CHECK 3: Vertex Star Manifold Topo
    // Evaluate local topological validity around every vertex in the new face
    // -------------------------------------------------------------------------
    for(size_t i = 0; i < n; ++i) {
        Index v_curr = Face[i];
        auto row = vv_he.get_edges(v_curr);
        if(row.size() == 0) continue;

        size_t boundary_count = 0;
        for(auto v : row) {
            if (EF().edge_face_count(v_curr, v) < 2) boundary_count++;
        }
        if (boundary_count == 0) return false;

        Index v_prev = Face[(i + n - 1) % n];
        Index v_next = Face[(i + 1) % n];
        bool reverse_prev_edge_exists = has_edge(v_curr, v_prev);
        bool reverse_next_edge_exists = has_edge(v_next, v_curr);

        if(!reverse_prev_edge_exists && !reverse_next_edge_exists && boundary_count > 1) {
            return false;
        }
    }

    return true;
}




PM_T
[[nodiscard]] std::tuple<typename PM::Index, typename PM::Index> PM::edge_bordering_faces(Index P0, Index P1) {
    if (P0 > P1) std::swap(P0, P1);
    const auto& ef = EF();
    auto row = ef.get_edges(P0);
    for (Index k = 0; k < row.size(); ++k) {
        if (row[k].v1 == P1) return {row[k].f_left, row[k].f_right};
    }
    return {T::INDEX_INVALID, T::INDEX_INVALID};
}




PM_T
[[nodiscard]] std::tuple<typename PM::Index, typename PM::Index> PM::try_get_flip_faces(Index P0, Index P1) {
    if (P0 == P1) return {T::INDEX_INVALID, T::INDEX_INVALID}; // degenerate edge

    auto [f0, f1] = edge_bordering_faces(P0, P1);
    if(f0 == T::INDEX_INVALID || f1 == T::INDEX_INVALID) return {T::INDEX_INVALID, T::INDEX_INVALID};
    if(f0 == f1) return {T::INDEX_INVALID, T::INDEX_INVALID};

    if(faces().face_degree(f0) != 3 || faces().face_degree(f1) != 3) return {T::INDEX_INVALID, T::INDEX_INVALID};
    return {f0, f1};
}



PM_T
std::tuple<typename PM::Index, typename PM::Index> PM::flip(Index F0, Index F1) {
    constexpr Index INVALID = T::INDEX_INVALID;
    if (F0 == F1) return {INVALID, INVALID};
    if (faces().degree(F0) != 3 || faces().degree(F1) != 3) return {INVALID, INVALID};

    auto f0 = faces()[F0];
    auto f1 = faces()[F1];
    Index shared[2]; int shared_count = 0;
    Index opp0 = INVALID, opp1 = INVALID;

    for (Index a : f0) {
        bool in_f1 = false;
        for (Index b : f1) if (a == b) { in_f1 = true; break; }
        if (in_f1) { if (shared_count < 2) shared[shared_count] = a; ++shared_count; }
        else opp0 = a;
    }
    for (Index b : f1) {
        bool in_f0 = false;
        for (Index a : f0) if (a == b) { in_f0 = true; break; }
        if (!in_f0) opp1 = b;
    }
    if (shared_count != 2 || opp0 == INVALID || opp1 == INVALID) return {INVALID, INVALID};
    if (edge_face_count(opp0, opp1) > 0) return {INVALID, INVALID}; // would create a duplicate edge

    Index p0 = shared[0], p1 = shared[1];
    delete_face(F0);
    delete_face(F1);
    Index nf0 = add_face({opp0, opp1, p0});
    Index nf1 = add_face({opp1, opp0, p1});
    return {nf0, nf1};
}




#pragma region SPLIT_COLLAPSE_MINIMAL
PM_T
typename PM::Index PM::split(Index F, std::span<const Scalar, 3> P) {
    auto row = faces()[F];
    std::vector<Index> face_verts(row.begin(), row.end());
    Index n = static_cast<Index>(face_verts.size());
    assert(n == 3 && "split: in-place fast path currently assumes triangles");

    Index new_v = add_vertex(P);

    _storage_edge_removes.clear();
    _storage_edge_adds.clear();
    _storage_edge_edits.clear();
    _storage_vf_removes.clear();
    _storage_vf_adds.clear();

    Index v0 = face_verts[0], v1 = face_verts[1], v2 = face_verts[2];

    Index f_reuse = F;
    Index f_new1  = _f.add(std::array<Index,3>{new_v, v1, v2});
    Index f_new2  = _f.add(std::array<Index,3>{new_v, v2, v0});

    // TODO:
    // make one copy
    //_f.set_row(f_reuse, std::array<Index,3>{new_v, v0, v1});
    auto f = _f[f_reuse];
    f[0] = new_v;
    f[1] = v0;
    f[2] = v1;
    

    _storage_edge_adds.emplace_back(new_v, v0, f_reuse);
    _storage_edge_adds.emplace_back(new_v, v1, f_new1);
    _storage_edge_adds.emplace_back(new_v, v2, f_new2);

    _storage_edge_edits.emplace_back(v0, v1, f_reuse);
    _storage_edge_edits.emplace_back(v1, v2, f_new1);
    _storage_edge_edits.emplace_back(v2, v0, f_new2);

    _storage_edge_adds.emplace_back(new_v, v0, f_reuse);
    _storage_edge_adds.emplace_back(v0, new_v, f_new2);
    _storage_edge_adds.emplace_back(new_v, v1, f_new1);
    _storage_edge_adds.emplace_back(v1, new_v, f_reuse);
    _storage_edge_adds.emplace_back(new_v, v2, f_new2);
    _storage_edge_adds.emplace_back(v2, new_v, f_new1);

    _storage_vf_removes.emplace_back(v0, F);
    _storage_vf_removes.emplace_back(v1, F);
    _storage_vf_removes.emplace_back(v2, F);
    _storage_vf_adds.emplace_back(v0, f_reuse); _storage_vf_adds.emplace_back(v0, f_new2);
    _storage_vf_adds.emplace_back(v1, f_reuse); _storage_vf_adds.emplace_back(v1, f_new1);
    _storage_vf_adds.emplace_back(v2, f_new1);  _storage_vf_adds.emplace_back(v2, f_new2);
    _storage_vf_adds.emplace_back(new_v, f_reuse);
    _storage_vf_adds.emplace_back(new_v, f_new1);
    _storage_vf_adds.emplace_back(new_v, f_new2);

    if(_validate_modifications || _update_VV_HE) _VV_HE_apply(_storage_edge_removes, _storage_edge_adds);
    else _relations.invalidate(CacheFlag::VV_HE);

    if(_validate_modifications || _update_EF) _EF_apply(_storage_edge_removes, _storage_edge_adds, _storage_edge_edits);
    else _relations.invalidate(CacheFlag::EF);

    if(_validate_modifications || _update_VF) _VF_apply(_storage_vf_removes, _storage_vf_adds);
    else _relations.invalidate(CacheFlag::VF);

    return new_v;
}





PM_T
void PM::collapse(Index To_Vertex, Index From_Vertex) {
    // bordering faces
    // OLD USING EF
    //auto [f0, f1] = edge_bordering_faces(To_Vertex, From_Vertex);

    Index f0 = T::INDEX_INVALID;
    Index f1 = T::INDEX_INVALID;
    for(Index fid : VF().get_faces(From_Vertex)) {
        auto row = faces()[fid];
        bool has_to = false;
        for(Index v : row) if(v == To_Vertex) { has_to = true; break; }
        if(!has_to) continue;

        if(f0 == T::INDEX_INVALID) f0 = fid;
        else { f1 = fid; break; }   // an edge borders at most 2 faces -- stop once both are found
    }

    // edit instructions for all matrices ( ef, vf, vv_he, ...)
    _storage_edge_removes.clear();
    _storage_edge_adds.clear();
    _storage_vf_removes.clear();
    _storage_vf_adds.clear();
    _storage_faces_to_delete.clear();

    auto& edge_removes    = _storage_edge_removes;
    auto& edge_adds       = _storage_edge_adds;
    auto& vf_removes      = _storage_vf_removes;
    auto& vf_adds         = _storage_vf_adds;
    auto& faces_to_delete = _storage_faces_to_delete;


    // checks if the adjacent faces are trifaces:
    // those faces wont survive, so its a delete
    // else ( polyface ): faces shrink the edge away which requires more editing then just delete
    auto handle_adjacent_face = [&](Index fid) {
        if(fid == T::INDEX_INVALID) return;

        auto row = faces()[fid];
        Index n = row.size();
        Index pos = T::INDEX_INVALID;
        for(Index i = 0; i < n; ++i) {
            if(row[i] == From_Vertex) { 
                pos = i; 
                break; 
            }
        }
        Index prev = row[(pos + n - 1) % n];
        Index next = row[(pos + 1) % n];

        if(n == 3) {
            for(Index i = 0; i < n; ++i) {
                edge_removes.emplace_back(row[i], row[(i + 1) % n], fid);
                vf_removes.emplace_back(row[i], fid);
            }
            faces_to_delete.push_back(fid);
        } else {
            edge_removes.emplace_back(prev, From_Vertex, fid);
            edge_removes.emplace_back(From_Vertex, next, fid);
            edge_adds.emplace_back(prev, next, fid);
            vf_removes.emplace_back(From_Vertex, fid);
        }
    };


    handle_adjacent_face(f0);
    if(f1 != f0) handle_adjacent_face(f1);



    // handle all non adjacent faces
    for(Index fid : VF().get_faces(From_Vertex)) {
        if(fid == f0 || fid == f1) continue;

        auto row = faces()[fid];
        Index n = row.size();
        Index pos = T::INDEX_INVALID;
        for(Index i = 0; i < n; ++i) {
            if(row[i] == From_Vertex) { 
                pos = i; 
                break; 
            }
        }
        Index prev = row[(pos + n - 1) % n];
        Index next = row[(pos + 1) % n];

        edge_removes.emplace_back(prev, From_Vertex, fid);
        edge_removes.emplace_back(From_Vertex, next, fid);
        edge_adds.emplace_back(prev, To_Vertex, fid);
        edge_adds.emplace_back(To_Vertex, next, fid);
        vf_removes.emplace_back(From_Vertex, fid);
        vf_adds.emplace_back(To_Vertex, fid);
    }


    // apply to all matrices
    if(_validate_modifications || _update_VV_HE) _VV_HE_apply(edge_removes, edge_adds);
    else _relations.invalidate(CacheFlag::VV_HE);

    if(_validate_modifications || _update_EF) _EF_apply(edge_removes, edge_adds, {});
    else _relations.invalidate(CacheFlag::EF);

    if(_validate_modifications || _update_VF) _VF_apply(vf_removes, vf_adds);
    else _relations.invalidate(CacheFlag::VF);

    for(Index fid : faces_to_delete) _f.remove(fid);
    _v.remove(From_Vertex);
}
#pragma endregion SPLIT_COLLAPSE_MINIMAL







#pragma region SPLIT_COLLAPSE_OLD_ADD_DELETE
/*
PM_T
PM::Index PM::split(Index F, std::span<const Scalar, 3> P) {
    auto row = faces()[F];
    std::vector<Index> face_verts(row.begin(), row.end());
    Index n = static_cast<Index>(face_verts.size());

    Index new_v = add_vertex(P);
    delete_face(F);   // free Fs edges before the new triangles try to reuse them

    for (Index k = 0; k < n; ++k) {
        Index a = face_verts[k], b = face_verts[(k + 1) % n];
        add_face({new_v, a, b});
    }
    return new_v;
}

PM_T
void PM::collapse(Index To_Vertex, Index From_Vertex) {
    const auto& vf_matrix = VF();
    std::vector<Index> from_faces;
    for(Index fid : vf_matrix.get_faces(From_Vertex)) from_faces.push_back(fid);

    for(Index fid : from_faces) {
        auto row = faces()[fid];
        bool has_to = false;
        for(Index v : row) if(v == To_Vertex) { has_to = true; break; }

        if(has_to) {
            delete_face(fid);
        } else {
            std::vector<Index> new_verts(row.begin(), row.end());
            for(auto& v : new_verts) if(v == From_Vertex) v = To_Vertex;
            delete_face(fid);
            add_face(new_verts);
        }
    }

    delete_vertex(From_Vertex);
}
*/
#pragma endregion SPLIT_COLLAPSE_OLD_ADD_DELETE

};
