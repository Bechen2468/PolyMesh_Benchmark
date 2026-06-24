
# Thesis structure:
## Name: Evaluation of a polygon mesh data structure using sparse matrices

### Forschungsfrage
Kann eine Polygonnetzdatenstruktur durch bedarfsgesteuerte Relationsstrukturen sowohl minimalen Speicherverbrauch als auch konkurrenzfähige Laufzeit gegenüber etablierten Bibliotheken (OpenMesh, <ANDERE_NETZE_EINFÜGEN>) erreichen?

### Kapitelstruktur
#### 1. Einleitung & Motivation
- Warum bestehende Strukturen suboptimal sind (OpenMesh immer Halfedge-Overhead, <ANDERE_NETZE_EINFÜGEN>)

#### 2. Grundlagen
- Polygonnetz, Datenstrukturen, Halfedge vs. flach
- Cache-Effizienz, SIMD, Benchmarking-Methodik

#### 3. Design & Implementierung - V1 (Basisstruktur)
- Flache Eigen-Matrix Speicherung
- Iteratoren mit Valid-Mask
- Benchmarkinfrastruktur (Google Benchmark, Ramusage)
- V2 (Adaptive Struktur)
- Lazy Relationsmatrizen (VV, VF, EF, ...)
- Lazy Garbagecollector (ramshortage)
- Invalidierungsstrategie bei Topologieänderungen
- Speicher-Laufzeit-Tradeoff je nach Anwendungsfall

#### 5. Evaluation
- Laufzeit
  - Iteration (V/F/E, sequentiell & random)
  - Adjazenzabfragen (VV/VF/FV/EF)
  - Topologiemanipulation (Edgecollapse, Facesplit)
- Speicherverbrauch
- Kompilierungszeit

#### 6. Fazit & Ausblick
- Wann ist PolyMesh die bessere Wahl?


# Treffen:
- pmp mesh vom paper
- matrixVV schmutz neu
- poly faces:
  vec ids
  face start + len
  ids:    f0, f0, f0, f0, f1, f1, f1, f2, f2, f2, f2, f2, ...
  start:  0, 4, 7, ...
  len;    4, 3, 5, ...
#### DONE
- Benchmark namespace instead of Benchmark_...
- libigl mesh
- anfangen notizen zu den kapiteln
- devnotes: bsp eigen vector operationen nur intern SIMD p += p2 
  - warum im energiesparmodus langsamer als openmesh
  - eigen5 vs eigen3
  - argumenitieren um entscheidungen 



# Notes 24.06:
- Eigen 5.0 MemoryLib new() override bugged (sparsematrix.makeCompressed swaps internal logic for Col-/RowMajor)
  - switched to Eigen 3.4.0
    - much slower iteration then Eigen 5.0.0 (prob their improved custom new())
    - same version as Pmp uses
- Turned Iterators into direct raw pointer wrapper ( much more definite for undefined compiler SIMD selection )
  - => basicly light datastructures guarding rawpointers

- Pmp tests integrated ( Paper )
- Validmask now linkedlist of vector next, prev
  - much more iterationfriendly ( ++ => id = next[id] ; no iteration over invalids)
- Now real Polymesh ( var facesize )
  - FIter currently very inefficient ( still much faster then openmesh and pmp )
- Preloaded vs Intest-Loaded comparison ( first vs consecutive iterations )
- Eigen::Sparse<bool> good
  - like FaceMatrix custom sparsematrix<size_t> still sounds much faster ( implement later as V3 ? ( short comparison in Thesis ) )



# Memory Tests 23.06:
For V = 1.000.000 and F = 2.000.000:



