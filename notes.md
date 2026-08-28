clear ; cmake .. -DCMAKE_BUILD_TYPE=Release ; cmake --build . --config Release ; sudo cpupower frequency-set --governor performance ; ./benchmark/PolyMesh_Benchmark ; sudo cpupower frequency-set --governor powersave






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




# TODO
## HERE
1. Atomic primitives (add/delete vertex, add/delete face, edge_flip)
2. V3 as separate class, coarse V2 stays untouched for comparison
3. RSM (CSR+slack) AND vec-of-vec, same interface, both implemented
4. Invalidation/touch table per primitive (write this down before coding)
5. Lazy queue, replay-on-access, per-matrix scope
6. Benchmark: RSM vs vec-of-vec vs Eigen-sparse (build/memory/query)
7. Benchmark: single-primitive cost, V2 vs V3 vs OpenMesh
8. Later: real-algorithm benchmarks (smoothing, decimation, etc.) — not now



# Termin Mittwoch 14:
ab übernächste woche!

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

