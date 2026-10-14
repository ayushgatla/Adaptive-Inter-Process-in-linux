# Phase 1: Presentation & Viva Guide
## University Defense Walkthrough for Benchmark Baselines

---

## 1. Opening Statement (15–30 Seconds)
> "In Phase 1, we constructed an empirical experimental control. Before building an adaptive system, we replicated the classical Linux IPC baselines—Pipes, Sockets, POSIX Queues, and Naive Shared Memory—using nanosecond monotonic timers and CPU core pinning across 1,000,000 round-trip exchanges to reproduce and validate the findings of Danyliuk (2026)."

---

## 2. Diagram Walkthrough (What to Point At)
1. Point to **Core Pinning (`sched_setaffinity`)**: Explain that Producer is pinned to Core 1 and Consumer to Core 2 to eliminate CPU migration artifacts and cache-warming noise.
2. Point to **Syscall Entry/Exit**: Highlight that for Pipes and Sockets, each message exchange requires crossing the user/kernel privilege boundary twice, forcing the OS scheduler to execute a voluntary context switch.
3. Point to **`getrusage` Metrics**: Show that 1,000,000 exchanges generated ~1,000,000 voluntary context switches, proving the scheduler penalty.

---

## 3. Code Demonstration (Which Files to Show)
- Open [`include/aipc_bench.h`](file:///include/aipc_bench.h) and highlight `CLOCK_MONOTONIC_RAW` and `aipc_pin_cpu()`.
- Open [`src/aipc_baselines.c`](file:///src/aipc_baselines.c) and trace `run_pipe_benchmark()` and `run_naive_shm_benchmark()`.

---

## 4. Test Demonstration (Safe Terminal Command)
```bash
# Run 100k iteration baseline sweep
./bench_baselines --payload 64 --iterations 100000
```
- **Fallback:** If executing on a non-Linux host, display `results/baseline_1m_64b.csv` and execute `python scripts/verify_all_4_phases.py` to demonstrate passing Phase 1 audit.

---

## 5. Research Comparison Key Talking Point
> "While Danyliuk reported ~2.0 μs for pipes on a 3.3 GHz Intel CPU, our testbed recorded 1.40 μs. The critical scientific insight is that the **relative hierarchy and syscall cost remain identical**: all blocking OS primitives hit a ~1.4 μs barrier due to user/kernel context switches."

---

## 6. Time Budget Breakdown
- **Short Summary (1 minute):** Problem, methodology, replicated hierarchy table, and context switch finding.
- **Deep Walkthrough (3–5 minutes):** Walk through memory ordering in naive shared memory, explain false sharing, and justify why Phase 2 was necessary.
