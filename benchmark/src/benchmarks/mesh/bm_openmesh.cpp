#include "benchmarks/mesh/bm_openmesh.h"


void Benchmark_OpenMesh_V11::_iterate_V() {
    Mesh::VIter vi;                          // Vertex index
    Mesh::VIter vend = mesh.vertices_end();  // Vertex index bounds
    OpenMesh::Vec3d p(0,0,0);                // Barycenter point

    for(vi = mesh.vertices_begin(); vi != vend; ++vi) {
        p += mesh.point(*vi);
    }

    p /= mesh.n_vertices();
    _benchmark_sink += p[0] + p[1] + p[2];
    benchmark::DoNotOptimize(p);
}


void Benchmark_OpenMesh_V11::_iterate_F() {
    Mesh::FIter fi;
    Mesh::FIter fend = mesh.faces_end();
    size_t sink = 0;

    for(fi = mesh.faces_begin(); fi != fend; ++fi) {
        for(Mesh::FVIter fv = mesh.fv_iter(*fi); fv.is_valid(); ++fv) {
            sink += fv->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_OpenMesh_V11::_iterate_E() {
    Mesh::EIter ei;
    Mesh::EIter eend = mesh.edges_end();
    size_t sink = 0;

    for(ei = mesh.edges_begin(); ei != eend; ++ei) {
        for(Mesh::EVIter ev = mesh.ev_iter(*ei); ev.is_valid(); ++ev) {
            sink += ev->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}



void Benchmark_OpenMesh_V11::_iterate_random_V(const std::vector<size_t>& rand) {
    OpenMesh::Vec3d p(0,0,0);

    for(size_t vi : rand) {
        auto vh = Mesh::VertexHandle(vi);
        p += mesh.point(vh);
    }

    _benchmark_sink += (p[0] + p[1] + p[2]) * 1.0000001;
    benchmark::DoNotOptimize(p);
}


void Benchmark_OpenMesh_V11::_iterate_random_F(const std::vector<size_t>& rand) {
    size_t sink = 0;

    for(size_t idx : rand) {
        auto fh = Mesh::FaceHandle(idx);
        for(Mesh::FVIter fv = mesh.fv_iter(fh); fv.is_valid(); ++fv) {
            sink += fv->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_OpenMesh_V11::_iterate_random_E(const std::vector<size_t>& rand) {
    size_t sink = 0;

    for(size_t idx : rand) {
        auto eh = Mesh::EdgeHandle(idx);
        for(Mesh::EVIter ev = mesh.ev_iter(eh); ev.is_valid(); ++ev) {
            sink += ev->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}



void Benchmark_OpenMesh_V11::_adjacent_VV() {
    size_t sink = 0;

    for(auto v : mesh.vertices()) {
        for(Mesh::VVIter vv = mesh.vv_iter(v); vv.is_valid(); ++vv) {
            sink += vv->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_OpenMesh_V11::_adjacent_VF() {
    size_t sink = 0;

    for(auto v : mesh.vertices()) {
        for(Mesh::VFIter vf = mesh.vf_iter(v); vf.is_valid(); ++vf) {
            sink += vf->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_OpenMesh_V11::_adjacent_FV() {
    size_t sink = 0;

    for(auto f : mesh.faces()) {
        for(Mesh::FVIter fv = mesh.fv_iter(f); fv.is_valid(); ++fv) {
            sink += fv->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_OpenMesh_V11::_adjacent_EF() {
    size_t sink = 0;

    for(auto e : mesh.edges()) {
        for(Mesh::EFIter ef = mesh.ef_iter(e); ef.is_valid(); ++ef) {
            sink += ef->idx();
        }
    }

    _benchmark_sink += (double)sink * 1.0000001;
    benchmark::DoNotOptimize(sink);
}






void Benchmark_OpenMesh_V11::_convert_to_trimesh() {
    size_t edit_count = 0;

    // presaving facehandles:
    // to not run for the newly added faces again
    std::vector<OpenMesh::FaceHandle> original_faces;
    original_faces.reserve(mesh.n_faces());
    for (auto fh : mesh.faces()) {
        original_faces.push_back(fh);
    }


    // edit loop
    for (auto fh : original_faces) {
        if(!fh.is_valid() || mesh.status(fh).deleted()) continue;

        // getting all vertices in the face
        std::vector<OpenMesh::VertexHandle> verts;
        verts.reserve(8);
        for (auto vh : mesh.fv_range(fh)){
            verts.push_back(vh);
        }

        if(verts.size() <= 3) continue; // already a triangle

        // mesh editing
        mesh.delete_face(fh);

        for (size_t i = 1; i + 1 < verts.size(); ++i) {
            auto new_fh = mesh.add_face(verts[0], verts[i], verts[i + 1]);
        }
        edit_count += verts.size();
    }

    //mesh.garbage_collection();
    _benchmark_sink += (double)edit_count * 1.0000001;
    //std::cout << edit_count << "_";
}






void Benchmark_OpenMesh_V11::_circular() {
    Mesh::VIter vit, vend=mesh.vertices_end();
    Mesh::FIter fit, fend=mesh.faces_end();
    int counter = 0;

    for (auto vit : mesh.vertices())
        for (auto vfit=mesh.vf_iter(vit); vfit; ++vfit)
            ++counter;

    for (fit=mesh.faces_begin(); fit!=fend; ++fit)
        for (Mesh::FVIter fvit=mesh.fv_iter(fit); fvit; ++fvit)
            --counter;

    _benchmark_sink += counter;
}



void Benchmark_OpenMesh_V11::_barycenter() {
    Mesh::VIter vit, vend=mesh.vertices_end();
    Mesh::Point p(0,0,0);

    for (vit=mesh.vertices_begin(); vit!=vend; ++vit)
        p += mesh.point(vit);

    p /= mesh.n_vertices();

    for (vit=mesh.vertices_begin(); vit!=vend; ++vit)
        mesh.point(vit) -= p;

    _benchmark_sink += p[0] + p[1] + p[2];
}



void Benchmark_OpenMesh_V11::_normal() {
    Mesh::VIter vit, vend=mesh.vertices_end();
    Mesh::FIter fit, fend=mesh.faces_end();
    Mesh::VFIter vfit;

    for (fit=mesh.faces_begin(); fit!=fend; ++fit)
    {
        Mesh::HalfedgeHandle h = mesh.halfedge_handle(fit);
        Mesh::Point p0 = mesh.point(mesh.to_vertex_handle(h));
        h = mesh.next_halfedge_handle(h);
        Mesh::Point p1 = mesh.point(mesh.to_vertex_handle(h));
        h = mesh.next_halfedge_handle(h);
        Mesh::Point p2 = mesh.point(mesh.to_vertex_handle(h));
        mesh.set_normal(fit, ((p2-=p1)%(p0-=p1)).normalize());
    }

    for (vit=mesh.vertices_begin(); vit!=vend; ++vit)
    {
        Mesh::Point n(0,0,0);
        for (vfit=mesh.vf_iter(vit); vfit; ++vfit)
            n += mesh.normal(vfit);
        mesh.set_normal(vit, n.normalize());
    }
}



void Benchmark_OpenMesh_V11::_smoothing() {
    Mesh::VIter vit, vend=mesh.vertices_end();

    for (vit=mesh.vertices_begin(); vit!=vend; ++vit)
    {
        if (!mesh.is_boundary(vit))
        {
            Mesh::Point  p(0,0,0);
            Mesh::Scalar c(0);
            for (Mesh::VVIter vvit = mesh.vv_iter(vit); vvit; ++vvit)
            {
                p += mesh.point(vvit);
                ++c;
            }
            p /= c;
            mesh.point(vit) = p;
        }
    }
}



void Benchmark_OpenMesh_V11::_subdivision() {
    // reserve memory
    int nv = mesh.n_vertices();
    int ne = mesh.n_edges();
    int nf = mesh.n_faces();
    mesh.reserve(nv+nf, ne+3*nf, 3*nf);


    // iterators
    Mesh::VIter vit, vend=mesh.vertices_end();
    Mesh::FIter fit, fend=mesh.faces_end();
    Mesh::EIter eit, eend=mesh.edges_end();


    // compute new positions of old vertices
    OpenMesh::VPropHandleT<Mesh::Point> new_pos;
    mesh.add_property(new_pos);
    for (vit=mesh.vertices_begin(); vit!=vend; ++vit)
    {
        if (!mesh.is_boundary(vit))
        {
            Mesh::Scalar n = mesh.valence(vit);
            Mesh::Scalar alpha = (4.0 - 2.0*cos(2.0*M_PI/n)) / 9.0;
            Mesh::Point  p(0,0,0);
            for (Mesh::VVIter vvit = mesh.vv_iter(vit); vvit; ++vvit)
                p += mesh.point(vvit);
            p = (1.0f-alpha)*mesh.point(vit) + alpha/n*p;
            mesh.property(new_pos, vit) = p;
        }
    }


    // split faces
    for (fit=mesh.faces_begin(); fit!=fend; ++fit)
    {
        Mesh::Point  p(0,0,0);
        Mesh::Scalar c(0);
        for (Mesh::FVIter fvit=mesh.fv_iter(fit); fvit; ++fvit)
        {
            p += mesh.point(fvit);
            ++c;
        }
        p /= c;

        mesh.split(fit, p);
    }


    // set new positions of old vertices
    for (vit=mesh.vertices_begin(); vit!=vend; ++vit)
        if (!mesh.is_boundary(vit))
            mesh.point(vit) = mesh.property(new_pos, vit);
    mesh.remove_property(new_pos);


    // flip old edges
    for (eit=mesh.edges_begin(); eit!=eend; ++eit)
        if (mesh.is_flip_ok(eit))
            mesh.flip(eit);

    mesh.garbage_collection();

    //std::cout << "|V|: " << mesh.n_vertices() << "  |F|: " << mesh.n_faces() << std::endl;
}



void Benchmark_OpenMesh_V11::_collapse() {
    // reserve memory
    int nv = mesh.n_vertices();
    int ne = mesh.n_edges();
    int nf = mesh.n_faces();
    mesh.reserve(nv+nf, ne+3*nf, 3*nf);


    // iterators
    Mesh::VIter vit, vend=mesh.vertices_end();
    Mesh::FIter fit, fend=mesh.faces_end();


    // split faces
    Mesh::Point p(0,0,0);
    for (fit=mesh.faces_begin(); fit!=fend; ++fit)
        mesh.split(fit, p);


    // collapse new edges
    vit = vend; vend=mesh.vertices_end();
    for (; vit!=vend; ++vit)
        mesh.collapse(mesh.halfedge_handle(vit));


    // remove deleted items
    mesh.garbage_collection();
    
    //std::cout << "|V|: " << mesh.n_vertices() << "  |F|: " << mesh.n_faces() << std::endl;
}




void  Benchmark_OpenMesh_V11::_render_export(std::vector<float>& out_positions, std::vector<int>& out_indices) {
    out_positions.resize(mesh.n_vertices() * 3);
    for (auto vh : mesh.vertices()) {
        auto p = mesh.point(vh);
        out_positions[vh.idx()*3+0] = p[0]; out_positions[vh.idx()*3+1] = p[1]; out_positions[vh.idx()*3+2] = p[2];
    }
    out_indices.clear();
    for (auto fh : mesh.faces())
        for (auto fv_it = mesh.fv_iter(fh); fv_it.is_valid(); ++fv_it)
            out_indices.push_back(fv_it->idx());
}