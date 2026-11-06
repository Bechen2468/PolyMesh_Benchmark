#ifndef BENCHMARK_LIBIGL_H
#define BENCHMARK_LIBIGL_H
// ----------------------------------------------------------------------------
#include <string>
#include <cmath>
#include <Eigen/Core>
#include <igl/read_triangle_mesh.h>
#include <igl/edges.h>
#include <igl/vertex_triangle_adjacency.h>
#include <igl/per_face_normals.h>
#include <igl/per_vertex_normals.h>
#include <igl/adjacency_list.h>
#include <igl/is_border_vertex.h>
#include <igl/triangle_triangle_adjacency.h>
#include <igl/edge_flaps.h>
#include <igl/collapse_edge.h>
#include <igl/remove_unreferenced.h>
#include "benchmarks/mesh/ibenchmark.h"
// ----------------------------------------------------------------------------



class Benchmark_Libigl_V2_5 : public IBenchmark {
protected:
    // libigl has no mesh class: the mesh is just a vertex matrix and a face matrix.
    using Scalar = float;

    using MatV = Eigen::MatrixX<Scalar>;  // #V x 3
    using MatF = Eigen::MatrixXi;         // #F x 3 (tri)
    using Vertex = Eigen::RowVector3<Scalar>;
    using AdjList = std::vector<std::vector<int>>;

    // libigl only rebuilds, so after an edit the relations that were loaded are built again,
    // the counterpart of PolyMesh keeping them up to date
    static constexpr bool REBUILD_AFTER_EDIT = true;

    MatV V, V_storage;
    MatF F, F_storage;

    // VV_HE equivalent: neighbour vertices per vertex + border flags
    AdjList VV, VV_storage;
    std::vector<bool> B,  B_storage;

    // VF equivalent: faces per vertex as flat CSR list (VF) + offsets (NI)
    Eigen::VectorXi VF, VF_storage;
    Eigen::VectorXi NI, NI_storage;

    // EF equivalent: unique edges, face corner -> edge, edge -> its 2 faces, edge -> corner in those faces
    Eigen::MatrixXi E, E_storage;
    Eigen::VectorXi EMAP, EMAP_storage;
    Eigen::MatrixXi EF, EF_storage;
    Eigen::MatrixXi EI, EI_storage;

    bool has_VV = false, has_VF = false, has_EF = false;

    // normals
    MatV N_V, N_F;

    static bool wants(int load_full, int which) { return load_full == 1 || load_full == which; }


protected:
    void load_mesh_storage(const std::string& FileName, int load_full = 0) override {
#if defined(__GLIBC__)
        mallopt(M_MMAP_THRESHOLD, 1 << 20);   // blocks >= 1 MB always come from / go back to the OS
#endif

        _clear_all();

        if(!igl::read_triangle_mesh(FileName, V_storage, F_storage)) {
            throw std::runtime_error("libigl: failed to read " + FileName);
        }

        // 1 for load full, 2 VV_HE, 3 VF, 4 EF
        if(wants(load_full, 2)) _build_VV(F_storage, VV_storage, B_storage);
        if(wants(load_full, 3)) _build_VF(F_storage, (int)V_storage.rows(), VF_storage, NI_storage);
        if(wants(load_full, 4)) _build_EF(F_storage, E_storage, EMAP_storage, EF_storage, EI_storage);

        _print_memory();
    }

    void load_mesh_from_storage(int load_full) override {
        V = V_storage;   // deep copies
        F = F_storage;

        has_VV = wants(load_full, 2);
        has_VF = wants(load_full, 3);
        has_EF = wants(load_full, 4);

        if(has_VV) { VV = VV_storage; B = B_storage; }
        if(has_VF) { VF = VF_storage; NI = NI_storage; }
        if(has_EF) { E = E_storage; EMAP = EMAP_storage; EF = EF_storage; EI = EI_storage; }
    }


    void destroy_mesh() override { _clear_all(); malloc_trim(0); }

    void _iterate_V_raw() override {}
    void _iterate_F_raw() override {}

    void _iterate_V() override {}
    void _iterate_F() override {}
    void _iterate_E() override {}

