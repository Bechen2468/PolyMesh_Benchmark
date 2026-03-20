#ifndef ALLOC_OBSERVER_H
#define ALLOC_OBSERVER_H
// EXPERIMENTAL

#include <cstddef>
#include <cstdlib>


struct Alloc_Observer {
    // DOES NOT DECREMENT !!
    inline static size_t Counter = 0;
};


void* operator new(size_t size);
void operator delete(void* ptr) noexcept;

#endif