#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

namespace nm {

/// A fixed-capacity pool of pre-built objects, handed out and taken back in O(1).
///
/// This is a standalone component: the order book does not use it (its queues are
/// std::list and allocate from the heap). It is not a standard-library allocator.
/// All `capacity` objects are default-constructed up front and live until the pool is
/// destroyed; allocate() and deallocate() hand slots out and take them back without
/// running constructors or destructors, so a reused slot still holds its previous value.
template <typename T>
class PoolAllocator {
public:
    /// Construct a pool with the given maximum capacity
    explicit PoolAllocator(size_t capacity)
        : pool_(capacity), free_list_(capacity), in_use_(capacity, 0) {
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
        in_use_[index] = 1;
        ++size_;
        return &pool_[index];
    }

    /// Return one object to the pool (O(1)).
    ///
    /// Throws std::out_of_range for a pointer outside the pool (including nullptr) and
    /// std::logic_error for a slot that is not currently allocated, which covers a
    /// second deallocate of the same pointer. The pool is unchanged when it throws.
    void deallocate(T* ptr) {
        // std::less gives a total order even for pointers into different objects
        const T* first = pool_.data();
        const T* last = first + capacity_;
        if (capacity_ == 0 || std::less<const T*>{}(ptr, first) ||
            !std::less<const T*>{}(ptr, last)) {
            throw std::out_of_range("PoolAllocator::deallocate: pointer is outside the pool");
        }
        size_t index = static_cast<size_t>(ptr - first);
        if (in_use_[index] == 0) {
            throw std::logic_error("PoolAllocator::deallocate: slot is not allocated");
        }
        in_use_[index] = 0;
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
    std::vector<uint8_t> in_use_;    // 1 while a slot is handed out
    size_t free_head_;               // Index of first free slot
    size_t size_;
    size_t capacity_;
};

}  // namespace nm
