# Systems & Concurrency Glossary
## Formal Definitions for Adaptive Inter-Process Communication

---

### Adaptive Hybrid Spin-Park (AHSP)
A two-phase synchronization protocol designed to eliminate kernel context-switch latency while preventing indefinite CPU spinning. The thread first executes a bounded micro-spin loop (e.g., 200 iterations of `_mm_pause()`, taking ~250–350 ns). If the condition is not met within this window, the thread transitions to a passive state by registering a waiter flag and parking in the kernel via `SYS_futex` (`FUTEX_WAIT_PRIVATE`).

### Adaptation Gain ($G_{\text{adapt}}$)
A dimensionless performance metric quantifying the throughput advantage achieved by dynamic runtime transport adaptation over the best statically chosen IPC mechanism under identical multi-modal workloads:
$$G_{\text{adapt}} = \frac{\text{Throughput}_{\text{adaptive}}}{\max_{T \in \text{Static}} (\text{Throughput}_T)}$$
A value of $G_{\text{adapt}} > 1.0$ indicates that runtime adaptation outperforms any single static primitive.

### Anonymous Pipe
A unidirectional, kernel-buffered byte stream created via `pipe(2)`. Exists only in kernel memory and connects related processes sharing common file descriptors. Lacks discrete message boundaries and has a default buffer capacity of 64 KB.

### Cache-Line Alignment & Isolation
The hardware memory technique of aligning data structures to the processor's native L1/L2 cache line boundary (64 bytes on modern x86-64 and ARM64 CPUs) using `alignas(64)`. Isolating frequently written variables into separate cache lines prevents false sharing.

### Cache Coherence (MESI Protocol)
The hardware protocol (Modified, Exclusive, Shared, Invalid) that maintains consistent data across per-core CPU caches. When Core 1 writes to a cache line shared by Core 2, the line in Core 2's cache is invalidated, forcing an expensive bus transaction or L3 cache fetch.

### Crossover Point
The specific operational boundary (e.g., payload size in bytes or arrival rate in msg/s) where the performance curve of one IPC transport intersects and surpasses another. For example, the point where pipe throughput overtakes shared-memory slot efficiency.

### Cumulative Distribution Function (CDF)
A statistical distribution curve $F(x) = P(X \le x)$ plotting latency percentiles from P0 (minimum) to P100 (maximum). CDFs visualize latency determinism and tail jitter (P99, P99.9) under benchmark stress.

### Exponentially Weighted Moving Average (EWMA)
A recursive statistical filtering algorithm that calculates a moving average where exponentially decreasing weights are assigned to older observations:
$$S_t = \alpha \cdot Y_t + (1 - \alpha) \cdot S_{t-1}$$
In A-IPC, $\alpha = 0.125$ ($1/8$), enabling low-cost integer-shift computation while smoothing high-frequency arrival rate jitter.

### False Sharing
A micro-architectural performance bug occurring when multiple CPU cores modify distinct, independent variables that happen to reside within the same 64-byte hardware cache line. The cache coherence protocol repeatedly invalidates the cache line across cores, causing severe memory stall cycles despite the absence of logical race conditions.

### FAS-IPC (Fast-Path Adaptive Shared-Memory IPC)
The custom ultra-low latency shared-memory engine developed in Phase 2. Features an SPSC circular ring buffer, 64-byte isolated cache headers, and AHSP futex synchronization to achieve sub-200 ns latency and zero kernel transitions.

### Futex (`fast userspace mutex`)
A Linux kernel system call (`sys_futex`) and memory primitive providing userspace lock checking via atomic operations. The kernel is invoked only when lock contention occurs (`FUTEX_WAIT`) or when waking sleeping threads (`FUTEX_WAKE`).

### Hysteresis
A control system decision mechanism that prevents rapid, destabilizing oscillations (thrashing) between states. In A-IPC, the adaptive selector requires candidate transport scores to exceed current transport scores by at least 15% ($\Delta \ge 0.15$) across a 16-sample window before triggering a transport switch.

### Kernel Page-Table Isolation (KPTI)
A Linux kernel security mitigation introduced in 2018 to prevent user-space attacks (Meltdown) from reading privileged kernel memory. KPTI splits user-space and kernel-space page tables, substantially increasing the CPU cycle cost of every system call entry and exit.

### Latency (One-Way vs. Round-Trip)
- **Round-Trip Time (RTT):** The total elapsed wall-clock time from when a Producer transmits a message to a Consumer until the Producer receives the Consumer's complete echo response.
- **One-Way Latency:** The estimated single-direction transit time, calculated under symmetric ping-pong benchmarks as:
  $$\text{Latency}_{\text{one-way}} = \frac{\text{RTT}}{2}$$

### POSIX Message Queue (`mq_overview`)
A kernel-managed, discrete message exchange mechanism providing prioritized delivery, asynchronous notification (`mq_notify`), and configurable queue limits. Operates through virtual filesystem `/dev/mqueue`.

### Single-Producer Single-Consumer (SPSC)
A concurrency pattern in which exactly one process/thread writes to a circular queue and exactly one process/thread reads from it. SPSC queues can be implemented lock-free using atomic release-acquire operations without atomic compare-and-swap (`CAS`) loops.

### UNIX Domain Socket (`AF_UNIX`)
A bidirectional, kernel-mediated IPC primitive implementing the standard BSD socket API (`SOCK_STREAM`, `SOCK_DGRAM`, `SOCK_SEQPACKET`) for local process communication. Bypasses the network protocol stack (IP/TCP) while maintaining kernel socket buffering.
