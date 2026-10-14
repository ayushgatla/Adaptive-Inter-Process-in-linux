# Missing Data & Limitations
## Transparent Disclosure of Experimental Gaps, Edge Cases, and Regressions

---

## 1. Workload Regimes Not Evaluated Empirically

In accordance with our research integrity mandate, we explicitly disclose the experimental domains that were not tested in this phase:
1. **Multi-Socket NUMA Topologies:**
   - All tests were conducted across CPU cores residing on a single physical processor socket. Cross-socket interconnect latencies (Intel UPI / AMD Infinity Fabric) and non-uniform memory access delays were not evaluated.
2. **Payloads Exceeding 64 Kilobytes in the Automated Matrix:**
   - While the crossover logic supports routing payloads $> 64$ KB to UNIX domain sockets, the automated matrix suite was capped at 65,536 bytes (64 KB). Multi-megabyte (1 MB – 100 MB) continuous matrix sweeps were evaluated in literature (Krishnaveni, 2016) but not included in `matrix_results.csv`.
3. **Multi-Producer Multi-Consumer (MPMC) Concurrency:**
   - All benchmarks strictly evaluated 1:1 producer-consumer topologies. Highly contented fan-in/fan-out scenarios with dozens of competing producers remain unmeasured on the FAS-IPC fast path.
4. **ARM64 Architecture Evaluation:**
   - Evaluated strictly on x86-64 hardware. Memory ordering behavior on weakly ordered ARM64 architectures requires further benchmarking.

---

## 2. Identified Performance Regressions & Overheads

1. **Selector Overhead on Pure Small-Message Traffic:**
   - On purely static 64-byte workloads, Static FAS-IPC achieved **196.17 ns** mean latency, whereas Adaptive A-IPC achieved **208.67 ns**.
   - *Regression:* An overhead of **12.50 nanoseconds (6.37%)** is introduced by the in-line Workload Observer and crossover conditional checks in `ipc_send()`. While negligible in practice, applications with 100% static small messages run slightly faster using raw FAS-IPC.
2. **POSIX Message Queue Algorithmic Regression:**
   - Under mixed workloads, POSIX Message Queues exhibited a negative latency improvement of **-28.58%** compared to Anonymous Pipes, proving that kernel priority queue management imposes severe penalties on non-prioritized bulk streaming.
3. **Pipe Bandwidth Plateau Beyond 64 KB:**
   - Because the Linux anonymous pipe circular buffer is fixed at 64 KB, attempting to stream larger buffers incurs repeated partial writes and syscall loops, causing bandwidth to plateau around 2780 MB/s.

---

## 3. Prioritized Engineering Backlog

- **`P0 (Blocking Multi-Node Scaling)`**: Implement NUMA-aware physical memory allocation (`numa_alloc_onnode`) for shared memory ring segments.
- **`P1 (Zero-Copy Bulk Transfer)`**: Implement file descriptor passing (`SCM_RIGHTS`) and `vmsplice()` on UNIX domain sockets to achieve multi-gigabyte zero-copy streaming.
- **`P2 (Cross-Architecture Portability)`**: Add ARM64 preprocessor directives substituting `__builtin_arm_yield()` for `_mm_pause()` and inserting explicit memory barriers.
