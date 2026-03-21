#!/usr/bin/env python3
"""Generate test order CSV files for NanoMatch."""

import argparse
import csv
import random

def generate_orders(n: int, seed: int = 42) -> list:
    random.seed(seed)
    orders = []
    order_ids = []
    next_id = 1

    for _ in range(n):
        # 20% chance of cancel if we have orders
        if order_ids and random.random() < 0.2:
            cancel_id = random.choice(order_ids)
            orders.append({
                'action': 'CANCEL',
                'id': cancel_id,
                'side': 'BUY',
                'type': 'LIMIT',
                'price': 0,
                'quantity': 0,
            })
            order_ids.remove(cancel_id)
        else:
            side = random.choice(['BUY', 'SELL'])
            order_type = random.choices(
                ['LIMIT', 'MARKET', 'IOC', 'FOK'],
                weights=[70, 10, 15, 5]
            )[0]
            price = random.randint(9800, 10200) if order_type != 'MARKET' else 0
            quantity = random.randint(1, 1000)

            orders.append({
                'action': 'NEW',
                'id': next_id,
                'side': side,
                'type': order_type,
                'price': price,
                'quantity': quantity,
            })
            if order_type == 'LIMIT':
                order_ids.append(next_id)
            next_id += 1

    return orders


def main():
    parser = argparse.ArgumentParser(description='Generate test orders for NanoMatch')
    parser.add_argument('-n', '--count', type=int, default=1000,
                        help='Number of orders to generate')
    parser.add_argument('-o', '--output', type=str, default='data/sample_orders.csv',
                        help='Output CSV file path')
    parser.add_argument('-s', '--seed', type=int, default=42,
                        help='Random seed for reproducibility')
    args = parser.parse_args()

    orders = generate_orders(args.count, args.seed)

    with open(args.output, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['action', 'id', 'side', 'type', 'price', 'quantity'])
        writer.writeheader()
        writer.writerows(orders)

    print(f"Generated {len(orders)} orders → {args.output}")


if __name__ == '__main__':
    main()
