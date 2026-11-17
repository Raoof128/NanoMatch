#pragma once
#include "nanomatch/core/order.hpp"

#include <charconv>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace nm {

/// Reads order requests from a CSV file with the columns
/// `action,id,side,type,price,quantity`.
///
/// Parsing is strict: a row with the wrong number of columns, an unknown word, or a
/// number that is negative, out of range or followed by other characters is an error
/// that names the line. Columns a request does not use may be left empty: a CANCEL row
/// needs only its ID, and a MODIFY row needs its ID, price and quantity. An empty side
/// or type reads as BUY and LIMIT, and an empty price or quantity as 0.
///
/// The reader checks syntax only. Whether a request makes sense (a positive quantity,
/// a positive limit price) is OrderValidator's job.
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
        size_t line_number = 0;
        while (std::getline(file, line)) {
            ++line_number;
            std::string_view row = trim(line);
            if (row.empty())
                continue;
            // The first line is a header unless it starts with an action word
            if (line_number == 1 && !starts_with_action(row))
                continue;
            orders.push_back(parse_line(row, line_number));
        }

        return orders;
    }

private:
    static constexpr size_t kColumns = 6;

    static std::string_view trim(std::string_view s) {
        const auto is_space = [](char c) { return c == ' ' || c == '\t' || c == '\r'; };
        while (!s.empty() && is_space(s.front()))
            s.remove_prefix(1);
        while (!s.empty() && is_space(s.back()))
            s.remove_suffix(1);
        return s;
    }

    static std::string_view first_field(std::string_view row) {
        return trim(row.substr(0, row.find(',')));
    }

    static bool starts_with_action(std::string_view row) {
        const std::string_view word = first_field(row);
        return word == "NEW" || word == "CANCEL" || word == "MODIFY";
    }

    [[noreturn]] static void fail(size_t line_number, const std::string& what,
                                  std::string_view token) {
        throw std::runtime_error("Line " + std::to_string(line_number) + ": " + what + " '" +
                                 std::string(token) + "'");
    }

    // Parses the whole token as a number of type T, or reports the field by name.
    template <typename T>
    static T parse_number(std::string_view token, size_t line_number, const char* field,
                          bool required) {
        if (token.empty()) {
            if (required)
                fail(line_number, std::string("missing ") + field, token);
            return T{};
        }
        T value{};
        const char* begin = token.data();
        const char* end = begin + token.size();
        auto [ptr, ec] = std::from_chars(begin, end, value);
        if (ec != std::errc() || ptr != end)
            fail(line_number, std::string("invalid ") + field, token);
        return value;
    }

    static OrderRequest parse_line(std::string_view line, size_t line_number) {
        // Split into exactly six fields
        std::string_view fields[kColumns];
        size_t count = 0;
        std::string_view rest = line;
        while (true) {
            const size_t comma = rest.find(',');
            if (count == kColumns) {
                fail(line_number, "too many columns in", line);
            }
            fields[count++] = trim(rest.substr(0, comma));
            if (comma == std::string_view::npos)
                break;
            rest.remove_prefix(comma + 1);
        }
        if (count != kColumns) {
            fail(line_number, "expected 6 columns in", line);
        }

        OrderRequest req{};

        // Format: action,id,side,type,price,quantity
        if (fields[0] == "NEW")
            req.action = OrderRequest::Action::New;
        else if (fields[0] == "CANCEL")
            req.action = OrderRequest::Action::Cancel;
        else if (fields[0] == "MODIFY")
            req.action = OrderRequest::Action::Modify;
        else
            fail(line_number, "unknown action", fields[0]);

        const bool is_new = (req.action == OrderRequest::Action::New);
        const bool is_cancel = (req.action == OrderRequest::Action::Cancel);

        req.id = parse_number<OrderID>(fields[1], line_number, "order ID", true);

        if (fields[2] == "BUY")
            req.side = Side::Buy;
        else if (fields[2] == "SELL")
            req.side = Side::Sell;
        else if (fields[2].empty() && !is_new)
            req.side = Side::Buy;
        else
            fail(line_number, "unknown side", fields[2]);

        if (fields[3] == "LIMIT")
            req.type = OrderType::Limit;
        else if (fields[3] == "MARKET")
            req.type = OrderType::Market;
        else if (fields[3] == "IOC")
            req.type = OrderType::ImmediateOrCancel;
        else if (fields[3] == "FOK")
            req.type = OrderType::FillOrKill;
        else if (fields[3].empty() && !is_new)
            req.type = OrderType::Limit;
        else
            fail(line_number, "unknown order type", fields[3]);

        // A market order has no price of its own, so its price column may be empty
        const bool price_required = !is_cancel && !(is_new && req.type == OrderType::Market);
        req.price = parse_number<Price>(fields[4], line_number, "price", price_required);
        req.quantity = parse_number<Quantity>(fields[5], line_number, "quantity", !is_cancel);

        return req;
    }

    std::string filepath_;
};

}  // namespace nm
