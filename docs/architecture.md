# NanoMatch Architecture

## Overview

NanoMatch implements a price-time priority (FIFO) matching engine — the core component of every financial exchange. It receives orders, matches buyers with sellers at the best available price, and produces trade execution reports.

## System Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                         NanoMatch                             │
│                                                               │
│  ┌───────────────┐                                            │
│  │  Order Input   │  CSV file, stdin, or programmatic API      │
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

This gives O(log M) insertion of new price levels and O(1) access to the best price.

### Order Queue: `std::list<Order>` per Price Level

At each price level, orders are stored in a **doubly-linked list** maintaining FIFO (time priority). This gives:
- O(1) append (new order arrives)
- O(1) removal (order cancelled or filled)
- O(1) access to front (next order to match)

### Order Lookup: `std::unordered_map<OrderID, OrderLocation>`

A hash map provides O(1) lookup for cancel and modify operations. Each entry stores the order's price, side, and an iterator into the linked list for direct removal.

### Complexity Summary

| Operation | Time Complexity | Space | Notes |
|-----------|----------------|-------|-------|
| Add order (new price level) | O(log M) | O(1) | M = number of price levels |
| Add order (existing level) | O(1) | O(1) | Append to linked list |
| Cancel order by ID | O(1) | — | Hash lookup + list erase |
| Match at best price | O(1) | — | Front of best level's queue |
| Get best bid/ask | O(1) | — | Map begin() iterator |
| Fill-or-Kill check | O(M) | O(1) | Must scan levels to verify quantity |
| Print book (top N) | O(N) | O(N) | Display only |

## Matching Algorithm

```
incoming order arrives
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

Top-level coordinator. Maintains a map of `OrderBook` instances keyed by symbol. Routes `OrderRequest` messages to the appropriate book. Auto-creates books for new symbols. Assigns sequential order IDs.

### OrderBook

Single-instrument order book. Contains bid side, ask side, and order lookup map. Implements the core matching algorithm: `add_order()`, `cancel_order()`, `modify_order()`.

### BookSide

One half of the order book (bids or asks). Templated on comparator for sort order. Manages price levels and provides best-price access.

### PriceLevel

FIFO queue of orders at a single price. Tracks aggregate quantity. Supports add, remove, and fill-front operations.

### PoolAllocator

Template memory pool that pre-allocates a fixed number of objects. Eliminates malloc/free from the hot path. Uses a free-list for O(1) allocate/deallocate.

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| Integer prices (cents) | Avoids floating-point comparison bugs |
| `std::map` over flat array | Handles sparse price levels efficiently |
| `std::list` over `std::deque` | O(1) iterator-based removal for cancels |
| Hash map for order lookup | O(1) cancel/modify by ID |
| Header-heavy design | Enables inlining for hot-path methods |
| Price-time priority | Industry standard; simplest fair matching |
| Modify = cancel + re-add | Preserves order type, loses time priority (correct behavior) |
