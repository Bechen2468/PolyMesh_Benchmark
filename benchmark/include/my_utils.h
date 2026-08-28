#ifndef MY_UTILS_H
#define MY_UTILS_H
// ----------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <span>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <random>
#include <algorithm>
#include <cstddef>

#if defined(_WIN32)
    #include <windows.h>
    #include <psapi.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif


// ----------------------------------------------------------------------------
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <pmp/algorithms/shapes.h>
#include <pmp/surface_mesh.h>
#include <pmp/io/io.h>
#include "QuickHull.hpp"
// ----------------------------------------------------------------------------
typedef OpenMesh::PolyMesh_ArrayKernelT<>  Mesh;



namespace my_utils {
    namespace off {
        void build_sphere_off(size_t Point_Count, const std::string& File_Path = "sphere.off");
        void build_random_sphere_off(size_t Point_Count, const std::string& File_Path, unsigned int Seed = 42);
        void build_quad_off(size_t Point_Count, const std::string& File_Path);
        void build_dual_off(size_t Point_Count, const std::string& File_Path, unsigned int Seed = 42);

    };

    namespace system {
        size_t get_current_memory_kb();
        size_t get_peak_memory_kb();
    };

    namespace num {
        std::vector<size_t> generate_random_indices(size_t Size);
    };
};

#endif