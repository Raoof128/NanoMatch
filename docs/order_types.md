# Order Types

## Supported Order Types

### Limit Order
- Rests on the book at a specified price until filled or cancelled
- Most common order type in production exchanges
- If the price crosses the opposite side, it matches immediately; any remainder rests

### Market Order
- Executes immediately at the best available price
- Internally represented with an aggressive price (MAX_PRICE for buys, MIN_PRICE for sells)
- Does **not** rest on the book — unfilled remainder is cancelled

### Immediate or Cancel (IOC)
- Fill whatever quantity is immediately available, cancel the rest
- Useful when partial fills are acceptable but resting is not
- Similar to market but with a price limit

### Fill or Kill (FOK)
- Must fill the entire order quantity or cancel completely
- Pre-checks available liquidity before executing
- No partial fills — all or nothing

## Order Lifecycle

```
New Order → Validate → Match Against Opposite Side → Place Remainder (or Cancel)
                                ↓
                         Generate Trades
```

## Cancel Operation
- Removes an existing resting order by OrderID
- O(1) via hash map lookup
- Returns `false` if order not found

## Modify Operation
- Implemented as cancel + re-add (loses time priority)
- Atomic: if cancel succeeds, new order is placed
- May trigger matching if new price crosses
