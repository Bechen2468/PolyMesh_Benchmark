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
    size_t v_count, f_count, e_count;
    file >> v_count >> f_count >> e_count;


    // resize matrix sizes
    _vertices.resize(3, v_count);
    _adjacency.resize(v_count, v_count);
    _face_vertices.resize(f_count, v_count);


    // get vertices
    for (size_t i = 0; i < v_count; ++i) {
        float x, y, z;
        file >> x >> y >> z;
        _vertices.col(i) << x, y, z;
        float d;
        for (int j = 0; j < 3 && file >> d; ++j);
    }


    std::vector<Eigen::Triplet<BoolType>> face_triplets;
    std::vector<Eigen::Triplet<BoolType>> adj_triplets;
    face_triplets.reserve(f_count * 3);  // approximate
    adj_triplets.reserve(f_count * 6);   // each triangle gives 3 edges



    // get faces
    for (size_t f = 0; f < f_count; ++f) {
        int n_vertices_in_face;
        file >> n_vertices_in_face;
        std::vector<int> face_indices(n_vertices_in_face);
        for (int j = 0; j < n_vertices_in_face; ++j) {
            file >> face_indices[j];
            face_triplets.emplace_back(f, face_indices[j], 1);
        }

        // build adjacency triplets (upper triangle)
        for (int j = 0; j < n_vertices_in_face; ++j) {
            int a = face_indices[j];
            int b = face_indices[(j+1)%n_vertices_in_face];
            if (a < b)
                adj_triplets.emplace_back(a, b, 1);
            else
                adj_triplets.emplace_back(b, a, 1);
        }
    }

    _face_vertices.setFromTriplets(face_triplets.begin(), face_triplets.end());
    // adjacency symmetric
    _adjacency.setFromTriplets(adj_triplets.begin(), adj_triplets.end());
    _adjacency = _adjacency + Eigen::SparseMatrix<uint8_t>(_adjacency.transpose()); // make symmetric

    _n_faces = f_count;
    _n_edges = _adjacency.nonZeros() / 2;
}



size_t PolyMesh::add_vertex(const PolyMesh::Vertex& Vert) {
    size_t id = _vertices.cols();
    this->add_vertices({Vert});
    return id;
}



void PolyMesh::replace_vertex(size_t Id, const PolyMesh::Vertex& Vert) {
    _vertices.col(Id) = Vert;
}



void PolyMesh::add_vertices(const std::vector<PolyMesh::Vertex>& Vertices) {
    // Build extending matrix
    Eigen::Matrix3Xf v(3, Vertices.size());
    for(int i = 0; i < Vertices.size(); ++i) {
        v.col(i) = Vertices[i];
    }

    // expand and add extending matrix to our matrix
    int old_size = _vertices.cols();
    _vertices.conservativeResize(Eigen::NoChange, old_size + Vertices.size());
    _vertices.block(0, old_size, 3, v.cols()) = v;

    // expand other matrices
    _adjacency.conservativeResize(n_vertices(), n_vertices());
    _face_vertices.conservativeResize(Eigen::NoChange, n_vertices());
}



void PolyMesh::add_edge(size_t I, size_t J) {
    _adjacency.coeffRef(I, J) = true;
    _adjacency.coeffRef(J, I) = true;
    _n_edges++;
    //_adjacency.coeffRef(std::min(I, J), std::max(I, J)) = true;       // upper triangle matrix
}



void PolyMesh::add_face(const std::vector<int>& Face) {
    // add edges
    for(int i = 0; i < Face.size() - 1; ++i) {
        this->add_edge(Face[i], Face[i + 1]);
    }
        this->add_edge(Face.back(), Face.front());
    

    // check if _face_vertices has enough rows for the new face
    if(_n_faces >= _face_vertices.rows()) {
        size_t new_rows = std::max(static_cast<size_t>(_face_vertices.rows()) * 2, _n_faces + 1);
        _face_vertices.conservativeResize(new_rows, _face_vertices.cols());
    }

    int f = _n_faces;      // new row index
    for (int v : Face) {
        // check if the vertex id exists
        if (v >= n_vertices()) throw std::runtime_error("Vertex index out of bounds in add_face");
        _face_vertices.insert(f, v) = true;
    }
    _n_faces++;
}



std::vector<int> PolyMesh::get_Neighbors(size_t Vertex_Id) const {
    std::vector<int> n;

    for (Eigen::SparseMatrix<BoolType>::InnerIterator it(_adjacency, Vertex_Id); it; ++it)
        n.push_back(it.row());
    return n;
}



bool PolyMesh::edge_exists(size_t I, size_t J) const {
    return _adjacency.coeff(I, J);
    // return _adjacency.coeff(std::min(I, J), std::max(I, J))      // upper triangle matrix 
}


Eigen::Matrix<size_t, 2, Eigen::Dynamic> PolyMesh::edges() {
    if(!_edges_build) _build_edges();

    return _edges;
}


void PolyMesh::_build_edges() {
    _edges.resize(2, n_edges());
}



// ----------------------------------------------------------------------------  
// Iterators
// ----------------------------------------------------------------------------  

PolyMesh::VIter::VIter(const PolyMesh* Mesh, size_t Id):
_mesh(Mesh),
id(Id)
{
    _skip_deleted();
}


PolyMesh::VIter::~VIter() {
    _mesh = nullptr;
}


inline PolyMesh::VIter& PolyMesh::VIter::operator++() {
    ++id;
    _skip_deleted();
    return *this;
}


void PolyMesh::VIter::_skip_deleted() {
    const size_t size = _mesh->n_vertices();
    while(id < size && _mesh->vertex_deleted(id)) ++id;
}





PolyMesh::FIter::FIter(const PolyMesh* Mesh, size_t Id):
_mesh(Mesh),
id(Id)
{
    _skip_deleted();
}


PolyMesh::FIter::~FIter() {
    _mesh = nullptr;
}


inline PolyMesh::FIter& PolyMesh::FIter::operator++() {
    ++id;
    _skip_deleted();
    return *this;
}


void PolyMesh::FIter::_skip_deleted() {
    const size_t size = _mesh->n_faces();
    while(id < size && _mesh->face_deleted(id)) ++id;
}
