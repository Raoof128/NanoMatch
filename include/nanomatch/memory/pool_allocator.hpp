#pragma once
#include <vector>
#include <cstddef>
#include <cassert>

namespace nm {

// Pre-allocates a block of objects to avoid malloc/new on the hot path.
// Used for Order objects in the matching engine.
template <typename T>
class PoolAllocator {
public:
    explicit PoolAllocator(size_t capacity)
        : pool_(capacity), free_list_(capacity) {
        // Initialise free list: every slot points to the next
        for (size_t i = 0; i < capacity; ++i) {
            free_list_[i] = i + 1;
        }
        free_head_ = 0;
        size_ = 0;
        capacity_ = capacity;
    }

    // Allocate one object — O(1)
    T* allocate() {
        if (free_head_ >= capacity_) {
            return nullptr;  // Pool exhausted
        }
        size_t index = free_head_;
        free_head_ = free_list_[index];
        ++size_;
        return &pool_[index];
    }

    // Deallocate one object — O(1)
    void deallocate(T* ptr) {
        size_t index = static_cast<size_t>(ptr - pool_.data());
        assert(index < capacity_);
        free_list_[index] = free_head_;
        free_head_ = index;
        --size_;
    }

    [[nodiscard]] size_t size()     const { return size_; }
    [[nodiscard]] size_t capacity() const { return capacity_; }
    [[nodiscard]] bool   full()     const { return size_ >= capacity_; }

private:
    std::vector<T>      pool_;       // Pre-allocated storage
    std::vector<size_t> free_list_;  // Free list (index → next free index)
    size_t              free_head_;  // Index of first free slot
    size_t              size_;
    size_t              capacity_;
};

} // namespace nm
