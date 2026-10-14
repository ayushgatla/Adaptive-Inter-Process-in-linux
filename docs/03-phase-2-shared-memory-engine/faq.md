# Phase 2: Frequently Asked Questions (FAQ)

---

### Q1: What is false sharing and how does FAS-IPC eliminate it?
**Answer:** False sharing occurs when two CPU cores update independent variables that reside within the same 64-byte hardware cache line. Even though the variables are logically unrelated, the CPU hardware cache coherence protocol (MESI) must invalidate the entire cache line across all cores whenever a write occurs. FAS-IPC eliminates this by using C11 `alignas(64)` and padding bytes, ensuring `tail`, `head`, and `futex_word` occupy three completely disjoint 64-byte memory lines.

### Q2: What memory order semantics are used in FAS-IPC and why?
**Answer:** FAS-IPC uses C11 explicit acquire-release semantics:
- The producer writes the payload into the ring slot and then updates `tail` using `atomic_store_explicit(..., memory_order_release)`. This ensures all payload bytes are globally visible in memory before `tail` is updated.
- The consumer reads `tail` using `atomic_load_explicit(..., memory_order_acquire)`. This guarantees subsequent reads of the slot payload see the producer's written data without processor instruction reordering.
- Using acquire-release avoids expensive sequential consistency (`memory_order_seq_cst`) hardware bus-locking instructions on x86-64.

### Q3: Why is the ring slot size fixed to 1024 bytes?
**Answer:** SPSC circular rings require fixed slot sizes to support lock-free arithmetic without dynamic memory allocation or fragmentation. 1024 bytes was selected based on Danyliuk's (2026) small-message boundary and empirical findings that signaling and coordination payloads in HPC and microservices are $\le 1024$ B. Larger payloads are routed to streaming transports in Phase 3.

### Q4: Why are slots padded to 1088 bytes instead of 1024 bytes?
**Answer:** Each slot structure contains `uint32_t len` (4 bytes), `uint32_t flags` (4 bytes), and `uint8_t payload[1024]` (1024 bytes), totaling 1032 bytes. 1032 is not a multiple of 64 ($1032 / 64 = 16.125$). Adding 56 padding bytes brings the total to $1088 = 17 \times 64$ bytes, guaranteeing that every slot begins exactly on an L1 cache line boundary.

### Q5: How does the Adaptive Hybrid Spin-Park (AHSP) algorithm work?
**Answer:** AHSP operates in two stages:
1. **Micro-Spin Stage:** When waiting for data, the consumer executes a bounded loop of up to 200 iterations of `_mm_pause()`. This consumes ~250–350 ns, matching the core-to-core cache line transfer latency. If data arrives, it is processed with zero system calls.
2. **Futex Stage:** If no message arrives after 200 iterations, the consumer marks `waiting_readers = 1` and calls `SYS_futex` with `FUTEX_WAIT_PRIVATE`. The producer checks this flag and calls `FUTEX_WAKE_PRIVATE` only when a reader is actually parked.

### Q6: Why use `_mm_pause()` in the spin loop instead of a tight `while` loop?
**Answer:** A tight spin loop without `_mm_pause()` causes CPU execution pipelines to speculatively execute instructions ahead of time, leading to pipeline flushes and excessive thermal throttling. The `PAUSE` instruction informs the CPU pipeline that a spin-wait is occurring, temporarily stalling pipeline execution by ~1.5 ns to conserve power and avoid memory bus saturation.

### Q7: Why is `FUTEX_WAIT_PRIVATE` used instead of `FUTEX_WAIT`?
**Answer:** `FUTEX_WAIT_PRIVATE` informs the Linux kernel that the futex word is located in memory mapped within the same process or shared among processes with the same address mapping semantics, allowing the kernel to optimize hash bucket locking and bypass global IPC namespace lookups.

### Q8: How did FAS-IPC achieve 0 system calls per operation during steady-state streaming?
**Answer:** During continuous streaming between pinned cores, the consumer's micro-spin loop consistently catches arriving messages within the 200-spin window before the loop expires. Because the queue never stays empty long enough to trigger futex sleep, all writes and reads execute purely in user-space virtual memory without invoking the Linux kernel.

### Q9: Can FAS-IPC handle multiple concurrent producers or consumers?
**Answer:** No. FAS-IPC is strictly a Single-Producer Single-Consumer (SPSC) ring. If multiple threads write to the ring concurrently, a data race would occur on `tail`. Supporting multiple producers would require an atomic compare-and-swap (`CAS`) loop or ticket locks, which would significantly increase latency. For multi-producer workloads, A-IPC routes traffic to POSIX message queues or UNIX domain datagram sockets.

### Q10: Examiner Challenge: "What happens if a process crashes while holding data in FAS-IPC?"
**Answer:** Because FAS-IPC is lock-free, there are no kernel mutexes held by either process. If the producer crashes, the consumer drains any remaining slots up to `tail` and then parks passively in futex sleep. However, unlike UNIX domain sockets which detect `ECONNRESET` via the kernel, shared memory requires a heartbeat or OS death notification (e.g., `pidfd_open` or socket control channel) to detect abnormal peer termination. This is addressed in the Phase 3 channel management layer.
