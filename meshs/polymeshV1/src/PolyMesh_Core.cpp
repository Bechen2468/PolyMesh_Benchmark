#include "PolyMesh/V1/PolyMesh.h"


namespace PolyMesh::V1 {


template<typename T>
PolyMesh<T>::PolyMesh() {

}



template<typename T>
PolyMesh<T>::~PolyMesh() {

}


template<typename T>
void PolyMesh<T>::read_file(const std::string& File_Path) {
    std::ifstream file(File_Path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open \"" << File_Path << "\"\n";
        return;
    }

    std::string line;
    std::getline(file, line);
    if(line.rfind("OFF", 0) != std::string::npos)
        std::getline(file, line);

    size_t v_count, f_count, e_count;
    std::istringstream(line) >> v_count >> f_count >> e_count;

    _v.resize(v_count, 3);      // RowMajor: rows=vertices, cols=xyz
    _f.resize(f_count, 3);      // RowMajor: rows=faces,    cols=v0v1v2
    _v_valid.assign(v_count, 1);
    _f_valid.assign(f_count, 1);
    _n_vertices = v_count;
    _n_faces    = f_count;
    _n_edges    = f_count * 3;

    for (size_t i = 0; i < v_count; ++i) {
        file >> _v(i, 0) >> _v(i, 1) >> _v(i, 2);
    }

    for (size_t f = 0; f < f_count; ++f) {
        int n;
        file >> n;                      // polygon vertex count (3 for triangles)
        file >> _f(f, 0) >> _f(f, 1) >> _f(f, 2);
        // skipping any extra indices for non-triangle faces
        for (int j = 3; j < n; ++j) { size_t tmp; file >> tmp; }
    }
}



template<typename T>
size_t PolyMesh<T>::add_vertex(const PolyMesh<T>::Vertex& Vert) {
    size_t id = _v.cols();
    this->add_vertices({Vert});
    return id;
}



template<typename T>
void PolyMesh<T>::replace_vertex(size_t Id, const PolyMesh<T>::Vertex& Vert) {
    _v.col(Id) = Vert;
}



template<typename T>
void PolyMesh<T>::add_vertices(const std::vector<PolyMesh::Vertex>& Vertices) {
    // Build extending matrix
    Eigen::Matrix3X<T> v(3, Vertices.size());
    for(int i = 0; i < Vertices.size(); ++i) {
        v.col(i) = Vertices[i];
    }

    // expand and add extending matrix to our matrix
    int old_size = _v.cols();
    _v.conservativeResize(Eigen::NoChange, old_size + Vertices.size());
    _v.block(0, old_size, 3, v.cols()) = v;

}



template<typename T>
std::vector<size_t> PolyMesh<T>::get_Neighbors(size_t Vertex_Id) const {
    std::vector<size_t> neighbors;
    neighbors.reserve(8);

    for (size_t f = 0; f < _n_faces; ++f) {
        if (face_deleted(f)) continue;

        const auto& face = _f.row(f);

        for (int i = 0; i < 3; ++i) {
            if (face[i] == Vertex_Id) {
                for (int j = 0; j < 3; ++j) {
                    size_t n = face[j];
                    if (n != Vertex_Id)
                        neighbors.push_back(n);
                }
            }
        }
    }

    std::sort(neighbors.begin(), neighbors.end());
    neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());

    return neighbors;
}



template class PolyMesh<float>;
template class PolyMesh<double>;
};