# Claims to Defend vs. Claims to Avoid
## Guidelines for Academic Rigor and Research Integrity in Oral Defense

---

## 1. Matrix of Defensible Claims vs. Indefensible Exaggerations

In a university project defense or peer review, making exaggerated or ungrounded claims destroys examiner credibility. The table below delineates the boundary between **fully defensible scientific statements** (grounded in repository code and CSV data) and **unsupported claims that must be avoided**:

| Topic / Area | ❌ Disallowed / Exaggerated Statement | ✅ Fully Defensible Scientific Statement | Grounding Repository Evidence |
| :--- | :--- | :--- | :--- |
| **Performance Superiority** | *"Adaptive A-IPC is universally faster than all other IPC mechanisms across all payloads and hardware."* | *"Adaptive A-IPC achieves a 1.88x throughput advantage and 46.86% latency reduction under dynamic mixed workloads (80% 64B + 20% 16KB) compared to any single static primitive."* | `results/mixed_workload_gain.csv`, `results/plots/fig4_adaptation_gain.png` |
| **Small-Message Fast Path** | *"Our shared memory engine is optimal and zero-latency."* | *"FAS-IPC achieves 196.17 ns mean latency at 64B (beating Danyliuk's 850 ns benchmark by 4.33x) with 0 system calls in steady-state streaming due to 64B cache line isolation and AHSP micro-spinning."* | `results/fas_ipc_results.csv`, `include/aipc_fas_ring.h` |
| **Literature Replication** | *"We replicated Danyliuk's exact microsecond numbers on our system."* | *"We replicated the classical IPC latency hierarchy reported by Danyliuk (2026) Table 1. On our modern testbed, Pipes and Sockets measured ~1.3–1.4 μs, confirming ~2 syscalls/op and voluntary context switches."* | `results/baseline_1m_64b.csv`, `docs/02-phase-1-baselines/research-alignment.md` |
| **Selector Overhead** | *"Dynamic transport selection adds zero overhead."* | *"The Workload Observer and crossover selector add approximately 11 to 14 nanoseconds of runtime overhead in `ipc_send()`, representing less than 6% of fast-path transit time."* | `src/aipc_selector.c`, `bench/test_unified.c` |
| **Transport Switching** | *"Transport switching is instant and completely free."* | *"Dynamic transport switching incurs demuxing checks and cache eviction, but anti-flapping hysteresis (15% barrier, 16-sample window) and discrete framing headers guarantee zero message loss."* | `src/aipc_channel.c`, `include/aipc.h` (`aipc_frame_hdr_t`) |
| **Concurrency Support** | *"A-IPC supports arbitrary multi-process clusters with dozens of competing writers."* | *"The FAS-IPC fast path is strictly Single-Producer Single-Consumer (1:1). For multi-producer topologies, traffic must be multiplexed or routed to POSIX message queues or UNIX domain sockets."* | `include/aipc_fas_ring.h`, `docs/00-project-overview/objectives-and-scope.md` |
| **Kernel Replacement** | *"A-IPC replaces the Linux kernel network and IPC subsystems."* | *"A-IPC operates entirely in user space, leveraging native Linux primitives (`shm_open`, `pipe`, `socketpair`, `SYS_futex`) and orchestrating them dynamically."* | `src/aipc_channel.c`, `Makefile` |

---

## 2. Key Defense Soundbites (Memorize for Viva)

1. **On False Sharing:** *"By aligning our producer and consumer control structures to 64-byte boundaries with `alignas(64)`, we completely prevented the MESI cache coherence protocol from bouncing cache lines across cores."*
2. **On AHSP:** *"AHSP gives us the best of both worlds: 200 iterations of `_mm_pause()` absorb 99.98% of handoffs in userspace without context switching, while the futex fallback ensures zero wasted CPU power if the channel goes idle."*
3. **On Dynamic Gain:** *"No single static IPC can win across both signaling and bulk data. Static shared memory fails on 16 KB bursts, and static sockets are bogged down by syscalls on 64 B frames. By adapting at runtime, A-IPC nearly doubles throughput."*
