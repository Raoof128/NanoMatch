# Benchmarks

## Running Benchmarks

```bash
# Build in Release mode (critical for accurate numbers)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCH=ON
cmake --build build -j$(nproc)

# Run benchmarks
./build/bench/bench_throughput    # Throughput: ops/second
./build/bench/bench_latency       # Latency: p50/p99/max in nanoseconds
./build/bench/bench_memory        # Memory pool vs new/delete comparison
```

## Throughput Benchmark

Processes 5M operations (80% new orders, 20% cancels) with random prices around a base price. Measures total wall-clock time and derives ops/second.

**Workload:**
- 5,000,000 operations
- Fixed seed (42) for reproducibility
- Price range: $98.00–$102.00
- Quantity range: 1–1000 shares
- 20% cancel rate

## Latency Benchmark

Times each individual `add_order` call with `high_resolution_clock` and collects 1M latency samples. Reports percentiles and writes raw data to CSV for histogram plotting.

**Output:**
- `latency_histogram.csv` — raw nanosecond latencies for plotting

## Plotting

```bash
cd build
../scripts/plot_latency.py latency_histogram.csv
```

Generates `latency_report.png` with histogram and CDF plots.

## Memory Benchmark

Compares pool allocator performance against standard `new`/`delete`:
- Allocate N objects
- Deallocate half
- Reallocate half

## Important Notes

1. **Always benchmark in Release mode** (`-O3 -march=native`). Debug mode numbers are meaningless.
2. **Pre-generate all test data** before starting the timer to avoid RNG noise.
3. **Fixed seed** ensures reproducible results across runs.
4. **Run on quiet system** — close other applications for consistent results.
