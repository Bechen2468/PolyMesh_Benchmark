cmake_minimum_required(VERSION 3.20)

include(FetchContent)

FetchContent_Declare(
    OpenMesh
    GIT_REPOSITORY https://gitlab.vci.rwth-aachen.de:9000/OpenMesh/OpenMesh.git
    GIT_TAG OpenMesh-11.0
    GIT_SHALLOW FALSE
)

# disable everything we dont need
set(OPENMESH_BUILD_SHARED      OFF CACHE BOOL "" FORCE)
set(BUILD_APPS                 OFF CACHE BOOL "" FORCE)
set(OPENMESH_DOCS              OFF CACHE BOOL "" FORCE)
set(OPENMESH_TESTS             OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(OpenMesh)

# Silence known OpenMesh warnings on modern GCC/Clang
foreach(t
    OpenMeshCore
    OpenMeshTools
    OpenMeshCoreStatic
    OpenMeshToolsStatic
)
    if (TARGET ${t})
        target_compile_options(${t} PRIVATE
            -Wno-deprecated-enum-enum-conversion
            -Wno-unused-result
        )
    endif()
endforeach()

