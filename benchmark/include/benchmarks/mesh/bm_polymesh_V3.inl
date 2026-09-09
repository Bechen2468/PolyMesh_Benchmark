//#include "benchmarks/mesh/bm_polymesh_V3.h"




template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_iterate_V_raw() {
    typename Mesh::T::Vertices::Vertex p({0, 0, 0});                    // Barycenter point

    for (size_t i = 0; i < mesh.n_vertices(); ++i) {
        //const auto& v = mesh.vertex(i);
        auto v = mesh.vertices()[i];
        p[0] += v[0];       // DEVNOTE: Eigen::Vector.operator+=() blocks simd vectorization, so its sadly slow af ... 
        p[1] += v[1];
        p[2] += v[2];
        //p += mesh.vertex(i);
    }
    
    _benchmark_sink += p[0] + p[1] + p[2];  // volatile write — compiler must emit the loop
    benchmark::DoNotOptimize(p);
}


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_iterate_F_raw() {
    size_t sink = 0;

    for(size_t fi = 0; fi < mesh.n_faces(); ++fi) {
        auto f = mesh.faces()[fi];
        for(size_t v = 0; v < f.size(); ++v) {
            sink += f[v];
        }
    }

    _benchmark_sink += sink;  // volatile write — compiler must emit the loop
    benchmark::DoNotOptimize(sink);
}


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_iterate_V() {
    typename Mesh::T::Vertices::Vertex p({0, 0, 0});                // Barycenter point

    for(auto vi : mesh.vertices()) {
        auto v = vi;
        //auto v = mesh.vertex(*vi);    // *vi == rawindex, but slower with the extra vertex() call which does not return unaligned
        p[0] += v[0];       // DEVNOTE: Eigen::Vector.operator+=() is a templated call, so its sadly slow af ... 
        p[1] += v[1];
        p[2] += v[2];
    }

    p[0] /= mesh.n_vertices();
    p[1] /= mesh.n_vertices();
    p[2] /= mesh.n_vertices();

    _benchmark_sink += p[0] + p[1] + p[2];
    benchmark::DoNotOptimize(p);
}


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_iterate_F() {
    size_t sink = 0;
    size_t iterations = 0;

    for(auto f : mesh.faces()) {
        for(auto fv : f) {
            sink += fv;
            iterations++;
        }
    }

    assert(mesh.n_faces() * 3 == iterations);
    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}

