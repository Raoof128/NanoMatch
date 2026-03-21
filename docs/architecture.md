# NanoMatch Architecture

## Overview

NanoMatch implements a price-time priority (FIFO) matching engine — the core component of every financial exchange.

## Data Flow

```
Order Input → Matching Engine → Order Book → Trade Output
                                    ↓
                              Stats Engine
```

## Core Data Structures

### Order Book (per instrument)

Each instrument (e.g., AAPL) has its own `OrderBook` containing:

- **BidSide** (`std::map<Price, PriceLevel, std::greater>`) — buy orders sorted highest-first
- **AskSide** (`std::map<Price, PriceLevel, std::less>`) — sell orders sorted lowest-first
- **Order Lookup** (`std::unordered_map<OrderID, OrderLocation>`) — O(1) cancel/modify

### Price Level

Each price level is a FIFO queue (`std::list<Order>`) maintaining time priority. Orders at the same price are matched in arrival order.

### Complexity

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Add order (new level) | O(log M) | M = number of price levels |
| Add order (existing level) | O(1) | Append to linked list |
| Cancel order by ID | O(1) | Hash lookup + list erase |
| Match (best price) | O(1) | Front of best level's list |
| Get best bid/ask | O(1) | Front of sorted map |

## Matching Algorithm

1. Incoming order checks opposite side for crossable prices
2. At each matching price level, fills against front-of-queue orders (FIFO)
3. Generates `Trade` records for each fill
4. Remaining quantity either rests on book (Limit) or is cancelled (Market/IOC)
5. FOK orders pre-check full fillability before executing

## Component Diagram

```
MatchingEngine
  └── OrderBook (per symbol)
        ├── BidSide (std::map<Price, PriceLevel, greater>)
        ├── AskSide (std::map<Price, PriceLevel, less>)
        └── OrderLookup (unordered_map<OrderID, Location>)
              └── PriceLevel
                    └── std::list<Order> (FIFO queue)
```

## Memory Pool

The `PoolAllocator` pre-allocates Order objects to eliminate heap allocation from the hot path. This is critical for achieving sub-microsecond latencies.
