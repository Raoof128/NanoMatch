#pragma once
#include "nanomatch/book/order_book.hpp"
#include <unordered_map>
#include <string>

namespace nm {

/// Top-level matching engine that routes order requests to per-symbol OrderBooks.
/// Automatically creates a new book when a previously unseen symbol is encountered.
class MatchingEngine {
public:
    /// Process an incoming order request for the given symbol. Returns any resulting trades.
    std::vector<Trade> process(const std::string& symbol,
                                OrderRequest request) {
        // Auto-create book if new symbol
        auto [it, _] = books_.try_emplace(symbol, symbol);

        // Assign order ID if new order
        if (request.action == OrderRequest::Action::New) {
            if (request.id == INVALID_ID) {
                request.id = next_id_++;
            }
        }

        switch (request.action) {
            case OrderRequest::Action::New: {
                Order order;
                order.id                = request.id;
                order.side              = request.side;
                order.type              = request.type;
                order.price             = request.price;
                order.initial_quantity  = request.quantity;
                order.remaining_quantity = request.quantity;
                return it->second.add_order(std::move(order));
            }

            case OrderRequest::Action::Cancel:
                it->second.cancel_order(request.id);
                return {};

            case OrderRequest::Action::Modify:
                return it->second.modify_order(
                    request.id, request.price, request.quantity);
        }

        return {};
    }

    /// Access a specific order book by symbol, or nullptr if not found
    OrderBook* get_book(const std::string& symbol) {
        auto it = books_.find(symbol);
        return (it != books_.end()) ? &it->second : nullptr;
    }

    [[nodiscard]] size_t book_count() const { return books_.size(); }

private:
    std::unordered_map<std::string, OrderBook> books_;
    OrderID next_id_ = 1;
};

} // namespace nm
