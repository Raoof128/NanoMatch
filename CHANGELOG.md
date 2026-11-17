# Changelog

All notable changes to NanoMatch are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

A correctness review of the book, engine, CSV reader and pool allocator (November 2026). Each item has a test in `tests/test_regressions.cpp` that failed before the fix.

- **Order book: duplicate order ID.** A second order under a resting order's ID took over its lookup entry and left the first order on the book where no cancel could reach it. Duplicate live IDs are now turned away.
- **Order book: 32-bit level totals.** A price level summed its quantity in 32 bits. Large orders wrapped the total, and a fill-or-kill order the level could cover was rejected. Level totals are 64-bit.
- **Order book: modify could delete an order.** `modify_order` with a zero quantity or a non-positive price cancelled the order and then failed to add it back. The request is now checked before anything is touched.
- **Order book: modify lost time priority on a reduction.** Lowering an order's quantity at the same price sent it to the back of the queue. It now keeps its place; raising quantity or changing price still re-queues.
- **Order book: invalid orders accepted.** A limit order priced at zero or below could rest; order ID 0 and zero-quantity orders were accepted.
- **`price_to_string`:** negating the most negative price was signed overflow.
- **Matching engine:** requests were never passed through `OrderValidator`; automatic IDs could collide with IDs callers had supplied; a cancel or modify for an unknown symbol created an empty book.
- **CSV reader:** unknown words were read as NEW, SELL and LIMIT; `-5` became 4,294,967,291 shares; `100abc` was read as 100; out-of-range prices were truncated; a row with a missing column was read with shifted fields; a file with no header lost its first order. Parsing is now strict and names the line.
- **Pool allocator:** returning a slot twice corrupted the free list, after which the pool handed one slot to two callers. A double free, or a slot that was never handed out, now throws.
- **CLI:** a missing file, malformed row or bad `--generate` count aborted the program; it now prints the error and exits with status 1.
- **CI:** the UndefinedBehaviorSanitizer job only printed reports; it now fails on undefined behaviour (`-fno-sanitize-recover=all`).

### Added

- `OrderBook::last_reject()`, `contains()`, `find_order()` and `check_invariants()`; `MatchingEngine::last_error()`.
- `tests/test_invariants.cpp`: a randomized differential test. Five fixed seeds, 6,000 mixed requests each (valid and invalid), applied to the real book and to a naive reference book; trades and resting orders must agree after every request.
- 170 test cases in 20 files (was 137 in 18).

### Changed

- **Benchmarks.** The throughput benchmark cancelled `id - 1` repeatedly, so most cancels missed and cost almost nothing. The workload is now planned before timing so every cancel names a resting order; adds, cancels, misses and trades are reported separately; the replay runs five times and prints slowest, median and fastest. The latency benchmark times adds that rest, adds that trade and cancels separately and prints the clock's own cost. All three print the compiler and build type.
- **Recorded figures.** The README's 9.29 million ops/s and 84 ns p50 came from the old benchmark and are replaced by figures from the new one (7.6 and 8.2 million ops/s median on two GitHub-hosted runs, 11.7 million on an idle Apple M2), with what they do and do not cover.
- **Documentation.** `docs/order_types.md` lists every reject reason and says how cancel and modify treat filled, partially filled and unknown orders, and that self-trades are not prevented. `docs/architecture.md` corrects the complexity table (adding to an existing level is O(log M), not O(1)) and the pool allocator description. `SECURITY.md` and `CODE_OF_CONDUCT.md` no longer give an email address; reports go through GitHub private vulnerability reporting.
- **Earlier in this cycle:** the README's pool allocator and benchmark claims were corrected, `clang-format` 18 was applied, and cppcheck findings in `src/main.cpp` and `src/book/order_book.cpp` were resolved.

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
