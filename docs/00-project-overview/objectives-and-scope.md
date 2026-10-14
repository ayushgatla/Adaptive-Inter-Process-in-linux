# Objectives and Scope
## Engineering Goals, Technical Constraints, and Scope Boundaries

---

## 1. Concrete Engineering Objectives

The Adaptive Inter-Process Communication (A-IPC) framework was engineered to satisfy six concrete, measurable objectives:

### Objective 1: Unified Transport Abstraction API
- Implement a clean, opaque C11 channel interface:
  ```c
  int ipc_create(ipc_channel_t *chan, const char *name, ipc_role_t role);
  int ipc_send(ipc_channel_t *chan, const void *buf, size_t len);
  int ipc_receive(ipc_channel_t *chan, void *buf, size_t max_len);
  int ipc_close(ipc_channel_t *chan);
  ```
- Shield application logic entirely from underlying transport selection, file descriptors, memory mappings, and socket lifecycles.

### Objective 2: High-Precision Benchmark Harness
- Construct a microsecond/nanosecond-accurate benchmarking framework utilizing `CLOCK_MONOTONIC_RAW` (unaffected by NTP slew).
- Enforce strict CPU core affinity pinning via `sched_setaffinity` to eliminate inter-core migration artifacts and thread scheduler noise.
- Collect full statistical distributions: Mean, Min, Max, P50, P90, P99, and P99.9 latencies, message throughput (msg/s), bandwidth (MB/s), and voluntary/involuntary context switches (`getrusage`).

### Objective 3: Baseline Characterization & Danyliuk (2026) Replication
- Replicate the empirical baseline hierarchy of classical Linux IPC primitives (Anonymous Pipes, UNIX Domain Sockets, POSIX Message Queues, Naive Shared Memory) across the small-message spectrum (64 B to 1024 B).
- Verify the ~2 system calls per exchange overhead on classical primitives.

### Objective 4: Low-Latency Zero-Syscall Fast Path (FAS-IPC)
- Design and implement a lock-free Single-Producer Single-Consumer (SPSC) circular ring buffer.
- Guarantee strict 64-byte hardware cache line isolation (`alignas(64)`) between producer mutable variables (`tail`), consumer mutable variables (`head`), and synchronization coordination words (`futex_word`), completely preventing cross-core false sharing under the MESI protocol.
- Outperform Danyliuk's (2026) 850 ns shared-memory baseline by achieving $< 400$ ns mean round-trip latency at 64 bytes.
- Achieve 0 system calls per operation during steady-state streaming.

### Objective 5: Real-Time Workload Telemetry & Dynamic Crossover Selector
- Implement an in-line Workload Observer that calculates payload sizes and exponentially smoothed arrival rates using EWMA ($\alpha = 0.125$) with $< 15$ ns runtime overhead.
- Implement an Adaptive Selector that dynamically routes payloads across FAS-IPC ($\le 1024$ B), Pipes ($1025\text{ B} - 65536\text{ B}$), and Sockets ($> 65536\text{ B}$).
- Implement anti-flapping hysteresis requiring a 15% improvement threshold over a 16-sample window before executing a transport change.
- Guarantee discrete message framing (`aipc_frame_hdr_t`) and coordinated demuxing to achieve zero message loss and strict FIFO ordering during rapid transport switches.

### Objective 6: Adaptation Gain Evaluation & Academic Defense Artifacts
- Evaluate the adaptive framework across an 11-payload matrix ($64\text{ B} - 65536\text{ B}$).
- Prove the core research hypothesis on dynamic mixed workloads (80% 64 B + 20% 16 KB):
  $$G_{\text{adapt}} = \frac{\text{Throughput}_{\text{Adaptive}}}{\max(\text{Throughput}_{\text{Static-Pipe}}, \text{Throughput}_{\text{Static-Socket}}, \text{Throughput}_{\text{Static-MQ}})} \ge 1.80\times$$
- Generate publication-quality 300 DPI visualizations (Figure 1 through Figure 4) and comprehensive defense documentation.

---

## 2. In-Scope Technical Architecture

The following components and configurations are explicitly within the implementation and verification scope:
1. **Operating System:** Linux x86-64 kernels (tested on Linux 5.x / 6.x POSIX interfaces).
2. **Languages & Standards:** C11 (`<stdatomic.h>`, `<stdalign.h>`), POSIX.1-2008, Python 3.10+ (visualization).
3. **Communication Topology:** 1:1 bidirectional producer-consumer pair pinned to dedicated CPU cores.
4. **Transport Primitives:**
   - Anonymous Pipes (`pipe(2)`)
   - UNIX Domain Stream Sockets (`socketpair(2)`, `AF_UNIX`)
   - POSIX Message Queues (`mq_open(3)`, `mq_send(3)`)
   - Naive Shared Memory with Futex (`shm_open`, `mmap`, `SYS_futex`)
   - Fast-Path Shared Memory (FAS-IPC SPSC ring with AHSP)
5. **Workload Spectrum:** 64 bytes to 65,536 bytes (64 KB), message rates from 100 msg/s to 2.4+ million msg/s.

---

## 3. Explicit Out-of-Scope Boundaries (Non-Goals)

To preserve scientific rigor and prevent scope creep, the following areas are explicitly excluded from this phase of the project:
1. **Networked / Distributed Transports:** TCP/IP, UDP, RDMA, or multi-node networking. All communication is strictly local inter-process on a single physical host.
2. **Multi-Producer / Multi-Consumer (MPMC) Fast Path:** The FAS-IPC shared memory ring buffer is strictly Single-Producer Single-Consumer (SPSC). Multi-endpoint fan-out requires multiplexed POSIX message queues or UNIX domain datagram sockets.
3. **Kernel-Space Loadable Modules:** The framework operates purely in user space and utilizes standard POSIX/Linux syscalls (`SYS_futex`, `mmap`, `sched_setaffinity`). No custom kernel drivers are required.
4. **Arbitrary Big-Data Streaming (> 1 GB):** Multi-gigabyte shared memory mappings and zero-copy splice pipelines are left as future work.
5. **Non-x86 Architecture Optimizations:** The current implementation leverages x86-64 TSO memory ordering and `_mm_pause()` SSE intrinsics. ARM64 memory barrier support (`DMB ISHLD`/`ISHST`) is identified as future extension.
