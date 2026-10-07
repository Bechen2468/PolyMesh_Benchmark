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

#target_compile_options(PolyMesh_Benchmark PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
#target_link_options(PolyMesh_Benchmark PRIVATE -fsanitize=address)

target_include_directories(PolyMesh_Benchmark PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


target_link_libraries(PolyMesh_Benchmark PRIVATE
    PolyMesh::V1
    PolyMesh::V2
    PolyMesh::V3
    OpenMeshCore
    polymesh_pmp
    igl::core
    benchmark::benchmark
    pthread
    quickhull
)
# openmesh include bug fix
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:OpenMeshCore,INTERFACE_INCLUDE_DIRECTORIES>
)


# ---------------------------------------------------------------------------
# AUTOMATIC BUILD CONFIGURATION (DEBUG VS RELEASE)
# ---------------------------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring PolyMesh_Benchmark for maximum DEBUGGING efficiency...")
    
    # 1. Apply ultimate debugging, safety, and stack tracing options
    target_compile_options(PolyMesh_Benchmark PRIVATE 
        -O0                         # Completely disable optimizations so variables never disappear
        -g3                         # Max debug info (includes extra macro expansion symbols)
        -fno-omit-frame-pointer     # Keep frame pointer for beautiful, human-readable stack traces
        -fsanitize=address          # Turn on AddressSanitizer to track memory corruption bugs
        -fsanitize=undefined        # Catch undefined behavior (null pointers, integer overflows, etc.)
    )
    
    # 2. Link the runtime sanitizer engines
    target_link_options(PolyMesh_Benchmark PRIVATE 
        -fsanitize=address 
        -fsanitize=undefined
    )

else()
    message(STATUS "Configuring PolyMesh_Benchmark for ultimate RELEASE performance...")
    
    cmake_policy(SET CMP0069 NEW)
    
    # 1. Apply absolute max speed options
    target_compile_options(PolyMesh_Benchmark PRIVATE 
        -O3                         # Aggressive performance optimization & loop auto-vectorization
        -march=native               # Unlock advanced vector instructions (AVX2 / AVX-512) for this CPU
        -DNDEBUG                    # Completely strip out assert() calls to prevent code halting
    )
    
    # 2. Check for and safely enable Link-Time Optimization (LTO / IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT error)
    if(ipo_supported)
        set_target_properties(PolyMesh_Benchmark PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "--> Global Link-Time Optimization (LTO) ENABLED.")
    else()
        message(STATUS "--> LTO not supported by compiler: ${error}")
    endif()

endif()