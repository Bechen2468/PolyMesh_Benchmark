#ifndef POLYMESH_V3_RELATIONFLAG_H
#define POLYMESH_V3_RELATIONFLAG_H
// ----------------------------------------------------------------------------
#include <cstdint>
#include <cstddef>
#include "PolyMesh/V3/Traits/Traits.h"
#include "PolyMesh/V3/DataStructs/D_Ragged_Matrix.h"
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {

    
enum class CacheFlag : uint8_t {
    NONE  = 0,
    VV_HE = 1 << 0,
    VF    = 1 << 1,
    EF = 1 << 2,
    ALL   = VV_HE | VF | EF
};

// bitwise operator override to combine flags
// Bitwise OR
inline constexpr CacheFlag operator|(CacheFlag a, CacheFlag b) noexcept {
    return static_cast<CacheFlag>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

// Bitwise AND
inline constexpr CacheFlag operator&(CacheFlag a, CacheFlag b) noexcept {
    return static_cast<CacheFlag>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

// Bitwise NOT
inline constexpr CacheFlag operator~(CacheFlag a) noexcept {
    return static_cast<CacheFlag>(~static_cast<std::uint8_t>(a));
}

// Compound Bitwise OR (|=)
inline constexpr CacheFlag& operator|=(CacheFlag& a, CacheFlag b) noexcept {
    return a = a | b;
}

// Compound Bitwise AND (&=)
inline constexpr CacheFlag& operator&=(CacheFlag& a, CacheFlag b) noexcept {
    return a = a & b;
}






template<typename Traits = Traits<std::uint32_t>>
struct RelationCache {
    using T = Traits;

private:
    CacheFlag _valid_flags = CacheFlag::NONE;

public:
    T::VV_HE VV_HE; // VV Relation as Halfedges
    T::VF VF;
    T::EF EF;
    

    // Fast status query
    [[nodiscard]] inline bool is_valid(CacheFlag Flag) const noexcept {
        return (_valid_flags & Flag) != CacheFlag::NONE;
    }
    
    inline void set_valid(CacheFlag Flag) noexcept {
        _valid_flags |= Flag;
    }

    // Called on every mesh modification
    inline void invalidate(CacheFlag flags = CacheFlag::ALL) noexcept {
        _valid_flags &= ~flags;
    }

    void collect_garbage() {
        if (!is_valid(CacheFlag::VV_HE)) VV_HE = typename T::VV_HE();
        if (!is_valid(CacheFlag::VF))    VF = typename T::VF();
        if (!is_valid(CacheFlag::EF))    EF = typename T::EF();
    }
};

};
#endif

