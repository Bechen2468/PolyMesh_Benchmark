#pragma once
// is never resolved true, just for intellicode linking
#ifndef POLY_MESH_V3_H
#include "PolyMesh/V3/Mesh.h"
#endif



namespace PolyMesh::V3 {


PM_T
PM::PolyMesh():
_v(T::Reserve_Perz),
_f(T::Reserve_Perz)
{
    _relations.invalidate(CacheFlag::ALL);
}



PM_T
PM::~PolyMesh() {
    _relations.invalidate(CacheFlag::ALL);
    _relations.collect_garbage();
}


PM_T
void PM::read_file(const std::string& File_Path) {
    std::ifstream file(File_Path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open \"" << File_Path << "\"\n";
        return;
    }

    std::string line;
    std::getline(file, line);
    if(line.rfind("OFF", 0) != std::string::npos)
        std::getline(file, line);

    Index v_count, f_count, e_count;
    std::istringstream(line) >> v_count >> f_count >> e_count;

    // Allocate vertices
    _v.load_off(file, v_count);

    // Allocate faces
    _f.load_off(file, f_count);
}


};