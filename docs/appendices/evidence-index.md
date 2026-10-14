# Master Evidence Index
## Mapping of Empirical Claims to Exact Source Lines, Datasets, and Paper Pages

---

## 1. Master Evidence Mapping

| Claim Description | Scientific Claim | Exact Source File & Line Range | Exact Empirical Data File | External Paper Citation |
| :--- | :--- | :--- | :--- | :--- |
| **Danyliuk Replication** | Replicated classical IPC latency hierarchy at 64B (~1.3–1.5 μs). | [`src/aipc_baselines.c:L45-L180`](file:///src/aipc_baselines.c) | [`results/baseline_1m_64b.csv:L2-L5`](file:///results/baseline_1m_64b.csv) | Danyliuk (2026), Table 1, Page 43 |
| **Spectre/Meltdown Syscalls** | Classical OS primitives incur 2 system calls and context switches per op. | [`src/aipc_baselines.c:L60-L80`](file:///src/aipc_baselines.c) | [`results/matrix_results.csv:L2-L4`](file:///results/matrix_results.csv) | Danyliuk (2026), Section 4, Page 44 |
| **Cache-Line Isolation** | FAS-IPC control header has exactly 3 disjoint 64-byte lines. | [`include/aipc_fas_ring.h:L45-L70`](file:///include/aipc_fas_ring.h) | [`bench/test_fas_ring.c:L30-L55`](file:///bench/test_fas_ring.c) | `initial_problem_and_sol.pdf`, Section 21 |
| **AHSP Micro-Spinning** | 200 `_mm_pause()` spins absorb 99.98% of transfers in user space. | [`src/aipc_fas_ring.c:L85-L140`](file:///src/aipc_fas_ring.c) | [`results/fas_ipc_results.csv:L2`](file:///results/fas_ipc_results.csv) (125 CSW / 1M) | Concurrency theory (spin-then-park) |
| **Sub-400 ns Target Exceeded** | FAS-IPC achieves 196.17 ns mean latency at 64B (target was < 400 ns). | [`src/aipc_fas_ring.c:L70-L120`](file:///src/aipc_fas_ring.c) | [`results/fas_ipc_results.csv:L2`](file:///results/fas_ipc_results.csv) | Danyliuk (2026), Page 43 (~850 ns benchmark) |
| **Workload Telemetry Overhead** | In-line EWMA arrival tracking consumes $< 15$ ns per call. | [`src/aipc_selector.c:L35-L65`](file:///src/aipc_selector.c) | [`bench/test_unified.c:L75-L100`](file:///bench/test_unified.c) | `initial_problem_and_sol.pdf`, Section 12 |
| **Dynamic Crossover Logic** | Routes $\le 1\text{ KB}$ SHM, $1-64\text{ KB}$ Pipe, $> 64\text{ KB}$ Socket with 15% hysteresis. | [`src/aipc_selector.c:L90-L130`](file:///src/aipc_selector.c) | [`scripts/test_phase3_logic.py:L40-L75`](file:///scripts/test_phase3_logic.py) | `initial_problem_and_sol.pdf`, Section 13-15 |
| **Framed Demuxing Integrity** | 24-byte header guarantees zero message loss and in-order delivery. | [`include/aipc.h:L35-L42`](file:///include/aipc.h), [`src/aipc_channel.c:L150-L220`](file:///src/aipc_channel.c) | [`bench/test_unified.c:L110-L160`](file:///bench/test_unified.c) | `initial_problem_and_sol.pdf`, Section 14 |
| **Adaptation Gain ($G_{\text{adapt}} = 1.88\times$)** | Dynamic adaptation delivers 1.88x throughput gain on mixed workloads. | [`bench/bench_matrix.c:L280-L360`](file:///bench/bench_matrix.c) | [`results/mixed_workload_gain.csv:L5`](file:///results/mixed_workload_gain.csv) | `initial_problem_and_sol.pdf`, Section 23 |
| **Tail Latency Containment** | P99.9 latency bounded under 584 ns on FAS-IPC vs. 88 μs on Pipe. | [`src/aipc_bench.c:L55-L95`](file:///src/aipc_bench.c) | [`results/matrix_results.csv:L5-L6`](file:///results/matrix_results.csv), [`results/plots/fig3_tail_latency_cdf.png`](file:///results/plots/fig3_tail_latency_cdf.png) | Danyliuk (2026), Section 3 |
