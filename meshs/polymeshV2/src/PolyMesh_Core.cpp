#include "PolyMesh/V2/PolyMesh.h"


namespace PolyMesh::V2 {


template<typename T>
PolyMesh<T>::PolyMesh():
_v_valid(0)
{

}



template<typename T>
PolyMesh<T>::~PolyMesh() {
    //_relations.invalidate(CacheFlag::All);
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

    _v.resize(v_count, 3);          // RowMajor: rows=vertices, cols=xyz
    _f = FaceMatrix(f_count);       // RowMajor: rows=faces,    cols=v0v1v2
    _v_valid = ValidMask(v_count);
    _n_vertices = v_count;
    _n_faces = f_count;

    for (size_t i = 0; i < v_count; ++i) {
        file >> _v(i, 0) >> _v(i, 1) >> _v(i, 2);
    }

    for(size_t fi = 0; fi < f_count; fi++) {
        int n;
        file >> n;
        std::vector<size_t> verts(n);
        for(int j = 0; j < n; j++) file >> verts[j];
        _f.add_face(verts.data(), n);
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
const VVMatrix& PolyMesh<T>::VV() {
    if(!_relations.has_VV) _build_VV();
    return _relations.VV;
}


template<typename T>
const VFMatrix& PolyMesh<T>::VF() {
    if(!_relations.has_VF) _build_VF();
    return _relations.VF;
}


template<typename T>
const EFMatrix& PolyMesh<T>::EF() {
    if(!_relations.has_EF) _build_EF();
    return _relations.EF;
}


template<typename T>
const FEMatrix& PolyMesh<T>::FE() {
    if(!_relations.has_FE) _build_FE();
    return _relations.FE;
}

template<typename T>
const EdgeMatrix& PolyMesh<T>::edges() {
    if(!_relations.has_edges) _build_edges();
    return _relations.edges;
}



template<typename T>
void PolyMesh<T>::_build_VV() {
    const size_t nv = _n_vertices;

    std::vector<uint64_t> edges;
    edges.reserve(_n_faces * 6);

    // walk valid faces via linked list
    size_t fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const size_t* fv   = _f.face_begin(fi);
        const size_t  fsize = _f.face_size(fi);

        // directed edges for all face edges
        for(size_t i = 0; i < fsize; i++) {
            size_t a = fv[i];
            size_t b = fv[(i + 1) % fsize];
            edges.push_back((uint64_t)a << 32 | b);
            edges.push_back((uint64_t)b << 32 | a);
        }

        fi = _f.valid.next[fi];
    }

    // deduplicate
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    // sort col-major for ColMajor insert order
    std::sort(edges.begin(), edges.end(), [](uint64_t a, uint64_t b) {
        uint32_t ca = (uint32_t)(a & 0xFFFFFFFF);
        uint32_t cb = (uint32_t)(b & 0xFFFFFFFF);
        if(ca != cb) return ca < cb;
        return (uint32_t)(a >> 32) < (uint32_t)(b >> 32);
    });

    // count per column
    std::vector<int> col_counts(nv, 0);
    for(uint64_t e : edges)
        col_counts[(uint32_t)(e & 0xFFFFFFFF)]++;

    _relations.VV.resize(nv, nv);
    _relations.VV.reserve(Eigen::VectorXi::Map(col_counts.data(), nv));

    for(uint64_t e : edges)
        _relations.VV.insert((int)(e >> 32), (int)(e & 0xFFFFFFFF)) = 1;

    _relations.VV.makeCompressed();
    _relations.has_VV = true;
}


template<typename T>
void PolyMesh<T>::_build_VF() {
    // count faces per vertex — for row reservation
    std::vector<int> counts(_n_vertices, 0);
    size_t fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const size_t* fv    = _f.face_begin(fi);
        const size_t  fsize = _f.face_size(fi);
        for(size_t i = 0; i < fsize; i++) counts[fv[i]]++;
        fi = _f.valid.next[fi];
    }

    _relations.VF.resize(_n_vertices, _n_faces);
    _relations.VF.reserve(Eigen::VectorXi::Map(counts.data(), _n_vertices));

    // RowMajor: insert in row-major order (sort by vertex, then face)
    // walk faces and insert — rows may be out of order so use a sort
    std::vector<std::pair<int,int>> entries;
    entries.reserve(_n_faces * 4);
    fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const size_t* fv    = _f.face_begin(fi);
        const size_t  fsize = _f.face_size(fi);
        for(size_t i = 0; i < fsize; i++)
            entries.emplace_back((int)fv[i], (int)fi);
        fi = _f.valid.next[fi];
    }
    // sort by row (vertex) for RowMajor insertion order
    std::sort(entries.begin(), entries.end());
    for(auto& [row, col] : entries)
        _relations.VF.insert(row, col) = 1;

