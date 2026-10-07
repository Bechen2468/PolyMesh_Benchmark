#include "benchmarks/mesh/bm_openmesh.h"
#include "benchmarks/mesh/bm_pmp.h"
#include "benchmarks/mesh/bm_libigl.h"
#include "benchmarks/mesh/bm_polymesh_V1.h"
#include "benchmarks/mesh/bm_polymesh_V2.h"
#include "benchmarks/structs/matrix/ibenchmark_matrix.h"
#include "benchmarks/mesh/bm_polymesh_V3.h"




/*  REMOVED TO GET TO NEW TESTS FASTER ( NOTHING WRONG HERE )
REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_Eigen_Sparse, Eigen__Iterate, run_iterate)
REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_D_Ragged_Matrix, D_CSR__Iterate, run_iterate)
REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_D_Ragged_Matrix, D_CSR__Iterate_WorstCase, run_iterate_worst_case)

REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_Eigen_Sparse, Eigen__Extend_InReserve, run_extend_InReserve)
REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_D_Ragged_Matrix, D_CSR__Extend_InReserve, run_extend_InReserve)

REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_Eigen_Sparse, Eigen__Extend_OverReserve, run_extend_OverReserve)
REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_D_Ragged_Matrix, D_CSR__Extend_OverReserve, run_extend_OverReserve)

REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_Eigen_Sparse, Eigen__Remove_from_Row, run_remove_from_Row)
REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_D_Ragged_Matrix, D_CSR__Remove_from_Row, run_remove_from_Row)

REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_Eigen_Sparse, Eigen__Delete_Rows, run_delete_rows)
REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_D_Ragged_Matrix, D_CSR__Delete_Rows, run_delete_rows)

REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_Eigen_Sparse, Eigen__Add_Rows, run_add_rows)
REGISTER_TEMPLATE(IBenchmark_Matrix, Adapter_D_Ragged_Matrix, D_CSR__Add_Rows, run_add_rows)





BENCHMARK_DEFINE_F(Benchmark_D_CSR_Matrix, Verify)(benchmark::State& state) {
    run_verification(state);                                             
}                                                           
BENCHMARK_REGISTER_F(Benchmark_D_CSR_Matrix, Verify)                  
    ->DenseRange(0, 4, 1)
    ->Unit(benchmark::kMicrosecond);



REGISTER_01(Benchmark_PolyMesh_V01, Load, run_load)
REGISTER_01(Benchmark_PolyMesh_V02, Load, run_load)
//REGISTER_01(Benchmark_PolyMesh_V02, Load_All, run_load_all)
REGISTER_01(Polymesh_V03, Benchmark_PolyMesh_V03, Traits_Default, Load, run_load)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Load, run_load)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Load, run_load)

REGISTER_01(Benchmark_PolyMesh_V01, Iterate_V_Raw, run_iterate_V_raw)
REGISTER_01(Benchmark_PolyMesh_V02, Iterate_V_Raw, run_iterate_V_raw)
REGISTER_01(Polymesh_V03, Benchmark_PolyMesh_V03, Traits_Default, Iterate_V_Raw, run_iterate_V_raw)

REGISTER_01(Benchmark_PolyMesh_V01, Iterate_V, run_iterate_V)
REGISTER_01(Benchmark_PolyMesh_V02, Iterate_V, run_iterate_V)
REGISTER_01(Polymesh_V03, Benchmark_PolyMesh_V03, Traits_Default, Iterate_V, run_iterate_V)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Iterate_V, run_iterate_V)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Iterate_V, run_iterate_V)

REGISTER_01(Benchmark_PolyMesh_V01, Iterate_F_Raw, run_iterate_F_raw)
REGISTER_01(Benchmark_PolyMesh_V02, Iterate_F_Raw, run_iterate_F_raw)
REGISTER_01(Polymesh_V03, Benchmark_PolyMesh_V03, Traits_Default, Iterate_F_Raw, run_iterate_F_raw)

REGISTER_01(Benchmark_PolyMesh_V01, Iterate_F, run_iterate_F)
REGISTER_01(Benchmark_PolyMesh_V02, Iterate_F, run_iterate_F)
REGISTER_01(Polymesh_V03, Benchmark_PolyMesh_V03, Traits_Default, Iterate_F, run_iterate_F)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Iterate_F, run_iterate_F)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Iterate_F, run_iterate_F)

REGISTER_01(Benchmark_PolyMesh_V01, Iterate_E, run_iterate_E)
REGISTER_01(Benchmark_PolyMesh_V02, Iterate_E, run_iterate_E)
//REGISTER_01(Benchmark_PolyMesh_V02, Iterate_E_Loaded, run_iterate_loaded_E)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Iterate_E, run_iterate_E)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Iterate_E, run_iterate_E)

*/

