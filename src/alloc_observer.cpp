#include "alloc_observer.h"


void* operator new(size_t size) {
    Alloc_Observer::Counter += size;
    return malloc(size);
}

void operator delete(void* ptr) noexcept {
    free(ptr);
}