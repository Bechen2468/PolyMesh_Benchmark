#include "benchmarks/mesh/bm_pmp.h"


void Benchmark_Pmp_Lib__V03::_iterate_V() {
    pmp::Point p(0.0, 0.0, 0.0);

    for (auto v : mesh.vertices()) {
        //p += mesh.position(v);    // causes heap buffer overflow with double precision
        auto& pv = mesh.position(v);
        p[0] += pv[0];
        p[1] += pv[1];
        p[2] += pv[2];
    }

    p /= static_cast<pmp::Scalar>(mesh.n_vertices());

    _benchmark_sink += p[0] + p[1] + p[2];
    benchmark::DoNotOptimize(p);
}


void Benchmark_Pmp_Lib__V03::_iterate_F() {
    size_t sink = 0;

    for (auto f : mesh.faces()) {
        for (auto v : mesh.vertices(f)) {
            sink += static_cast<size_t>(v.idx());
        }
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_iterate_E() {
    size_t sink = 0;

    for (auto e : mesh.edges()) {
        auto h0 = mesh.halfedge(e, 0);
        auto h1 = mesh.halfedge(e, 1);

        sink += static_cast<size_t>(mesh.to_vertex(h0).idx());
        sink += static_cast<size_t>(mesh.to_vertex(h1).idx());
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_iterate_random_V(const std::vector<size_t>& rand) {
    pmp::Point p(0.0, 0.0, 0.0);

    for (size_t vi : rand) {
        auto vh = pmp::Vertex(static_cast<pmp::IndexType>(vi));
        p += mesh.position(vh);
    }

    _benchmark_sink += (p[0] + p[1] + p[2]) * 1.0000001;
    benchmark::DoNotOptimize(p);
}


void Benchmark_Pmp_Lib__V03::_iterate_random_F(const std::vector<size_t>& rand) {
    size_t sink = 0;

    for (size_t idx : rand) {
        auto fh = pmp::Face(static_cast<pmp::IndexType>(idx));
        for (auto v : mesh.vertices(fh)) {
            sink += static_cast<size_t>(v.idx());
        }
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_iterate_random_E(const std::vector<size_t>& rand) {
    size_t sink = 0;

    for (size_t idx : rand) {
        auto eh = pmp::Edge(static_cast<unsigned int>(idx));

        auto h0 = mesh.halfedge(eh, 0);
        auto h1 = mesh.halfedge(eh, 1);

        sink += static_cast<size_t>(mesh.to_vertex(h0).idx());
        sink += static_cast<size_t>(mesh.to_vertex(h1).idx());
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_adjacent_VV() {
    size_t sink = 0;

    for (auto v : mesh.vertices()) {
        for (auto vv : mesh.vertices(v)) {
            sink += static_cast<size_t>(vv.idx());
        }
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_adjacent_VF() {
    size_t sink = 0;

    for (auto v : mesh.vertices()) {
        for (auto f : mesh.faces(v)) {
            sink += static_cast<size_t>(f.idx());
        }
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_adjacent_FV() {
    size_t sink = 0;

    for (auto f : mesh.faces()) {
        for (auto v : mesh.vertices(f)) {
            sink += static_cast<size_t>(v.idx());
        }
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}


void Benchmark_Pmp_Lib__V03::_adjacent_EF() {
    size_t sink = 0;

    for (auto e : mesh.edges()) {
        auto h0 = mesh.halfedge(e, 0);
        auto h1 = mesh.halfedge(e, 1);

        auto f0 = mesh.face(h0);
        auto f1 = mesh.face(h1);

        if (f0.is_valid()) sink += static_cast<size_t>(f0.idx());
        if (f1.is_valid()) sink += static_cast<size_t>(f1.idx());
    }

    _benchmark_sink += static_cast<double>(sink) * 1.0000001;
    benchmark::DoNotOptimize(sink);
}



void Benchmark_Pmp_Lib__V03::_convert_to_trimesh() {
    size_t edit_count = 0;

    // presaving face handles:
    // to not run for the newly added faces again
    std::vector<pmp::Face> original_faces;
    original_faces.reserve(mesh.n_faces());
    for (auto fh : mesh.faces()) {
        original_faces.push_back(fh);
    }

    // edit loop
    for (auto fh : original_faces) {
        if (!mesh.is_valid(fh) || mesh.is_deleted(fh)) continue;

        // getting all vertices in the face
        std::array<pmp::Vertex, 16> verts; // matches your degree upper bound
        size_t vcount = 0;
        for (auto vh : mesh.vertices(fh))
            verts[vcount++] = vh;

        if (vcount <= 3) continue; // already a triangle

        // mesh editing
        mesh.delete_face(fh);

        for (size_t i = 1; i + 1 < vcount; ++i) {
            auto new_fh = mesh.add_triangle(verts[0], verts[i], verts[i + 1]);
        }

        edit_count += vcount;
    }

    _benchmark_sink += (double)edit_count * 1.0000001;
}















void Benchmark_Pmp_Lib__V03::_circular() {
    int counter = 0;

    for (auto vit : mesh.vertices())
        for (auto vfit : mesh.faces(vit))
            ++counter;

    for (auto fit : mesh.faces())
        for (auto v : mesh.vertices(fit))
            --counter;

    _benchmark_sink += counter;
}



void Benchmark_Pmp_Lib__V03::_barycenter() {
    pmp::Point p(0,0,0);

    for (auto vit : mesh.vertices())
        p += mesh.position(vit);

    p /= mesh.n_vertices();

    for (auto vit : mesh.vertices())
        mesh.position(vit) -= p;

    _benchmark_sink += p[0] + p[1] + p[2];
}



void Benchmark_Pmp_Lib__V03::_normal() {
    Mesh::VertexIterator vit, vend=mesh.vertices_end();
    Mesh::FaceIterator   fit, fend=mesh.faces_end();
    Mesh::FaceAroundVertexCirculator vfit, vfend;

    for (auto fit : mesh.faces())
    {
        pmp::Halfedge h = mesh.halfedge(fit);
        pmp::Point p0 = points[mesh.to_vertex(h)];
        h = mesh.next_halfedge(h);
        pmp::Point p1 = points[mesh.to_vertex(h)];
        p1 -= p0;
        h = mesh.next_halfedge(h);
        pmp::Point p2 = points[mesh.to_vertex(h)];
        p2 -= p0;
        fnormals[fit] = cross(p1, p2);
        fnormals[fit].normalize();
    }

    for (auto vit : mesh.vertices())
    {
        pmp::Point n(0,0,0);
        vfit = vfend = mesh.faces(vit);
        if (vfit) do
        {
            n += fnormals[*vfit];
        }
        while (++vfit != vfend);
        n.normalize();
        vnormals[vit] = n;
    }
}



void Benchmark_Pmp_Lib__V03::_smoothing() {
    Mesh::VertexIterator vit, vend=mesh.vertices_end();
    Mesh::VertexAroundVertexCirculator vvit, vvend;

    for (auto vit : mesh.vertices())
    {
        if (!mesh.is_boundary(vit))
        {
            pmp::Point  p(0,0,0);
            pmp::Scalar c(0);
            vvit = vvend = mesh.vertices(vit);
            do
            {
                p += points[*vvit];
                ++c;
            }
            while (++vvit != vvend);
            p /= c;
            points[vit] = p;
        }
    }
}



void Benchmark_Pmp_Lib__V03::_subdivision() {
    // reserve memory
        int nv = mesh.n_vertices();
        int ne = mesh.n_edges();
        int nf = mesh.n_faces();
        mesh.reserve(nv+nf, ne+3*nf, 3*nf);


        // iterators
        Mesh::VertexIterator vit, vend=mesh.vertices_end();
        Mesh::FaceIterator fit, fend=mesh.faces_end();
        Mesh::EdgeIterator eit, eend=mesh.edges_end();


        // compute new positions of old vertices
        pmp::VertexProperty<pmp::Point> new_pos = mesh.add_vertex_property<pmp::Point>("v:np");
        for (auto vit : mesh.vertices())
        {
            if (!mesh.is_boundary(vit))
            {
                pmp::Scalar n = mesh.valence(vit);
                pmp::Scalar alpha = (4.0 - 2.0*cos(2.0*M_PI/n)) / 9.0;
                pmp::Point  p(0,0,0);
                Mesh::VertexAroundVertexCirculator vvit=mesh.vertices(vit), vvend=vvit;
                do
                {
                    p += points[*vvit];
                }
                while (++vvit != vvend);
                p = (1.0f-alpha)*points[vit] + alpha/n*p;
                new_pos[vit] = p;
            }
        }


        // split faces
        for (auto fit : mesh.faces())
        {
            pmp::Point  p(0,0,0);
            pmp::Scalar c(0);
            Mesh::VertexAroundFaceCirculator fvit = mesh.vertices(fit), fvend=fvit;
            do
            {
                p += points[*fvit];
                ++c;
            }
            while (++fvit!=fvend);
            p /= c;

            mesh.split(fit, p);
        }


        // set new positions of old vertices
        for (auto vit : mesh.vertices())
            if (!mesh.is_boundary(vit))
                points[vit] = new_pos[vit];
        mesh.remove_vertex_property(new_pos);


        // flip old edges
        for (auto eit : mesh.edges())
            if (mesh.is_flip_ok(eit))
                mesh.flip(eit);
}



void Benchmark_Pmp_Lib__V03::_collapse() {
    // reserve memory
    int nv = mesh.n_vertices();
    int ne = mesh.n_edges();
    int nf = mesh.n_faces();
    mesh.reserve(nv+nf, ne+3*nf, 3*nf);


    // iterators
    Mesh::VertexIterator vit, vend=mesh.vertices_end();
    Mesh::FaceIterator fit, fend=mesh.faces_end();

    // split faces
    pmp::Point p(0,0,0);
    for (auto fit : mesh.faces())
        mesh.split(fit, p);

    // collapse new edges
    vit = vend; vend=mesh.vertices_end();
    for (; vit!=vend; ++vit)
        mesh.collapse(mesh.halfedge(*vit));

    // remove deleted items
    mesh.garbage_collection();
}




void Benchmark_Pmp_Lib__V03::_render_export(std::vector<float>& out_positions, std::vector<int>& out_indices) {
    out_positions.resize(mesh.n_vertices() * 3);
    for (auto v : mesh.vertices()) {
        auto p = mesh.position(v);
        out_positions[v.idx()*3+0] = p[0];
        out_positions[v.idx()*3+1] = p[1];
        out_positions[v.idx()*3+2] = p[2];
    }

    out_indices.clear();
    for (auto f : mesh.faces()) {
        for (auto v : mesh.vertices(f)) {   // PMP's face-vertex circulator, range-for compatible
            out_indices.push_back(v.idx());
        }
    }
}