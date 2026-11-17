# PolyMesh

> This README's wording was improved with AI. The information in it is my own.

Code and benchmarks of my bachelor thesis *Evaluation of a polygon mesh data structure using sparse matrices* (TU Berlin, Computer Science B.Sc., 2026).

**PolyMesh** is a polygon mesh data structure that stores a mesh as matrices instead of a linked half-edge structure. This repository contains the implementation and the benchmark suite that compares it with three existing libraries:

| Library | Design |
|---|---|
| **OpenMesh** | Half-Edge |
| **PMP** (Polygon Mesh Processing Library, `SurfaceMesh`) | Half-Edge |
| **Libigl** | Face-Index (vertex and face matrices) |

## The idea

A mesh always needs its vertices and faces. **PolyMesh** keeps only these two in its `Core`: a dense matrix for the vertices and a ragged matrix for the faces, which allows faces with any number of vertices. Everything else, the adjacency, is optional. Three relation matrices can be built when an algorithm needs them:

- `VV_HE`: the neighbors of each vertex through its outgoing half-edges
- `VF`: the faces around each vertex
- `EF`: the faces on each side of an edge

They are built on first access and are only kept up to date while they are in use, so runtime and memory adapt to what an algorithm actually needs.

The storage behind these matrices is exchangeable through a traits type. Four matrix types are implemented and compared:

- **Append**: rows are appended, deleted rows leave holes that are removed by compacting
- **Reuse**: like **Append**, but it also tracks free blocks and reuses them
- **VecVec**: every row is its own `std::vector`
- **EigenCSR**: an `Eigen` sparse matrix in CSR layout wrapped in a helper that allows editing

## Benchmarks

The benchmarks compare the libraries on meshes of different sizes (Stanford Bunny up to about 2.2 million vertices) with these tasks:

`Load`, `Render Export`, `Circular` (iterating the neighbors of every vertex), `Barycenter`, `Normal`, `Smoothing`, `Subdivision` and `Collapse`.

`Subdivision` and `Collapse` change the topology and are also run with edit validation (`PM-Validate`). The memory use of every library is measured separately.

## Requirements

Nothing has to be installed beforehand. All libraries (**OpenMesh**, **PMP**, **Libigl**, **Eigen**) are downloaded and built together with the project.

## Build and run

The scripts in `./scripts` compile the project in release mode and run all benchmarks.

**Linux / macOS**

```sh
./scripts/run_benchmarks_performance.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\run_benchmarks_performance.ps1
```

## Results

The results of every run are written to

    ./build_release/results/<date>/

as one Google Benchmark JSON file per benchmark. Each benchmark runs in its own process, and Google Benchmark can not merge results from different processes.

## Repository layout

- `./meshs`: the CMake file of each mesh library (for external libraries it only loads them) and, for our own meshes, their code.
- `./benchmark`: the benchmark suite. It has a base class `ibenchmark` for the mesh benchmarks, and every mesh library has its own class derived from it.
- `./results`: all current final results. `./scripts/plot_benchmarks_compare.sh` reads all benchmark JSON files in here to build the plots.
- `./plots`: the plots created by `./scripts/plot_benchmarks_compare.sh`, including the benchmark plots used in the thesis.
- `./meshs/polymeshV3`: the `PolyMesh` version used in the thesis.
