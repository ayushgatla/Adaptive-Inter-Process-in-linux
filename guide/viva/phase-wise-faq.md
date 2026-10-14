# Viva Voce Master Guide: Consolidated Phase-Wise FAQ

This reference compiles the key technical questions and answers for each implementation phase of the **Adaptive Inter-Process Communication (A-IPC)** framework. Use this guide to review specific implementation decisions and phase-level engineering details.

---

## Phase 1: Baseline Harness & Classical Replication

1. **Why was Phase 1 necessary before writing any adaptive code?**  
   Without an empirical baseline measured on our exact hardware, we could not prove whether our adaptive framework improved performance or merely introduced useless overhead.
2. **Why use `CLOCK_MONOTONIC_RAW` instead of `gettimeofday()`?**  
   `gettimeofday()` has microsecond resolution and can jump backwards during NTP sync. `CLOCK_MONOTONIC_RAW` accesses the CPU hardware clock directly with nanosecond precision and is monotonic.
3. **Why is CPU core pinning (`sched_setaffinity`) necessary?**  
   If the Linux OS scheduler migrates the communicating processes across CPU cores, cold L1/L2 caches produce massive artificial latency spikes of several microseconds.
4. **Why divide round-trip time (RTT) by 2 instead of measuring one-way latency directly?**  
   Measuring one-way latency across independent processes requires synchronized clocks across cores. Measuring RTT on the Producer and dividing by 2 provides jitter-free one-way timing from a single clock source.
5. **Why are Pipes and UNIX Domain Sockets comparable in latency at 64 bytes?**  
   At 64 bytes, data copy time is negligible (< 5 ns). Both primitives require 2 system calls and 1 voluntary context switch per hop; latency is dominated by entering and exiting the Linux kernel.
6. **Why did POSIX Message Queues exhibit the highest latency in Phase 1?**  
   POSIX MQ enforces kernel priority sorting on every message, requiring internal locking and red-black tree insertion on each transfer.
7. **What is a voluntary context switch and why did ~1,000,000 occur in Phase 1?**  
   A voluntary context switch occurs when a thread sleeps waiting for I/O. In a 1:1 echo exchange, the sender must sleep until the receiver echoes, yielding the CPU on every single message.
8. **How does Phase 1 replicate Danyliuk (2026)?**  
   Danyliuk Table 1 benchmarked 64-byte IPC latencies on HPC Linux. Phase 1 replicated that exact benchmark locally, verifying the performance hierarchy on modern Linux 6.x.
9. **Could baseline numbers change on another computer?**  
   Absolute nanosecond numbers vary with CPU clock frequency, but the relative hierarchy (Shared Memory $\ll$ Socket $\approx$ Pipe $<$ POSIX MQ) remains identical.
10. **What role does Phase 1 code play in the final system?**  
    The Anonymous Pipe and UNIX Domain Socket code written in Phase 1 becomes the active streaming fallback engine for payloads $> 1024$ bytes in Phase 3 and Phase 4.

---

## Phase 2: Ultra-Low Latency FAS-IPC Shared Memory Engine

1. **What makes FAS-IPC 4.33x faster than Danyliuk's 850 ns benchmark?**  
   Danyliuk used naive shared memory with kernel synchronization (semaphores/futexes) on every message. FAS-IPC uses a lock-free SPSC ring with 64-byte cache isolation and AHSP micro-spinning, executing with zero system calls during active streaming.
2. **What is "False Sharing" and how does `alignas(64)` fix it?**  
   False sharing occurs when two cores write to different variables on the same 64-byte cache line, causing inter-core MESI cache invalidations. Padding Producer `tail` and Consumer `head` to separate 64-byte boundaries eliminates this penalty.
3. **What is the function of the `_mm_pause()` instruction in AHSP?**  
   `_mm_pause()` provides a small delay (~10–15 CPU cycles) in spin loops, preventing memory pipeline stalls and yielding CPU execution resources to sibling hyper-threads.
4. **Why limit micro-spinning to 200 iterations (`FAS_AHSP_SPIN_LIMIT`)?**  
   200 pause iterations take ~300 ns. If data arrives during this window, we avoid a 1,300 ns context switch. If the channel is idle, continuing to spin would waste CPU power, so the thread parks via `SYS_futex`.
5. **What memory ordering is enforced in FAS-IPC?**  
   Acquire-Release ordering (`memory_order_release` when updating indices, `memory_order_acquire` when reading). This guarantees payload data is committed to memory before the updated index is visible.
6. **Why is the ring buffer size capped at 1,024 slots?**  
   1,024 is a power of two ($2^{10}$), allowing ring index wrapping to use a 1-cycle bitwise AND (`index & 1023`) instead of an expensive 30-cycle division instruction (`index % 1024`).
7. **Why are slot payloads limited to 1,024 bytes?**  
   1,024 slots of 1,024 bytes consume ~1.1 MB of RAM, fitting comfortably inside CPU L3 cache (typically 16–32 MB).
