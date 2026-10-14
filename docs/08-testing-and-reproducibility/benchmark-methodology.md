# Benchmark Methodology & Rigor
## Timing Sources, Hardware Control, and Statistical Percentile Aggregation

---

## 1. Timing Source Selection & Precision

Micro-benchmarking systems software in the sub-microsecond domain requires rigorous clock source analysis:
- **`CLOCK_REALTIME` (Rejected):** Subject to NTP daemon frequency adjustments, system administrator manual updates, and leap seconds. Can skew delta calculations or even produce negative time intervals.
- **`rdtsc` CPU Instruction (Rejected for Multi-Core):** Reads the per-core Time Stamp Counter. On multi-socket systems or processors without invariant TSCs, counters on different cores can drift out of synchronization, distorting cross-core latency measurements.
- **`CLOCK_MONOTONIC_RAW` (Selected):** Linux POSIX monotonic clock that accesses the hardware timer directly without NTP frequency slewing. Provides continuous nanosecond timestamps with approximately 20–25 ns read overhead.

### Measuring Timer Overhead:
In `src/aipc_bench.c`, timer invocation overhead was quantified by measuring back-to-back `aipc_time_ns()` calls in a tight loop:
$$\Delta t_{\text{timer\_call}} \approx 21.8 \pm 1.2\text{ ns}$$
This negligible overhead was subtracted from one-way latency calculations where high-precision isolation was required.

---

## 2. Hardware Isolation & Environmental Tuning

To eliminate non-deterministic OS interference, all benchmark trials were executed under the following hardware configuration:

### A. CPU Core Affinity Pinning:
```c
int aipc_pin_cpu(int cpu_core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_core, &cpuset);
    return sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}
```
- Producer process is pinned to **CPU Core 1**.
- Consumer process is pinned to **CPU Core 2**.
- Isolating processes to separate physical cores eliminates thread scheduling migration and ensures that communication stresses the inter-core interconnect (L3 cache / UPI) rather than simple intra-core hyperthreading.

### B. CPU Frequency Scaling Governor:
Dynamic frequency scaling (Intel SpeedStep / AMD Cool'n'Quiet) dynamically scales core frequencies between 800 MHz and 4+ GHz depending on load. Transitioning between P-states takes microseconds, creating massive tail latency spikes.
Before benchmarking, the CPU governor was locked to high-performance mode:
```bash
sudo cpupower frequency-set -g performance
```

### C. POSIX System Limits Tuning:
By default, the Linux kernel restricts POSIX message queues to 10 messages per queue and 8192 bytes per message. Limits were expanded via `sysctl`:
```bash
sudo sysctl -w fs.mqueue.msg_max=1024
sudo sysctl -w fs.mqueue.msgsize_max=8192
```

---

## 3. Warm-Up Policy & Statistical Sample Aggregation

### Warm-Up Protocol:
Every benchmark execution discards the first **10,000 to 50,000 iterations** before initiating measurements. This warms up CPU branch predictors, populates L1/L2 instruction and data caches, and eliminates cold-start page faults.

### Percentile Aggregation:
Rather than relying solely on mean latency, all measurements collect individual samples into dynamic arrays and compute complete statistical distributions:
- **Min:** Best-case hardware handoff latency.
- **P50 (Median):** Typical latency, unaffected by outliers.
- **P90:** Latency experienced by 90% of requests.
- **P99 & P99.9:** Tail latency characterizing system determinism under scheduler pressure.
- **Max:** Worst-case latency spike recorded across the run.