/*
REGISTER_01(Benchmark_PolyMesh_V01, Iterate_Random_V, run_iterate_random_V)
REGISTER_01(Benchmark_PolyMesh_V02, Iterate_Random_V, run_iterate_random_V)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Iterate_Random_V, run_iterate_random_V)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Iterate_Random_V, run_iterate_random_V)

REGISTER_01(Benchmark_PolyMesh_V01, Iterate_Random_F, run_iterate_random_F)
REGISTER_01(Benchmark_PolyMesh_V02, Iterate_Random_F, run_iterate_random_F)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Iterate_Random_F, run_iterate_random_F)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Iterate_Random_F, run_iterate_random_F)

REGISTER_01(Benchmark_PolyMesh_V01, Iterate_Random_E, run_iterate_random_E)
REGISTER_01(Benchmark_PolyMesh_V02, Iterate_Random_E, run_iterate_random_E)
//REGISTER_01(Benchmark_PolyMesh_V02, Iterate_Random_E_Loaded, run_iterate_loaded_random_E)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Iterate_Random_E, run_iterate_random_E)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Iterate_Random_E, run_iterate_random_E)
*/


/*

REGISTER_01(Benchmark_PolyMesh_V01, Adjacent_VV, run_adjacent_VV)
REGISTER_01(Benchmark_PolyMesh_V02, Adjacent_VV, run_adjacent_VV)
REGISTER_01(Benchmark_PolyMesh_V02, Adjacent_VV_Loaded, run_adjacent_loaded_VV)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Adjacent_VV, run_adjacent_VV)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Adjacent_VV, run_adjacent_VV)


REGISTER_01(Benchmark_PolyMesh_V01, Adjacent_VF, run_adjacent_VF)
REGISTER_01(Benchmark_PolyMesh_V02, Adjacent_VF, run_adjacent_VF)
REGISTER_01(Benchmark_PolyMesh_V02, Adjacent_VF_Loaded, run_adjacent_loaded_VF)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Adjacent_VF, run_adjacent_VF)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Adjacent_VF, run_adjacent_VF)


REGISTER_01(Benchmark_PolyMesh_V01, Adjacent_FV, run_adjacent_FV)
REGISTER_01(Benchmark_PolyMesh_V02, Adjacent_FV, run_adjacent_FV)
REGISTER_01(Benchmark_PolyMesh_V02, Adjacent_FV_Loaded, run_adjacent_loaded_FV)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Adjacent_FV, run_adjacent_FV)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Adjacent_FV, run_adjacent_FV)


REGISTER_01(Benchmark_PolyMesh_V01, Adjacent_EF, run_adjacent_EF)
REGISTER_01(Benchmark_PolyMesh_V02, Adjacent_EF, run_adjacent_EF)
REGISTER_01(Benchmark_PolyMesh_V02, Adjacent_EF_Loaded, run_adjacent_loaded_EF)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Adjacent_EF, run_adjacent_EF)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Adjacent_EF, run_adjacent_EF)
*/



/*
REGISTER_01(Polymesh_V03, Benchmark_PolyMesh_V03, Traits_Default, Tri__to_TriMesh, run_convert_to_trimesh)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Tri__to_TriMesh, run_convert_to_trimesh)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Tri__to_TriMesh, run_convert_to_trimesh)

REGISTER_01(Polymesh_V03, Benchmark_PolyMesh_V03, Traits_Default, Quad_to_TriMesh, run_convert_to_trimesh)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Quad_to_TriMesh, run_convert_to_trimesh)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Quad_to_TriMesh, run_convert_to_trimesh)

REGISTER_01(Polymesh_V03, Benchmark_PolyMesh_V03, Traits_Default, Dual_to_TriMesh, run_convert_to_trimesh)
REGISTER_01(OpenMesh_V11, Benchmark_OpenMesh_V11, Dual_to_TriMesh, run_convert_to_trimesh)
REGISTER_01(Pmp_Lib__V03, Benchmark_Pmp_Lib__V03, Dual_to_TriMesh, run_convert_to_trimesh)
*/




