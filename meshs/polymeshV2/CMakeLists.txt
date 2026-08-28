cmake_minimum_required(VERSION 3.20)
project(PolyMeshV2 VERSION 2.0 LANGUAGES CXX)

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


#file(GLOB_RECURSE my_sources
#    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
#)
#
#add_library(${PROJECT_NAME} STATIC ${my_sources})


add_library(${PROJECT_NAME} INTERFACE)
add_library(PolyMesh::V2 ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} INTERFACE Eigen3::Eigen)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

