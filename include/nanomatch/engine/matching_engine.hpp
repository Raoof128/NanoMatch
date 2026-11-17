#pragma once
#include "nanomatch/book/order_book.hpp"
#include "nanomatch/engine/order_validator.hpp"

#include <string>
#include <unordered_map>

namespace nm {

/// Top-level matching engine that routes order requests to per-symbol OrderBooks.
/// A book is created the first time a valid new order arrives for a symbol.
class MatchingEngine {
public:
    /// Process an incoming order request for the given symbol. Returns any resulting trades.
    ///
    /// A request that OrderValidator rejects is dropped: no trades, no change to any book,
    /// and last_error() holds the reason. A cancel or modify for a symbol with no book is
    /// a no-op and does not create one.
    std::vector<Trade> process(const std::string& symbol, OrderRequest request) {
        last_error_.clear();

        auto validation = OrderValidator::validate(request);
        if (!validation.valid) {
            last_error_ = std::move(validation.error);
            return {};
        }

        if (request.action != OrderRequest::Action::New) {
            auto found = books_.find(symbol);
            if (found == books_.end()) {
                last_error_ = "Unknown symbol";
                return {};
            }
            if (request.action == OrderRequest::Action::Cancel) {
                if (!found->second.cancel_order(request.id)) {
                    last_error_ = "No resting order with that ID";
                }
                return {};
            }
            auto trades = found->second.modify_order(request.id, request.price, request.quantity);
            note_reject(found->second);
            return trades;
        }

        // Auto-create book if new symbol
        auto [it, _] = books_.try_emplace(symbol, symbol);
        OrderBook& book = it->second;

        // Assign an order ID if the caller did not supply one. Callers may mix their own
        // IDs with automatic ones, so skip any ID that is resting on this book.
        if (request.id == INVALID_ID) {
            do {
                request.id = next_id_++;
                if (next_id_ == INVALID_ID) {
                    next_id_ = 1;
                }
            } while (request.id == INVALID_ID || book.contains(request.id));
        }

        Order order{};
        order.id = request.id;
        order.side = request.side;
        order.type = request.type;
        order.price = request.price;
        order.initial_quantity = request.quantity;
        order.remaining_quantity = request.quantity;

        auto trades = book.add_order(std::move(order));
        note_reject(book);
        return trades;
    }

    /// Access a specific order book by symbol, or nullptr if not found
    OrderBook* get_book(const std::string& symbol) {
        auto it = books_.find(symbol);
        return (it != books_.end()) ? &it->second : nullptr;
    }

    [[nodiscard]] size_t book_count() const {
        return books_.size();
    }

    /// Why the most recent process() call had no effect, or an empty string if it was
    /// applied. A fill-or-kill order that could not be filled is reported here too.
    [[nodiscard]] const std::string& last_error() const {
        return last_error_;
    }

private:
    void note_reject(const OrderBook& book) {
        switch (book.last_reject()) {
            case RejectReason::None:
                break;
            case RejectReason::InvalidOrderId:
                last_error_ = "Order ID 0 is reserved";
                break;
            case RejectReason::InvalidQuantity:
                last_error_ = "Quantity must be > 0";
                break;
            case RejectReason::InvalidPrice:
                last_error_ = "Price must be positive";
                break;
            case RejectReason::DuplicateOrderId:
                last_error_ = "An order with that ID is already resting";
                break;
            case RejectReason::UnknownOrder:
                last_error_ = "No resting order with that ID";
                break;
            case RejectReason::FillOrKillUnfilled:
                last_error_ = "Fill-or-kill order could not be filled in full";
                break;
        }
    }

    std::unordered_map<std::string, OrderBook> books_;
    OrderID next_id_ = 1;
    std::string last_error_;
};

}  // namespace nm
