#include "benchmarks/mesh/bm_libigl.h"



void Benchmark_Libigl_V2_5::_circular() {
    // vertex -> faces, as a flat CSR list (VF) + offsets (NI)
    Eigen::VectorXi VF, NI;
    igl::vertex_triangle_adjacency(F, (int)V.rows(), VF, NI);

    int counter = 0;

    for (int v = 0; v < V.rows(); ++v)
        for (int k = NI(v); k < NI(v + 1); ++k)
            counter += (VF(k) >= 0);

            
    for (int f = 0; f < F.rows(); ++f)
        for (int c = 0; c < 3; ++c)
            counter -= (F(f, c) >= 0);

    _benchmark_sink += counter;
}



void Benchmark_Libigl_V2_5::_barycenter() {
    Vertex p = V.colwise().sum() / Scalar(V.rows());

    V.rowwise() -= p;

    _benchmark_sink += p[0] + p[1] + p[2];
}



void Benchmark_Libigl_V2_5::_normal() {
    N_V.resize(V.rows(), 3);
    N_F.resize(F.rows(), 3);

    igl::per_face_normals(V, F, N_F);
    igl::per_vertex_normals(V, F, igl::PER_VERTEX_NORMALS_WEIGHTING_TYPE_UNIFORM, N_F, N_V);
}



void Benchmark_Libigl_V2_5::_smoothing() {
    std::vector<std::vector<int>> A;      // vertex -> neighbor vertices
    igl::adjacency_list(F, A);
    std::vector<bool> border = igl::is_border_vertex(F);

    // In place, vertex by vertex (later vertices see already-smoothed positions),
    for (int v = 0; v < V.rows(); ++v) {
        if (border[v] || A[v].empty()) continue;

        Vertex p = Vertex::Zero();
        for (int n : A[v]) p += V.row(n);
        V.row(v) = p / Scalar(A[v].size());
    }
}



void Benchmark_Libigl_V2_5::_subdivision() {
    // sqrt(3) subdivision. libigl has no split / flip operations, so instead of
    // split-then-flip we build the final mesh directly:
    //   - every face gets a centroid vertex
    //   - every old interior edge ends up flipped, i.e. replaced by the edge between
    //     the two adjacent centroids
    //   - old boundary edges stay, connected to the centroid of their face
    const int nV = (int)V.rows();
    const int nF = (int)F.rows();

    std::vector<std::vector<int>> A;
    igl::adjacency_list(F, A);
    std::vector<bool> border = igl::is_border_vertex(F);

    Eigen::MatrixXi TT, TTi;               // TT(f,e): face across edge e of f (-1 = boundary)
    igl::triangle_triangle_adjacency(F, TT, TTi);

    MatV Vn(nV + nF, 3);

    // new positions of old vertices (computed from old positions only)
    for (int v = 0; v < nV; ++v) {
        const Scalar n = (Scalar)A[v].size();
        if (border[v] || A[v].empty()) {
            Vn.row(v) = V.row(v);
            continue;
        }
        const Scalar alpha = (4.0 - 2.0 * std::cos(2.0 * M_PI / n)) / 9.0;
        Vertex p = Vertex::Zero();
        for (int nb : A[v]) p += V.row(nb);
        Vn.row(v) = (1.0 - alpha) * V.row(v) + (alpha / n) * p;
    }

    // face centroids
    for (int f = 0; f < nF; ++f)
        Vn.row(nV + f) = (V.row(F(f, 0)) + V.row(F(f, 1)) + V.row(F(f, 2))) / 3.0;

    // one new triangle per directed edge a->b of face f
    MatF Fn(3 * nF, 3);
    for (int f = 0; f < nF; ++f) {
        for (int e = 0; e < 3; ++e) {
            const int a = F(f, e);
            const int b = F(f, (e + 1) % 3);
            const int g = TT(f, e);
            if (g < 0) Fn.row(3 * f + e) << a, b, nV + f;          // boundary: keep edge
            else       Fn.row(3 * f + e) << a, nV + g, nV + f;     // flipped interior edge
        }
    }

    V.swap(Vn);
    F.swap(Fn);
}



