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
    if(_update_EF) _relations.EF.set_largest_vertex_idx(idx + 1);
    else _relations.invalidate(CacheFlag::EF);

    if(_update_VV_HE) _relations.VV_HE.set_largest_vertex_idx(idx + 1);
    else _relations.invalidate(CacheFlag::VV_HE);

    if(_validate_modifications || _update_VF) _relations.VF.set_largest_vertex_idx(idx + 1);
    else _relations.invalidate(CacheFlag::VF);

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
        thread_local std::vector<Index> incident;   // reused, no allocation after first call
        auto incident_view = VF().get_faces(Idx);
        incident.assign(incident_view.begin(), incident_view.end());

        for (Index f_idx : incident) delete_face(f_idx);
    }

    _v.remove(Idx);
}



PM_T
PM::Index PM::add_face(std::span<const Index> Face) {
    if(_validate_modifications && !is_face_addable(Face)) return T::INDEX_INVALID;

    Index f_idx = _f.add(Face);

    if(_update_EF) _EF_add_face(Face, f_idx);
    else _relations.invalidate(CacheFlag::EF);

    if(_update_VV_HE) _VV_HE_add_face(Face, f_idx);
    else _relations.invalidate(CacheFlag::VV_HE);

    if(_validate_modifications || _update_VF) _VF_add_face(Face, f_idx);
    else _relations.invalidate(CacheFlag::VF);

    return f_idx;
}


PM_T
PM::Index PM::add_face(std::initializer_list<Index> Face) {
    return add_face(std::span<const Index>{Face.begin(), Face.size()});
}


PM_T
void PM::delete_face(Index Idx) {
    auto face = faces()[Idx];

    if(_update_EF) _EF_remove_face(face, Idx);
    else _relations.invalidate(CacheFlag::EF);

    if(_update_VV_HE) _VV_HE_remove_face(face, Idx);
    else _relations.invalidate(CacheFlag::VV_HE);

    if(_validate_modifications || _update_VF) _VF_remove_face(face, Idx);
    else _relations.invalidate(CacheFlag::VF);

    _f.remove(Idx);
}