8. **What happens if the FAS-IPC ring buffer fills up?**  
   The Producer micro-spins and waits for the Consumer to advance `head`. Because the Consumer drains slots in sub-microsecond time, the buffer rarely stays full under normal load.
9. **Is FAS-IPC thread-safe for multiple concurrent producers?**  
   No. FAS-IPC is strictly Single-Producer Single-Consumer (SPSC). Multi-producer support requires Compare-And-Swap (CAS) loops, which are deferred to Tier-2.
10. **How was zero-syscall execution verified?**  
    Using Linux `perf stat -e syscalls:sys_enter`. In steady-state streaming, zero syscall events occur, and `/proc/self/status` registers only 125 context switches over 1,000,000 transfers.

---

## Phase 3: Workload Telemetry & Adaptive Selection Engine

1. **Why not use Machine Learning to choose the transport?**  
   ML models introduce microseconds of execution latency. Our entire communication target is 200 nanoseconds. Our arithmetic EWMA heuristic executes in **12.5 nanoseconds**.
2. **What does the magic word `0xA1PC0001` do?**  
   It is a 32-bit validation constant ("A-IPC 0001") placed at the start of every binary frame. If a stream descriptor experiences corruption, the receiver scans for this magic word to recover frame alignment.
3. **Why is the smoothing constant $\alpha = 0.125$ ($1/8$)?**  
   Dividing by 8 is a single CPU right-shift instruction (`>> 3`). It gives recent samples 12.5% weight while filtering out single-message noise.
4. **Why can't FAS-IPC handle messages $> 1024$ bytes?**  
   Slots are pre-allocated to 1,024 bytes. Messages larger than 1,024 bytes must be routed to Pipes or Sockets to avoid memory overruns.
5. **What is "transport flapping" and how does hysteresis stop it?**  
   Flapping is rapid oscillation between transports. Hysteresis requires a candidate transport to be at least 15% better for 16 consecutive samples before switching between streaming transports.
6. **How does `ipc_receive()` guarantee sequential message delivery?**  
   Every message carries an incrementing `tx_seq` number. The receiver maintains `rx_seq`. When checking transports, `ipc_receive()` only delivers a payload if its sequence matches `rx_seq`.
7. **What happens if the selected transport fails during `ipc_send()`?**  
   `ipc_send()` falls through: if shared memory fails, it routes to Pipe; if Pipe fails, it routes to UNIX Socket.
8. **Does Phase 3 add system calls to the fast path?**  
   No. When the selector picks FAS-IPC ($\le 1024$ B), transmission occurs purely in user space with zero system calls.
9. **Why use named FIFOs instead of anonymous pipes in `ipc_channel.c`?**  
   Anonymous pipes require processes to share file descriptors via `fork()`. Named FIFOs allow unrelated, independent processes to communicate using paths in `/tmp`.
10. **Isn't holding three open transports wasteful of system resources?**  
    An open file descriptor consumes ~1 KB of kernel memory. In exchange, the system gains dynamic microsecond adaptation and prevents packet drops.

---

## Phase 4: Full Matrix Sweeps & Adaptation Gain Validation

1. **Why did static shared memory fail the mixed workload test?**  
   FAS-IPC slots are capped at 1,024 bytes. When 16 KB packets arrived, static shared memory dropped packets. A-IPC survived because it automatically routed the 16 KB packets to pipes.
2. **What is the exact formula for Adaptation Gain ($G_{\text{adapt}}$)?**  
   $$G_{\text{adapt}} = \frac{\text{Throughput}_{\text{Adaptive}}}{\text{Throughput}_{\text{Best-Static}}} = \frac{536,143.12}{284,909.77} = 1.88\times$$
3. **What is the exact formula for Latency Reduction Percentage?**  
   $$\text{Latency Reduction} = \frac{1754.94\text{ ns} - 932.59\text{ ns}}{1754.94\text{ ns}} \times 100 = 46.86\%$$
4. **Why does Anonymous Pipe have higher latency at 65 KB than UNIX Socket?**  
   Linux default pipe buffer capacity is 64 KB. Sending 65,536 bytes pushes the pipe to its limit, triggering buffer backpressure. Sockets auto-tune buffers for large streams.
5. **How many iterations were run per payload size in the matrix?**  
   100,000 timed round-trip ping-pong exchanges per size, preceded by 10,000 untimed warmup iterations.
6. **Why did POSIX Message Queues have negative latency reduction (-28.58%)?**  
   POSIX MQ kernel priority sorting makes it significantly slower than Pipes and Sockets, causing an average latency of 2,671 ns on mixed traffic.
7. **Could a developer achieve the same speed by expanding pipe buffers?**  
   No. Even with a large buffer, every pipe transfer requires 2 system calls and a context switch, which can never break the ~1,300 ns syscall barrier.
8. **What explains the 12.5 ns difference between A-IPC and raw Pipe at 2 KB?**  
   That 12.5 nanoseconds is the cost of running telemetry, checking crossover bounds, and formatting the 24-byte `aipc_frame_hdr_t` envelope.
