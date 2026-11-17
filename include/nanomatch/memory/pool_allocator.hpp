#pragma once
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace nm {

/// Pre-allocates a fixed-size block of objects to avoid malloc/new on the hot path.
/// Used for Order objects in the matching engine.
template <typename T>
class PoolAllocator {
public:
    /// Construct a pool with the given maximum capacity
    explicit PoolAllocator(size_t capacity) : pool_(capacity), free_list_(capacity) {
        // Initialise free list: every slot points to the next
        for (size_t i = 0; i < capacity; ++i) {
            free_list_[i] = i + 1;
        }
        free_head_ = 0;
        size_ = 0;
        capacity_ = capacity;
    }

    /// Allocate one object from the pool (O(1)), returns nullptr if exhausted
    T* allocate() {
        if (free_head_ >= capacity_) {
            return nullptr;  // Pool exhausted
        }
        size_t index = free_head_;
        free_head_ = free_list_[index];
        ++size_;
        return &pool_[index];
    }

    /// Return one object to the pool (O(1))
    void deallocate(T* ptr) {
        if (ptr < pool_.data() || ptr >= pool_.data() + capacity_) {
            throw std::out_of_range("PoolAllocator::deallocate: pointer is outside the pool");
        }
        size_t index = static_cast<size_t>(ptr - pool_.data());
        if (index >= capacity_) {
            throw std::out_of_range("PoolAllocator::deallocate: index out of range");
        }
        free_list_[index] = free_head_;
        free_head_ = index;
        --size_;
    }

    [[nodiscard]] size_t size() const {
        return size_;
    }
    [[nodiscard]] size_t capacity() const {
        return capacity_;
    }
    [[nodiscard]] bool full() const {
        return size_ >= capacity_;
    }

private:
    std::vector<T> pool_;            // Pre-allocated storage
    std::vector<size_t> free_list_;  // Free list (index -> next free index)
    size_t free_head_;               // Index of first free slot
    size_t size_;
    size_t capacity_;
};

}  // namespace nm
