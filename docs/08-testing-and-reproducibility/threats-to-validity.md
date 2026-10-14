# Threats to Validity
## Methodological Assumptions, Environmental Biases, and Experimental Scope

---

## 1. Construct Validity

Construct validity assesses whether our operational tests actually measure the theoretical concepts they claim to evaluate.

### Threat: Latency Definition Ambiguity (One-Way vs. Round-Trip)
- **Mitigation:** In symmetric ping-pong benchmarks, measuring one-way latency by recording timestamps on a single process and dividing round-trip time by 2 ($\text{RTT}/2$) assumes symmetric network/bus transit. If the return trip encounters asymmetric scheduling delays, the estimate can deviate. We acknowledge that $\text{RTT}/2$ represents the *effective amortized one-way latency* under balanced peer conditions.

### Threat: Synthetic vs. Real-World Application Workloads
- **Mitigation:** While our 80/20 mixed workload (80% 64 B + 20% 16 KB) models typical microservice signaling and data bursts, real application workloads may experience variable inter-arrival burstiness and Poisson distributions. In Chapter 9, we explicitly scope our claims to the tested synthetic distributions.

---

## 2. Internal Validity

Internal validity examines whether observed performance gains are genuinely caused by our adaptive transport selection rather than confounding system variables.

### Threat: CPU Frequency Scaling & Core Migration
- **Mitigation:** Uncontrolled CPU throttling or core hopping can create artificial 10x latency swings. We mitigated this by locking the Linux CPU governor to `performance` mode and explicitly pinning producer and consumer processes to dedicated physical cores (`Core 1` and `Core 2`) via `sched_setaffinity()`.

### Threat: Cold-Cache & JIT Artifacts
- **Mitigation:** First-access page faults and cold L1 instruction misses can severely skew initial latency readings. We enforced a strict 10,000 to 50,000 iteration warm-up policy before recording measurement samples.

---

## 3. External Validity

External validity evaluates whether our conclusions generalize across other hardware architectures and operating system environments.

### Threat: Multi-Socket NUMA Topologies
- **Limitation:** Our benchmarks were evaluated across cores on the same physical CPU socket sharing an L3 cache. When producer and consumer processes reside on different physical NUMA nodes, cross-socket interconnect latencies (QPI/UPI) increase shared-memory handoff times. NUMA-aware multi-node tuning remains future work.

### Threat: Non-x86 Processor Architectures (ARM64)
- **Limitation:** The FAS-IPC implementation relies on x86-64 Total Store Order (TSO) hardware memory semantics and `_mm_pause()` SSE intrinsics. Porting to weakly ordered architectures like ARM64 requires explicit memory barrier instructions (`dmb ishld` / `dmb ishst`) and `yield` assembly instructions.

---

## 4. Statistical Conclusion Validity

### Threat: Outliers & Sample Distortion
- **Mitigation:** Rather than relying solely on mean latency, which can be distorted by OS interrupts or page faults, we collected full statistical distributions across 1,000,000 samples and evaluated median (P50), P90, P99, and P99.9 tail percentiles.
