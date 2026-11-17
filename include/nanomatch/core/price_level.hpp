#pragma once
#include "nanomatch/core/order.hpp"

#include <iterator>
#include <list>

namespace nm {

/// A single price level in the order book, holding a FIFO queue of orders
/// at the same price. Supports O(1) insert and removal via std::list.
class PriceLevel {
public:
    explicit PriceLevel(Price price) : price_(price), total_quantity_(0) {}

    [[nodiscard]] Price price() const {
        return price_;
    }
    [[nodiscard]] TotalQuantity total_quantity() const {
        return total_quantity_;
    }
    [[nodiscard]] size_t order_count() const {
        return orders_.size();
    }
    [[nodiscard]] bool empty() const {
        return orders_.empty();
    }

    /// Iterator type for external reference (used by order lookup map)
    using OrderIterator = std::list<Order>::iterator;
    using ConstOrderIterator = std::list<Order>::const_iterator;

    /// Add order to back of queue (FIFO), returns iterator to the new order
    OrderIterator add_order(Order order) {
        total_quantity_ += order.remaining_quantity;
        orders_.push_back(std::move(order));
        return std::prev(orders_.end());
    }

    /// Remove order by iterator (O(1) for doubly-linked list)
    void remove_order(OrderIterator it) {
        total_quantity_ -= it->remaining_quantity;
        orders_.erase(it);
    }

    /// Lower an order's remaining quantity in place. The order keeps its position in the
    /// queue. `new_remaining` must be greater than zero and no more than the current
    /// remaining quantity; anything else is ignored and reported as false.
    bool reduce_order(OrderIterator it, Quantity new_remaining) {
        if (new_remaining == 0 || new_remaining > it->remaining_quantity)
            return false;
        const Quantity delta = it->remaining_quantity - new_remaining;
        it->remaining_quantity = new_remaining;
        it->initial_quantity -= std::min(delta, it->initial_quantity);
        total_quantity_ -= delta;
        return true;
    }

    /// Get front order (best time priority)
    [[nodiscard]] Order& front() {
        return orders_.front();
    }
    [[nodiscard]] const Order& front() const {
        return orders_.front();
    }

    /// Fill front order partially, removing it if fully filled
    Quantity fill_front(Quantity qty) {
        auto& front_order = orders_.front();
        Quantity fill_qty = std::min(qty, front_order.remaining_quantity);
        front_order.fill(fill_qty);
        total_quantity_ -= fill_qty;

        // Remove if fully filled
        if (front_order.is_filled()) {
            orders_.pop_front();
        }

        return fill_qty;
    }

    // Iterators for traversal
    auto begin() {
        return orders_.begin();
    }
    auto end() {
        return orders_.end();
    }
    auto begin() const {
        return orders_.begin();
    }
    auto end() const {
        return orders_.end();
    }
    auto cbegin() const {
        return orders_.cbegin();
    }
    auto cend() const {
        return orders_.cend();
    }

private:
    Price price_;
    TotalQuantity total_quantity_;
    std::list<Order> orders_;  // Doubly-linked list for O(1) insert/remove
};

}  // namespace nm
