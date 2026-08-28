#ifndef MY_ALLIGNED_ALLOCATOR_H
#define MY_ALLIGNED_ALLOCATOR_H
// ----------------------------------------------------------------------------
#include <vector>
#include <memory>
#include <new>
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {

// A simple C++20 custom allocator that forces 64-byte alignment
template <typename T, std::size_t Alignment = 64>
struct Aligned_Allocator {
    using value_type = T;

    // REQUIRED — without this, libstdc++'s allocator_traits::rebind_alloc
    // cannot instantiate this allocator for internal node types, and
    // std::vector fails to compile entirely (as it did in your original)
    template <typename U>
    struct rebind { using other = Aligned_Allocator<U, Alignment>; };

    Aligned_Allocator() noexcept = default;
    template <typename U> Aligned_Allocator(const Aligned_Allocator<U, Alignment>&) noexcept {}

    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        if (auto p = static_cast<T*>(::operator new[](n * sizeof(T), std::align_val_t{Alignment}, std::nothrow))) {
            return p;
        }
        throw std::bad_alloc();
    }
    void deallocate(T* p, std::size_t) noexcept {
        ::operator delete[](p, std::align_val_t{Alignment});
    }

    friend bool operator==(const Aligned_Allocator&, const Aligned_Allocator&) noexcept { return true; }
    friend bool operator!=(const Aligned_Allocator&, const Aligned_Allocator&) noexcept { return false; }
};

template <typename T, typename U, std::size_t Align>
bool operator==(const Aligned_Allocator<T, Align>&, const Aligned_Allocator<U, Align>&) { return true; }
template <typename T, typename U, std::size_t Align>
bool operator!=(const Aligned_Allocator<T, Align>&, const Aligned_Allocator<U, Align>&) { return false; }

};
#endif
