#pragma once
#include "nanomatch/core/order.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace nm {

class CSVReader {
public:
    explicit CSVReader(const std::string& filepath) : filepath_(filepath) {}

    std::vector<OrderRequest> read_orders() {
        std::vector<OrderRequest> orders;
        std::ifstream file(filepath_);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filepath_);
        }

        std::string line;
        // Skip header
        std::getline(file, line);

        while (std::getline(file, line)) {
            if (line.empty()) continue;
            auto req = parse_line(line);
            orders.push_back(req);
        }

        return orders;
    }

private:
    static OrderRequest parse_line(const std::string& line) {
        std::istringstream ss(line);
        std::string token;
        OrderRequest req{};

        // Format: action,id,side,type,price,quantity
        std::getline(ss, token, ',');
        if (token == "NEW") req.action = OrderRequest::Action::New;
        else if (token == "CANCEL") req.action = OrderRequest::Action::Cancel;
        else if (token == "MODIFY") req.action = OrderRequest::Action::Modify;

        std::getline(ss, token, ',');
        req.id = std::stoull(token);

        std::getline(ss, token, ',');
        req.side = (token == "BUY") ? Side::Buy : Side::Sell;

        std::getline(ss, token, ',');
        if (token == "LIMIT") req.type = OrderType::Limit;
        else if (token == "MARKET") req.type = OrderType::Market;
        else if (token == "IOC") req.type = OrderType::ImmediateOrCancel;
        else if (token == "FOK") req.type = OrderType::FillOrKill;

        std::getline(ss, token, ',');
        req.price = static_cast<Price>(std::stol(token));

        std::getline(ss, token, ',');
        req.quantity = static_cast<Quantity>(std::stoul(token));

        return req;
    }

    std::string filepath_;
};

} // namespace nm
