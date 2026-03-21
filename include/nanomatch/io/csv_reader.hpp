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

        size_t line_number = 1; // header was line 1
        while (std::getline(file, line)) {
            ++line_number;
            if (line.empty()) continue;
            auto req = parse_line(line, line_number);
            orders.push_back(req);
        }

        return orders;
    }

private:
    static OrderRequest parse_line(const std::string& line, size_t line_number) {
        std::istringstream ss(line);
        std::string token;
        OrderRequest req{};

        // Format: action,id,side,type,price,quantity
        std::getline(ss, token, ',');
        if (token == "NEW") req.action = OrderRequest::Action::New;
        else if (token == "CANCEL") req.action = OrderRequest::Action::Cancel;
        else if (token == "MODIFY") req.action = OrderRequest::Action::Modify;

        try {
            std::getline(ss, token, ',');
            req.id = std::stoull(token);
        } catch (const std::exception&) {
            throw std::runtime_error("Line " + std::to_string(line_number) +
                ": invalid order ID '" + token + "'");
        }

        std::getline(ss, token, ',');
        req.side = (token == "BUY") ? Side::Buy : Side::Sell;

        std::getline(ss, token, ',');
        if (token == "LIMIT") req.type = OrderType::Limit;
        else if (token == "MARKET") req.type = OrderType::Market;
        else if (token == "IOC") req.type = OrderType::ImmediateOrCancel;
        else if (token == "FOK") req.type = OrderType::FillOrKill;

        try {
            std::getline(ss, token, ',');
            req.price = static_cast<Price>(std::stol(token));
        } catch (const std::exception&) {
            throw std::runtime_error("Line " + std::to_string(line_number) +
                ": invalid price '" + token + "'");
        }

        try {
            std::getline(ss, token, ',');
            req.quantity = static_cast<Quantity>(std::stoul(token));
        } catch (const std::exception&) {
            throw std::runtime_error("Line " + std::to_string(line_number) +
                ": invalid quantity '" + token + "'");
        }

        return req;
    }

    std::string filepath_;
};

} // namespace nm
