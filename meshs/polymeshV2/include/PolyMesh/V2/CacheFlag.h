#ifndef CACHEFLAG_H
#define CACHEFLAG_H
// ----------------------------------------------------------------------------
#include <cstdint>
// ----------------------------------------------------------------------------



namespace PolyMesh::V2 {

enum class CacheFlag : uint8_t {
    VV    = 1 << 0,
    VF    = 1 << 1,
    EF    = 1 << 2,
    FE    = 1 << 3,
    Edges = 1 << 4,
    All   = VV | VF | EF | FE | Edges
};


// bitwise ops to combine flags
inline CacheFlag operator|(CacheFlag a, CacheFlag b) {
    return static_cast<CacheFlag>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
    );
}

inline bool operator&(CacheFlag a, CacheFlag b) {
    return static_cast<uint8_t>(a) & static_cast<uint8_t>(b);
}

};
#endif