PM_T
[[nodiscard]] bool PM::is_face_addable(std::span<const Index> Face) {
    // -------------------------------------------------------------------------
    // Init
    // -------------------------------------------------------------------------
    const size_t n = Face.size();
    if(n < 3) return false; // Minimal face must be a triangle

    const typename T::VF& vf = VF();

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
    // CHECK 2 + 3, one pass per vertex of the new face
    // outs: vertices v points to (v -> w exists), ins: vertices pointing to v (w -> v exists),
    // both collected from the faces around v
    // -------------------------------------------------------------------------
    thread_local std::vector<Index> outs, ins;   // no allocation after the first call

    for(size_t i = 0; i < n; ++i) {
        const Index v_curr = Face[i];
        const Index v_next = Face[(i + 1) % n];

        outs.clear(); ins.clear();
        for(Index f : vf.get_faces(v_curr)) {
            if(f == T::INDEX_INVALID || !_f.is_valid(f)) continue;

            auto face = _f[f];
            const size_t m = face.size();
            for(size_t k = 0; k < m; ++k) {
                if(face[k] == v_curr) {
                    ins.push_back(face[(k + m - 1) % m]);
                    outs.push_back(face[(k + 1) % m]);
                    break;
                }
            }
        }
        if(outs.empty()) continue; // isolated vertex, nothing to check

        // CHECK 2: Directed Edge Saturation
        // the new face uses v_curr -> v_next, which must not exist yet
        if(std::find(outs.begin(), outs.end(), v_next) != outs.end()) return false;

        // CHECK 3: Vertex Star
        // outgoing edge v -> w is boundary if w -> v does not exist;
        // a closed star (no boundary edge left) can't take another face
        bool has_boundary = false;
        for(Index w : outs) {
            if(std::find(ins.begin(), ins.end(), w) == ins.end()) { has_boundary = true; break; }
        }
        if(!has_boundary) return false;
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

    if(_update_VV_HE) _VV_HE_apply(_storage_edge_removes, _storage_edge_adds);
    else _relations.invalidate(CacheFlag::VV_HE);

    if(_update_EF) _EF_apply(_storage_edge_removes, _storage_edge_adds, _storage_edge_edits);
    else _relations.invalidate(CacheFlag::EF);

    if(_validate_modifications || _update_VF) _VF_apply(_storage_vf_removes, _storage_vf_adds);
    else _relations.invalidate(CacheFlag::VF);

    return new_v;
}



PM_T
[[nodiscard]] bool PM::is_collapse_ok(Index To_Vertex, Index From_Vertex) {
    constexpr Index INVALID = T::INDEX_INVALID;
 
    // CHECK 1a: same or deleted vertices
    if(To_Vertex == From_Vertex) return false;
    if(!_v.is_valid(To_Vertex) || !_v.is_valid(From_Vertex)) return false;
 
    const typename T::VF& vf = VF();
 
    auto contains = [](const std::vector<Index>& vec, Index x) {
        return std::find(vec.begin(), vec.end(), x) != vec.end();
    };
    auto face_contains = [](auto face, Index x) {
        for(Index v : face) if(v == x) return true;
        return false;
    };
 
    // -------------------------------------------------------------------------
    // Find the two faces at the edge and their opposite vertices
    // f_ft: face with From -> To,  f_tf: face with To -> From
    // o_ft / o_tf: third vertex if that face is a triangle
    // -------------------------------------------------------------------------
    Index f_ft = INVALID, f_tf = INVALID;
    Index o_ft = INVALID, o_tf = INVALID;
 
    for(Index f : vf.get_faces(From_Vertex)) {
        if(f == INVALID || !_f.is_valid(f)) continue;
 
        auto face = _f[f];
        const size_t m = face.size();
        size_t pos = 0;
        while(face[pos] != From_Vertex) ++pos;
 
        const Index prev = face[(pos + m - 1) % m];
        const Index next = face[(pos + 1) % m];
 
        if(next == To_Vertex) {
            f_ft = f;
            if(m == 3) o_ft = prev;
        }
        else if(prev == To_Vertex) {
            f_tf = f;
            if(m == 3) o_tf = next;
        }
        // CHECK 2: both in one face but not next to each other
        else if(face_contains(face, To_Vertex)) return false;
    }
 
    // CHECK 1b: no edge between them
    if(f_ft == INVALID && f_tf == INVALID) return false;
 
    // CHECK 3: both triangles have the same opposite vertex
    if(o_ft != INVALID && o_ft == o_tf) return false;
 
    // -------------------------------------------------------------------------
    // Neighbours of both vertices
    // outs: v -> w exists, ins: w -> v exists
    // -------------------------------------------------------------------------
    thread_local std::vector<Index> from_out, from_in, to_out, to_in;   // no allocation after first call
 
    auto collect = [&](Index v, std::vector<Index>& outs, std::vector<Index>& ins) {
        outs.clear(); ins.clear();
        for(Index f : vf.get_faces(v)) {
            if(f == INVALID || !_f.is_valid(f)) continue;
 
            auto face = _f[f];
            const size_t m = face.size();
            for(size_t k = 0; k < m; ++k) {
                if(face[k] == v) {
                    ins.push_back(face[(k + m - 1) % m]);
                    outs.push_back(face[(k + 1) % m]);
                    break;
                }
            }
        }
    };
    collect(From_Vertex, from_out, from_in);
    collect(To_Vertex,   to_out,   to_in);
 
    // a vertex is on the border if one of its outgoing edges has no way back
    auto is_border_vertex = [&](const std::vector<Index>& outs, const std::vector<Index>& ins) {
        for(Index w : outs) if(!contains(ins, w)) return true;
        return false;
    };
 
    // CHECK 4: two border vertices, but the edge between them is not a border
    const bool edge_is_border = (f_ft == INVALID || f_tf == INVALID);
    if(!edge_is_border
       && is_border_vertex(from_out, from_in)
       && is_border_vertex(to_out, to_in)) return false;
 
    // CHECK 5: triangle at the edge with both other edges on the border
    // f_ft = (From, To, o): edges To -> o and o -> From, border if o -> To and From -> o are missing
    if(o_ft != INVALID && !contains(to_in, o_ft) && !contains(from_out, o_ft)) return false;
    // f_tf = (To, From, o): edges From -> o and o -> To, border if o -> From and To -> o are missing
    if(o_tf != INVALID && !contains(from_in, o_tf) && !contains(to_out, o_tf)) return false;
 
    // CHECK 6: link condition
    auto is_neighbour_of_to = [&](Index w) { return contains(to_out, w) || contains(to_in, w); };
    auto allowed = [&](Index w) { return w == To_Vertex || w == o_ft || w == o_tf; };
 
    for(Index w : from_out) if(!allowed(w) && is_neighbour_of_to(w)) return false;
    for(Index w : from_in)  if(!allowed(w) && is_neighbour_of_to(w)) return false;
 
    // CHECK 7: duplicate face after the collapse (only possible with both opposite vertices)
    if(o_ft != INVALID && o_tf != INVALID) {
        auto has_face_with_both = [&](Index v) {
            for(Index f : vf.get_faces(v)) {
                if(f == INVALID || f == f_ft || f == f_tf || !_f.is_valid(f)) continue;
                auto face = _f[f];
                if(face_contains(face, o_ft) && face_contains(face, o_tf)) return true;
            }
            return false;
        };
        if(has_face_with_both(From_Vertex) && has_face_with_both(To_Vertex)) return false;
    }
 
    return true;
}





PM_T
bool PM::collapse(Index To_Vertex, Index From_Vertex) {
    constexpr Index INVALID = T::INDEX_INVALID;
 
    if(_validate_modifications && !is_collapse_ok(To_Vertex, From_Vertex)) return false;
 
    // bordering faces
    Index f0 = INVALID;
    Index f1 = INVALID;
    for(Index fid : VF().get_faces(From_Vertex)) {
        if(fid == INVALID || !_f.is_valid(fid)) continue;
 
        auto row = faces()[fid];
        bool has_to = false;
        for(Index v : row) if(v == To_Vertex) { has_to = true; break; }
        if(!has_to) continue;
 
        if(f0 == INVALID) f0 = fid;
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
 
    // polyfaces at the edge that have to be rebuilt without From_Vertex
    thread_local std::vector<Index> poly_old_ids;
    thread_local std::vector<std::vector<Index>> poly_new_rows;
    poly_old_ids.clear();
    poly_new_rows.clear();
 
 
    // trifaces at the edge wont survive, so its a delete
    // polyfaces only lose From_Vertex
    auto handle_adjacent_face = [&](Index fid) {
        if(fid == INVALID) return;
 
        auto row = faces()[fid];
        const Index n = row.size();
 
        // the old face is removed from all relations in both cases
        for(Index i = 0; i < n; ++i) {
            edge_removes.emplace_back(row[i], row[(i + 1) % n], fid);
            vf_removes.emplace_back(row[i], fid);
        }
        faces_to_delete.push_back(fid);
 
        if(n == 3) return;
 
        poly_old_ids.push_back(fid);
        auto& new_row = poly_new_rows.emplace_back();
        new_row.reserve(n - 1);
        for(Index v : row) if(v != From_Vertex) new_row.push_back(v);
    };
 
    handle_adjacent_face(f0);
    if(f1 != f0) handle_adjacent_face(f1);
 
 
    // handle all non adjacent faces: From_Vertex is replaced by To_Vertex
    for(Index fid : VF().get_faces(From_Vertex)) {
        if(fid == f0 || fid == f1) continue;
        if(fid == INVALID || !_f.is_valid(fid)) continue;
 
        auto row = _f[fid];
        const Index n = row.size();
        Index pos = 0;
        while(row[pos] != From_Vertex) ++pos;
 
        const Index prev = row[(pos + n - 1) % n];
        const Index next = row[(pos + 1) % n];
 
        row[pos] = To_Vertex;   // <- was missing
 
        edge_removes.emplace_back(prev, From_Vertex, fid);
        edge_removes.emplace_back(From_Vertex, next, fid);
        edge_adds.emplace_back(prev, To_Vertex, fid);
        edge_adds.emplace_back(To_Vertex, next, fid);
        vf_removes.emplace_back(From_Vertex, fid);
        vf_adds.emplace_back(To_Vertex, fid);
    }
 
 
    // add the shrunk polyfaces again (after the loop above, as adding can move the face storage)
    for(auto& new_row : poly_new_rows) {
        const Index new_fid = _f.add(std::span<const Index>(new_row));
        const Index m = new_row.size();
        for(Index i = 0; i < m; ++i) {
            edge_adds.emplace_back(new_row[i], new_row[(i + 1) % m], new_fid);
            vf_adds.emplace_back(new_row[i], new_fid);
        }
    }
 
 
    // apply to all matrices
    if(_update_VV_HE) _VV_HE_apply(edge_removes, edge_adds);
    else _relations.invalidate(CacheFlag::VV_HE);
 
    if(_update_EF) _EF_apply(edge_removes, edge_adds, {});
    else _relations.invalidate(CacheFlag::EF);
 
    if(_validate_modifications || _update_VF) _VF_apply(vf_removes, vf_adds);
    else _relations.invalidate(CacheFlag::VF);
 
    for(Index fid : faces_to_delete) _f.remove(fid);
    _v.remove(From_Vertex);
 
    return true;
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
