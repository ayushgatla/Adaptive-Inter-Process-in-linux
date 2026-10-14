# Phase 4: Frequently Asked Questions (FAQ)

---

### Q1: What is the formal mathematical definition of Adaptation Gain ($G_{\text{adapt}}$)?
**Answer:** Adaptation Gain is defined as the ratio of the throughput achieved by the adaptive IPC engine to the maximum throughput achieved by any single static baseline under the exact same dynamic workload:
$$G_{\text{adapt}} = \frac{\text{Throughput}_{\text{adaptive}}}{\max_{T \in \{\text{Pipe}, \text{Socket}, \text{MQ}\}} (\text{Throughput}_T)}$$
If $G_{\text{adapt}} > 1.0$, the adaptive framework delivers a net performance benefit over hardcoding any single static mechanism. In Phase 4, we achieved $G_{\text{adapt}} = 1.88\times$.

### Q2: Why wasn't Static Shared Memory included in the denominator of $G_{\text{adapt}}$?
**Answer:** Static FAS-IPC shared memory has a hard physical slot limit of 1024 bytes. When evaluated under the mixed workload, it fails on the 16 KB payload transfers (20% of the traffic), returning error code `-1`. Because static shared memory cannot functionally complete the workload without dropping or rejecting data, its throughput on the complete workload is technically 0, making UNIX domain socket (284,909 msg/s) the highest-performing valid static baseline.

### Q3: Why did you choose an 80/20 mixed distribution (80% 64 B + 20% 16 KB)?
**Answer:** The 80/20 distribution reflects typical microservice and robotics inter-process traffic patterns. In systems like ROS 2 or automated trading, high-frequency synchronization barriers, sensor heartbeats, and control commands (64 B) occur constantly, punctuated periodically by bulk telemetry frames or image bursts (16 KB).

### Q4: Why does Adaptive A-IPC achieve a 46.86% latency reduction over Static Socket?
**Answer:** Under Static Socket, 100% of the messages (both 64 B and 16 KB) traverse the Linux kernel socket buffer, incurring 2 system calls and ~1331 ns of latency per small message. In Adaptive A-IPC, the 80% small messages are routed to the FAS-IPC fast path (taking ~208 ns and 0 syscalls), while only the 20% bulk messages traverse the kernel. Blended latency drops from 1754 ns to 932 ns:
$$\text{Reduction} = \frac{1754.94 - 932.59}{1754.94} \times 100 = 46.86\%$$

### Q5: How do the tail latencies (P99) compare under the mixed workload?
**Answer:** Under the mixed workload, Static Pipe exhibited a P99 tail latency of 3124.19 ns due to buffer fill stalls during the 16 KB bursts. Adaptive A-IPC achieved a P99 tail latency of 1284.29 ns—a **58.9% reduction in tail jitter**—because fast-path signaling traffic was completely decoupled from the streaming pipe buffer.

### Q6: What happens at the 1024-byte crossover point in the matrix sweep?
**Answer:** At $\le 1024$ B, Adaptive A-IPC tracks FAS-IPC closely (208 ns vs 196 ns). At 2048 B, the payload exceeds the 1024 B slot boundary. The adaptive selector automatically diverts traffic to the anonymous pipe streaming path, registering a latency of 1817.85 ns (matching the static pipe baseline of 1805.35 ns) and 2 system calls per operation without any application intervention or error.

### Q7: Why is there an ~12 ns difference between FAS-IPC and Adaptive A-IPC at 64 B (196 ns vs. 208 ns)?
**Answer:** That ~12.5 ns difference represents the runtime cost of the Workload Observer and Adaptive Selector: inspecting payload length, updating the EWMA arrival rate, and executing the crossover branch check in `ipc_send()`. This empirically proves that dynamic adaptation incurs less than 13 ns of overhead.

### Q8: Why did POSIX Message Queues exhibit negative latency improvement (-28.58%)?
**Answer:** POSIX message queues have heavy kernel priority queue management overhead. When subjected to mixed workloads with large payloads, message copying and kernel buffer lock contention degrade performance significantly, making them the slowest static primitive (2671.44 ns mean latency, only 187,164 msg/s).

### Q9: Were matrix sweeps executed under identical CPU affinity masks?
**Answer:** Yes. Across all matrix trials, the Producer process was strictly pinned to CPU Core 1 and the Consumer process to CPU Core 2 using `sched_setaffinity()`. The CPU frequency scaling governor was set to `performance` via `setup_env.sh` to eliminate thermal throttling and dynamic clock scaling artifacts.

### Q10: Examiner Challenge: "Could an application just use a larger shared-memory slot size instead of switching transports?"
**Answer:** Enlarging shared-memory slots to 16 KB or 64 KB introduces two fatal architectural flaws:
1. **Severe Internal Fragmentation:** A 64 KB slot holding a 64-byte message wastes 99.9% of the allocated memory bandwidth and pollutes CPU cache lines with unused memory.
2. **Exploding Memory Footprint:** A 1024-slot ring with 64 KB slots consumes 64 MB of non-swappable physical RAM per channel. For microservice architectures with dozens of channels, memory footprint becomes unsustainable. Adapting between small shared-memory slots and streaming pipes achieves both sub-microsecond latency and minimal RAM consumption.
