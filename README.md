# NanoMatch

[![CI](https://github.com/Raoof128/NanoMatch/actions/workflows/ci.yml/badge.svg)](https://github.com/Raoof128/NanoMatch/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS-green.svg)]()

**High-performance limit order book and matching engine built in modern C++20.**

NanoMatch simulates the core of a financial exchange — processing buy/sell orders using price-time priority, matching trades, and benchmarking throughput at millions of operations per second. It is a study implementation of the textbook design: a price-ordered `std::map` of FIFO queues, with a hash map for order lookup.

## Performance

```
=========================================            =========================================
     NanoMatch Throughput Benchmark                        NanoMatch Latency Benchmark
=========================================            =========================================
  Operations:       5,000,000                           p50:                84 ns
  Throughput:       9,290,192 ops/s                     p99:               625 ns
  Avg latency:          107.6 ns                        p99.9:           1,250 ns
=========================================            =========================================
```

> Recorded once on an Apple M-series laptop: Release build (`-O3 -march=native`), single thread, synthetic orders from a fixed seed. The seed fixes the inputs, not the timings, which vary by machine and by run.
>
> What each number covers. The throughput run is about 80% adds and 20% cancel attempts; a cancel aimed at an order that has already been removed returns at once, so it is cheaper than a real cancel. The latency run times add-order calls only and includes the timer's own overhead. CI prints both benchmarks on every run.

## Features

- **Price-time priority (FIFO) matching** — industry-standard exchange algorithm
- **Four order types** — Limit, Market, Immediate-or-Cancel (IOC), Fill-or-Kill (FOK)
- **Cancel and modify** — O(1) lookup by order ID via hash map
- **Multi-instrument** — independent order books per symbol via `MatchingEngine`
- **Custom pool allocator** — a standalone fixed-size pool, tested and benchmarked against `new`/`delete`. The order book does not use it yet; it allocates through `std::map`, `std::list` and `std::unordered_map`
- **Testing** — 137 Google Test cases across 18 test files, run under AddressSanitizer and UndefinedBehaviorSanitizer in CI
- **Latency benchmarks** — p50/p99/p99.9 percentile histograms
- **CSV I/O** — read orders from CSV, write trade reports
- **CI/CD** — GitHub Actions with matrix builds, sanitizers, static analysis, format checks

## Architecture

```
┌──────────────────────────────────────────────────────┐
│                    MatchingEngine                      │
│  ┌────────────────────────────────────────────────┐   │
│  │              OrderBook (per symbol)             │   │
│  │  ┌──────────────────┐ ┌──────────────────┐     │   │
│  │  │     BidSide       │ │     AskSide       │     │   │
│  │  │ map<Price,Level,> │ │ map<Price,Level,<>│     │   │
│  │  │                   │ │                   │     │   │
│  │  │  $101.00 [O1→O2] │ │  $101.50 [O4→O7] │     │   │
│  │  │  $100.75 [O3]    │ │  $102.00 [O8]    │     │   │
│  │  └──────────────────┘ └──────────────────┘     │   │
│  │                                                 │   │
│  │  OrderLookup: unordered_map<OrderID, Location>  │   │
│  └────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────┘
```

| Operation | Complexity | Implementation |
|-----------|-----------|----------------|
| Add order (new price level) | O(log M) | Red-black tree insertion |
| Add order (existing level) | O(1) | Linked list append |
| Cancel order by ID | O(1) | Hash map lookup + list erase |
| Match at best price | O(1) | Front of best level's queue |
| Get best bid/ask | O(1) | Map iterator dereference |

See [docs/architecture.md](docs/architecture.md) for the full design overview.

## Quick Start

### Prerequisites

- C++20 compiler (GCC 11+, Clang 14+, AppleClang 15+)
- CMake 3.20+
- Google Test (installed via package manager or built from source)

### Build

```bash
# Clone
git clone https://github.com/Raoof128/NanoMatch.git
cd NanoMatch

# Configure and build (Release for benchmarks)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Test

```bash
cd build && ctest --output-on-failure
```

### Benchmark

```bash
./build/bench/bench_throughput    # Throughput: ops/second
./build/bench/bench_latency       # Latency: p50/p99/p99.9 in nanoseconds
./build/bench/bench_memory        # Pool allocator vs new/delete comparison
```

## Usage

```bash
# Interactive demo — places sample orders and shows the book
./build/nanomatch

# Process orders from CSV file
./build/nanomatch --file data/sample_orders.csv --verbose

# Generate and process random orders
./build/nanomatch --generate 100000 --symbol AAPL
```

### CSV Order Format

```csv
action,id,side,type,price,quantity
NEW,1,BUY,LIMIT,10100,500
NEW,2,SELL,MARKET,0,150
CANCEL,1,BUY,LIMIT,0,0
MODIFY,3,BUY,LIMIT,10200,300
```

### Order Book Display

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
  Total orders: 6
================================
```

## Project Structure

```
NanoMatch/
├── include/nanomatch/
│   ├── core/          # Order, Trade, PriceLevel, type definitions
│   ├── book/          # OrderBook, BookSide (bid/ask sides)
│   ├── engine/        # MatchingEngine, OrderValidator
│   ├── memory/        # PoolAllocator (custom allocator)
│   ├── io/            # CSV reader/writer, order generator
│   └── stats/         # Latency and throughput statistics
├── src/               # Implementation files + main.cpp
├── tests/             # Google Test suites (18 files, 60+ tests)
├── bench/             # Throughput, latency, and memory benchmarks
├── data/              # Sample order CSV files
├── scripts/           # Python utilities (order generation, plotting)
├── docs/              # Architecture, order types, benchmarks
└── .github/workflows/ # CI: matrix builds, sanitizers, format, analysis
```

## Tech Stack

| Component | Technology | Purpose |
|-----------|-----------|---------|
| Language | C++20 | `std::optional`, `[[nodiscard]]`, structured bindings |
| Build | CMake 3.20+ | Cross-platform build system |
| Testing | Google Test 1.17 | Unit and integration testing |
| CI/CD | GitHub Actions | Build matrix, sanitizers, static analysis |
| Analysis | cppcheck, ASan, UBSan | Memory safety and undefined behavior detection |
| Formatting | clang-format | Consistent code style |

## Documentation

- [Architecture Overview](docs/architecture.md) — data structures, matching algorithm, complexity analysis
- [Order Types Reference](docs/order_types.md) — Limit, Market, IOC, FOK behavior and lifecycle
- [Benchmarking Guide](docs/benchmarks.md) — how to run and interpret performance results
- [Contributing Guide](CONTRIBUTING.md) — how to build, test, and submit changes
- [Security Policy](SECURITY.md) — vulnerability reporting

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.
