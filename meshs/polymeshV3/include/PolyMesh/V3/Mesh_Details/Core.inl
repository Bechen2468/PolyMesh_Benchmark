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
    std::ifstream file(File_Path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open \"" << File_Path << "\"\n";
        return;
    }
    std::string buf{std::istreambuf_iterator<char>(file), {}};
    std::string_view in = buf;

    off_file_helper::skip_ws(in);
    if (in.substr(0, 3) == "OFF") in.remove_prefix(3);

    Index v_count, f_count, e_count;
    if (!off_file_helper::parse_next(in, v_count) ||
        !off_file_helper::parse_next(in, f_count) ||
        !off_file_helper::parse_next(in, e_count)) {
        std::cerr << "ERROR: Bad OFF header\n";
        return;
    }

    if (!_v.load_off(in, v_count)) {
        std::cerr << "ERROR: Bad vertex data\n";
        return;
    }

    // Allocate faces
    if (!_f.load_off(in, f_count)) {
        std::cerr << "ERROR: Bad face data\n";
        return;
    }
}


};