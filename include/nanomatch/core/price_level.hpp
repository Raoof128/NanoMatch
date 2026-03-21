#pragma once
#include "nanomatch/core/order.hpp"
#include <list>

namespace nm {

class PriceLevel {
public:
    explicit PriceLevel(Price price) : price_(price), total_quantity_(0) {}

    [[nodiscard]] Price    price()          const { return price_; }
    [[nodiscard]] Quantity total_quantity()  const { return total_quantity_; }
    [[nodiscard]] size_t   order_count()    const { return orders_.size(); }
    [[nodiscard]] bool     empty()          const { return orders_.empty(); }

    // Iterator type for external reference (used by order lookup map)
    using OrderIterator = std::list<Order>::iterator;

    // Add order to back of queue (FIFO)
    OrderIterator add_order(Order order) {
        total_quantity_ += order.remaining_quantity;
        orders_.push_back(std::move(order));
        return std::prev(orders_.end());
    }

    // Remove order by iterator (O(1) for doubly-linked list)
    void remove_order(OrderIterator it) {
        total_quantity_ -= it->remaining_quantity;
        orders_.erase(it);
    }

    // Get front order (best time priority)
    [[nodiscard]] Order& front() { return orders_.front(); }
    [[nodiscard]] const Order& front() const { return orders_.front(); }

    // Fill front order partially
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
    auto begin() { return orders_.begin(); }
    auto end()   { return orders_.end(); }

private:
    Price           price_;
    Quantity        total_quantity_;
    std::list<Order> orders_;  // Doubly-linked list for O(1) insert/remove
};

} // namespace nm
