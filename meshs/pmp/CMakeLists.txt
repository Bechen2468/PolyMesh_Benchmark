cmake_minimum_required(VERSION 3.20)

include(FetchContent)

add_compile_definitions(_USE_MATH_DEFINES)
#set(PMP_SCALAR_TYPE 64)
#set(PMP_INDEX_TYPE 64)


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