/*

template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_iterate_E() {
    Mesh::EIter ei = mesh.edges_begin();
    Mesh::EIter eend = mesh.edges_end();
    size_t sink = 0;
    for(; ei != eend; ++ei) {
        sink += ei.v0();
        sink += ei.v1();
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_iterate_random_V(const std::vector<size_t>& rand) {
    Mesh::T::Vertex p(0,0,0); 

    for(size_t idx : rand) {
        const auto& v = mesh.vertex(idx);
        p[0] += v[0];
        p[1] += v[1];
        p[2] += v[2];
    }

    _benchmark_sink += (p[0] + p[1] + p[2]) * 1.0000001;
    benchmark::DoNotOptimize(p);
}


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_iterate_random_F(const std::vector<size_t>& rand) {
    size_t sink = 0;

    for(size_t idx : rand) {
        auto f = mesh.face(idx);
        sink += f[0];
        sink += f[1];
        sink += f[2];
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_iterate_random_E(const std::vector<size_t>& rand) {
    size_t sink = 0;
    
    for(size_t idx : rand) {
        size_t fi = idx / 3;
        uint8_t ei = idx % 3;
        auto f = mesh.face(fi);
        sink += f[ei] + f[(ei + 1) % 3];  // only half-edge possible without edgeiterationmatrix
    }
    
    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_adjacent_VV() {
    size_t sink = 0;
    auto vend = mesh.vertices_end();
    
    for(auto vi = mesh.vertices_begin(); vi != vend; ++vi) {
        for(auto vv = mesh.vv_begin(vi._rawId); vv; ++vv) {
            sink += *vv;
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_adjacent_VF() {
    size_t sink = 0;
    Mesh::VIter vi;
    Mesh::VIter vend = mesh.vertices_end();

    for(vi = mesh.vertices_begin(); vi != vend; ++vi) {
        for(auto vf = mesh.vf_begin(*vi); vf; ++vf) {
            sink += *vf;
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_adjacent_FV() {
    size_t sink = 0;
    Mesh::FIter fi;
    Mesh::FIter fend = mesh.faces_end();

    for(fi = mesh.faces_begin(); fi != fend; ++fi) {
        for(auto fv = mesh.fv_begin(*fi); fv != mesh.fv_end(*fi); ++fv) {
            sink += *fv;
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_adjacent_EF() {
    size_t sink = 0;
    Mesh::EIter ei = mesh.edges_begin();
    Mesh::EIter eend = mesh.edges_end();
    
    for(; ei != eend; ++ei) {
        for(auto ef = mesh.ef_begin(*ei); ef; ++ef) {
            sink += *ef;
        }
        sink += *ei;
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}




template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::run_load_all(benchmark::State& state) {
    for (auto _ : state) {
        const std::string fileName = "./mesh_convex_" + std::to_string(state.range(0)) + ".off";
        load_mesh_storage(fileName, true);
    }
}


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::run_iterate_loaded_E(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_E();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::run_iterate_loaded_random_E(benchmark::State& state) {
    for (auto _ : state) {
        _iterate_random_E(_rand_e);
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::run_adjacent_loaded_VV(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_VV();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::run_adjacent_loaded_VF(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_VF();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::run_adjacent_loaded_FV(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_FV();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::run_adjacent_loaded_EF(benchmark::State& state) {
    for (auto _ : state) {
        _adjacent_EF();
        benchmark::DoNotOptimize(_benchmark_sink);
    }
}

*/


template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_convert_to_trimesh() {
    size_t edit_count = 0;

    // presaving facehandles:
    // to not run for the newly added faces again
    std::vector<typename Mesh::T::Index> original_faces(0);
    original_faces.reserve(mesh.n_faces());
    for(auto fIter : mesh.faces()) {
        original_faces.push_back(fIter.idx());
    }


    // edit loop
    for(auto fh : original_faces) {
        if(!mesh.faces().is_valid(fh)) continue;

        // getting all vertices in the face
        auto f = mesh.faces()[fh];

        if(f.size() <= 3) continue; // already a triangle

        std::array<typename Mesh::T::Index, 16> verts; // matches your degree upper bound
        for(typename Mesh::T::Index k = 0; k < f.size(); ++k) verts[k] = f[k];

        // mesh editing
        mesh.delete_face(fh);

        for(size_t i = 1; i + 1 < f.size(); ++i) {
            typename Mesh::T::Index tri[3] = {verts[0], verts[i], verts[i+1]};
            auto new_fh = mesh.add_face(std::span<const typename Mesh::T::Index>(tri, 3));
        }


        edit_count += verts.size();
    }

    _benchmark_sink += (double)edit_count * 1.0000001;
    //std::cout << edit_count << "_";
}







