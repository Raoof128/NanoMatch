#!/usr/bin/env python3
"""Plot latency histogram from NanoMatch benchmark output."""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys

def main():
    csv_file = sys.argv[1] if len(sys.argv) > 1 else "latency_histogram.csv"

    df = pd.read_csv(csv_file)
    latencies = df["latency_ns"].values

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

    # Histogram
    ax1.hist(latencies, bins=200, color="#0C2340", edgecolor="white", alpha=0.85)
    ax1.set_xlabel("Latency (ns)")
    ax1.set_ylabel("Frequency")
    ax1.set_title("NanoMatch — Per-Operation Latency Distribution")
    ax1.axvline(np.percentile(latencies, 50), color="orange", linestyle="--",
                label=f"p50: {np.percentile(latencies, 50):.0f} ns")
    ax1.axvline(np.percentile(latencies, 99), color="red", linestyle="--",
                label=f"p99: {np.percentile(latencies, 99):.0f} ns")
    ax1.legend()

    # CDF
    sorted_lat = np.sort(latencies)
    cdf = np.arange(1, len(sorted_lat) + 1) / len(sorted_lat)
    ax2.plot(sorted_lat, cdf * 100, color="#0C2340", linewidth=1.5)
    ax2.set_xlabel("Latency (ns)")
    ax2.set_ylabel("Percentile (%)")
    ax2.set_title("NanoMatch — Cumulative Latency Distribution")
    ax2.axhline(99, color="red", linestyle="--", alpha=0.5, label="p99")
    ax2.axhline(50, color="orange", linestyle="--", alpha=0.5, label="p50")
    ax2.legend()

    plt.tight_layout()
    plt.savefig("latency_report.png", dpi=150)
    plt.show()
    print("Saved latency_report.png")


if __name__ == '__main__':
    main()
