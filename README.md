# NanoMatch

High-performance limit order book and matching engine built in C++20.

## Features

- Price-time priority (FIFO) matching
- Order types: Limit, Market, IOC, FOK
- Cancel and modify operations
- O(1) order lookup by ID (hash map)
- O(log M) price level insertion (red-black tree)
- O(1) order queue operations (doubly-linked list)
- Custom pool allocator (zero heap allocation on hot path)
- Comprehensive Google Test suite (50+ tests)
- Reproducible benchmarks with latency percentiles (p50/p99/p99.9)
- Multi-instrument support via `MatchingEngine`
- CSV I/O for orders and trades

## Architecture

```
MatchingEngine
  └── OrderBook (per symbol)
        ├── BidSide  (std::map<Price, PriceLevel, greater>)
        ├── AskSide  (std::map<Price, PriceLevel, less>)
        └── OrderLookup (unordered_map<OrderID, Location>)
              └── PriceLevel
                    └── std::list<Order> (FIFO queue)
```

See [docs/architecture.md](docs/architecture.md) for details.

## Build

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run tests
cd build && ctest --output-on-failure

# Run benchmarks
./bench/bench_throughput
./bench/bench_latency
./bench/bench_memory
```

## Usage

```bash
# Interactive demo
./build/nanomatch

# Process orders from CSV
./build/nanomatch --file data/sample_orders.csv --verbose

# Generate and process random orders
./build/nanomatch --generate 100000 --symbol AAPL
```

## Order Book Visualisation

```
===== AAPL Order Book =====
  ASKS:
      102.00  x 200
      101.75  x 300
      101.50  x 500
  --- spread: 0.50 ---
  BIDS:
      101.00  x 800
      100.75  x 600
      100.50  x 400
================================
```

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C++20 |
| Build | CMake 3.20+ |
| Testing | Google Test |
| CI/CD | GitHub Actions |
| Platform | Linux / macOS |

## License

MIT
