#include "poly_mesh/poly_mesh.h"



PolyMesh::PolyMesh() {

}



PolyMesh::~PolyMesh() {

}



void PolyMesh::read_file(std::string File_Path) {
    std::ifstream file(File_Path);

    // if file cant be opened
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open the file \"" << File_Path << "\"\n";
        return;
    }

    std::string line;
    std::getline(file, line);

    // check for OFF preline; get next line if true
    if(line.find_first_of("OFF") != std::string::npos) std::getline(file, line);

    // get object counts
    int v_count, f_count, e_count;
    sscanf(line.c_str(), "%d %d %d", &v_count, &f_count, &e_count);


    // resize matrix sizes
    _v.resize(3, v_count);
    _adjacency.resize(v_count, v_count);


    // get vertices
    for(int i = 0, v_cur = 0; i < v_count; ++i) {
        std::getline(file, line);

        // check for an empty or comment line
        if(line.empty() || line[0] == '#') {
            i--;
            continue;
        }

        // convert line into space split values
        std::istringstream iss(line);
        PolyMesh::Vertex v;
        PolyMesh::Vertex normal;
        float val;
        iss>>val;
        for(int i = 0; i < 6 && iss>>val; ++i) {
            if(i < 3) v[i] = val;
            else normal[i-3] = val;
        }

        // push vertex into matrix
        this->change_vertex(v_cur++, v);
    }


    // get faces
    for(int i = 0, f_cur = 0; i < f_count; i++) {
        std::getline(file, line);

        // check for an empty or comment line
        if(line.empty() || line[0] == '#') {
            i--;
            continue;
        }

        // convert line into space split values
        std::istringstream iss(line);
        std::vector<int> face;
        float val;
        while (iss >> val) {
            face.push_back(val);
        }

        // push face
        this->add_face(face);
    }

    _adjacency.prune([](int, int, const bool &val) { return val; });  // Just in case, making sure only our values are present / for savety
}



int PolyMesh::add_vertex(const PolyMesh::Vertex& Vert) {
    int id = _v.cols();
    this->add_vertices({Vert});
    return id;
}



void PolyMesh::change_vertex(const int& Id, const PolyMesh::Vertex& Vert) {
    _v.col(Id) = Vert;
}



void PolyMesh::add_vertices(const std::vector<PolyMesh::Vertex>& Vertices) {
    // Build extending matrix
    Eigen::Matrix3Xf v(3, Vertices.size());
    for(int i = 0; i < Vertices.size(); ++i) {
        v.col(i) = Vertices[i];
    }

    // expand and add extending matrix to our matrix
    int old_size = _v.cols();
    _v.conservativeResize(Eigen::NoChange, old_size + Vertices.size());
    _v.block(0, old_size, 3, v.cols()) = v;
}



void PolyMesh::add_edge(const int& I, const int& J) {
    _adjacency.coeffRef(I, J) = true;
    _adjacency.coeffRef(J, I) = true;
    //_adjacency.coeffRef(std::min(I, J), std::max(I, J)) = true;       // upper triangle matrix
}



void PolyMesh::add_face(const std::vector<int>& Face) {
    for(int i = 0; i < Face.size() - 1; ++i) {
        this->add_edge(Face[i], Face[i + 1]);
    }
        this->add_edge(Face.back(), Face.front());
}



PolyMesh::Vertex PolyMesh::get_vertex(const int& Id) const {
    return _v.col(Id);
}



std::vector<int> PolyMesh::get_Neighbors(const int& Vertex_Id) const {
    std::vector<int> n;

    for (Eigen::SparseMatrix<bool>::InnerIterator it(_adjacency, Vertex_Id); it; ++it)
        n.push_back(it.row());
    return n;
}



size_t PolyMesh::get_vertex_count() const {
    return _v.cols();
}



bool PolyMesh::edge_exists(const int& I, const int& J) const {
    return _adjacency.coeff(I, J);
    // return _adjacency.coeff(std::min(I, J), std::max(I, J))      // upper triangle matrix 
}




Eigen::Matrix3Xf& PolyMesh::get_v() {
    return _v;
}



Eigen::SparseMatrix<bool>& PolyMesh::get_adjacency() {
    return _adjacency;
}





