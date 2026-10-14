# Architectural Evolution
## Lifecycle and Progression of the A-IPC Codebase

---

## 1. Evolution Timeline Overview

The A-IPC project evolved through a deliberate, structured engineering progression from an experimental benchmark harness to a complete adaptive runtime framework:

```
[Phase 1] Classical Baselines & Timing
    │   • Proved 1.4 us syscall latency floor in Pipes, Sockets, and MQ
    │   • Identified false-sharing flaw in Naive SHM (1451 ns)
    ▼
[Phase 2] Ultra-Low Latency FAS-IPC Engine
    │   • Introduced alignas(64) cache-line isolation
    │   • Introduced AHSP micro-spinning (200 _mm_pause iters)
    │   • Dropped latency to 196.17 ns (4.33x speedup vs Danyliuk)
    ▼
[Phase 3] Telemetry & Unified Channel Layer
    │   • Introduced ipc_send / ipc_receive abstraction
    │   • Integrated EWMA arrival rate observer (< 15 ns overhead)
    │   • Implemented 3-tier dynamic crossover with 15% hysteresis
    │   • Engineered framed demuxing (aipc_frame_hdr_t) for zero loss
    ▼
[Phase 4] Full Experimental Matrix & Adaptation Gain
    │   • Swept 11 payload sizes (64B to 64KB) across 5 configurations
    │   • Disproved viability of static shared memory on mixed traffic
    │   • Proved Adaptation Gain G_adapt = 1.88x and 46.86% latency reduction
    ▼
[Phase 5] Automated Visualization & Academic Defense
        • Generated 4 publication-standard 300 DPI figures
        • Synthesized comprehensive defense tables & viva question banks
```

---

## 2. Key Architectural Pivots & Engineering Decisions

### Pivot 1: From Naive Futex to AHSP Micro-Spinning
- **Initial State:** In early Phase 1 tests, shared memory synchronization immediately invoked `SYS_futex` upon finding the ready flag unset.
- **Problem:** Entering the kernel took ~1.45 μs, eliminating the zero-copy advantage of shared memory.
- **Pivot:** In Phase 2, we implemented the Adaptive Hybrid Spin-Park (AHSP) algorithm. By spinning for 200 iterations of `_mm_pause()` (~250–350 ns), steady-state handoffs were absorbed in user space, reducing context switches by 99.98% and dropping latency to 196 ns.

### Pivot 2: From Unpadded Structures to Strict 64-Byte Hardware Isolation
- **Initial State:** `naive_shm_t` placed the ready flag and data buffer in contiguous memory.
- **Problem:** Cross-core cache coherence invalidations (false sharing) degraded multi-core throughput.
- **Pivot:** In Phase 2, `fas_control_header_t` was strictly partitioned into three 64-byte aligned blocks (`alignas(64)`), completely eliminating cache line invalidation between producer and consumer cores.

### Pivot 3: From Unframed Byte Streams to Discrete Binary Framing
- **Initial State:** Streaming pipes and domain sockets merge consecutive writes into an unstructured stream.
- **Problem:** Dynamically switching from Pipe to FAS-IPC risked misaligning message boundaries and corrupting payload data.
- **Pivot:** In Phase 3, we introduced `aipc_frame_hdr_t` (magic word, sequence number, payload length, and checksum) to enforce strict framing and global sequence ordering across heterogeneous transports.
