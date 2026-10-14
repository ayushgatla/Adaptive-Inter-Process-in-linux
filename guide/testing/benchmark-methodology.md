# Testing & Benchmarking: Benchmark Methodology & Statistics

## 1. The Ping-Pong Echo Measurement Model

Measuring one-way latency between two independent processes on modern multi-core computers is notoriously difficult because each CPU core maintains its own local registers and clock frequency adjustments. Trying to measure one-way latency by reading a timestamp on Core 1 and another on Core 2 introduces inter-core clock skew errors.

To eliminate this variable, our benchmark harness ([bench/bench_baselines.c](file:///bench/bench_baselines.c), [bench/bench_matrix.c](file:///bench/bench_matrix.c)) implements the gold-standard **1:1 Echo Ping-Pong Model**:

```mermaid
sequenceDiagram
    autonumber
    participant P as Producer Process (Core 1)
    participant Channel as IPC Transport (SHM / Pipe / Socket)
    participant C as Consumer Process (Core 2)

    Note over P: t_start = aipc_time_ns() [CLOCK_MONOTONIC_RAW]
    P->>Channel: Send payload (N bytes)
    Channel->>C: Deliver payload
    C->>Channel: Immediately echo payload back (N bytes)
    Channel->>P: Deliver echoed payload
    Note over P: t_end = aipc_time_ns()
    Note over P: RTT = (t_end - t_start); Latency = RTT / 2.0
```

### Mathematical Formulation:
$$\text{RTT} = t_{\text{end}} - t_{\text{start}}$$
$$\text{One-Way Latency} = \frac{\text{RTT}}{2.0}$$

Because both $t_{\text{start}}$ and $t_{\text{end}}$ are sampled on the **exact same CPU core** using the **exact same monotonic clock**, clock skew is physically eliminated.

---

## 2. Timer Selection: Why `CLOCK_MONOTONIC_RAW`?

In Linux, standard POSIX provides several clock sources:
- `CLOCK_REALTIME`: System wall-clock time. Can jump forwards or backwards if the system administrator or an NTP daemon updates the clock, producing negative latencies.
- `CLOCK_MONOTONIC`: Guaranteed not to jump backwards, but subject to incremental frequency slewing by `adjtime()` and NTP.
- **`CLOCK_MONOTONIC_RAW` (Our Choice):** Accesses a hardware-based clock that is completely untouched by NTP adjustments. It provides pure, raw CPU nanosecond increments.

---

## 3. Warmup Period & Cache Stabilization

Every benchmark execution begins with **10,000 untimed warmup iterations**:
- Forces the Linux kernel to allocate and map all virtual memory pages (eliminating page fault latency during measurement).
- Warms instruction caches (L1i) and data caches (L1d).
- Locks the CPU frequency governor into its maximum performance state (preventing Intel SpeedStep or AMD Cool'n'Quiet downclocking artifacts).

---

## 4. Statistical Distribution & Percentile Calculations

Average (mean) latency can be misleading because a single 1-millisecond operating system interrupt can distort the mean of a million nanosecond-scale samples.
To provide complete visibility, our sample collector ([src/aipc_bench.c](file:///src/aipc_bench.c#L58-L76)) records every individual iteration into a dynamically allocated array and computes exact percentiles:

```c
// Sorting 1,000,000 latency samples for exact percentile extraction
qsort(col->samples, col->count, sizeof(double), compare_doubles);

out_result->min_lat_ns  = col->samples[0];
out_result->p50_lat_ns  = col->samples[(size_t)(col->count * 0.50)]; // Median
out_result->p90_lat_ns  = col->samples[(size_t)(col->count * 0.90)];
out_result->p99_lat_ns  = col->samples[(size_t)(col->count * 0.99)]; // 99th Percentile
out_result->p999_lat_ns = col->samples[(size_t)(col->count * 0.999)];// 99.9th Percentile
out_result->max_lat_ns  = col->samples[col->count - 1];
```

---

## 5. Throughput & Resource Accounting Formulas

### Saturated Message Throughput (Messages per Second)
$$\text{Throughput (msg/s)} = \frac{N}{\text{Total Wall Time (seconds)}}$$

### Bandwidth Throughput (Megabytes per Second)
$$\text{Bandwidth (MB/s)} = \frac{N \times \text{Payload Size (Bytes)}}{1024 \times 1024 \times \text{Total Wall Time (seconds)}}$$

### Operating System Context Switch Tracking
At the conclusion of each benchmark run, the harness queries the operating system via `getrusage(RUSAGE_SELF, &usage)`:
- `usage.ru_nvcsw`: Counts **voluntary context switches** (when a process sleeps waiting for I/O).
- `usage.ru_nivcsw`: Counts **involuntary context switches** (when the Linux scheduler forcibly preempts the process because its time quantum expired).
