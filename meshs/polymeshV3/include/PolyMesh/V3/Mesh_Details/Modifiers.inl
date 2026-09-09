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
    Index v0 = P0, v1 = P1;
    if (v0 > v1) std::swap(v0, v1);
    const auto& ef = EF();
    if (!ef.edge_exists(v0, v1)) return {T::INDEX_INVALID, T::INDEX_INVALID};
    auto e = ef.get_edge(v0, v1);
    return {e.f_left, e.f_right};
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

};
