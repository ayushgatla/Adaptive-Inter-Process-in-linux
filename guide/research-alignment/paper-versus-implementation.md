# Research Alignment: Paper versus Implementation

## 1. Context & Comparative Foundations

In systems research, comparing numbers across different physical computers is dangerous unless testing conditions, CPU architectures, and kernel versions are disclosed.
Our project references two primary published studies:
1. **Danyliuk, I. M. (2026):** Evaluated 64-byte latency of classical Linux IPC primitives on an HPC Linux testbed.
2. **Krishnaveni, S., & Ruby, D. (2016):** Evaluated bulk payload throughput scaling up to 99 MB.

To create an academically defensible comparison, we:
- Preserved the original published units and methodology from both papers.
- Replicated their classical primitives locally on our exact machine to establish an apples-to-apples control baseline.
- Evaluated our new FAS-IPC shared memory engine and Adaptive A-IPC framework under the identical benchmark harness.

---

## 2. Direct Feature-by-Feature Academic Comparison

| Research Dimension | Danyliuk (2026) HPC Reference | Krishnaveni & Ruby (2016) | Our Local Baseline (Phase 1) | Our Adaptive Implementation (Phase 3 & 4) |
| :--- | :--- | :--- | :--- | :--- |
| **Problem Addressed** | 64-byte latency in HPC clusters | Bulk IPC transfer time (10–99 MB) | Control verification of classical primitives | Dynamic runtime selection across fluctuating traffic |
| **Payload Scope** | Static single size: 64 bytes | Bulk sizes: 10 MB to 99 MB | 64 bytes (1M iterations) | Full 11-payload matrix (64 B to 64 KB) + 80/20 mixed traffic |
| **Workload Dynamic** | Static only | Static only | Static only | Dynamic mixed traffic (80% 64 B + 20% 16 KB) |
| **Core Pinning** | Core pinning applied | Not explicitly controlled | Pinned: Producer C1, Consumer C2 | Pinned: Producer C1, Consumer C2 |
| **Shared Memory Design**| Naive SHM with semaphore/futex | Not benchmarked | Naive SHM with futex (`aipc_baselines.c`)| Lock-free SPSC ring, `alignas(64)`, AHSP micro-spin |
| **Syscalls per Transfer**| 2 to 4 syscalls | 2 syscalls | 2 syscalls (Pipes/Sockets) | **0 syscalls** ($\le 1024$ B); 2 syscalls ($> 1024$ B) |
| **Context Switch Audit**| Not recorded | Not recorded | ~1,000,000 voluntary CSW / 1M msgs | **125 voluntary CSW** / 1M msgs (99.98% bypass) |
| **Adaptation Logic** | None | None | None | Real-time EWMA (< 15 ns) + Crossover + Hysteresis |

---

## 3. Empirical Results Comparison Table

The table below contrasts the reported findings from published literature with our locally measured results on identical modern hardware:

| Metric / Transport | Danyliuk (2026) Published Ref | Our Replicated Baseline | Our Adaptive Implementation | Difference vs Danyliuk | Difference vs Baseline | Conditions & Caveats |
| :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **Pipe (64 B Latency)** | ~2,100 ns | 1,404.53 ns | 1,404.53 ns (Fallback) | 33.1% faster | 0.0% | Faster CPU clock and modern Linux kernel pipe optimizations. |
| **Socket (64 B Latency)** | ~2,050 ns | 1,370.62 ns | 1,370.62 ns (Fallback) | 33.1% faster | 0.0% | Local AF_UNIX stream socket buffer routing. |
| **POSIX MQ (64 B Latency)**| ~14,500 ns | 1,530.48 ns | N/A (Excluded) | 89.4% faster | 0.0% | Linux 6.x replaced older linked-lists with red-black trees. |
| **Shared Memory (64 B)** | ~850 ns (Naive) | 1,451.05 ns (Naive) | **196.17 ns (FAS-IPC)** | **4.33x faster** | **7.40x faster** | Lock-free SPSC ring + cache isolation + AHSP micro-spin. |
| **Adaptive A-IPC (64 B)** | *Not Evaluated* | N/A | **208.67 ns (A-IPC)** | **4.07x faster** | **6.57x faster** | Includes EWMA telemetry and crossover evaluation overhead. |
| **Mixed Workload (Mean)** | *Not Evaluated* | 1,754.94 ns (Socket) | **932.59 ns (A-IPC)** | N/A | **46.86% latency cut** | Dynamic mixed traffic: 80% 64 B + 20% 16 KB. |
| **Adaptation Gain ($G_{\text{adapt}}$)**| *Not Evaluated* | 1.00x (Baseline) | **1.88x (A-IPC)** | N/A | **+88% throughput** | Proves adaptive advantage over best static baseline. |

---

## 4. Mathematical Formulas Used in Evaluation

To ensure absolute transparency and consistency across all chapters, all percentage calculations adhere to standard formulas:

### 1. Adaptation Gain ($G_{\text{adapt}}$)
$$G_{\text{adapt}} = \frac{\text{Throughput}_{\text{Adaptive}}}{\text{Throughput}_{\text{Best-Static}}}$$
- For mixed traffic: $\frac{536,143.12\text{ msg/s}}{284,909.77\text{ msg/s}} = 1.8818\times \implies \mathbf{1.88\times}$.

### 2. Latency Reduction Percentage
$$\text{Latency Reduction (\%)} = \left( \frac{\text{Baseline Latency} - \text{Adaptive Latency}}{\text{Baseline Latency}} \right) \times 100$$
- Calculated against the fastest static alternative (UNIX Domain Socket at 1,754.94 ns):
  $$\frac{1754.94 - 932.59}{1754.94} \times 100 = \frac{822.35}{1754.94} \times 100 = \mathbf{46.86\%}$$

### 3. Latency Speedup Factor (vs Danyliuk 2026)
$$\text{Speedup} = \frac{\text{Danyliuk Ref Latency}}{\text{Our Measured Latency}}$$
- For FAS-IPC: $\frac{850.0\text{ ns}}{196.17\text{ ns}} = \mathbf{4.33\times}$.
- For Adaptive A-IPC: $\frac{850.0\text{ ns}}{208.67\text{ ns}} = \mathbf{4.07\times}$.

---

## 5. Limitations of the Comparison

To maintain academic honesty, the following caveats must be stated:
1. **Different Test Machines:** Danyliuk's numbers were recorded on his HPC environment. Our speedup of 4.33x reflects both our lock-free cache-isolated architecture AND hardware generation improvements.
2. **Replicated Baselines Serve as Control:** The definitive proof of our architectural contribution is that on our **exact same hardware**, FAS-IPC runs at **196.17 ns** while our local replica of Danyliuk's naive shared memory runs at **1,451.05 ns** (a 7.4x improvement on identical silicon).
