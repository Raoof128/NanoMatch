# Changelog

All notable changes to NanoMatch are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- **README accuracy**: The pool allocator was described as removing heap allocation from the hot path. It is a standalone component that the order book does not use, and the README now says so. The benchmark note states what each figure covers (about 80% adds with cancel attempts that often miss; add-only latency including timer overhead) and that timings vary by machine. The test count is corrected from "60+" to 137.

### Fixed

- **CI, Format Check**: Ran `clang-format` 18 with the repository `.clang-format` over `include/`, `src/`, `tests/` and `bench/`. Formatting only.
- **CI, Static Analysis**: Resolved the cppcheck findings in `src/main.cpp` and `src/book/order_book.cpp`: read-only references, pointers and the demo lambda parameter are now `const`, and a raw copy loop uses `std::vector::insert`. `main`'s `argv` keeps its standard signature under an inline suppression. No behaviour change; 18/18 test suites pass.

## [1.0.0] - 2026-03-21

### Added

- **Matching Engine**: Price-time priority (FIFO) matching algorithm with multi-instrument support via `MatchingEngine`
- **Order Types**: Limit, Market, Immediate-or-Cancel (IOC), Fill-or-Kill (FOK)
- **Order Operations**: Add, cancel, and modify with O(1) lookup by order ID
- **Order Book**: Dual-sided book using `std::map` (red-black tree) for price levels and `std::list` for FIFO order queues
- **Pool Allocator**: Custom `PoolAllocator<T>` template with bounds-checked deallocation (standalone; not yet used by the order book)
- **Order Validator**: Input validation for order requests (price, quantity, ID checks)
- **CSV I/O**: `CSVReader` for order input with line-level error reporting, `CSVWriter` for trade output
- **Order Generator**: Deterministic random order generator with configurable seed, price range, and cancel rate
- **Statistics**: `LatencyStats` (p50/p99/p99.9 percentiles with lazy sorting) and `ThroughputStats` (ops/second measurement)
- **Benchmarks**: Throughput (5M ops), per-operation latency (1M ops with histogram CSV), and memory pool vs `new`/`delete` comparison
- **Test Suite**: 18 test files with 137 Google Test cases covering all components
- **CI/CD**: GitHub Actions pipeline with multi-compiler matrix (GCC/Clang), Debug/Release builds, AddressSanitizer, UndefinedBehaviorSanitizer, clang-format check, and cppcheck static analysis
- **CLI**: Interactive demo mode, CSV file processing, random order generation with `--file`, `--generate`, `--symbol`, `--verbose` flags
- **Documentation**: Architecture overview, order types reference, benchmarking guide, contributing guidelines, security policy, code of conduct
- **Developer Tooling**: `.clang-format`, `.clang-tidy`, `.editorconfig`, `.gitattributes`, Doxyfile, GitHub issue/PR templates
- **Python Scripts**: `generate_orders.py` for test data generation, `plot_latency.py` for histogram visualization