template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_circular() {
    int counter = 0;

    for(auto v : mesh.vertices()) {
        for(auto vf : mesh.VF().get_faces(v.idx())) {
            ++counter;
        }
    }

    for(auto f : mesh.faces()) {
        for(auto fv : f) {
            --counter;
        }
    }

    _benchmark_sink += counter;
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_barycenter() {
    typename Mesh::T::Vertices::Vertex point({0, 0, 0});

    for (auto vit : mesh.vertices()) {
        point[0] += vit[0];
        point[1] += vit[1];
        point[2] += vit[2];
    }
        
    size_t n_v = mesh.n_vertices();
    point[0] /= mesh.n_vertices();
    point[1] /= mesh.n_vertices();
    point[2] /= mesh.n_vertices();

    for (auto vit : mesh.vertices()) {
        vit[0] -= point[0];
        vit[1] -= point[1];
        vit[2] -= point[2];
    }

    _benchmark_sink += point[0] + point[1] + point[2];
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_normal() {
    // face normals: first 3 vertices of each face, same planar assumption PMP uses
    if(fnormals.size() != mesh.n_faces())
        fnormals.resize(mesh.n_faces());

    typename Mesh::Index face_idx = 0;
    for(auto f : mesh.faces()) {
        auto p0 = mesh.vertices()[f[0]];
        auto p1 = mesh.vertices()[f[1]];
        auto p2 = mesh.vertices()[f[2]];

        float e1x = p1[0]-p0[0], e1y = p1[1]-p0[1], e1z = p1[2]-p0[2];
        float e2x = p2[0]-p0[0], e2y = p2[1]-p0[1], e2z = p2[2]-p0[2];

        typename Mesh::T::Vertices::Vertex n = {
            e1y*e2z - e1z*e2y,
            e1z*e2x - e1x*e2z,
            e1x*e2y - e1y*e2x
        };
        float len = std::sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        if (len > 0.0f) { 
            n[0]/=len; 
            n[1]/=len; 
            n[2]/=len; 
        }

        fnormals[face_idx] = n;
    }

    // vertex normals: accumulate incident face normals via cached VF
    if(vnormals.size() != mesh.n_vertices())
        vnormals.resize(mesh.n_vertices());

    const auto& vf_matrix = mesh.VF(); // builds+caches on first call, free after

    for(auto v : mesh.vertices()) {
        typename Mesh::T::Vertices::Vertex n = {0, 0, 0};
        for (auto vf : vf_matrix.get_faces(v.idx()) ) {
            auto& fn = fnormals[vf];
            n[0] += fn[0]; 
            n[1] += fn[1]; 
            n[2] += fn[2];
        }
        float len = std::sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        if (len > 0.0f) { 
            n[0]/=len; 
            n[1]/=len; 
            n[2]/=len; 
        }

        vnormals[v.idx()] = n;
    }
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_smoothing() {
    auto& verts = mesh.vertices();
    const auto& vv_matrix = mesh.VV_HE();

    for (typename Mesh::Index v = 0; v < verts.capacity(); ++v) {
        if (!verts.is_valid(v)) continue;

        typename Mesh::T::Vertex p = {0, 0, 0};
        typename Mesh::Index c = 0;

        for (auto w_id : vv_matrix.get_edges(v)) {
            auto w = verts[w_id];
            p[0] += w[0]; p[1] += w[1]; p[2] += w[2];
            ++c;
        }

        if (c > 0) {
            p[0] /= c; p[1] /= c; p[2] /= c;
            auto pv = verts[v];
            pv[0] = p[0]; pv[1] = p[1]; pv[2] = p[2];
        }
    }
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_subdivision() {
    using Index = typename Mesh::Index;
    
    mesh.set_validate_modifications(false);


    auto& verts = mesh.vertices();
    auto& faces = mesh.faces();

    // 1. snapshot original vertices
    std::vector<Index> original_vertices;
    original_vertices.reserve(mesh.n_vertices());
    for(Index v = 0; v < verts.capacity(); ++v) {
        if(verts.is_valid(v)) original_vertices.push_back(v);
    }

    // 2. snapshot original faces + centroids (read before any mutation)
    std::vector<Index> original_faces;
    original_faces.reserve(mesh.n_faces());
    for(auto f : faces) original_faces.push_back(f.idx());

    std::vector<typename Mesh::T::Vertex> centroids(faces.capacity());
    for(Index fid : original_faces) {
        auto row = faces[fid];
        Index n = row.size();
        typename Mesh::T::Vertex c = {0, 0, 0};
        for(Index vid : row) {
            auto p = verts[vid];
            c[0] += p[0]; c[1] += p[1]; c[2] += p[2];
        }
        c[0] /= n; c[1] /= n; c[2] /= n;
        centroids[fid] = c;
    }

    // 3. new positions of original vertices, via VV_HE
    std::vector<typename Mesh::T::Vertex> new_positions(verts.capacity());
    const auto& vv = mesh.VV_HE();

    for(Index v : original_vertices) {
        Index n = 0;
        typename Mesh::T::Vertex sum = {0, 0, 0};
        for(Index w : vv.get_edges(v)) {
            auto p = verts[w];
            sum[0] += p[0]; sum[1] += p[1]; sum[2] += p[2];
            ++n;
        }
        if(n == 0) { new_positions[v] = {verts[v][0], verts[v][1], verts[v][2]}; continue; }

        float alpha = (4.0f - 2.0f * std::cos(2.0f * float(M_PI) / n)) / 9.0f;
        auto old = verts[v];
        new_positions[v] = {
            (1.0f - alpha) * old[0] + (alpha / n) * sum[0],
            (1.0f - alpha) * old[1] + (alpha / n) * sum[1],
            (1.0f - alpha) * old[2] + (alpha / n) * sum[2]
        };
    }

    // 4. snapshot every edge ONCE via EF, with enough info to build the
    //    final triangles directly -- no split(), no flip(), no transient faces
    struct PendingEdge { Index v0, v1, f_left, f_right; };
    std::vector<PendingEdge> edges;
    edges.reserve(original_faces.size() * 3 / 2); // rough estimate, euler-ish

    const auto& ef = mesh.EF();
    for(Index v0 = 0; v0 < verts.capacity(); ++v0) {
        if(!verts.is_valid(v0)) continue;
        for(const auto& e : ef.get_edges(v0)) {
            edges.push_back({v0, e.v1, e.f_left, e.f_right});
        }
    }

    // 4b. insert centroid vertices now, cache their new vertex IDs per original face
    std::vector<Index> centroid_vid(faces.capacity());
    for(Index fid : original_faces) {
        centroid_vid[fid] = mesh.add_vertex(centroids[fid]);
    }

    // 5. build final triangles directly from cached edge/centroid-vertex data
    for(const auto& e : edges) {
        bool has_left = e.f_left  != Mesh::T::INDEX_INVALID;
        bool has_right = e.f_right != Mesh::T::INDEX_INVALID;

        if(has_left && has_right) {
            // interior edge -> two triangles, matching flip()'s own winding convention
            Index cL = centroid_vid[e.f_left];
            Index cR = centroid_vid[e.f_right];
            mesh.add_face({cL, cR, e.v0});
            mesh.add_face({cR, cL, e.v1});
        } else if(has_left) {
            // boundary edge, owning face traversed it v0 -> v1 (natural/unswapped)
            Index c = centroid_vid[e.f_left];
            mesh.add_face({c, e.v0, e.v1});
        } else if(has_right) {
            // boundary edge, owning face traversed it v1 -> v0 (swapped)
            Index c = centroid_vid[e.f_right];
            mesh.add_face({c, e.v1, e.v0});
        }
        // has neither: shouldn't occur for a valid edge, skip defensively
    }

    // 6. delete original faces, apply new vertex positions
    for(Index fid : original_faces) {
        mesh.delete_face(fid);
    }

    for(Index v : original_vertices) {
        auto row = verts[v];
        row[0] = new_positions[v][0];
        row[1] = new_positions[v][1];
        row[2] = new_positions[v][2];
    }
    
    mesh.compact(false, false);
    //std::cout << "|V|: " << mesh.n_vertices() << "  |F|: " << mesh.n_faces() << std::endl;
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_temp_collapse_vertex(typename Mesh::Index From, typename Mesh::Index To) {
    using Index = typename Mesh::Index;
    const auto& vf_matrix = mesh.VF();
    std::vector<Index> from_faces;
    for(auto vf : vf_matrix.get_faces(From)) {
        from_faces.push_back(vf);
    }

    for(Index fid : from_faces) {
        auto row = mesh.faces()[fid];
        bool has_to = false;
        for(Index v : row) if(v == To) { has_to = true; break; }

        if(has_to) {
            mesh.delete_face(fid);
        } else {
            std::vector<Index> new_verts(row.begin(), row.end());
            for(auto& v : new_verts) if(v == From) v = To;
            mesh.delete_face(fid);
            mesh.add_face(new_verts);
        }
    }

    mesh.delete_vertex(From);
}



template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_collapse() {
    using Index = typename Mesh::Index;  

    mesh.set_validate_modifications(false);
    mesh.set_update_VF(true);
    //mesh.set_validate_modifications(true);
    

    auto& faces = mesh.faces();

    std::vector<Index> orig_face_ids;
    orig_face_ids.reserve(mesh.n_faces());
    for(auto f : faces) orig_face_ids.push_back(f.idx());

    std::vector<std::pair<Index, Index>> to_collapse;
    to_collapse.reserve(orig_face_ids.size());

    for(Index fid : orig_face_ids) {
        auto row = faces[fid];
        Index n = row.size();

        typename Mesh::T::Vertex centroid = {0, 0, 0};
        for(Index vid : row) {
            auto p = mesh.vertices()[vid];
            centroid[0] += p[0]; centroid[1] += p[1]; centroid[2] += p[2];
        }
        centroid[0] /= n; centroid[1] /= n; centroid[2] /= n;

        Index neighbor = row[0];
        Index new_v = mesh.split(fid, centroid);
        to_collapse.emplace_back(new_v, neighbor);
    }

    Index max_v = mesh.vertices().capacity();
    std::vector<std::vector<Index>> incident(max_v);
    {
        const auto& vf_matrix = mesh.VF();
        for(auto v : mesh.vertices()) {
            if(!mesh.vertices().is_valid(v.idx())) continue;
            for(auto vf : vf_matrix.get_faces(v.idx())) {
                incident[v.idx()].push_back(vf);
            }
        }
    }

    std::vector<bool> face_dead(faces.capacity(), false);

    std::vector<std::vector<Index>> to_readd;

    for(auto [from, to] : to_collapse) {
        to_readd.clear();

        for(Index fid : incident[from]) {
            if(face_dead[fid]) continue;

            auto row = mesh.faces()[fid];
            bool has_to = false;
            for(Index v : row) if(v == to) { has_to = true; break; }

            std::vector<Index> new_verts(row.begin(), row.end());
            for(auto& v : new_verts) if(v == from) v = to;

            mesh.delete_face(fid);
            face_dead[fid] = true;

            if(!has_to) {
                to_readd.push_back(std::move(new_verts));
            }
        }

        for(auto& verts : to_readd) {
            Index new_fid = mesh.add_face(verts);
            incident[to].push_back(new_fid);
        }

        mesh.delete_vertex(from);
    }

    mesh.compact(false, false);
    //std::cout << "|V|: " << mesh.n_vertices() << "  |F|: " << mesh.n_faces() << std::endl;
}









template<PolyMesh::V3::TraitsC TraitsT>
void Benchmark_PolyMesh_V03<TraitsT>::_render_export(std::vector<float>& out_positions, std::vector<int>& out_indices) {
    using Index = typename Mesh::Index;  

    out_positions.resize(mesh.n_vertices() * 3);
    Index i = 0;
    for(auto v : mesh.vertices()) {
        out_positions[i*3+0] = v[0]; out_positions[i*3+1] = v[1]; out_positions[i*3+2] = v[2];
        ++i;
    }

    Index total_deg = 0;
    for(auto f : mesh.faces()) total_deg += f.size();
    out_indices.resize(total_deg);

    Index j = 0;
    for(auto f : mesh.faces())
        for(Index vid : f) out_indices[j++] = vid;
}