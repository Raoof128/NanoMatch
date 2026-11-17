# NanoMatch Architecture

## Overview

NanoMatch implements a price-time priority (FIFO) matching engine, the textbook core of an exchange, as a study project. It receives orders, matches buyers with sellers at the best available price, and returns the resulting trades. It runs on one thread and has no networking, persistence or accounts.

## System Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                         NanoMatch                             │
│                                                               │
│  ┌───────────────┐                                            │
│  │  Order Input   │  CSV file or programmatic API              │
│  │  Gateway       │                                            │
│  └───────┬───────┘                                            │
│          │  OrderRequest                                      │
│          ▼                                                    │
│  ┌───────────────┐     ┌─────────────────────────────────┐    │
│  │  Matching      │     │        Limit Order Book          │    │
│  │  Engine        │────▶│                                  │    │
│  │               │     │  ┌─────────┐    ┌─────────┐     │    │
│  │  - add_order  │     │  │ BID     │    │ ASK     │     │    │
│  │  - cancel     │     │  │ SIDE    │    │ SIDE    │     │    │
│  │  - modify     │     │  │ (> cmp) │    │ (< cmp) │     │    │
│  │  - match      │     │  │         │    │         │     │    │
│  │               │     │  │ $101.00 │    │ $101.50 │     │    │
│  │               │     │  │ [O1→O2] │    │ [O4→O7] │     │    │
│  │               │     │  │         │    │         │     │    │
│  │               │     │  │ $100.75 │    │ $102.00 │     │    │
│  │               │     │  │ [O3]    │    │ [O8]    │     │    │
│  └───────┬───────┘     │  └─────────┘    └─────────┘     │    │
│          │             │                                  │    │
│          │             │  Order Lookup:                    │    │
│          │             │  unordered_map<OrderID, Location> │    │
│          │             └─────────────────────────────────┘    │
│          │                                                    │
│          ▼                                                    │
│  ┌───────────────┐     ┌───────────────┐                      │
│  │  Trade Output  │     │  Stats Engine  │                      │
│  │  - trades.csv │     │  - throughput  │                      │
│  │  - book state │     │  - p50/p99    │                      │
│  └───────────────┘     └───────────────┘                      │
└──────────────────────────────────────────────────────────────┘
```

## Data Structure Design

### Price Levels: `std::map<Price, PriceLevel, Compare>`

Each side of the book uses a **red-black tree** (`std::map`) keyed by price:
- **BidSide** uses `std::greater<Price>` — highest price (best bid) at the front
- **AskSide** uses `std::less<Price>` — lowest price (best ask) at the front

This gives O(log M) lookup and insertion of price levels and O(1) access to the best price.

### Order Queue: `std::list<Order>` per Price Level

At each price level, orders are stored in a **doubly-linked list** maintaining FIFO (time priority). This gives:
- O(1) append (new order arrives)
- O(1) removal (order cancelled or filled)
- O(1) access to front (next order to match)

### Order Lookup: `std::unordered_map<OrderID, OrderLocation>`

A hash map provides average O(1) lookup for cancel and modify operations, and is consulted on every add to turn away an ID that is already resting. Each entry stores the order's price, side, and an iterator into the linked list for direct removal.

### Complexity Summary

| Operation | Time Complexity | Notes |
|-----------|----------------|-------|
| Add order that rests | O(log M) | Find or create the price level in the tree (M = price levels on that side), append to its list, insert into the hash map |
| Cancel order by ID | O(1) average | Hash lookup and list erase; O(log M) more if the level empties and is removed |
| Match one resting order | O(1) | Front of the best level's queue; O(log M) more when a level empties |
| Get best bid/ask | O(1) | Map `begin()` |
| Fill-or-Kill check | O(L) | L = crossable price levels; uses each level's stored total |
| Modify, lower quantity at same price | O(1) average | In place |
| Modify, otherwise | cancel + add | |
| `check_invariants` | O(N) | N = resting orders; for tests |

Every node of the tree, the lists and the hash map is a separate heap allocation, so the constants are those of pointer-chasing containers.

## Matching Algorithm

```
incoming order arrives
    │
    ├── reject if the ID is 0 or already resting, the quantity is 0,
    │   or it is a limit order priced at zero or below
    │
    ├── if Market order → set aggressive price (MAX/MIN)
    │
    ├── if Fill-or-Kill → pre-check total available quantity
    │   └── if insufficient → reject immediately
    │
    ├── MATCH LOOP:
    │   while (incoming has remaining qty AND opposite side not empty):
    │       get best opposing price level
    │       if prices don't cross → break
    │       match against front order at that level (FIFO)
    │       create Trade record
    │       update quantities for both orders
    │       if resting order fully filled → remove from lookup
    │       if price level empty → remove from map
    │
    └── PLACE REMAINDER:
        if Limit order → rest on book
        if Market/IOC → cancel remainder (don't place)
        if FOK → never reaches here (pre-checked)
```

## Component Design

### MatchingEngine

Top-level coordinator. Maintains a map of `OrderBook` instances keyed by symbol. Runs each `OrderRequest` through `OrderValidator`, then routes it to the book for its symbol. A valid new order creates the book if needed; a cancel or modify for an unknown symbol does nothing. Assigns an order ID when the request carries 0, skipping any ID resting on that book. `last_error()` says why the last request had no effect.

### OrderBook

Single-instrument order book. Contains bid side, ask side, and order lookup map. Implements the core matching algorithm: `add_order()`, `cancel_order()`, `modify_order()`. `last_reject()` reports why a request was turned away, and `check_invariants()` walks the whole book to confirm it is consistent (used by the randomized test).

### BookSide

One half of the order book (bids or asks). Templated on comparator for sort order. Manages price levels and provides best-price access.

### PriceLevel

FIFO queue of orders at a single price. Tracks aggregate quantity in 64 bits. Supports add, remove, reduce-in-place and fill-front operations.

### PoolAllocator

A standalone fixed-capacity pool with a free list for O(1) allocate and deallocate. **The order book does not use it**: the book's containers allocate from the heap. It hands out pre-built objects without running constructors or destructors, rejects a pointer from outside the pool, and rejects a double free. Making the book use a pool would mean a standard-conforming allocator for the list and tree nodes, which this class is not.

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| Integer prices (cents) | Avoids floating-point comparison bugs |
| `std::map` over flat array | Handles sparse price levels efficiently |
| `std::list` over `std::deque` | O(1) iterator-based removal for cancels |
| Hash map for order lookup | O(1) cancel/modify by ID |
| Header-heavy design | Lets the compiler inline the small per-level methods |
| Price-time priority | Industry standard; simplest fair matching |
| Modify = cancel + re-add, except a quantity reduction | A reduction gives nothing up, so it keeps its place; anything else re-queues |
| No self-trade prevention | Orders have no owner field, so there is nothing to compare |
