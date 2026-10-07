cmake_minimum_required(VERSION 3.20)
project(PolyMesh_Benchmark VERSION 3.0 LANGUAGES CXX)

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

add_compile_definitions(_USE_MATH_DEFINES)




include(FetchContent)


add_subdirectory(meshs/openmesh)
add_subdirectory(meshs/libigl)
add_subdirectory(meshs/polymeshV1)
add_subdirectory(meshs/polymeshV2)
add_subdirectory(meshs/polymeshV3)
add_subdirectory(meshs/pmp)
add_subdirectory(benchmark)


