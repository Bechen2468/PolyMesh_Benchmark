#include "benchmarks/mesh/bm_libigl.h"


void Benchmark_Libigl_V2_5::_circular() {
    _ensure_VF();

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
    // per_vertex_normals scatters the face normals onto their vertices, no adjacency needed
    N_V.resize(V.rows(), 3);
    N_F.resize(F.rows(), 3);

    igl::per_face_normals(V, F, N_F);
    igl::per_vertex_normals(V, F, igl::PER_VERTEX_NORMALS_WEIGHTING_TYPE_UNIFORM, N_F, N_V);
}



void Benchmark_Libigl_V2_5::_smoothing() {
    _ensure_VV();

    // In place, vertex by vertex (later vertices see already-smoothed positions),
    for (int v = 0; v < V.rows(); ++v) {
        if (B[v] || VV[v].empty()) continue;

        Vertex p = Vertex::Zero();
        for (int n : VV[v]) p += V.row(n);
        V.row(v) = p / Scalar(VV[v].size());
    }
}



void Benchmark_Libigl_V2_5::_subdivision() {
    // sqrt(3) subdivision. libigl has no split / flip operations, so instead of
    // split-then-flip we build the final mesh directly:
    //   - every face gets a centroid vertex
    //   - every old interior edge ends up flipped, i.e. replaced by the edge between
    //     the two adjacent centroids
    //   - old boundary edges stay, connected to the centroid of their face
    _ensure_VV();
    _ensure_EF();

    const int nV = (int)V.rows();
    const int nF = (int)F.rows();

    MatV Vn(nV + nF, 3);

    // new positions of old vertices (computed from old positions only)
    for (int v = 0; v < nV; ++v) {
        const Scalar n = (Scalar)VV[v].size();
        if (B[v] || VV[v].empty()) {
            Vn.row(v) = V.row(v);
            continue;
        }
        const Scalar alpha = (4.0 - 2.0 * std::cos(2.0 * M_PI / n)) / 9.0;
        Vertex p = Vertex::Zero();
        for (int nb : VV[v]) p += V.row(nb);
        Vn.row(v) = (1.0 - alpha) * V.row(v) + (alpha / n) * p;
    }

    // face centroids
    for (int f = 0; f < nF; ++f)
        Vn.row(nV + f) = (V.row(F(f, 0)) + V.row(F(f, 1)) + V.row(F(f, 2))) / 3.0;

    MatF Fn(3 * nF, 3);
    for (int f = 0; f < nF; ++f) {
        for (int e = 0; e < 3; ++e) {
            const int a  = F(f, e);
            const int b  = F(f, (e + 1) % 3);
            const int ei = EMAP(f + ((e + 2) % 3) * nF);
            const int g  = (EF(ei, 0) == f) ? EF(ei, 1) : EF(ei, 0);   // face across the edge, -1 = boundary
            if (g < 0) Fn.row(3 * f + e) << a, b, nV + f;          // boundary: keep edge
            else       Fn.row(3 * f + e) << a, nV + g, nV + f;     // flipped interior edge
        }
    }

    V.swap(Vn);
    F.swap(Fn);

    _relations_changed();
}





void Benchmark_Libigl_V2_5::_collapse() {
    const int nV = (int)V.rows();
    const int nF = (int)F.rows();

    // ------------------------------------------------------------------
    // 1) Split every face into 3 around a new centre vertex.
    //    The old V / F are released when Vs / Fs leave this scope (after swap).
    // ------------------------------------------------------------------
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
    _free(V);

    // ------------------------------------------------------------------
    // 2) Collapse one edge of every new vertex into its neighbour.
    //    The edge structures (the largest temporaries) only live in this scope.
    // ------------------------------------------------------------------
    {
        Eigen::MatrixXi cE, cEF, cEI;
        Eigen::VectorXi cEMAP;
        igl::edge_flaps(F, cE, cEMAP, cEF, cEI);
        const int nFs = (int)F.rows();

        const std::vector<bool> border = igl::is_border_vertex(F);

        Eigen::RowVectorXd p(3);   // allocated once instead of once per collapse
        int e1, e2, f1, f2;

        for (int f = 0; f < nF; ++f) {
            if (F(f, 0) == IGL_COLLAPSE_EDGE_NULL && F(f, 1) == IGL_COLLAPSE_EDGE_NULL) continue;

            const int v = nV + f;

            // pick an edge from the new vertex v to a neighbour that is NOT on the border
            int e = -1, other = -1;
            for (int c = 0; c < 3; ++c) {
                const int ec = cEMAP(f + c * nFs);
                if (cE(ec, 0) != v && cE(ec, 1) != v) continue;      // edge does not touch v
                const int o = (cE(ec, 0) == v) ? cE(ec, 1) : cE(ec, 0);
                if (border[o]) continue;                            // unsafe for collapse_edge
                if (cEF(ec, 0) < 0 || cEF(ec, 1) < 0) continue;     // edge itself is open
                e = ec; other = o;
                break;
            }
            if (e < 0) continue;
            p = Vd.row(other);   // merge into the neighbour, keep its position

            igl::collapse_edge(e, p, Vd, F, cE, cEMAP, cEF, cEI, e1, e2, f1, f2);
        }
    }   // cE / cEMAP / cEF / cEI freed here

    // ------------------------------------------------------------------
    // 3) Remove deleted faces in place (no extra copy of F).
    // ------------------------------------------------------------------
    {
        int w = 0;
        for (int r = 0; r < (int)F.rows(); ++r) {
            const bool deleted = F(r, 0) == IGL_COLLAPSE_EDGE_NULL
                              && F(r, 1) == IGL_COLLAPSE_EDGE_NULL
                              && F(r, 2) == IGL_COLLAPSE_EDGE_NULL;
            if (deleted) continue;
            if (w != r) F.row(w) = F.row(r);
            ++w;
        }
        F.conservativeResize(w, 3);
    }

    // ------------------------------------------------------------------
    // 4) Remove unreferenced vertices (counterpart of garbage_collection()).
    //    Inputs are freed as soon as the outputs exist.
    // ------------------------------------------------------------------
    {
        Eigen::MatrixXd NVd;
        MatF NF;
        Eigen::VectorXi I;
        igl::remove_unreferenced(Vd, F, NVd, NF, I);

        _free(Vd);
        _free(I);

        F.swap(NF);
        _free(NF);              // old, uncompacted F

        V = NVd.cast<Scalar>();
    }

    _relations_changed();
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

