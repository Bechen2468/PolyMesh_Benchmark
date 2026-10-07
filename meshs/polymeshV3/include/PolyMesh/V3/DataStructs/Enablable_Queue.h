#ifndef ENABLABLE_QUEUE_H
#define ENABLABLE_QUEUE_H
// ----------------------------------------------------------------------------
#include <queue>
// ----------------------------------------------------------------------------



namespace PolyMesh::V3 {

    
// ----------------------------------------------------------------------------
// Queue Type
// If set false:    removes the queue itself and never uses it ( for new assigned vs preexisting ids )
// ----------------------------------------------------------------------------
template<bool EnableT, typename IndexT>
struct Enablable_Queue {
    std::queue<IndexT> q;
    [[nodiscard]] bool empty() const { return q.empty(); }
    void pop() { q.pop(); }
    [[nodiscard]] IndexT front() { return q.front(); }
    void push(IndexT i) { q.push(i); }
};


template<typename IndexT>
struct Enablable_Queue<false, IndexT> {
    [[nodiscard]] bool empty() const { return true; }
    void pop() {}
    [[nodiscard]] IndexT front() { return IndexT{}; }
    void push(IndexT) {}
};


};
#endif