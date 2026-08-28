#ifndef RELATIONBUILDER_H
#define RELATIONBUILDER_H
// ----------------------------------------------------------------------------
#include <span>
#include <Eigen/Sparse>
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
#include "PolyMesh/V3/Traits/Traits.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {

struct RelationBuilder {
public:
    struct VV_HE {
        template<typename Traits = Traits<std::uint32_t>>
        static void build_from_faces(typename Traits::Faces& Faces, const typename Traits::Index Vertex_Count, typename Traits::VV_HE& Result);
        
        
        template<typename Traits = Traits<std::uint32_t>>
        static void add_face(std::span<const typename Traits::Index> Face, typename Traits::VV_HE& Result);
        
        template<typename Traits = Traits<std::uint32_t>>
        static void delete_face(std::span<const typename Traits::Index> Face, typename Traits::VV_HE& Result);
    };
    
    
    struct VF {
        template<typename Traits = Traits<std::uint32_t>>
        static typename Traits::VF build_from_faces(typename Traits::Faces& Faces, const typename Traits::Index Vertex_Count);
    };

};

};

#include "PolyMesh/V3/Relation_Matrices/RelationBuilder.inl"
#endif


