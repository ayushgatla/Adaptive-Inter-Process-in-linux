# Roadmap: Pending Work & Current Limitations

## 1. Overview of Unimplemented & Deferred Capabilities

To maintain absolute academic transparency, this document details the architectural features and extensions that are **currently pending, planned, or deferred** beyond the completed Tier-1 implementation.

---

## 2. Granular Inventory of Pending Work

### 1. Multi-Producer Support (MPSC / MPMC Shared Memory)
- **Current Reality:** FAS-IPC is strictly a Single-Producer Single-Consumer (SPSC) ring buffer. Only one Producer process may write to the ring.
- **Why It Is Pending:** SPSC allows completely lock-free updates with simple atomic Acquire-Release stores. Supporting Multi-Producer Single-Consumer (MPSC) requires atomic Compare-And-Swap (`atomic_compare_exchange_weak`) loops to reserve slot indices, which introduces cache line contention and inter-core bus locking.
- **Estimated Complexity:** Moderate (~150 lines of C in `src/aipc_fas_ring.c`).
- **Expected Trade-off:** 64-byte latency will increase from 196 ns to ~320–380 ns under multi-threaded contention.

---

### 2. Instantaneous Peer Crash Detection (Robust Futexes)
- **Current Reality:** If a Producer process crashes abruptly via `SIGKILL` while a Consumer is parked in a futex, the Consumer wakes up via a **failsafe 50-millisecond timeout** (`fas_futex_wait()`).
- **Why It Is Pending:** While a 50 ms timeout prevents permanent deadlock, 50 milliseconds is thousands of times longer than our sub-microsecond operational latency.
- **Planned Improvement:** Integrating Linux robust futexes (`FUTEX_WAIT_REQUEUE_PI` or `pthread_mutexattr_setrobust`) or monitoring peer PID liveness using Linux `pidfd_open()` to receive instant `EPOLLIN` notifications upon process death.

---

### 3. Dynamic Shared Memory Buffer Resizing
- **Current Reality:** The FAS-IPC ring size (1,024 slots $\times$ 1,088 bytes $\approx$ 1.1 MB) is fixed at channel initialization. Payloads exceeding 1,024 bytes cannot be stored in the ring and must route to pipes.
- **Why It Is Pending:** Dynamically expanding a POSIX shared memory segment at runtime requires calling `ftruncate()` and re-executing `mmap()` across multiple independent processes simultaneously without race conditions or pointer invalidation.
- **Planned Improvement:** A multi-tier pooled shared-memory allocator with separate slabs for small (256 B), medium (4 KB), and large (64 KB) chunks.

---

### 4. Direct Zero-Copy Splice Integration for Bulk Streams
- **Current Reality:** Payloads $> 1024$ bytes routed to pipes use standard `write()` and `read()` system calls, which perform two memory copies (user space to kernel page, and kernel page to user space).
- **Why It Is Pending:** The Linux kernel provides the `vmsplice()` and `splice()` system calls, which can transfer pages between pipe buffers and user memory without copying.
- **Planned Improvement:** Benchmark whether `vmsplice()` improves bulk throughput for multi-megabyte transfers or whether page alignment constraints introduce excessive overhead.

---

### 5. Multi-Node / Network Distributed Communication
- **Current Reality:** A-IPC operates strictly between processes sharing physical memory on a single host.
- **Out of Scope for Tier-1:** Extending A-IPC across a network would require wrapping TCP/IP, RDMA, or DPDK. This was deliberately excluded to keep the project focused on single-host operating system IPC optimization.
