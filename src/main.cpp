#include "nanomatch/engine/matching_engine.hpp"
#include "nanomatch/io/csv_reader.hpp"
#include "nanomatch/io/csv_writer.hpp"
#include "nanomatch/io/order_generator.hpp"

#include <exception>
#include <iostream>
#include <string>

using namespace nm;

void print_usage() {
    std::cout << "Usage: nanomatch [OPTIONS]\n"
              << "  --file <csv>     Process orders from CSV file\n"
              << "  --generate <n>   Generate and process n random orders\n"
              << "  --symbol <sym>   Symbol name (default: AAPL)\n"
              << "  --verbose        Print book state after each order\n"
              << "  --help           Show this help\n";
}

int run(int argc, char* argv[]);

// cppcheck-suppress constParameter ; main's signature is fixed by the standard
int main(int argc, char* argv[]) {
    try {
        return run(argc, argv);
    } catch (const std::exception& e) {
        // A missing file, a malformed CSV row or a bad --generate count ends up here
        std::cerr << "nanomatch: " << e.what() << "\n";
        return 1;
    }
}

// cppcheck-suppress constParameter ; mirrors main's signature
int run(int argc, char* argv[]) {
    std::string symbol = "AAPL";
    std::string csv_file;
    uint32_t generate_count = 0;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--file" && i + 1 < argc) {
            csv_file = argv[++i];
        } else if (arg == "--generate" && i + 1 < argc) {
            generate_count = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--symbol" && i + 1 < argc) {
            symbol = argv[++i];
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--help") {
            print_usage();
            return 0;
        }
    }

    MatchingEngine engine;
    std::vector<Trade> all_trades;

    if (!csv_file.empty()) {
        // Process from CSV file
        CSVReader reader(csv_file);
        auto requests = reader.read_orders();
        std::cout << "Processing " << requests.size() << " orders from " << csv_file << "\n";

        for (const auto& req : requests) {
            auto trades = engine.process(symbol, req);
            for (const auto& t : trades) {
                all_trades.push_back(t);
                if (verbose) {
                    std::cout << "TRADE: " << t.buy_order_id << " x " << t.sell_order_id << " @ "
                              << price_to_string(t.price) << " x " << t.quantity << "\n";
                }
            }
            if (verbose) {
                const auto* book = engine.get_book(symbol);
                if (book)
                    book->print_book(3);
            }
        }
    } else if (generate_count > 0) {
        // Generate random orders
        OrderGenerator gen;
        std::cout << "Generating and processing " << generate_count << " orders\n";

        for (uint32_t i = 0; i < generate_count; ++i) {
            auto req = gen.generate();
            auto trades = engine.process(symbol, req);
            all_trades.insert(all_trades.end(), trades.begin(), trades.end());
        }
    } else {
        // Interactive demo
        std::cout << "NanoMatch: limit order book and matching engine\n";
        std::cout << "Running demo with sample orders...\n\n";

        // Demo: place some orders
        auto process_and_report = [&](const OrderRequest& req) {
            auto trades = engine.process(symbol, req);
            for (const auto& t : trades) {
                all_trades.push_back(t);
                std::cout << "  TRADE: Buy#" << t.buy_order_id << " x Sell#" << t.sell_order_id
                          << " @ " << price_to_string(t.price) << " x " << t.quantity
                          << " shares\n";
            }
        };

        // Limit sells
        process_and_report(
            {OrderRequest::Action::New, 0, Side::Sell, OrderType::Limit, 10150, 500});
        process_and_report(
            {OrderRequest::Action::New, 0, Side::Sell, OrderType::Limit, 10175, 300});
        process_and_report(
            {OrderRequest::Action::New, 0, Side::Sell, OrderType::Limit, 10200, 200});

        // Limit buys
        process_and_report({OrderRequest::Action::New, 0, Side::Buy, OrderType::Limit, 10100, 800});
        process_and_report({OrderRequest::Action::New, 0, Side::Buy, OrderType::Limit, 10075, 600});

        const auto* book = engine.get_book(symbol);
        if (book)
            book->print_book();

        // Crossing order
        std::cout << ">> Incoming: BUY 200 @ $101.50 (crosses best ask)\n";
        process_and_report({OrderRequest::Action::New, 0, Side::Buy, OrderType::Limit, 10150, 200});

        if (book)
            book->print_book();
    }

    // Summary
    std::cout << "\nTotal trades executed: " << all_trades.size() << "\n";

    const auto* book = engine.get_book(symbol);
    if (book) {
        std::cout << "Remaining orders on book: " << book->total_orders() << "\n";
        auto bid = book->best_bid();
        auto ask = book->best_ask();
        if (bid)
            std::cout << "Best bid: " << price_to_string(*bid) << "\n";
        if (ask)
            std::cout << "Best ask: " << price_to_string(*ask) << "\n";
        auto sp = book->spread();
        if (sp)
            std::cout << "Spread: " << price_to_string(*sp) << "\n";
    }

    // Write trades to CSV if any
    if (!all_trades.empty()) {
        CSVWriter writer("trades_output.csv");
        writer.write_trades(all_trades);
        std::cout << "Trades written to trades_output.csv\n";
    }

    return 0;
}
