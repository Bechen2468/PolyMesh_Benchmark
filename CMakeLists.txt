cmake_minimum_required(VERSION 3.24)
project(PolyMesh_Benchmark VERSION 3.0 LANGUAGES CXX)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries" FORCE)

# never write build trees into the user package registry
set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY ON)          # for projects using the old CMP0090 behaviour
set(CMAKE_POLICY_DEFAULT_CMP0090 NEW)             # newer behaviour: export(PACKAGE) does nothing by default
set(CMAKE_EXPORT_PACKAGE_REGISTRY OFF)            # ...and stays off under the new behaviour

# and never read from it either
set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON)



set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

add_compile_definitions(_USE_MATH_DEFINES)

set(EIGEN_BUILD_TESTING    OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_DOC        OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_PKGCONFIG  OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING          OFF CACHE BOOL "" FORCE)


include(FetchContent)
set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON)   # never use registry entries

if(NOT TARGET Eigen3::Eigen)                       # another file already provided it
    set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(EIGEN_BUILD_DOC     OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(Eigen3
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG        3.4.0
        GIT_SHALLOW    TRUE
        FIND_PACKAGE_ARGS 3.4 CONFIG)              # try an installed Eigen first, download only if none is found
    FetchContent_MakeAvailable(Eigen3)
endif()



include(FetchContent)


add_subdirectory(meshs/openmesh)
add_subdirectory(meshs/libigl)
add_subdirectory(meshs/polymeshV1)
add_subdirectory(meshs/polymeshV2)
add_subdirectory(meshs/polymeshV3)
add_subdirectory(meshs/pmp)
add_subdirectory(benchmark)