    void _iterate_random_V(const std::vector<size_t>& rand) override {}
    void _iterate_random_F(const std::vector<size_t>& rand) override {}
    void _iterate_random_E(const std::vector<size_t>& rand) override {}

    void _adjacent_VV() override {}
    void _adjacent_VF() override {}
    void _adjacent_FV() override {}
    void _adjacent_EF() override {}

    void _convert_to_trimesh() override {}

    // real algorithms
    void _circular() override;
    void _barycenter() override;
    void _normal() override;
    void _smoothing() override;
    void _subdivision() override;
    void _collapse() override;
    void _render_export(std::vector<float>& out_positions, std::vector<int>& out_indices) override;



protected:
    // build on demand (like mesh.VV_HE() / VF() / EF() in PolyMesh)
    static void _build_VV(const MatF& F_, AdjList& VV_, std::vector<bool>& B_) {
        igl::adjacency_list(F_, VV_);
        B_ = igl::is_border_vertex(F_);
    }

    static void _build_VF(const MatF& F_, int n, Eigen::VectorXi& VF_, Eigen::VectorXi& NI_) {
        igl::vertex_triangle_adjacency(F_, n, VF_, NI_);
    }

    static void _build_EF(const MatF& F_, Eigen::MatrixXi& E_, Eigen::VectorXi& EMAP_, Eigen::MatrixXi& EF_, Eigen::MatrixXi& EI_) {
        igl::edge_flaps(F_, E_, EMAP_, EF_, EI_);
    }

    void _ensure_VV() { if(!has_VV) { _build_VV(F, VV, B); has_VV = true; } }
    void _ensure_VF() { if(!has_VF) { _build_VF(F, (int)V.rows(), VF, NI); has_VF = true; } }
    void _ensure_EF() { if(!has_EF) { _build_EF(F, E, EMAP, EF, EI); has_EF = true; } }

    // call after every structural edit: relations are invalid now
    void _relations_changed() {
        const bool vv = has_VV, vf = has_VF, ef = has_EF;
        has_VV = has_VF = has_EF = false;
        if constexpr (REBUILD_AFTER_EDIT) {
            if(vv) _ensure_VV();
            if(vf) _ensure_VF();
            if(ef) _ensure_EF();
        }
    }



private:
    template<typename T>
    static void _free(T& X) { T().swap(X); }   // releases the memory, clear() would keep the capacity

    void _clear_all() {
        _free(V);    _free(V_storage);
        _free(F);    _free(F_storage);
        _free(VV);   _free(VV_storage);
        _free(B);    _free(B_storage);
        _free(VF);   _free(VF_storage);
        _free(NI);   _free(NI_storage);
        _free(E);    _free(E_storage);
        _free(EMAP); _free(EMAP_storage);
        _free(EF);   _free(EF_storage);
        _free(EI);   _free(EI_storage);
        _free(N_V);  _free(N_F);
        has_VV = has_VF = has_EF = false;
    }


    // memory stuff
    template<typename Derived>
    static size_t _bytes(const Eigen::PlainObjectBase<Derived>& M) {
        return (size_t)M.size() * sizeof(typename Derived::Scalar);
    }

    static size_t _bytes(const AdjList& L) {
        size_t b = L.capacity() * sizeof(std::vector<int>);
        for(const auto& row : L) b += row.capacity() * sizeof(int);
        return b;
    }

    static size_t _bytes(const std::vector<bool>& L) { return (L.capacity() + 7) / 8; }

    void _print_memory() const {
        const size_t core  = _bytes(V_storage) + _bytes(F_storage);
        const size_t vv_he = _bytes(VV_storage) + _bytes(B_storage);
        const size_t vf    = _bytes(VF_storage) + _bytes(NI_storage);
        const size_t ef    = _bytes(E_storage) + _bytes(EMAP_storage) + _bytes(EF_storage) + _bytes(EI_storage);
        const auto mb = [](size_t b) { return b / 1e6; };

        std::cout << "Libigl Mem usage (MB): Core " << mb(core)
                  << " | VV_HE " << mb(vv_he)
                  << " | VF "    << mb(vf)
                  << " | EF "    << mb(ef)
                  << " | All "   << mb(core + vv_he + vf + ef) << std::endl;
    }
};
#endif
