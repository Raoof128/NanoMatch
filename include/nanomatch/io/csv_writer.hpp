#pragma once
#include "nanomatch/core/trade.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace nm {

class CSVWriter {
public:
    explicit CSVWriter(const std::string& filepath) : filepath_(filepath) {}

    void write_trades(const std::vector<Trade>& trades) {
        std::ofstream file(filepath_);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filepath_);
        }

        file << "buy_order_id,sell_order_id,price,quantity\n";
        for (const auto& trade : trades) {
            file << trade.buy_order_id << "," << trade.sell_order_id << "," << trade.price << ","
                 << trade.quantity << "\n";
        }
    }

    void append_trades(const std::vector<Trade>& trades) {
        std::ofstream file(filepath_, std::ios::app);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filepath_);
        }

        for (const auto& trade : trades) {
            file << trade.buy_order_id << "," << trade.sell_order_id << "," << trade.price << ","
                 << trade.quantity << "\n";
        }
    }

private:
    std::string filepath_;
};

}  // namespace nm
