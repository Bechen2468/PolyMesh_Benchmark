cmake_minimum_required(VERSION 3.24)

include(FetchContent)

add_compile_definitions(_USE_MATH_DEFINES)
#set(PMP_SCALAR_TYPE 64)
#set(PMP_INDEX_TYPE 64)


#set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON)
#set(Eigen3_DIR "Eigen3_DIR-NOTFOUND" CACHE PATH "" FORCE)
#set(EIGEN3_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/eigen-src" CACHE PATH "" FORCE)
#set(EIGEN3_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/eigen-src")


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


FetchContent_GetProperties(Eigen3)
if(eigen3_SOURCE_DIR)                              # only set if it was downloaded
    set(EIGEN3_INCLUDE_DIR "${eigen3_SOURCE_DIR}")
endif()



set(PMP_BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(PMP_BUILD_VIEWERS    OFF CACHE BOOL "" FORCE)  # newer pmp versions: GLFW, GLAD, ImGui
set(PMP_BUILD_VIS        OFF CACHE BOOL "" FORCE)
set(PMP_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(PMP_BUILD_DOCS       OFF CACHE BOOL "" FORCE)
#set(PMP_INSTALL          OFF CACHE BOOL "" FORCE)


FetchContent_Declare(
    pmp_library
    GIT_REPOSITORY https://github.com/pmp-library/pmp-library.git
    GIT_TAG 3.0.0
)

FetchContent_MakeAvailable(pmp_library)


add_library(polymesh_pmp INTERFACE)
if(PMP_SCALAR_TYPE EQUAL 64)
    target_compile_definitions(polymesh_pmp INTERFACE PMP_SCALAR_TYPE_64 PMP_INDEX_TYPE_64)
endif()

if(TARGET pmp)
    target_link_libraries(polymesh_pmp INTERFACE pmp)
elseif(TARGET PMP::pmp)
    target_link_libraries(polymesh_pmp INTERFACE PMP::pmp)
else()
    message(FATAL_ERROR "Could not find PMP target after FetchContent.")
endif()

