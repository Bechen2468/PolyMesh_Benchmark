#ifndef POLYMESH_V3_TRAITS
#define POLYMESH_V3_TRAITS
// ----------------------------------------------------------------------------
#include <cstdint>
#include <vector>
#include <span>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "PolyMesh/V3/DataStructs/Pool_Matrix.h"
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
#include "PolyMesh/V3/Traits/Faces.h"
#include "PolyMesh/V3/Traits/Vertices.h"
#include "PolyMesh/V3/Traits/EF.h"
#include "PolyMesh/V3/Traits/VV_HE.h"
#include "PolyMesh/V3/Traits/VF.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {
    
    
// checks private friend accessable function requirements
struct _Traits_InternalC {
    template<typename T>
    static constexpr bool check_internal_requirements() {
        return requires {
            requires Vertices_InternalC<typename T::Vertices>;
            requires Faces_InternalC<typename T::Faces>;
            requires EF_InternalC<typename T::EF>;
            requires VV_HE_InternalC<typename T::VV_HE>;
            requires VF_InternalC<typename T::VF>;
        };
    }
};


template<typename T>
concept TraitsC = requires {
    typename T::Scalar;
    typename T::Index;

    typename T::Vertex;
    typename T::Face;
    typename T::Edge;

    typename T::Vertices;
    typename T::Faces;

    typename T::VV_HE;
    typename T::EF;
    typename T::VF;
    
    { T::INDEX_INVALID } -> std::convertible_to<typename T::Index>;

    { T::Reserve_Perz } -> std::convertible_to<float>;
    requires std::same_as<std::decay_t<decltype(T::Reserve_Perz)>, float>;

    typename T::template Rebind<_Traits_InternalC>;
}
&& Vertices_ExternalC<typename T::Vertices>
&& Faces_ExternalC<typename T::Faces>
&& EF_ExternalC<typename T::EF>
&& VV_HE_ExternalC<typename T::VV_HE>
&& VF_ExternalC<typename T::VF>
&& _Traits_InternalC::check_internal_requirements<typename T::template Rebind<_Traits_InternalC>>();












template<typename ScalarT, typename IndexT = std::uint32_t, typename BinSizeT = std::uint8_t, typename MeshT = void>
struct Traits {
    template<typename NewMeshT>
    using Rebind = Traits<ScalarT, IndexT, BinSizeT, NewMeshT>;   // for rebinding the friend object types
protected:
    friend MeshT;


public:
    using Scalar = ScalarT;
    using SparseScalar = std::uint8_t;

    using Index = IndexT;
    static constexpr Index INDEX_INVALID = std::numeric_limits<Index>::max();
    using Signed_Index = typename std::make_signed<Index>;

    using Vertices = Vertices_Pool_Matrix<ScalarT, IndexT, MeshT>;
    using Faces = Faces_Ragged_Matrix<IndexT, BinSizeT, MeshT>;
    //using Faces = Faces_Vec_Vec_Matrix<IndexT, BinSizeT, MeshT>;

    using Vertex = std::array<Scalar, 3>;
    using Face = std::vector<Index>;
    
    using VV_HE = VV_HE_Ragged_Matrix<IndexT, BinSizeT, MeshT>;
    using VF = VF_Ragged_Matrix<IndexT, BinSizeT, MeshT>;

    using EF = EF_Ragged_Matrix<IndexT, BinSizeT, MeshT>;
    using Edge = EF::Edge;


    static constexpr float Reserve_Perz = 0.25f;
};

};

#endif