static const std::vector<int64_t> VALUES_0    = {0};
static const std::vector<int64_t> VALUES_1    = {1};
static const std::vector<int64_t> VALUES_0_1  = {0, 1};
static const std::vector<int64_t> VALUES_ALL  = {0, 1, 2, 3, 4};



using Traits_Default = PolyMesh::V3::Traits<float, std::uint32_t, std::uint8_t>;


template<typename ScalarT, typename IndexT = std::uint32_t, typename BinSizeT = std::uint8_t, typename MeshT = void>
struct Traits_AppendT : public PolyMesh::V3::Traits<ScalarT, IndexT, BinSizeT, MeshT> {
    template<typename NewMeshT>
    using Rebind = Traits_AppendT<ScalarT, IndexT, BinSizeT, NewMeshT>;   //requires rebind to itself ( overload always required )

    using Faces = PolyMesh::V3::Faces_Append_Ragged_Matrix<IndexT, MeshT>;
    using VV_HE = PolyMesh::V3::VV_HE_Append_Ragged_Matrix<IndexT, MeshT>;
    using VF = PolyMesh::V3::VF_Append_Ragged_Matrix<IndexT, MeshT>;
    using EF = PolyMesh::V3::EF_Append_Ragged_Matrix<IndexT, MeshT>;
};

using Traits_Append = Traits_AppendT<float, std::uint32_t, std::uint8_t>;




template<typename ScalarT, typename IndexT = std::uint32_t, typename BinSizeT = std::uint8_t, typename MeshT = void>
struct Traits_VecVecT : public PolyMesh::V3::Traits<ScalarT, IndexT, BinSizeT, MeshT> {
    template<typename NewMeshT>
    using Rebind = Traits_VecVecT<ScalarT, IndexT, BinSizeT, NewMeshT>;   //requires rebind to itself ( overload always required )

    using Faces = PolyMesh::V3::Faces_Vec_Vec_Matrix<IndexT, MeshT>;
    using VV_HE = PolyMesh::V3::VV_HE_Vec_Vec_Matrix<IndexT, MeshT>;
    using VF = PolyMesh::V3::VF_Vec_Vec_Matrix<IndexT, MeshT>;
    using EF = PolyMesh::V3::EF_Vec_Vec_Matrix<IndexT, MeshT>;
};

using Traits_VecVec = Traits_VecVecT<float, std::uint32_t, std::uint8_t>;





template<typename ScalarT, typename IndexT = std::uint32_t, typename BinSizeT = std::uint8_t, typename MeshT = void>
struct Traits_VecEigenT : public PolyMesh::V3::Traits<ScalarT, IndexT, BinSizeT, MeshT> {
    template<typename NewMeshT>
    using Rebind = Traits_VecEigenT<ScalarT, IndexT, BinSizeT, NewMeshT>;   //requires rebind to itself ( overload always required )

    using Faces = PolyMesh::V3::Faces_Vec_Eigen_Matrix<IndexT, MeshT>;
    using VV_HE = PolyMesh::V3::VV_HE_Vec_Eigen_Matrix<IndexT, MeshT>;
    using VF = PolyMesh::V3::VF_Vec_Eigen_Matrix<IndexT, MeshT>;
    //using EF = PolyMesh::V3::EF_Vec_Eigen_Matrix<IndexT, MeshT>;  // doesnt exist as its not realisticly implimentable
};

using Traits_VecEigen = Traits_VecEigenT<float, std::uint32_t, std::uint8_t>;






