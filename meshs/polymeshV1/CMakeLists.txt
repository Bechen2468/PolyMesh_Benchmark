cmake_minimum_required(VERSION 3.24)
project(PolyMeshV1 VERSION 1.0 LANGUAGES CXX)

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


file(GLOB_RECURSE my_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)


add_library(PolyMeshV1 STATIC ${my_sources})
add_library(PolyMesh::V1 ALIAS ${PROJECT_NAME})

# Optimizations
#if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#    target_compile_options(PolyMeshV1 PRIVATE
#        -O3
#        -march=native
#    )
#elseif(MSVC)
#    target_compile_options(PolyMeshV1 PRIVATE
#        /O2
#    )
#endif()




target_include_directories(PolyMeshV1
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(PolyMeshV1 PUBLIC Eigen3::Eigen)
target_compile_features(PolyMeshV1 PUBLIC cxx_std_20)