9. **Why was an 80/20 ratio chosen for mixed traffic?**  
   Empirical studies of microservices (e.g., Redis, gRPC) show that ~80% of packets are small control/sync signals and ~20% are batched data chunks.
10. **Isn't the 1.88x gain purely because 80% of traffic was small messages?**  
    Yes! That is precisely the scientific thesis: applications are forced to use heavy socket/pipe transports because of occasional large payloads. A-IPC provides fast-path speed for the 80% without losing large-payload capability.

---

## Phase 5: Performance Analysis & Micro-Architectural Profiling

1. **What is the purpose of Phase 5 if code was completed in Phases 1–4?**  
   Phase 5 provides scientific validation: auditing hardware counters, confirming cache coherence behavior, and rendering visual figures (Figures 1–4).
2. **What does the Tail Latency CDF (Figure 3) prove?**  
   It proves that our 196 ns latency is consistent and predictable, not an artifact skewed by outliers. 99% of messages complete in under 323 nanoseconds.
3. **What causes the P99.9 latency spike in Figure 3?**  
   Non-real-time Linux kernel hardware interrupts (timer ticks, network interrupts) periodically interrupt user execution.
4. **Why generate both PNG and PDF files for each plot?**  
   PNG files are raster images for web documentation; PDF files are vector graphics for academic paper submissions and slide decks.
5. **What does the 2D Crossover Contour Map (Figure 2) demonstrate?**  
   It shows a vertical boundary at 1,024 bytes, proving that payload size is the primary governing variable for transport selection.
6. **How was false sharing elimination verified?**  
   Measuring latency with and without `alignas(64)` padding. Without padding, latency degraded from 196 ns to ~480 ns due to MESI cache bouncing.
7. **What is an L1 data cache miss and why does it matter?**  
   An L1 hit takes ~1 nanosecond (4 cycles). An L1 miss costs ~50–100 nanoseconds fetching from main RAM. Keeping the ring buffer inside CPU cache is why FAS-IPC achieves sub-200 ns speeds.
8. **Why did static pipes generate ~1,000,000 context switches in `perf stat`?**  
   In a ping-pong exchange over pipes, each process calls `read()`. When empty, the kernel puts the thread to sleep, forcing two voluntary context switches per round-trip.
9. **How do we know `scripts/plot_results.py` uses real data?**  
   The script directly ingests `results/matrix_results.csv` and `results/mixed_workload_gain.csv`. If those CSVs are missing, the script halts with an error.
10. **Were `perf stat` audits run on the same hardware as benchmarks?**  
    Yes. All CSV data, hardware counter logs, and plot figures were collected on the exact same multi-core testbed with CPU affinity active.

---

## Phase 6: Final Evaluation, Packaging & Viva Readiness

1. **What does `scripts/verify_all_4_phases.py` do?**  
   It is our master automated audit suite. It parses all experimental CSV files, tests mathematical convergence, evaluates crossover logic, and ensures all 4 operational phases satisfy their research gates in under 2 seconds.
2. **Why is automated verification critical for an academic capstone?**  
   Reviewers demand reproducibility. An automated script proves that our results are programmatic, reproducible, and verifiable by anyone.
3. **How do you prove that Phase 1 and Phase 2 are not disconnected?**  
   In `src/aipc_channel.c`, `ipc_create()` explicitly provisions the Phase 2 FAS-IPC shared memory ring alongside the Phase 1 Pipe and Socket descriptors, and `ipc_send()` dispatches across both.
4. **What command should you run if an examiner asks for a live demo?**  
   `python3 scripts/verify_all_4_phases.py`.
5. **How do we know the codebase has no memory leaks?**  
   `ipc_destroy()` unmaps shared memory (`munmap`), unlinks `/dev/shm` nodes (`shm_unlink`), deletes `/tmp` FIFO files, and closes descriptors. Clean execution was confirmed under Valgrind.
6. **Does this project require root/sudo privileges?**  
   No. All operations run completely in user space using standard POSIX user-level primitives.
7. **What is the single most impressive metric in this project?**  
   The **1.88x Adaptation Gain ($G_{\text{adapt}}$)** under mixed traffic, combined with **196.17 nanoseconds** 64-byte latency (4.33x faster than Danyliuk) and a **99.98% reduction in context switches**.
8. **What happens if run on a single-core virtual machine?**  
   Pinning falls back to Core 0. Latencies increase due to CPU preemption, but the relative performance hierarchy and adaptation gain remain consistent.
9. **What is the recommended next development step?**  
   Expanding the SPSC shared memory engine into an MPSC (Multi-Producer Single-Consumer) design using atomic Compare-And-Swap (CAS) instructions.
10. **How can a student prove academic honesty during a viva?**  
    By openly acknowledging limitations: that FAS-IPC is single-host and SPSC, that A-IPC does not modify the kernel, and that static primitives are sufficient if workloads never fluctuate.
