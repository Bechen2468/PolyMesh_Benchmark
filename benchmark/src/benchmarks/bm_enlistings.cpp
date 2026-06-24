#include "benchmarks/bm_polymesh_V1.h"
#include "benchmarks/bm_polymesh_V2.h"
#include "benchmarks/bm_openmesh.h"
#include "benchmarks/bm_pmp.h"




REGISTER(Benchmark_PolyMesh_V01, Load, run_load)
REGISTER(Benchmark_PolyMesh_V02, Load, run_load)
REGISTER(Benchmark_PolyMesh_V02, Load_All, run_load_all)
REGISTER(Benchmark_OpenMesh_V11, Load, run_load)
REGISTER(Benchmark_Pmp_Lib__V03, Load, run_load)

REGISTER(Benchmark_PolyMesh_V01, Iterate_V_Raw, run_iterate_V_raw)
REGISTER(Benchmark_PolyMesh_V02, Iterate_V_Raw, run_iterate_V_raw)

REGISTER(Benchmark_PolyMesh_V01, Iterate_V, run_iterate_V)
REGISTER(Benchmark_PolyMesh_V02, Iterate_V, run_iterate_V)
REGISTER(Benchmark_OpenMesh_V11, Iterate_V, run_iterate_V)
REGISTER(Benchmark_Pmp_Lib__V03, Iterate_V, run_iterate_V)

REGISTER(Benchmark_PolyMesh_V01, Iterate_F_Raw, run_iterate_F_raw)
REGISTER(Benchmark_PolyMesh_V02, Iterate_F_Raw, run_iterate_F_raw)
REGISTER(Benchmark_PolyMesh_V01, Iterate_F, run_iterate_F)
REGISTER(Benchmark_PolyMesh_V02, Iterate_F, run_iterate_F)
REGISTER(Benchmark_OpenMesh_V11, Iterate_F, run_iterate_F)
REGISTER(Benchmark_Pmp_Lib__V03, Iterate_F, run_iterate_F)

REGISTER(Benchmark_PolyMesh_V01, Iterate_E, run_iterate_E)
REGISTER(Benchmark_PolyMesh_V02, Iterate_E, run_iterate_E)
REGISTER(Benchmark_PolyMesh_V02, Iterate_E_Loaded, run_iterate_loaded_E)
REGISTER(Benchmark_OpenMesh_V11, Iterate_E, run_iterate_E)
REGISTER(Benchmark_Pmp_Lib__V03, Iterate_E, run_iterate_E)

REGISTER(Benchmark_PolyMesh_V01, Iterate_Random_V, run_iterate_random_V)
REGISTER(Benchmark_PolyMesh_V02, Iterate_Random_V, run_iterate_random_V)
REGISTER(Benchmark_OpenMesh_V11, Iterate_Random_V, run_iterate_random_V)
REGISTER(Benchmark_Pmp_Lib__V03, Iterate_Random_V, run_iterate_random_V)

REGISTER(Benchmark_PolyMesh_V01, Iterate_Random_F, run_iterate_random_F)
REGISTER(Benchmark_PolyMesh_V02, Iterate_Random_F, run_iterate_random_F)
REGISTER(Benchmark_OpenMesh_V11, Iterate_Random_F, run_iterate_random_F)
REGISTER(Benchmark_Pmp_Lib__V03, Iterate_Random_F, run_iterate_random_F)

REGISTER(Benchmark_PolyMesh_V01, Iterate_Random_E, run_iterate_random_E)
REGISTER(Benchmark_PolyMesh_V02, Iterate_Random_E, run_iterate_random_E)
REGISTER(Benchmark_PolyMesh_V02, Iterate_Random_E_Loaded, run_iterate_loaded_random_E)
REGISTER(Benchmark_OpenMesh_V11, Iterate_Random_E, run_iterate_random_E)
REGISTER(Benchmark_Pmp_Lib__V03, Iterate_Random_E, run_iterate_random_E)


// REGISTER(Benchmark_PolyMesh_V01, Adjacent_VV, run_adjacent_VV)
REGISTER(Benchmark_PolyMesh_V02, Adjacent_VV, run_adjacent_VV)
REGISTER(Benchmark_PolyMesh_V02, Adjacent_VV_Loaded, run_adjacent_loaded_VV)
REGISTER(Benchmark_OpenMesh_V11, Adjacent_VV, run_adjacent_VV)
REGISTER(Benchmark_Pmp_Lib__V03, Adjacent_VV, run_adjacent_VV)


// REGISTER(Benchmark_PolyMesh_V01, Adjacent_VF, run_adjacent_VF)
REGISTER(Benchmark_PolyMesh_V02, Adjacent_VF, run_adjacent_VF)
REGISTER(Benchmark_PolyMesh_V02, Adjacent_VF_Loaded, run_adjacent_loaded_VF)
REGISTER(Benchmark_OpenMesh_V11, Adjacent_VF, run_adjacent_VF)
REGISTER(Benchmark_Pmp_Lib__V03, Adjacent_VF, run_adjacent_VF)


// REGISTER(Benchmark_PolyMesh_V01, Adjacent_FV, run_adjacent_FV)
REGISTER(Benchmark_PolyMesh_V02, Adjacent_FV, run_adjacent_FV)
REGISTER(Benchmark_PolyMesh_V02, Adjacent_FV_Loaded, run_adjacent_loaded_FV)
REGISTER(Benchmark_OpenMesh_V11, Adjacent_FV, run_adjacent_FV)
REGISTER(Benchmark_Pmp_Lib__V03, Adjacent_FV, run_adjacent_FV)


// REGISTER(Benchmark_PolyMesh_V01, Adjacent_EF, run_adjacent_EF)
REGISTER(Benchmark_PolyMesh_V02, Adjacent_EF, run_adjacent_EF)
REGISTER(Benchmark_PolyMesh_V02, Adjacent_EF_Loaded, run_adjacent_loaded_EF)
REGISTER(Benchmark_OpenMesh_V11, Adjacent_EF, run_adjacent_EF)
REGISTER(Benchmark_Pmp_Lib__V03, Adjacent_EF, run_adjacent_EF)




