# Benchmarks

Three programs in `bench/`. Each prints the compiler, the build type and whether the build was optimised, and says so loudly if it was not. Recorded figures and where they came from are in the [README](../README.md#performance).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

./build/bench/bench_throughput
./build/bench/bench_latency
./build/bench/bench_memory
```

Without `-DCMAKE_BUILD_TYPE=Release` nothing is optimised and the numbers mean nothing.

## The workload

`bench/bench_common.hpp` builds one synthetic workload that the throughput and latency benchmarks share:

- limit orders only, buy or sell with equal probability
- prices uniform within ±$2.00 of $100.00 (401 price levels), quantities uniform from 1 to 1000 (throughput) or 1 to 500 (latency)
- about one step in five is a cancel; the rest are adds
- a fixed seed (42)

**Every cancel names an order that is resting at that moment**, chosen uniformly at random. To know which orders are resting, the workload is run once through a scratch book before any timing, following the fills. An earlier version cancelled `id - 1` over and over, so most of its cancels were hash-map misses that cost almost nothing and were still counted as operations.

Two properties of this workload matter when reading the numbers:

- **The book stays shallow.** Buys and sells are drawn from the same price range, so most resting orders are hit soon after they arrive. Of 5,000,000 steps about 3,000,000 trades result and a few dozen orders are resting at the end. Deep-book behaviour (long queues, many levels, cache misses on a large hash map) is not exercised.
- **The same seed does not give the same orders everywhere.** `std::uniform_int_distribution` is implementation-defined, so libstdc++ and libc++ produce slightly different workloads (4,000,225 adds in one, 4,000,898 in the other). Runs are comparable in kind, not operation for operation.

## bench_throughput

Replays 5,000,000 steps through `OrderBook::add_order` and `cancel_order` on one thread, five times, each on a fresh book, and prints the slowest, median and fastest run. It reports adds (rested or traded on arrival), cancels (removed a resting order, or missed) and trades separately. Misses should be zero.

Timed: the `add_order` and `cancel_order` calls, building each `Order`, and counting the results. Not timed: generating the workload. Not measured at all: parsing, validation, the `MatchingEngine` layer, I/O, or more than one thread.

## bench_latency

Replays 1,000,000 steps and times each call on its own with `steady_clock`, in three groups: an add that rests, an add that trades on arrival, and a cancel of a resting order. For each it prints the count, mean, p50, p90, p99, p99.9 and max.

Two properties of the clock limit what these numbers can show, so the program measures and prints both:

- **The cost of reading the clock** is inside every sample. It is about 20 ns on the CI runners. On an Apple M2 it is below the clock's step, so two readings in a row usually show a difference of 0.
- **The clock's step.** The program prints the smallest non-zero difference between two consecutive readings. On Linux that is just the cost of a reading. On Apple silicon `steady_clock` advances in steps of about 41.7 ns, so every sample is a multiple of that and percentiles land on 125, 167, 208 ns and so on. A p50 of "84 ns" on such a machine means "two ticks", not 84 ns.

The maximum is whatever the worst interruption was (a context switch, a page fault) and says little about the book.

Samples are written to `latency_histogram.csv` (`op,latency_ns`) for `scripts/plot_latency.py`.

## bench_memory

`PoolAllocator` against `new`/`delete`: allocate 500,000 objects, free half, allocate that many again. The order book does not use the pool, so this says nothing about matching speed. The two sides do not do the same work either: the pool builds all its objects before the timed section (that time is printed separately), while `new` pays for construction inside it.

## Running them well

- Release build, quiet machine. On a busy laptop the same binary ran at a fifth to a third of its idle throughput, and the per-call medians roughly doubled.
- Compare runs on the same machine and compiler. The CI figures come from shared virtual machines and move from run to run.
