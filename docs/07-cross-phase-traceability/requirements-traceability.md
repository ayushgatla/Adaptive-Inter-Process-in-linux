# Requirements Traceability Matrix (RTM)
## Research Problem to Source Code, Test Evidence, and Presentation Claims

---

## 1. Master Traceability Matrix

The table below maps every high-level research requirement and hypothesis through to its design decision, concrete source implementation, test verification, measured result, and defensible presentation claim:

| ID | Research Requirement / Hypothesis | Design Decision | Source Implementation | Test Evidence | Measured Result | Defensible Presentation Claim | Claim Allowed? |
| :---: | :--- | :--- | :--- | :--- | :--- | :--- | :---: |
| **REQ-01** | Unified IPC Abstraction: Mask OS transport complexity behind single API. | Opaque channel handle with function pointer ops table. | [`include/aipc.h`](file:///include/aipc.h), [`src/aipc_channel.c`](file:///src/aipc_channel.c) | `test_unified --test-basic` | Clean compile, 10,000 exchanges verified. | "A-IPC provides a unified API (`ipc_send`, `ipc_receive`) decoupling apps from OS primitives." | **YES** |
| **REQ-02** | Replicate Classical Baselines: Establish local control against Danyliuk (2026). | 1:1 echo ping-pong testbeds for Pipe, Socket, MQ, Naive SHM. | [`src/aipc_baselines.c`](file:///src/aipc_baselines.c), [`bench/bench_baselines.c`](file:///bench/bench_baselines.c) | `baseline_1m_64b.csv` | Pipe: 1404 ns, Socket: 1370 ns, MQ: 1530 ns. | "Locally replicated classical IPC hierarchy matching Danyliuk (2026) Table 1." | **YES** |
| **REQ-03** | Eradicate False Sharing: Eliminate MESI cache line bouncing. | Strict 64B cache line isolation using `alignas(64)`. | [`include/aipc_fas_ring.h`](file:///include/aipc_fas_ring.h) (`pad0`, `pad1`, `pad2`) | `test_fas_ring --test-alignment` | All struct offsets `% 64 == 0`. | "FAS-IPC eliminates cross-core false sharing via strict 64-byte hardware cache alignment." | **YES** |
| **REQ-04** | Outperform 850 ns Baseline: Drop small-message latency $< 400$ ns. | SPSC lock-free circular ring with AHSP micro-spinning. | [`src/aipc_fas_ring.c`](file:///src/aipc_fas_ring.c) (`FAS_AHSP_SPIN_LIMIT 200`) | `fas_ipc_results.csv` | Mean latency: **196.17 ns** at 64 B. | "FAS-IPC achieves 196 ns latency at 64B, beating Danyliuk's 850 ns benchmark by 4.33x." | **YES** |
| **REQ-05** | Eliminate Kernel Transitions: 0 syscalls on streaming shared memory. | 200 `_mm_pause()` spins absorb handoffs in user memory. | [`src/aipc_fas_ring.c`](file:///src/aipc_fas_ring.c) (`SYS_futex`) | `perf stat -e raw_syscalls:sys_enter` | **0.00 syscalls/op** in steady-state streaming. | "FAS-IPC eliminates system calls during streaming, achieving 100% userspace transfer." | **YES** |
| **REQ-06** | Real-Time Workload Telemetry: Track arrival rate with $< 15$ ns overhead. | In-line EWMA arrival tracker with $\alpha = 0.125$ ($1/8$). | [`src/aipc_selector.c`](file:///src/aipc_selector.c) | `test_phase3_logic.py` | Converges within 30 samples (<2% error); overhead 11.4 ns. | "Workload telemetry tracks arrival rate in real-time with sub-15 ns overhead." | **YES** |
| **REQ-07** | Dynamic Crossover Dispatch: Route based on measured payload zones. | 3-tier empirical crossover: $\le 1\text{ KB}$ SHM, $1-64\text{ KB}$ Pipe, $> 64\text{ KB}$ Socket. | [`src/aipc_selector.c`](file:///src/aipc_selector.c) | `matrix_results.csv` | Monotonic bandwidth scaling up to 2783 MB/s. | "A-IPC routes payloads dynamically across fast-path SHM, pipes, and sockets." | **YES** |
| **REQ-08** | Anti-Flapping Stability: Prevent rapid transport thrashing. | Hysteresis: require 15% superiority over 16-sample window. | [`src/aipc_selector.c`](file:///src/aipc_selector.c) (`AIPC_HYSTERESIS_MARGIN`) | `test_unified --test-mixed-sizes` | 100,000 rapid switches executed without oscillation. | "Hysteresis dampening ensures stable transport switching without flapping." | **YES** |
| **REQ-09** | Zero Message Loss & In-Order Delivery: Prevent packet stranding during switches. | Discrete framing header (`aipc_frame_hdr_t`) and demuxing. | [`src/aipc_channel.c`](file:///src/aipc_channel.c) | `test_unified --test-mixed-sizes` | 100,000 / 100,000 messages verified with exact sequence match. | "Framed demuxing guarantees zero message drop and strict FIFO ordering during transitions." | **YES** |
| **REQ-10** | Adaptation Gain ($G_{\text{adapt}} \ge 1.80\times$): Prove adaptive beats static. | Evaluate 80% 64B + 20% 16KB mixed dynamic workload. | [`bench/bench_matrix.c`](file:///bench/bench_matrix.c) | `mixed_workload_gain.csv` | **$G_{\text{adapt}} = 1.88\times$**, **46.86% latency reduction**. | "Adaptive A-IPC delivers an Adaptation Gain of 1.88x and 46.86% latency reduction over static IPC." | **YES** |

---

## 2. Audited Disallowed Claims (Scientific Integrity Checks)

In accordance with Critical Rule 2, the following statements are strictly disallowed:
- ❌ *"A-IPC is universally faster than all other IPCs on all payloads."* (Disallowed: at 64B, pure FAS-IPC is ~12 ns faster than Adaptive A-IPC due to zero selector overhead; at 64KB, raw socket is slightly faster than adaptive pipe).
- ❌ *"A-IPC completely replaces the Linux kernel socket subsystem."* (Disallowed: A-IPC relies on UNIX domain sockets as an underlying bulk transport).
- ❌ *"A-IPC supports unlimited concurrent multi-producer processes."* (Disallowed: FAS-IPC fast path is strictly Single-Producer Single-Consumer).
