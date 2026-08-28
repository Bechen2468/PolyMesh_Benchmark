#ifndef RELATIONBUILDER_H
#include "PolyMesh/V3/Relation_Matrices/RelationBuilder.h"
#endif
// ----------------------------------------------------------------------------  



namespace PolyMesh::V3 {

template<typename Traits>
void RelationBuilder::VV_HE::build_from_faces(typename Traits::Faces& Faces, const typename Traits::Index Vertex_Count, typename Traits::VV_HE& Result) {
    using T = Traits;
    using Index = typename T::Index;

    // get required sizes
    Index data_size = 0;
    std::vector<Index> v_edge_count(Vertex_Count, 0);

    for(auto f : Faces) {
        data_size += f.size();

        for(auto fv : f) {
            v_edge_count[fv]++;
        }
    }

    // raw Result data
    typename T::VV_HE::template Vec<Index> data(data_size);
    typename T::VV_HE::template Vec<typename T::VV_HE::Row> rows(Vertex_Count);

    // scale Result data
    // set block start indices
    for(Index i = 0, idx = 0; i < v_edge_count.size(); ++i) {
        rows[i].begin = idx;
        idx += v_edge_count[i];
    }

    // insert data
    for(auto fv : Faces) {

        // DEVNOTE: Not using just one insert with %, because Debug compiler does not optimze it to this below
        // insert end to begin edge
        auto& wrap_block = rows[fv[fv.size() - 1]];
        data[wrap_block.begin + wrap_block.size++] = fv[0];

        // insert the remaining edges
        Index prev = fv[0];
        for(size_t i = 1; i < fv.size(); ++i) {
            auto& row = rows[prev];
            data[row.begin + row.size++] = fv[i];
            prev = fv[i - 1];
        }
    }
    

    // build id_to_rawId
    typename T::VV_HE::template Vec<Index> id_to_rawId(Vertex_Count); 
    
    for(Index i = 0; i < Vertex_Count; ++i) {
        id_to_rawId[i] = i;
    }
    
    typename T::VV_HE::template Vec<Index> rawId_to_id = id_to_rawId;

    Result = typename T::VV_HE(std::move(data), std::move(rows), std::move(id_to_rawId), std::move(rawId_to_id), T::Reserve_Perz);
}




template<typename Traits>
void RelationBuilder::VV_HE::add_face(std::span<const typename Traits::Index> Face, typename Traits::VV_HE& Result) {
    using Index = typename Traits::Index;

    for(size_t i = 0; i < Face.size(); ++i) {
        Index v0 = Face[i];
        Index v1 = Face[(i+1) % Face.size()];
        Index new_len = Result.row_size(v0) + 1;

        Result.row_resize(v0, new_len);
        Result.get(v0, new_len - 1) = v1;
    }
}




template<typename Traits>
void RelationBuilder::VV_HE::delete_face(std::span<const typename Traits::Index> Face, typename Traits::VV_HE& Result) {
    using Index = typename Traits::Index;

    for(Index i = 0; i < Face.size(); ++i) {
        Index v0 = Face[i];
        Index v1 = Face[(i+1) % Face.size()];
        auto f = Result.row(v0);

        for(Index fi = 0; fi < f.size(); ++fi) {
            if(f[fi] == v1) {
                std::swap(f[fi], f[f.size() - 1]);
                Result.row_resize(v0, f.size() - 1);
                break;
            }
        }
    }
}






template<typename Traits>
typename Traits::VF RelationBuilder::VF::build_from_faces(typename Traits::Faces& Faces, const typename Traits::Index Vertex_Count) {
    using T = Traits;
    using Index = typename T::Index;
    using Mat = typename T::VF;
    using Scalar = typename T::SparseScalar;
    using Triplet = Eigen::Triplet<Scalar, Index>;

    const Index face_capacity = static_cast<Index>(Faces.capacity());

    Index total_entries = 0;
    for (auto face_row : Faces) {
        total_entries += face_row.size();
    }

    std::vector<Triplet> triplets;
    triplets.reserve(total_entries);

    for (auto face_row : Faces) {
        Index real_face_id = face_row.idx();
        for (Index vertex : face_row) {
            // row: vertex, column: face
            triplets.emplace_back(vertex, real_face_id, static_cast<Scalar>(1));
        }
    }

    Mat m(Vertex_Count, face_capacity);
    m.setFromTriplets(triplets.begin(), triplets.end());
    // DEVNOTE: setFromTriplets calls makeCompressed at the end!

    return m;
}


};