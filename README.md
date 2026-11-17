# NanoMatch

[![CI](https://github.com/Raoof128/NanoMatch/actions/workflows/ci.yml/badge.svg)](https://github.com/Raoof128/NanoMatch/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS-green.svg)]()

**A limit order book and matching engine in C++20. A study project.**

NanoMatch implements the core of an exchange as the textbook describes it: buy and sell orders matched by price-time priority, in a price-ordered `std::map` of FIFO queues with a hash map for order lookup. It is single-threaded, has no networking or persistence, and exists to be read, tested and measured.

## Performance

One thread, synthetic limit orders at random prices around $100, about one step in five a cancel of an order that is resting at that moment. Release build (`-O3 -march=native`).

**Throughput** (`bench_throughput`: 5,000,000 `add_order` / `cancel_order` calls, replayed five times):

| Where | Compiler | Slowest | Median | Fastest |
|---|---|---|---|---|
| GitHub-hosted Ubuntu runner, CI run 1 | GCC 15.2 | 7,558,711 ops/s | 7,575,135 | 7,584,778 |
| GitHub-hosted Ubuntu runner, CI run 2 | GCC 15.2 | 8,143,283 ops/s | 8,157,600 | 8,169,559 |
| Apple M2 laptop, macOS | Apple clang 21 | 11,538,916 ops/s | 11,723,239 | 11,759,580 |

The M2 row is three invocations of the benchmark (fifteen runs): the slowest and fastest single runs, and the middle one of the three medians (11,686,991, 11,723,239 and 11,745,154).

**Latency per call** (`bench_latency`: 1,000,000 calls, each timed on its own; nanoseconds):

| Call | CI run 1: p50 / p99 / p99.9 | CI run 2: p50 / p99 / p99.9 | Apple M2: p50 / p99 / p99.9 |
|---|---|---|---|
| add, order rests | 128 / 238 / 317 | 120 / 220 / 290 | 83 / 125 / 167 |
| add, trades on arrival | 186 / 494 / 710 | 170 / 441 / 621 | 125 / 334 / 458 |
| cancel, resting order | 99 / 175 / 214 | 90 / 160 / 210 | 42 / 125 / 125 |

All figures are from 17 November 2026. The CI runs are on shared virtual machines; the two differ by about 8%, and other runs will differ again.

How to read these:

- **Every CI latency sample includes reading the clock twice**, about 20 ns. Subtract that before comparing with the throughput table's mean time per call (122 to 132 ns).
- **The M2 latency column is quantised.** That machine's clock advances in steps of about 41.7 ns, so every sample is a whole number of steps: 42 is one, 83 is two, 125 is three. A cancel "p50 of 42 ns" means "most cancels took between one and two steps", no finer than that, and the column is not comparable digit for digit with the CI ones.
- **A quiet machine matters.** With other jobs running on the same laptop, the same binary gave medians between 1.9 and 4.5 million ops/s and per-call medians about twice those above. The M2 figures in the tables were taken with the machine otherwise idle.
- **The book is shallow in this workload.** Buys and sells are drawn from the same price range, so most resting orders trade soon after they arrive: 5,000,000 steps produce about 3,000,000 trades and leave a few dozen orders resting. Long queues and a large lookup table are not exercised.
- **What is timed** is the order book alone. Parsing, validation, the `MatchingEngine` layer and I/O are outside the loop.
- An earlier version of this README reported 9.29 million ops/s and a p50 of 84 ns. That benchmark cancelled the same ID over and over, so most of its cancels were lookups that missed and cost almost nothing, and its latency figure covered adds only (84 ns is two steps of the M2's clock). The figures above replace it.

[docs/benchmarks.md](docs/benchmarks.md) has the details. CI prints all three benchmarks on every Release GCC build.

## Features

- **Price-time priority (FIFO) matching**
- **Four order types** — Limit, Market, Immediate-or-Cancel (IOC), Fill-or-Kill (FOK)
- **Cancel and modify** by order ID through a hash map (average O(1) lookup). Reducing an order's quantity keeps its place in the queue; any other change re-queues it
- **Multi-instrument** — independent order books per symbol via `MatchingEngine`
- **Custom pool allocator** — a standalone fixed-size pool, tested and benchmarked against `new`/`delete`. The order book does not use it yet; it allocates through `std::map`, `std::list` and `std::unordered_map`
- **Input checks** — duplicate, zero-quantity and non-positive-price orders are turned away with a reason (`last_reject()`); the CSV reader rejects malformed rows by line number
- **Testing** — 170 Google Test cases across 20 test files, run under AddressSanitizer and UndefinedBehaviorSanitizer in CI. They include a regression test for each defect found in review and a randomized differential test: five fixed seeds, 6,000 mixed requests each, compared after every request against a naive reference book
- **Benchmarks** — throughput over five runs, and per-call latency percentiles for adds that rest, adds that trade and cancels
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
| Add order that rests | O(log M) | Find or create the level in the red-black tree (M = price levels), append to its list |
| Cancel order by ID | O(1) average | Hash map lookup + list erase (O(log M) more if the level empties) |
| Match one resting order | O(1) | Front of best level's queue |
| Get best bid/ask | O(1) | Map `begin()` |

Not here: self-trade prevention (orders have no owner), order expiry, more than one thread, and any use of the pool allocator by the book. [docs/order_types.md](docs/order_types.md) says exactly what each request does, including the ones that are turned away.

See [docs/architecture.md](docs/architecture.md) for the full design overview.

## Quick Start

### Prerequisites

- A C++20 compiler. CI builds with GCC 15.2 and Clang 21.1 on Ubuntu; it also builds with Apple clang 21 on macOS. Older versions have not been tried
- CMake 3.20+
- Google Test, installed where CMake can find it (Debian/Ubuntu: `libgtest-dev`; macOS: `brew install googletest`)

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
./build/bench/bench_throughput    # ops/second over five runs
./build/bench/bench_latency       # per-call percentiles in nanoseconds
./build/bench/bench_memory        # the standalone pool allocator vs new/delete
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
NEW,2,SELL,MARKET,,150
CANCEL,1,,,,
MODIFY,3,,,10200,300
```

Prices are integers in cents. An `id` of 0 on a `NEW` row asks the engine to assign one. Columns a row does not use may be left empty: a `CANCEL` needs only its ID, a `MODIFY` its ID, price and quantity, and a market order has no price. Anything else that is missing, unknown, negative, out of range or followed by stray characters stops the run with an error naming the line.

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
│   ├── memory/        # PoolAllocator (standalone; not used by the book)
│   ├── io/            # CSV reader/writer, order generator
│   └── stats/         # Latency and throughput statistics
├── src/               # Implementation files + main.cpp
├── tests/             # Google Test suites (20 files, 170 test cases)
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
| Testing | Google Test | Unit, regression and randomized differential tests |
| CI/CD | GitHub Actions | Build matrix, sanitizers, static analysis |
| Analysis | cppcheck, ASan, UBSan | Memory safety and undefined behavior detection |
| Formatting | clang-format | Consistent code style |

## Documentation

- [Architecture Overview](docs/architecture.md) — data structures, matching algorithm, complexity analysis
- [Order Types Reference](docs/order_types.md) — Limit, Market, IOC, FOK behavior and lifecycle
- [Benchmarking Guide](docs/benchmarks.md) — what each benchmark measures and how to read the numbers
- [Contributing Guide](CONTRIBUTING.md) — how to build, test, and submit changes
- [Security Policy](SECURITY.md) — vulnerability reporting

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.