    _relations.VF.makeCompressed();
    _relations.has_VF = true;
}



template<typename T>
void PolyMesh<T>::_build_edges() {
    // count canonical edges first
    size_t count = 0;
    size_t fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const size_t* fv    = _f.face_begin(fi);
        const size_t  fsize = _f.face_size(fi);
        for(size_t i = 0; i < fsize; i++) {
            size_t a = fv[i], b = fv[(i + 1) % fsize];
            if(a < b) count++;
        }
        fi = _f.valid.next[fi];
    }

    EdgeMatrix mat(count, 2);
    size_t row = 0;

    fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const size_t* fv    = _f.face_begin(fi);
        const size_t  fsize = _f.face_size(fi);
        for(size_t i = 0; i < fsize; i++) {
            size_t a = fv[i], b = fv[(i + 1) % fsize];
            if(a < b) { mat(row, 0) = a; mat(row, 1) = b; row++; }
        }
        fi = _f.valid.next[fi];
    }

    _relations.edges     = std::move(mat);
    _relations.has_edges = true;
}



template<typename T>
void PolyMesh<T>::_build_EF() {
    if(!_relations.has_edges) _build_edges();
    auto& emat    = _relations.edges;
    const size_t ne = emat.rows();
    const size_t nf = _n_faces;

    std::unordered_map<size_t, size_t> edge_to_idx;
    edge_to_idx.reserve(ne);
    for(size_t ei = 0; ei < ne; ei++)
        edge_to_idx[emat(ei, 0) * _n_vertices + emat(ei, 1)] = ei;

    std::vector<int> counts(ne, 0);
    size_t fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const size_t* fv    = _f.face_begin(fi);
        const size_t  fsize = _f.face_size(fi);
        for(size_t i = 0; i < fsize; i++) {
            size_t a = fv[i], b = fv[(i + 1) % fsize];
            if(a > b) std::swap(a, b);
            counts[edge_to_idx[a * _n_vertices + b]]++;
        }
        fi = _f.valid.next[fi];
    }

    _relations.EF.resize(ne, nf);
    _relations.EF.reserve(Eigen::VectorXi::Map(counts.data(), ne));

    fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const size_t* fv    = _f.face_begin(fi);
        const size_t  fsize = _f.face_size(fi);
        for(size_t i = 0; i < fsize; i++) {
            size_t a = fv[i], b = fv[(i + 1) % fsize];
            if(a > b) std::swap(a, b);
            _relations.EF.insert(edge_to_idx[a * _n_vertices + b], fi) = 1;
        }
        fi = _f.valid.next[fi];
    }

    _relations.EF.makeCompressed();
    _relations.has_EF = true;
}



template<typename T>
void PolyMesh<T>::_build_FE() {
    if(!_relations.has_edges) _build_edges();
    auto& emat    = _relations.edges;
    const size_t ne = emat.rows();
    const size_t nf = _n_faces;

    std::unordered_map<size_t,size_t> edge_to_idx;
    edge_to_idx.reserve(ne);
    for(size_t ei = 0; ei < ne; ei++)
        edge_to_idx[emat(ei,0) * _n_vertices + emat(ei,1)] = ei;

    // count edges per face for reservation
    std::vector<int> counts(nf, 0);
    size_t fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        counts[fi] = (int)_f.face_size(fi);
        fi = _f.valid.next[fi];
    }

    _relations.FE.resize(nf, ne);
    _relations.FE.reserve(Eigen::VectorXi::Map(counts.data(), nf));

    // RowMajor: insert row by row — face order is already row order
    // but within each face, cols (edges) must be sorted
    fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const size_t* fv    = _f.face_begin(fi);
        const size_t  fsize = _f.face_size(fi);

        std::vector<int> cols;
        cols.reserve(fsize);
        for(size_t i = 0; i < fsize; i++) {
            size_t a = fv[i], b = fv[(i+1) % fsize];
            if(a > b) std::swap(a, b);
            cols.push_back((int)edge_to_idx[a * _n_vertices + b]);
        }
        std::sort(cols.begin(), cols.end());  // RowMajor needs sorted cols
        for(int col : cols)
            _relations.FE.insert((int)fi, col) = 1;

        fi = _f.valid.next[fi];
    }

    _relations.FE.makeCompressed();
    _relations.has_FE = true;
}



template class PolyMesh<float>;
template class PolyMesh<double>;
};