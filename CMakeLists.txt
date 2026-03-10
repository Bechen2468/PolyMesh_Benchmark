cmake_minimum_required(VERSION 3.20)
project(PolyMesh_Benchmark VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

add_compile_definitions(_USE_MATH_DEFINES)

include(FetchContent)


# Declare OpenMesh
FetchContent_Declare(
  openmesh
  GIT_REPOSITORY https://gitlab.vci.rwth-aachen.de:9000/OpenMesh/OpenMesh.git
  GIT_TAG OpenMesh-11.0
  GIT_SHALLOW FALSE         # required for subrepositorys
)
# Fetch OpenMesh
FetchContent_MakeAvailable(openmesh)

# Silence known OpenMesh warnings on modern GCC/Clang
foreach(t
    OpenMeshCore
    OpenMeshTools
    OpenMeshCoreStatic
    OpenMeshToolsStatic
)
    if (TARGET ${t})
        target_compile_options(${t} PRIVATE
            -Wno-deprecated-enum-enum-conversion
            -Wno-unused-result
        )
    endif()
endforeach()



# Declare Eigen
FetchContent_Declare(
  eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG 5.0
  GIT_SHALLOW FALSE         # required for subrepositorys
)
# Fetch Eigen
FetchContent_MakeAvailable(eigen)


# Get all src files
file(GLOB_RECURSE my_sources
  "src/*.cpp"
)

# Add the main executable
add_executable(${PROJECT_NAME} ${my_sources})


# Include headers
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include) # My Files
#target_link_libraries(${PROJECT_NAME} PRIVATE OpenMeshCore OpenMeshTools)       # OpenMesh
target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)                    # Eigen3::Eigen

# Include OpenMesh  ( suppressing warnings (openmeshs enum handling is outdated) )
target_link_libraries(${PROJECT_NAME} PRIVATE OpenMeshCore)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:OpenMeshCore,INTERFACE_INCLUDE_DIRECTORIES>
)
