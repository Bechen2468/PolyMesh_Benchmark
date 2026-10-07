cmake_minimum_required(VERSION 3.20)


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

