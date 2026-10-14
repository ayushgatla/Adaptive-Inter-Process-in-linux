# Mathematical & Operational Metric Definitions

---

## 1. Latency Metrics

### Round-Trip Time (RTT):
The total elapsed wall-clock time from when the Producer initiates a transmission to the Consumer until the Producer receives the Consumer's complete echo response:
$$\text{RTT} = t_{\text{echo\_recv}} - t_{\text{send\_start}} \quad (\text{nanoseconds})$$

### One-Way Amortized Latency ($L_{\text{one-way}}$):
Under symmetric ping-pong echo testbeds where send and echo paths traverse identical hardware and kernel structures:
$$L_{\text{one-way}} = \frac{\text{RTT}}{2} \quad (\text{nanoseconds})$$

### Percentile Ranks ($P_k$):
For a sorted sequence of $N$ latency observations $X = (x_{(1)}, x_{(2)}, \dots, x_{(N)})$ where $x_{(1)} \le x_{(2)} \le \dots \le x_{(N)}$:
$$P_k = x_{(\lceil \frac{k}{100} \cdot N \rceil)}$$
Evaluated at $k \in \{50, 90, 99, 99.9\}$.

---

## 2. Throughput & Bandwidth Metrics

### Message Throughput ($\text{MPS}$):
The number of completed discrete communication exchanges processed per wall-clock second:
$$\text{MPS} = \frac{N_{\text{iterations}}}{\Delta t_{\text{wall\_seconds}}} \quad (\text{messages/second})$$

### Effective Bandwidth ($\text{MBps}$):
The volume of user payload data successfully transferred per second:
$$\text{MBps} = \frac{\text{MPS} \times \text{Payload Bytes}}{1024 \times 1024} \quad (\text{MiB/s})$$

---

## 3. Adaptation Gain & Optimization Metrics

### Adaptation Gain ($G_{\text{adapt}}$):
The dimensionless performance ratio quantifying the throughput advantage achieved by dynamic runtime transport adaptation over the best statically chosen IPC primitive under identical dynamic workloads:
$$G_{\text{adapt}} = \frac{\text{Throughput}_{\text{adaptive}}}{\max_{T \in \text{Static}} (\text{Throughput}_T)}$$
- $G_{\text{adapt}} > 1.0$: Dynamic adaptation outperforms any static mechanism.
- In Phase 4, on an 80/20 mixed workload, $G_{\text{adapt}} = \frac{536,143.12}{284,909.77} = \mathbf{1.88\times}$.

### Latency Reduction Percentage ($\text{Red}_{\%}$):
The percentage reduction in blended mean latency delivered by the adaptive framework relative to a reference static baseline:
$$\text{Red}_{\%} = \frac{\text{Latency}_{\text{static}} - \text{Latency}_{\text{adaptive}}}{\text{Latency}_{\text{static}}} \times 100$$
In Phase 4, relative to Static Socket:
$$\text{Red}_{\%} = \frac{1754.94 - 932.59}{1754.94} \times 100 = \mathbf{46.86\%}$$
