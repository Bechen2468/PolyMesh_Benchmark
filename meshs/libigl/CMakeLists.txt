cmake_minimum_required(VERSION 3.24)


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



FetchContent_Declare(
    libigl
    GIT_REPOSITORY https://github.com/libigl/libigl.git
    GIT_TAG v2.5.0
    GIT_SHALLOW TRUE
)


# disabling everything not required
set(LIBIGL_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
set(LIBIGL_BUILD_TUTORIALS OFF CACHE BOOL "" FORCE)
set(LIBIGL_INSTALL         OFF CACHE BOOL "" FORCE)
# OFF = headeronly | ON = static lib
set(LIBIGL_USE_STATIC_LIBRARY OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(libigl)