REGISTER("OpenMesh____/V11/Load", Benchmark_OpenMesh_V11, run_load, VALUES_ALL)
REGISTER("SurfaceMesh_/V03/Load", Benchmark_Pmp_Lib__V03, run_load, VALUES_1)
REGISTER("Libigl______/V25/Load", Benchmark_Libigl_V2_5, run_load, VALUES_1)
REGISTER_T("D_Ragged____/V03/Load", Benchmark_PolyMesh_V03, Traits_Default, run_load, VALUES_ALL)
REGISTER_T("VecVec______/V03/Load", Benchmark_PolyMesh_V03, Traits_VecVec, run_load, VALUES_ALL)
REGISTER_T("Apd_Ragged__/V03/Load", Benchmark_PolyMesh_V03, Traits_Append, run_load, VALUES_ALL)
REGISTER_T("Vec_Eigen___/V03/Load", Benchmark_PolyMesh_V03, Traits_VecEigen, run_load, VALUES_ALL)


//REGISTER_01T("D_Ragged____/V03/Load", Benchmark_PolyMesh_V03, Traits_Default, run_load)
//REGISTER_01T("Apd_Ragged__/V03/Load", Benchmark_PolyMesh_V03, Traits_Append, run_load)

REGISTER("OpenMesh____/V11/Circular", Benchmark_OpenMesh_V11, run_circular, VALUES_1)
REGISTER("SurfaceMesh_/V03/Circular", Benchmark_Pmp_Lib__V03, run_circular, VALUES_1)
REGISTER("Libigl______/V25/Circular", Benchmark_Libigl_V2_5, run_circular, VALUES_1)
REGISTER_T("D_Ragged____/V03/Circular", Benchmark_PolyMesh_V03, Traits_Default, run_circular, VALUES_0_1)
REGISTER_T("VecVec______/V03/Circular", Benchmark_PolyMesh_V03, Traits_VecVec, run_circular, VALUES_0_1)
REGISTER_T("Apd_Ragged__/V03/Circular", Benchmark_PolyMesh_V03, Traits_Append, run_circular, VALUES_0_1)
REGISTER_T("Vec_Eigen___/V03/Circular", Benchmark_PolyMesh_V03, Traits_VecEigen, run_circular, VALUES_0_1)

REGISTER("OpenMesh____/V11/Barycenter", Benchmark_OpenMesh_V11, run_barycenter, VALUES_1)
REGISTER("SurfaceMesh_/V03/Barycenter", Benchmark_Pmp_Lib__V03, run_barycenter, VALUES_1)
REGISTER("Libigl______/V25/Barycenter", Benchmark_Libigl_V2_5, run_barycenter, VALUES_1)
REGISTER_T("D_Ragged____/V03/Barycenter", Benchmark_PolyMesh_V03, Traits_Default, run_barycenter, VALUES_0_1)
REGISTER_T("VecVec______/V03/Barycenter", Benchmark_PolyMesh_V03, Traits_VecVec, run_barycenter, VALUES_0_1)
REGISTER_T("Apd_Ragged__/V03/Barycenter", Benchmark_PolyMesh_V03, Traits_Append, run_barycenter, VALUES_0_1)
REGISTER_T("Vec_Eigen___/V03/Barycenter", Benchmark_PolyMesh_V03, Traits_VecEigen, run_barycenter, VALUES_0_1)


REGISTER("OpenMesh____/V11/Normal", Benchmark_OpenMesh_V11, run_normal, VALUES_1)
REGISTER("SurfaceMesh_/V03/Normal", Benchmark_Pmp_Lib__V03, run_normal, VALUES_1)
REGISTER("Libigl______/V25/Normal", Benchmark_Libigl_V2_5, run_normal, VALUES_1)
REGISTER_T("D_Ragged____/V03/Normal", Benchmark_PolyMesh_V03, Traits_Default, run_normal, VALUES_0_1)
REGISTER_T("VecVec______/V03/Normal", Benchmark_PolyMesh_V03, Traits_VecVec, run_normal, VALUES_0_1)
REGISTER_T("Apd_Ragged__/V03/Normal", Benchmark_PolyMesh_V03, Traits_Append, run_normal, VALUES_0_1)
REGISTER_T("Vec_Eigen___/V03/Normal", Benchmark_PolyMesh_V03, Traits_VecEigen, run_normal, VALUES_0_1)


