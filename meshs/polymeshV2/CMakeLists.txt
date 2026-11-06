cmake_minimum_required(VERSION 3.24)
project(PolyMeshV2 VERSION 2.0 LANGUAGES CXX)

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

