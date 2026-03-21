#pragma once
#include "nanomatch/core/order.hpp"
#include <random>
#include <vector>

namespace nm {

class OrderGenerator {
public:
    explicit OrderGenerator(uint32_t seed = 42,
                            Price base_price = 10000,
                            Price price_range = 200)
        : rng_(seed),
          side_dist_(0, 1),
          price_dist_(base_price - price_range, base_price + price_range),
          qty_dist_(1, 1000),
          action_dist_(0, 9),
          next_id_(1) {}

    OrderRequest generate() {
        OrderRequest req{};
        bool is_cancel = (action_dist_(rng_) < 2);  // 20% cancels

        if (is_cancel && next_id_ > 2) {
            req.action = OrderRequest::Action::Cancel;
            req.id = next_id_ - 2;
        } else {
            req.action = OrderRequest::Action::New;
            req.id = next_id_++;
            req.side = side_dist_(rng_) ? Side::Buy : Side::Sell;
            req.type = OrderType::Limit;
            req.price = price_dist_(rng_);
            req.quantity = qty_dist_(rng_);
        }

        return req;
    }

    std::vector<OrderRequest> generate_batch(size_t count) {
        std::vector<OrderRequest> requests;
        requests.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            requests.push_back(generate());
        }
        return requests;
    }

private:
    std::mt19937 rng_;
    std::uniform_int_distribution<int> side_dist_;
    std::uniform_int_distribution<Price> price_dist_;
    std::uniform_int_distribution<Quantity> qty_dist_;
    std::uniform_int_distribution<int> action_dist_;
    OrderID next_id_;
};

} // namespace nm
