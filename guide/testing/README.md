# Testing & Benchmarking: Master Overview

Welcome to the Testing, Benchmarking & Reproducibility section of the **Adaptive Inter-Process Communication (A-IPC)** framework.

This directory documents the empirical validation methods, test suites, benchmark harnesses, raw performance datasets, and plotting tools that substantiate all claims in this research.

---

## Testing Documentation Directory

1. [Test Environment](test-environment.md)  
   Specifications of the test hardware, Linux operating system configuration, kernel build parameters, CPU affinity settings, and isolation rules.

2. [Test Cases & Correctness](test-cases-and-correctness.md)  
   Verification of functional correctness: zero-corruption payload validation, sequential FIFO ordering across transport hops, boundary edge cases, and crash recovery.

3. [Benchmark Methodology](benchmark-methodology.md)  
   Mathematical methodology: 1:1 echo ping-pong testing, monotonic nanosecond timers (`CLOCK_MONOTONIC_RAW`), warmup iterations, and exact percentile calculations (Min, P50, P90, P99, Max).

4. [Results & Comparison](results-and-comparison.md)  
   Master compilation of all raw empirical results: Danyliuk (2026) 64-byte replication, 11-payload matrix sweep (64 B to 64 KB), and mixed workload adaptation gain.

5. [Plotting & Visualization](plotting-and-visualization.md)  
   Documentation of our Python visualization pipeline (`scripts/plot_results.py`) and detailed analytical breakdowns of generated Figures 1 through 4.

6. [Reproducibility Guide](reproducibility.md)  
   Step-by-step instructions allowing any student, professor, or reviewer to clone the repository, compile the binaries, execute the test suites, and regenerate all figures in minutes.