REGISTER("OpenMesh____/V11/Smoothing", Benchmark_OpenMesh_V11, run_smoothing, VALUES_1)
REGISTER("SurfaceMesh_/V03/Smoothing", Benchmark_Pmp_Lib__V03, run_smoothing, VALUES_1)
REGISTER("Libigl______/V25/Smoothing", Benchmark_Libigl_V2_5, run_smoothing, VALUES_1)
REGISTER_T("D_Ragged____/V03/Smoothing", Benchmark_PolyMesh_V03, Traits_Default, run_smoothing, VALUES_0_1)
REGISTER_T("VecVec______/V03/Smoothing", Benchmark_PolyMesh_V03, Traits_VecVec, run_smoothing, VALUES_0_1)
REGISTER_T("Apd_Ragged__/V03/Smoothing", Benchmark_PolyMesh_V03, Traits_Append, run_smoothing, VALUES_0_1)
REGISTER_T("Vec_Eigen___/V03/Smoothing", Benchmark_PolyMesh_V03, Traits_VecEigen, run_smoothing, VALUES_0_1)

REGISTER("OpenMesh____/V11/Subdivision", Benchmark_OpenMesh_V11, run_subdivision, VALUES_1)
REGISTER("SurfaceMesh_/V03/Subdivision", Benchmark_Pmp_Lib__V03, run_subdivision, VALUES_1)
REGISTER("Libigl______/V25/Subdivision", Benchmark_Libigl_V2_5, run_subdivision, VALUES_1)
REGISTER_T("D_Ragged____/V03/Subdivision", Benchmark_PolyMesh_V03, Traits_Default, run_subdivision, VALUES_0_1)
REGISTER_T("VecVec______/V03/Subdivision", Benchmark_PolyMesh_V03, Traits_VecVec, run_subdivision, VALUES_0_1)
REGISTER_T("Apd_Ragged__/V03/Subdivision", Benchmark_PolyMesh_V03, Traits_Append, run_subdivision, VALUES_0_1)
REGISTER_T("Vec_Eigen___/V03/Subdivision", Benchmark_PolyMesh_V03, Traits_VecEigen, run_subdivision, VALUES_0_1)


REGISTER("OpenMesh____/V11/Collapse", Benchmark_OpenMesh_V11, run_collapse, VALUES_1)
REGISTER("SurfaceMesh_/V03/Collapse", Benchmark_Pmp_Lib__V03, run_collapse, VALUES_1)
REGISTER("Libigl______/V25/Collapse", Benchmark_Libigl_V2_5, run_collapse, VALUES_1)
REGISTER_T("D_Ragged____/V03/Collapse", Benchmark_PolyMesh_V03, Traits_Default, run_collapse, VALUES_0_1)
REGISTER_T("VecVec______/V03/Collapse", Benchmark_PolyMesh_V03, Traits_VecVec, run_collapse, VALUES_0_1)
REGISTER_T("Apd_Ragged__/V03/Collapse", Benchmark_PolyMesh_V03, Traits_Append, run_collapse, VALUES_0_1)
REGISTER_T("Vec_Eigen___/V03/Collapse", Benchmark_PolyMesh_V03, Traits_VecEigen, run_collapse, VALUES_0_1)


REGISTER("OpenMesh____/V11/Render_Export", Benchmark_OpenMesh_V11, run_render_export, VALUES_1)
REGISTER("SurfaceMesh_/V03/Render_Export", Benchmark_Pmp_Lib__V03, run_render_export, VALUES_1)
REGISTER("Libigl______/V25/Render_Export", Benchmark_Libigl_V2_5, run_render_export, VALUES_1)
REGISTER_T("D_Ragged____/V03/Render_Export", Benchmark_PolyMesh_V03, Traits_Default, run_render_export, VALUES_0_1)
REGISTER_T("VecVec______/V03/Render_Export", Benchmark_PolyMesh_V03, Traits_VecVec, run_render_export, VALUES_0_1)
REGISTER_T("Apd_Ragged__/V03/Render_Export", Benchmark_PolyMesh_V03, Traits_Append, run_render_export, VALUES_0_1)
REGISTER_T("Vec_Eigen___/V03/Render_Export", Benchmark_PolyMesh_V03, Traits_VecEigen, run_render_export, VALUES_0_1)


