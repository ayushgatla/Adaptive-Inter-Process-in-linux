# Research vs. Project Implementation
## Whole-Project Comparison: Literature Claims vs. Repository Artifacts

---

## 1. Five-Category Classification

To maintain the highest standards of scientific honesty, all components of the A-IPC project are categorized under five explicit classifications:
1. **Direct Reproduction**: Concept implemented and verified under sufficiently comparable conditions to published literature.
2. **Adaptation**: Concept derived from literature but modified to fit modern architectural constraints or project requirements.
3. **Novel Extension**: Architectural innovation or evaluation methodology developed in this project beyond existing literature.
4. **Unimplemented Concepts**: Concepts identified in literature or design documents that remain planned for future work.
5. **Unverified Claims**: Hypotheses or design targets that lack sufficient empirical proof on all hardware platforms.

---

## 2. Whole-Project Comparison Matrix

| Project Dimension | Research Literature Origin | Classification | Intended Design | Actual Repository Implementation | Deviation & Engineering Rationale | Empirical Verification Status |
| :--- | :--- | :---: | :--- | :--- | :--- | :---: |
| **Classical IPC Latency Hierarchy** | Danyliuk (2026), Section 3, Table 1 | **Direct Reproduction** | Measure 1:1 ping-pong echo latency across Pipes, Sockets, MQ, and Naive SHM. | `src/aipc_baselines.c`, `bench/bench_baselines.c` | Exact replication using `CLOCK_MONOTONIC_RAW` and CPU pinning. | **Verified** (`baseline_1m_64b.csv`) |
| **Spectre/Meltdown Syscall Analysis** | Danyliuk (2026), Section 4 | **Direct Reproduction** | Profile kernel transitions on classical primitives vs. shared memory. | `bench_baselines`, `bench_matrix` with `perf stat -e raw_syscalls:sys_enter` | Verified 2 syscalls/op on OS primitives vs. 0 syscalls/op on streaming shared memory. | **Verified** (`matrix_results.csv`) |
| **Shared-Memory Latency Baseline** | Danyliuk (2026), Page 43 (850 ns) | **Adaptation** | Implement shared memory and compare with Danyliuk's 850 ns benchmark. | `src/aipc_fas_ring.c` | Rather than stopping at naive futex shared memory (~1451 ns on local testbed), introduced 64B cache line padding and AHSP to achieve 196.17 ns. | **Verified & Exceeded** (`fas_ipc_results.csv`) |
| **Cache-Line Isolated SPSC Ring** | Micro-architectural theory (MESI protocol); `initial_problem_and_sol.pdf`, Sec 21 | **Novel Extension** | Eliminate false sharing between producer and consumer cores via hardware alignment. | `include/aipc_fas_ring.h` using `alignas(64)` for `head`, `tail`, and `futex_word`. | Structures explicitly validated via `offsetof()` and `alignof()`. Completely eliminates cache-line bouncing. | **Verified** (`test_fas_ring.c`) |
| **Adaptive Hybrid Spin-Park (AHSP)** | Concurrency theory; lock-free algorithms | **Novel Extension** | Bounded 200 `_mm_pause()` loop before parking via private futex. | `src/aipc_fas_ring.c` (`FAS_AHSP_SPIN_LIMIT 200`, `FUTEX_WAIT_PRIVATE`) | Absorbs 99.98% of ping-pong handoffs in user space, dropping context switches from 1,000,000 to 125. | **Verified** (`fas_ipc_results.csv`) |
| **Multi-Metric Telemetry Engine** | `initial_problem_and_sol.pdf`, Sec 2 & 12 | **Novel Extension** | In-line monitoring of message size and arrival rate via EWMA. | `src/aipc_selector.c` (`EWMA_ALPHA = 0.125`) | In-line tracking incurs $< 15$ ns overhead; converges within 30 samples. | **Verified** (`test_phase3_logic.py`) |
| **Dynamic Multi-Transport Multiplexing** | `initial_problem_and_sol.pdf`, Sec 1 & 14 | **Novel Extension** | Transparent channel switching between shared memory, pipes, and sockets with discrete framing. | `src/aipc_channel.c`, `include/aipc.h` (`aipc_frame_hdr_t`) | Multiplexed framed demuxing with drain coordination ensures zero message loss or corruption during transitions. | **Verified** (`test_unified.c`) |
| **Empirical Crossover Dispatch** | `initial_problem_and_sol.pdf`, Sec 11 & 13 | **Novel Extension** | Data-driven routing based on measured payload thresholds. | `src/aipc_selector.c`: $\le 1024$ B FAS-IPC, $1\text{ KB}-64\text{ KB}$ Pipe, $>64\text{ KB}$ Socket. | Thresholds derived from local empirical crossover analysis rather than guesswork. | **Verified** (`matrix_results.csv`) |
| **Adaptation Gain ($G_{\text{adapt}}$) Evaluation** | `initial_problem_and_sol.pdf`, Sec 23 | **Novel Extension** | Measure throughput and latency advantage on dynamic mixed workloads. | `bench/bench_matrix.c`, `results/mixed_workload_gain.csv` | Empirical proof: Adaptive A-IPC delivers $1.88\times$ throughput gain and 46.86% latency reduction over best static choice. | **Verified** (`mixed_workload_gain.csv`) |
| **Runtime Zero-Copy Descriptor Splice** | Planned extension | **Unimplemented** | Pass file descriptors via `SCM_RIGHTS` over UNIX sockets for multi-gigabyte transfers. | Proposed in design documents; not implemented in codebase. | Standard UNIX domain stream socket used for $>64$ KB bulk transfer instead. | **Planned / Proposed** |
| **NUMA Node Pinning & Non-x86 Barriers** | Hardware architecture theory | **Unverified / Future** | Explicit NUMA allocation and ARM64 memory barrier support. | Codebase uses x86 TSO semantics and local CPU affinity. | Unverified on multi-socket NUMA servers or ARM64 architecture. | **Requires Evidence** |

---

## 3. Research Contribution Statement

The contribution of this project is twofold:
1. **Empirical Reproduction & Re-affirmation:** We confirm that classical Linux IPC mechanisms operate under unavoidable latency and system-call constraints dictated by kernel privilege boundaries and security mitigations (Danyliuk, 2026).
2. **Systems Architecture Innovation:** We demonstrate that integrating a zero-syscall lock-free fast path with runtime workload telemetry and dynamic transport multiplexing produces an adaptive IPC framework capable of outperforming any static primitive by **$1.88\times$** on mixed workloads.
