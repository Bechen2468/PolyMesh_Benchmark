cmake_minimum_required(VERSION 3.20)
project(PolyMesh_Benchmark LANGUAGES CXX)

include(FetchContent)

# google benchmark
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.8.3
    GIT_SHALLOW    TRUE
)
set(BENCHMARK_ENABLE_TESTING        OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS    OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS          OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)


# quickhull for mesh generation
FetchContent_Declare(
    quickhull
    GIT_REPOSITORY https://github.com/akuukka/quickhull.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(quickhull)


add_library(quickhull STATIC
    ${quickhull_SOURCE_DIR}/QuickHull.cpp
)
target_include_directories(quickhull PUBLIC
    ${quickhull_SOURCE_DIR}
)


# collect benchmark sources
file(GLOB_RECURSE benchmark_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(PolyMesh_Benchmark ${benchmark_sources})

target_compile_options(PolyMesh_Benchmark PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
target_link_options(PolyMesh_Benchmark PRIVATE -fsanitize=address)

target_include_directories(PolyMesh_Benchmark PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


target_link_libraries(PolyMesh_Benchmark PRIVATE
    PolyMesh::V1
    PolyMesh::V2
    OpenMeshCore
    polymesh_pmp
    benchmark::benchmark
    pthread
    quickhull
)
# openmesh include bug fix
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:OpenMeshCore,INTERFACE_INCLUDE_DIRECTORIES>
)



target_compile_options(PolyMesh_Benchmark PRIVATE -O2 -march=native)

