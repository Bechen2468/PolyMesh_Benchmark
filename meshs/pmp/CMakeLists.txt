cmake_minimum_required(VERSION 3.20)

include(FetchContent)

add_compile_definitions(_USE_MATH_DEFINES)
set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)

set(PMP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PMP_BUILD_DOCS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    pmp_library
    GIT_REPOSITORY https://github.com/pmp-library/pmp-library.git
    GIT_TAG 3.0.0
)

FetchContent_MakeAvailable(pmp_library)

if(TARGET pmp)
    add_library(polymesh_pmp INTERFACE)
    target_link_libraries(polymesh_pmp INTERFACE pmp)
elseif(TARGET PMP::pmp)
    add_library(polymesh_pmp INTERFACE)
    target_link_libraries(polymesh_pmp INTERFACE PMP::pmp)
else()
    message(FATAL_ERROR "Could not find PMP target after FetchContent.")
endif()

