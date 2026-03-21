# Changelog

All notable changes to NanoMatch are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-03-21

### Added

- **Matching Engine**: Price-time priority (FIFO) matching algorithm with multi-instrument support via `MatchingEngine`
- **Order Types**: Limit, Market, Immediate-or-Cancel (IOC), Fill-or-Kill (FOK)
- **Order Operations**: Add, cancel, and modify with O(1) lookup by order ID
- **Order Book**: Dual-sided book using `std::map` (red-black tree) for price levels and `std::list` for FIFO order queues
- **Pool Allocator**: Custom `PoolAllocator<T>` template for zero-allocation hot path, with bounds-checked deallocation
- **Order Validator**: Input validation for order requests (price, quantity, ID checks)
- **CSV I/O**: `CSVReader` for order input with line-level error reporting, `CSVWriter` for trade output
- **Order Generator**: Deterministic random order generator with configurable seed, price range, and cancel rate
- **Statistics**: `LatencyStats` (p50/p99/p99.9 percentiles with lazy sorting) and `ThroughputStats` (ops/second measurement)
- **Benchmarks**: Throughput (5M ops), per-operation latency (1M ops with histogram CSV), and memory pool vs `new`/`delete` comparison
- **Test Suite**: 18 test suites with 60+ Google Test cases covering all components
- **CI/CD**: GitHub Actions pipeline with multi-compiler matrix (GCC/Clang), Debug/Release builds, AddressSanitizer, UndefinedBehaviorSanitizer, clang-format check, and cppcheck static analysis
- **CLI**: Interactive demo mode, CSV file processing, random order generation with `--file`, `--generate`, `--symbol`, `--verbose` flags
- **Documentation**: Architecture overview, order types reference, benchmarking guide, contributing guidelines, security policy, code of conduct
- **Developer Tooling**: `.clang-format`, `.clang-tidy`, `.editorconfig`, `.gitattributes`, Doxyfile, GitHub issue/PR templates
- **Python Scripts**: `generate_orders.py` for test data generation, `plot_latency.py` for histogram visualization
