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
    using MatF = Eigen::MatrixXi;  // #F x 3 (tri)
    using Vertex = Eigen::RowVector3<Scalar>;

    MatV V, V_storage;
    MatF F, F_storage;

    // normals
    MatV N_V, N_F;

protected:
    std::vector<size_t> load_mesh_storage(const std::string& FileName, int load_full = 0) override {
        _clear_all();

        if(!igl::read_triangle_mesh(FileName, V_storage, F_storage)) {
            throw std::runtime_error("libigl: failed to read " + FileName);
        }

        // n_edges
        Eigen::MatrixXi E;
        igl::edges(F_storage, E);

        return {(size_t)V_storage.rows(), (size_t)F_storage.rows(), (size_t)E.rows()};
    }

    void load_mesh_from_storage(int load_full) override {
        V = V_storage;   // deep copy
        F = F_storage;

        N_V.resize(V.rows(), 3);
        N_F.resize(F.rows(), 3);
    }

    void destroy_mesh() override { _clear_all(); }

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

private:
    // swap with an empty matrix so the memory is actually freed
    // (resize(0,3) would keep the allocation)
    template<typename M> static void _release(M& m) { 
        M().swap(m); 
    }

    void _clear_all() {
        _release(V); 
        _release(V_storage);
        _release(F); 
        _release(F_storage);
        _release(N_V); 
        _release(N_F);
    }
};
#endif
