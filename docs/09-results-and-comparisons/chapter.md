# Chapter 9: Results & Comparative Analysis
## Comprehensive Empirical Synthesis & Performance Deep Dive

---

## 1. Executive Summary

This chapter synthesizes the empirical evidence gathered across all benchmark runs, contrasting our results with published literature, analyzing the underlying micro-architectural mechanisms, and disclosing all project limitations with complete scientific transparency.

### Core Verified Achievements:
1. **Danyliuk (2026) Classical Baseline Replication:** Replicated the classical IPC latency hierarchy on local hardware across 1,000,000 exchanges:
   - Anonymous Pipe: **1404.53 ns** (vs. Danyliuk ~2100 ns)
   - UNIX Domain Socket: **1370.62 ns** (vs. Danyliuk ~2050 ns)
   - POSIX Message Queue: **1530.48 ns** (vs. Danyliuk ~14,500 ns)
   - Voluntary context switches confirmed at ~1 per round-trip exchange (~1,000,000 per 1M ops).
2. **FAS-IPC Shared Memory Breakthrough:** Outperformed Danyliuk's 850 ns benchmark by **$4.33\times$**, achieving:
   - Mean latency at 64 B: **196.17 ns** (P50: 190.00 ns, P99: 322.50 ns)
   - Steady-state kernel transitions: **0.00 syscalls/op**
   - Voluntary context switches: **125 per 1,000,000 ops** (a 99.98% kernel bypass rate)
   - Saturated throughput: **2,427,544 msg/s** (a 7.15x speedup over pipes)
3. **Core Thesis Empirical Validation ($G_{\text{adapt}}$):** Evaluated across an 80/20 dynamic mixed workload (80% 64 B + 20% 16 KB):
   - **Adaptation Gain ($G_{\text{adapt}}$):** **$1.88\times$** over the best static baseline (UNIX Socket).
   - **Latency Reduction:** **46.86%** lower latency than the best static baseline.
   - Proved that static shared memory fails functionally on mixed workloads due to slot overflow.

---

## 2. Chapter Structure

- [Results Summary](file:///docs/09-results-and-comparisons/results-summary.md): Master data tables across all 11 payload sizes and mixed workloads.
- [Paper vs. Project](file:///docs/09-results-and-comparisons/paper-vs-project.md): Detailed comparison against Danyliuk (2026) and Krishnaveni & Ruby (2016).
- [Performance Analysis](file:///docs/09-results-and-comparisons/performance-analysis.md): Micro-architectural breakdown of CPU cache lines, TLB, and syscall traps.
- [Missing Data & Limitations](file:///docs/09-results-and-comparisons/missing-data-and-limitations.md): Explicit disclosure of untested regimes and architectural boundaries.