# Benchmark<float> 23.06:
--------------------------------------------------------------------
Benchmark                                                       Time
--------------------------------------------------------------------
Benchmark_PolyMesh_V01/Load/1000000                    2553227536 ns
Benchmark_PolyMesh_V02/Load/1000000                    3644656706 ns
Benchmark_PolyMesh_V02/Load_All/1000000                1.2345e+10 ns
Benchmark_OpenMesh_V11/Load/1000000                    3466404913 ns
Benchmark_Pmp_Lib__V03/Load/1000000                    3793239635 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V01/Iterate_V_Raw/1000000             16343018 ns
Benchmark_PolyMesh_V02/Iterate_V_Raw/1000000             10754678 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V01/Iterate_V/1000000                 18287190 ns
Benchmark_PolyMesh_V02/Iterate_V/1000000                 13484312 ns
Benchmark_OpenMesh_V11/Iterate_V/1000000                 11047203 ns
Benchmark_Pmp_Lib__V03/Iterate_V/1000000                 11493068 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V01/Iterate_F_Raw/1000000             20668537 ns
Benchmark_PolyMesh_V02/Iterate_F_Raw/1000000             11355000 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V01/Iterate_F/1000000                 22259501 ns
Benchmark_PolyMesh_V02/Iterate_F/1000000                 31010295 ns
Benchmark_OpenMesh_V11/Iterate_F/1000000                147044253 ns
Benchmark_Pmp_Lib__V03/Iterate_F/1000000                118919005 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V01/Iterate_E/1000000                 53846925 ns
Benchmark_PolyMesh_V02/Iterate_E/1000000                  8551252 ns
Benchmark_PolyMesh_V02/Iterate_E_Loaded/1000000           5893297 ns
Benchmark_OpenMesh_V11/Iterate_E/1000000                127507860 ns
Benchmark_Pmp_Lib__V03/Iterate_E/1000000                 60749014 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V01/Iterate_Random_V/1000000          39928895 ns
Benchmark_PolyMesh_V02/Iterate_Random_V/1000000          28659949 ns
Benchmark_OpenMesh_V11/Iterate_Random_V/1000000          29933920 ns
Benchmark_Pmp_Lib__V03/Iterate_Random_V/1000000          27356707 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V01/Iterate_Random_F/1000000          68821617 ns
Benchmark_PolyMesh_V02/Iterate_Random_F/1000000          55661722 ns
Benchmark_OpenMesh_V11/Iterate_Random_F/1000000         452173010 ns
Benchmark_Pmp_Lib__V03/Iterate_Random_F/1000000         400398816 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V01/Iterate_Random_E/1000000         201199198 ns
Benchmark_PolyMesh_V02/Iterate_Random_E/1000000          75768097 ns
Benchmark_PolyMesh_V02/Iterate_Random_E_Loaded/1000000   75711793 ns
Benchmark_OpenMesh_V11/Iterate_Random_E/1000000         346227028 ns
Benchmark_Pmp_Lib__V03/Iterate_Random_E/1000000         145766698 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V02/Adjacent_VV/1000000             3049507620 ns
Benchmark_PolyMesh_V02/Adjacent_VV_Loaded/1000000        65239860 ns
Benchmark_OpenMesh_V11/Adjacent_VV/1000000              148016184 ns
Benchmark_Pmp_Lib__V03/Adjacent_VV/1000000              108776988 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V02/Adjacent_VF/1000000             2483529583 ns
Benchmark_PolyMesh_V02/Adjacent_VF_Loaded/1000000        68115750 ns
Benchmark_OpenMesh_V11/Adjacent_VF/1000000              191737213 ns
Benchmark_Pmp_Lib__V03/Adjacent_VF/1000000              125765012 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V02/Adjacent_FV/1000000               82409453 ns
Benchmark_PolyMesh_V02/Adjacent_FV_Loaded/1000000        79156974 ns
Benchmark_OpenMesh_V11/Adjacent_FV/1000000              147412701 ns
Benchmark_Pmp_Lib__V03/Adjacent_FV/1000000              116572487 ns
--------------------------------------------------------------------
Benchmark_PolyMesh_V02/Adjacent_EF/1000000             4530501397 ns
Benchmark_PolyMesh_V02/Adjacent_EF_Loaded/1000000       156681509 ns
Benchmark_OpenMesh_V11/Adjacent_EF/1000000              188720136 ns
Benchmark_Pmp_Lib__V03/Adjacent_EF/1000000               64712024 ns



# Benchmark<double> 23.06:














# Notes:
## AoS vs SoA Vertices
While a structure-of-arrays layout provides optimal streaming access for per-component operations, most mesh processing algorithms operate on complete vertex positions. Therefore, an array-of-structures layout (N × 3) improves cache locality and overall performance for vertex-centric workloads such as smoothing, normal computation, and adjacency traversal and especially random access!
structure-of-arrays is more aligned with special hpc usecases, which are to specific and rare for a general meshstruct.

## Tests:
- Iterate Random:
  - All random iteration tests are executed without validity check as in an ideal execution, the developer would only get by id if he got to that id over a valid way.

## Next steps:
- restructure project for individual cmakes and independent compilation
- V2:
  - implement relationmatrices
  - implement adjacency tests
<DONE>
  - evaluate storage usage with relationmatrices
  - implement lazygarbagecontroller
  - edgecollapse
  - facesplit
  - gtests (polymesh == openmesh values)

