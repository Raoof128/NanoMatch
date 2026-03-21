#pragma once
#include "nanomatch/core/order.hpp"
#include <string>

namespace nm {

struct ValidationResult {
    bool valid;
    std::string error;
};

class OrderValidator {
public:
    static ValidationResult validate(const OrderRequest& request) {
        if (request.action == OrderRequest::Action::New) {
            if (request.quantity == 0) {
                return {false, "Quantity must be > 0"};
            }
            if (request.type == OrderType::Limit && request.price <= 0) {
                return {false, "Limit order must have positive price"};
            }
        }

        if (request.action == OrderRequest::Action::Cancel) {
            if (request.id == INVALID_ID) {
                return {false, "Cancel requires valid order ID"};
            }
        }

        if (request.action == OrderRequest::Action::Modify) {
            if (request.id == INVALID_ID) {
                return {false, "Modify requires valid order ID"};
            }
            if (request.price <= 0) {
                return {false, "Modified price must be positive"};
            }
            if (request.quantity == 0) {
                return {false, "Modified quantity must be > 0"};
            }
        }

        return {true, ""};
    }
};

} // namespace nm
