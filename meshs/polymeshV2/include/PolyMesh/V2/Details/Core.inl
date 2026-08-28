#pragma once
// is never resolved true, just for intellicode linking
#ifndef POLY_MESH_V2_H
#include "PolyMesh/V2/Mesh.h"
#endif


namespace PolyMesh::V2 {


template<typename ScalarT, typename IndexT>
PolyMesh<ScalarT, IndexT>::PolyMesh():
_v_valid(0)
{

}



template<typename ScalarT, typename IndexT>
PolyMesh<ScalarT, IndexT>::~PolyMesh() {
    //_relations.invalidate(CacheFlag::All);
}


template<typename ScalarT, typename IndexT>
void PolyMesh<ScalarT, IndexT>::read_file(const std::string& File_Path) {
    std::ifstream file(File_Path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open \"" << File_Path << "\"\n";
        return;
    }

    std::string line;
    std::getline(file, line);
    if(line.rfind("OFF", 0) != std::string::npos)
        std::getline(file, line);

    IndexT v_count, f_count, e_count;
    std::istringstream(line) >> v_count >> f_count >> e_count;

    _v.resize(v_count, 3);          // RowMajor: rows=vertices, cols=xyz
    _f = FaceMatrix(0);
    _v_valid = ValidMask(v_count);
    _n_vertices = v_count;

    for (IndexT i = 0; i < v_count; ++i) {
        file >> _v(i, 0) >> _v(i, 1) >> _v(i, 2);
    }

    for(IndexT fi = 0; fi < f_count; fi++) {
        uint8_t n;
        file >> n;
        std::vector<IndexT> verts(n);
        for(int j = 0; j < n; j++) file >> verts[j];
        add_face(verts.data(), n);
    }
}







template<typename ScalarT, typename IndexT>
const VVMatrix<IndexT>& PolyMesh<ScalarT, IndexT>::VV() {
    if(!_relations.has_VV) _build_VV();
    return _relations.VV;
}


template<typename ScalarT, typename IndexT>
const VFMatrix<IndexT>& PolyMesh<ScalarT, IndexT>::VF() {
    if(!_relations.has_VF) _build_VF();
    return _relations.VF;
}


template<typename ScalarT, typename IndexT>
const EFMatrix<IndexT>& PolyMesh<ScalarT, IndexT>::EF() {
    if(!_relations.has_EF) _build_EF();
    return _relations.EF;
}


template<typename ScalarT, typename IndexT>
const FEMatrix<IndexT>& PolyMesh<ScalarT, IndexT>::FE() {
    if(!_relations.has_FE) _build_FE();
    return _relations.FE;
}

template<typename ScalarT, typename IndexT>
const EdgeMatrix<IndexT>& PolyMesh<ScalarT, IndexT>::edges() {
    if(!_relations.has_edges) _build_edges();
    return _relations.edges;
}



template<typename ScalarT, typename IndexT>
void PolyMesh<ScalarT, IndexT>::_build_VV() {
    const IndexT nv = _n_vertices;

    std::vector<uint64_t> edges;
    edges.reserve(_n_faces * 6);

    // walk valid faces via linked list
    IndexT fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const IndexT* fv   = _f.face_begin(fi);
        const IndexT  fsize = _f.face_size(fi);

        // directed edges for all face edges
        for(IndexT i = 0; i < fsize; i++) {
            IndexT a = fv[i];
            IndexT b = fv[(i + 1) % fsize];
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


template<typename ScalarT, typename IndexT>
void PolyMesh<ScalarT, IndexT>::_build_VF() {
    // count faces per vertex — for row reservation
    std::vector<int> counts(_n_vertices, 0);
    IndexT fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const IndexT* fv    = _f.face_begin(fi);
        const IndexT  fsize = _f.face_size(fi);
        for(IndexT i = 0; i < fsize; i++) counts[fv[i]]++;
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
        const IndexT* fv    = _f.face_begin(fi);
        const IndexT  fsize = _f.face_size(fi);
        for(IndexT i = 0; i < fsize; i++)
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



template<typename ScalarT, typename IndexT>
void PolyMesh<ScalarT, IndexT>::_build_edges() {
    // count canonical edges first
    IndexT count = 0;
    IndexT fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const IndexT* fv    = _f.face_begin(fi);
        const IndexT  fsize = _f.face_size(fi);
        for(IndexT i = 0; i < fsize; i++) {
            IndexT a = fv[i], b = fv[(i + 1) % fsize];
            if(a < b) count++;
        }
        fi = _f.valid.next[fi];
    }

    EdgeMatrix<IndexT> mat(count, 2);
    IndexT row = 0;

    fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const IndexT* fv    = _f.face_begin(fi);
        const IndexT  fsize = _f.face_size(fi);
        for(IndexT i = 0; i < fsize; i++) {
            IndexT a = fv[i], b = fv[(i + 1) % fsize];
            if(a < b) { mat(row, 0) = a; mat(row, 1) = b; row++; }
        }
        fi = _f.valid.next[fi];
    }

    _relations.edges     = std::move(mat);
    _relations.has_edges = true;
}



template<typename ScalarT, typename IndexT>
void PolyMesh<ScalarT, IndexT>::_build_EF() {
    if(!_relations.has_edges) _build_edges();
    auto& emat    = _relations.edges;
    const IndexT ne = emat.rows();
    const IndexT nf = _n_faces;

    std::unordered_map<IndexT, IndexT> edge_to_idx;
    edge_to_idx.reserve(ne);
    for(IndexT ei = 0; ei < ne; ei++)
        edge_to_idx[emat(ei, 0) * _n_vertices + emat(ei, 1)] = ei;

    std::vector<int> counts(ne, 0);
    IndexT fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const IndexT* fv    = _f.face_begin(fi);
        const IndexT  fsize = _f.face_size(fi);
        for(IndexT i = 0; i < fsize; i++) {
            IndexT a = fv[i], b = fv[(i + 1) % fsize];
            if(a > b) std::swap(a, b);
            counts[edge_to_idx[a * _n_vertices + b]]++;
        }
        fi = _f.valid.next[fi];
    }

    _relations.EF.resize(ne, nf);
    _relations.EF.reserve(Eigen::VectorXi::Map(counts.data(), ne));

    fi = _f.valid.begin;
    while(fi != _f.valid.end) {
        const IndexT* fv    = _f.face_begin(fi);
        const IndexT  fsize = _f.face_size(fi);
        for(IndexT i = 0; i < fsize; i++) {
            IndexT a = fv[i], b = fv[(i + 1) % fsize];
            if(a > b) std::swap(a, b);
            _relations.EF.insert(edge_to_idx[a * _n_vertices + b], fi) = 1;
        }
        fi = _f.valid.next[fi];
    }

    _relations.EF.makeCompressed();
    _relations.has_EF = true;
}



template<typename ScalarT, typename IndexT>
void PolyMesh<ScalarT, IndexT>::_build_FE() {
    if(!_relations.has_edges) _build_edges();
    auto& emat    = _relations.edges;
    const IndexT ne = emat.rows();
    const IndexT nf = _n_faces;

    std::unordered_map<IndexT,IndexT> edge_to_idx;
    edge_to_idx.reserve(ne);
    for(IndexT ei = 0; ei < ne; ei++)
        edge_to_idx[emat(ei,0) * _n_vertices + emat(ei,1)] = ei;

    // count edges per face for reservation
    std::vector<int> counts(nf, 0);
    IndexT fi = _f.valid.begin;
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
        const IndexT* fv    = _f.face_begin(fi);
        const IndexT  fsize = _f.face_size(fi);

        std::vector<int> cols;
        cols.reserve(fsize);
        for(IndexT i = 0; i < fsize; i++) {
            IndexT a = fv[i], b = fv[(i+1) % fsize];
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
template class PolyMesh<float, std::uint64_t>;
template class PolyMesh<double, std::uint64_t>;
};