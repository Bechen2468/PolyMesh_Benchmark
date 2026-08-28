#pragma once
// ----------------------------------------------------------------------------
#include <vector>
// ----------------------------------------------------------------------------

namespace PolyMesh::V3 {

template<typename IndexT>
struct ValidMask {
    bool has_invalids;
    std::vector<uint8_t> valid;
    std::vector<IndexT> next;
    std::vector<IndexT> prev;
    IndexT begin;
    IndexT end;

    std::vector<IndexT> _freeStack;

    ValidMask(IndexT Size):
        valid(Size + 1, 1),   // +1 for end entry (invalid end)
        next(Size + 1),
        prev(Size + 1),
        begin(0),
        end(Size)             // invalid end at Size, always invalid
    {
        valid[Size] = 0;      // invalid start is always "invalid" but used as terminator    

        if (Size == 0) {
            next[0] = 0;
            prev[0] = 0;
            return; 
        }

        for(IndexT i = 0; i < Size; i++) {
            next[i] = i + 1;
            prev[i] = i == 0 ? 0 : i - 1;
        }
        next[Size] = Size;    // invalid end points to itself
        prev[Size] = Size - 1;
    }

    inline bool is_valid(IndexT Idx) const { return valid[Idx]; }

    inline void set_invalid(IndexT Idx) {
        valid[Idx] = false;
        next[prev[Idx]] = next[Idx];
        prev[next[Idx]] = prev[Idx];
        _freeStack.push_back(Idx);
    }

    IndexT get_free() {
        IndexT i;
        if(!_freeStack.empty()) {
            i = _freeStack.back();
            _freeStack.pop_back();
        } else {
            i = valid.size() - 1;
            valid.insert(valid.end() - 1, 1);
            next.insert(next.end() - 1, 0);
            prev.insert(prev.end() - 1, 0);
            end = valid.size() - 1;
        }

        // relink into list after current end
        IndexT last = prev[end];  // last valid element
        valid[i] = 1;
        next[last] = i;
        prev[i] = last;
        next[i] = end;
        prev[end]  = i;
        return i;
    }
};

};