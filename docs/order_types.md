# Order Types

## Supported Order Types

### Limit Order (`OrderType::Limit`)

Rests on the book at a specified price until filled or cancelled.

- If the limit price crosses the opposite side, it matches immediately
- Any unfilled remainder is placed on the book at the specified price
- The most common order type on production exchanges

**Example:**
```
Book state:   Best Ask = $101.50 x 500
Incoming:     BUY 200 shares @ $101.50 (Limit)
Result:       Trade: 200 @ $101.50, Ask reduced to 300 remaining
```

### Market Order (`OrderType::Market`)

Executes immediately at the best available price. Does not specify a price.

- Internally represented with an aggressive price (`MAX_PRICE` for buys, `MIN_PRICE` for sells)
- Sweeps through available price levels until filled
- Does **not** rest on the book — unfilled remainder is cancelled
- Guarantees execution (if liquidity exists) but not price

**Example:**
```
Book state:   Best Ask = $101.50 x 100, Next Ask = $102.00 x 200
Incoming:     BUY 250 shares (Market)
Result:       Trade 1: 100 @ $101.50, Trade 2: 150 @ $102.00
```

### Immediate or Cancel (`OrderType::ImmediateOrCancel`)

Fill whatever quantity is immediately available, cancel the rest.

- Like a market order but with a price limit
- Partial fills are acceptable
- Never rests on the book
- Unlike a limit order, its price is not checked: a price of zero or below is accepted and simply crosses nothing (buy) or everything (sell)

**Example:**
```
Book state:   Best Ask = $101.50 x 100
Incoming:     BUY 300 shares @ $101.50 (IOC)
Result:       Trade: 100 @ $101.50, remaining 200 cancelled
```

### Fill or Kill (`OrderType::FillOrKill`)

Must fill the entire order quantity or cancel completely. No partial fills.

- Pre-checks available liquidity across all crossable price levels
- If total available >= order quantity → executes
- If insufficient liquidity → rejects entirely (no modification to the book)
- Atomic: either all fills happen or none

**Example (rejected):**
```
Book state:   Best Ask = $101.50 x 100
Incoming:     BUY 500 shares @ $101.50 (FOK)
Result:       Rejected — only 100 available, need 500
```

## Order Lifecycle

```
New Order
  │
  ├── Validate (OrderValidator, applied by MatchingEngine::process)
  │
  ├── Book checks → REJECT if the ID is 0 or already resting, the quantity is 0,
  │                 or a limit price is zero or below
  │
  ├── Market? → set aggressive price
  │
  ├── FOK? → pre-check fillability
  │   └── insufficient? → REJECT
  │
  ├── MATCH against opposite side
  │   └── generate Trade records
  │
  └── Handle remainder:
      ├── Limit   → PLACE on book
      ├── Market  → CANCEL remainder
      ├── IOC     → CANCEL remainder
      └── FOK     → (fully filled, no remainder)
```

## What gets turned away

`OrderBook::add_order` and `modify_order` return no trades and leave the book unchanged when a request cannot be honoured. `OrderBook::last_reject()` says why, and `MatchingEngine::last_error()` gives the same as text.

| Reason | When |
|--------|------|
| `InvalidOrderId` | the order ID is 0, which is reserved |
| `DuplicateOrderId` | an order with that ID is already resting. A filled or cancelled order's ID may be used again |
| `InvalidQuantity` | the quantity is 0 |
| `InvalidPrice` | a limit order, or a modify, with a price of zero or below |
| `UnknownOrder` | a modify for an ID that is not resting |
| `FillOrKillUnfilled` | a fill-or-kill order that the book cannot fill completely |

An empty trade list on its own does not mean rejection: a limit order that rests without trading returns one too.

## Cancel Operation

Removes a resting order by `OrderID`.

- Average O(1): hash map lookup, then linked-list erase (plus a tree erase if the level empties)
- Returns `false` if no order with that ID is resting. That includes an order that has been filled completely: it is no longer on the book
- Cancelling a partially filled order removes what is left of it
- Removes the price level from the map if it becomes empty

## Modify Operation

Changes the price and/or remaining quantity of a resting order. The new quantity is the new *remaining* quantity.

- **Lower quantity, same price:** changed in place. The order keeps its position in the queue
- **Higher quantity, or a different price:** cancel and re-add. The order joins the back of the queue at the new price and may match immediately; any trades are returned
- A quantity of 0, a price of zero or below, or an unknown ID changes nothing (see the table above). To remove an order, cancel it
- The order keeps its ID and side

## Self-trades

Orders carry an ID, a side, a price and a quantity, and nothing that says who sent them. The book therefore cannot tell when both sides of a trade belong to the same participant, and there is no self-trade prevention: such orders match like any others.

## Limits

- Prices are 32-bit integers (cents) and a single order's quantity is 32-bit. A level's total is kept in 64 bits, so many large orders at one price do not overflow it.
- There is no notional (price × quantity) calculation anywhere in the engine, so nothing to overflow there.
- One thread. Nothing is synchronised.

## Time in Force

| Value | Behavior |
|-------|----------|
| `GoodTillCancel` | Remains on book until filled or explicitly cancelled. This is how every limit order behaves |
| `Day` | Not implemented. The enum value exists and nothing reads it |
