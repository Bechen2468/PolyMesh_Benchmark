cmake_minimum_required(VERSION 3.20)
project(PolyMesh_Benchmark VERSION 3.0 LANGUAGES CXX)

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

if(NOT DEFINED EIGEN3_INCLUDE_DIR)
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(eigen)
endif()
find_package(Eigen3 3.4.0 REQUIRED)



include(FetchContent)


add_subdirectory(meshs/openmesh)
add_subdirectory(meshs/libigl)
add_subdirectory(meshs/polymeshV1)
add_subdirectory(meshs/polymeshV2)
add_subdirectory(meshs/polymeshV3)
add_subdirectory(meshs/pmp)
add_subdirectory(benchmark)