void Benchmark_Libigl_V2_5::_collapse() {
    const int nV = (int)V.rows();
    const int nF = (int)F.rows();

    // split every face at the origin (same as OpenMesh: new vertices at (0,0,0))
    {
        MatV Vs(nV + nF, 3);
        Vs.topRows(nV) = V;
        Vs.bottomRows(nF).setZero();

        MatF Fs(3 * nF, 3);
        for (int f = 0; f < nF; ++f) {
            const int a = F(f, 0), b = F(f, 1), c = F(f, 2), v = nV + f;
            Fs.row(f)          << a, b, v;
            Fs.row(nF + f)     << b, c, v;
            Fs.row(2 * nF + f) << c, a, v;
        }
        V.swap(Vs);
        F.swap(Fs);
    }

    Eigen::MatrixXd Vd = V.cast<double>();

    // libigls collapse needs the edge topology
    Eigen::MatrixXi E, EF, EI;
    Eigen::VectorXi EMAP;
    igl::edge_flaps(F, E, EMAP, EF, EI);
    const int nFs = (int)F.rows();

    // collapsing one edge of every new vertex into its neighbour
    for (int f = 0; f < nF; ++f) {
        if (F(f, 0) == IGL_COLLAPSE_EDGE_NULL && F(f, 1) == IGL_COLLAPSE_EDGE_NULL) continue;

        const int v = nV + f;

        int e = -1;
        for (int c = 0; c < 3; ++c) {
            const int ec = EMAP(f + c * nFs);
            if (E(ec, 0) == v || E(ec, 1) == v) { e = ec; break; }
        }
        if (e < 0) continue;

        const int other = (E(e, 0) == v) ? E(e, 1) : E(e, 0);
        Eigen::RowVectorXd p = Vd.row(other);   // merge into the neighbour, keep its position

        int e1, e2, f1, f2;
        igl::collapse_edge(e, p, Vd, F, E, EMAP, EF, EI, e1, e2, f1, f2);
    }

    // removing deleted items (counterpart of garbage_collection())
    {
        int keep = 0;
        for (int r = 0; r < F.rows(); ++r)
            if (!(F(r, 0) == IGL_COLLAPSE_EDGE_NULL && F(r, 1) == IGL_COLLAPSE_EDGE_NULL && F(r, 2) == IGL_COLLAPSE_EDGE_NULL))
                ++keep;

        MatF Fc(keep, 3);
        int w = 0;
        for (int r = 0; r < F.rows(); ++r)
            if (!(F(r, 0) == IGL_COLLAPSE_EDGE_NULL && F(r, 1) == IGL_COLLAPSE_EDGE_NULL && F(r, 2) == IGL_COLLAPSE_EDGE_NULL))
                Fc.row(w++) = F.row(r);

        Eigen::MatrixXd NVd;
        MatF NF;
        Eigen::VectorXi I;
        igl::remove_unreferenced(Vd, Fc, NVd, NF, I);
        V = NVd.cast<Scalar>();
        F.swap(NF);
    }
}



void Benchmark_Libigl_V2_5::_render_export(std::vector<float>& out_positions, std::vector<int>& out_indices) {
    out_positions.resize(V.rows() * 3);
    for (int i = 0; i < V.rows(); ++i) {
        out_positions[i * 3 + 0] = (float)V(i, 0);
        out_positions[i * 3 + 1] = (float)V(i, 1);
        out_positions[i * 3 + 2] = (float)V(i, 2);
    }

    out_indices.clear();
    for (int f = 0; f < F.rows(); ++f)
        for (int c = 0; c < 3; ++c)
            out_indices.push_back(F(f, c));
}
