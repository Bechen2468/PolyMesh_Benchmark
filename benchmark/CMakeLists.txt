cmake_minimum_required(VERSION 3.24)
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

# Windows only 
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

add_executable(PolyMesh_Benchmark ${benchmark_sources})

#target_compile_options(PolyMesh_Benchmark PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
#target_link_options(PolyMesh_Benchmark PRIVATE -fsanitize=address)

target_include_directories(PolyMesh_Benchmark PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

find_package(Threads REQUIRED)


target_link_libraries(PolyMesh_Benchmark PRIVATE
    PolyMesh::V1
    PolyMesh::V2
    PolyMesh::V3
    OpenMeshCore
    polymesh_pmp
    igl::core
    benchmark::benchmark
    Threads::Threads
    quickhull
)
# openmesh include bug fix
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:OpenMeshCore,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ROOT_DIR="${CMAKE_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# AUTOMATIC BUILD CONFIGURATION (DEBUG VS RELEASE)
# ---------------------------------------------------------------------------

# Visual Studio and Xcode generators ignore CMAKE_BUILD_TYPE (they choose Debug/Release
# when building), so the if below would always take the Release branch there.
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(IS_MULTI_CONFIG)
    message(WARNING "Multi-config generator detected (Visual Studio / Xcode). "
                    "Use Ninja or Makefiles with -DCMAKE_BUILD_TYPE=Debug|Release instead.")
endif()

# CPU specific instructions: Apple Silicon uses -mcpu, everything else -march
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(NATIVE_ARCH_FLAG -mcpu=native)
else()
    set(NATIVE_ARCH_FLAG -march=native)
endif()

# MSVC: follow the C++ standard strictly (closer to GCC/Clang behaviour)
if(MSVC)
    target_compile_options(PolyMesh_Benchmark PRIVATE /permissive-)
endif()


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring PolyMesh_Benchmark for maximum DEBUGGING efficiency...")

    if(MSVC)
        # /RTC1 (CMake default for Debug) is not compatible with AddressSanitizer
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

        target_compile_options(PolyMesh_Benchmark PRIVATE
            /Od                         # Disable optimizations
            /Zi                         # Full debug info
            /fsanitize=address          # AddressSanitizer (MSVC has no UndefinedBehaviorSanitizer)
        )
    else()
        target_compile_options(PolyMesh_Benchmark PRIVATE
            -O0                         # Completely disable optimizations so variables never disappear
            -g3                         # Max debug info (includes extra macro expansion symbols)
            -fno-omit-frame-pointer     # Keep frame pointer for human-readable stack traces
            -fsanitize=address          # AddressSanitizer: memory corruption bugs
            -fsanitize=undefined        # Undefined behavior (null pointers, integer overflows, ...)
        )
        target_link_options(PolyMesh_Benchmark PRIVATE
            -fsanitize=address
            -fsanitize=undefined
        )
    endif()

else()
    message(STATUS "Configuring PolyMesh_Benchmark for ultimate RELEASE performance...")

    if(MSVC)
        target_compile_options(PolyMesh_Benchmark PRIVATE
            /O2                         # Maximum speed
            /Ob3                        # Aggressive inlining
            /Oi                         # Intrinsic functions
            /arch:AVX2                  # MSVC has no "native": use AVX2 (or /arch:AVX512 if the CPU supports it)
            /DNDEBUG                    # Strip assert() calls
        )
    else()
        target_compile_options(PolyMesh_Benchmark PRIVATE
            -O3                         # Aggressive optimization & loop auto-vectorization
            ${NATIVE_ARCH_FLAG}         # Use all vector instructions of this CPU
            -DNDEBUG                    # Strip assert() calls
        )
    endif()

    # Link-Time Optimization, works for GCC, Clang and MSVC
    cmake_policy(SET CMP0069 NEW)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT error)
    if(ipo_supported)
        set_target_properties(PolyMesh_Benchmark PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "--> Global Link-Time Optimization (LTO) ENABLED.")
    else()
        message(STATUS "--> LTO not supported by compiler: ${error}")
    endif()

endif()