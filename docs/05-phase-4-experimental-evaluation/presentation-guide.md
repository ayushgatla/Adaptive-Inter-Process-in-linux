# Phase 4: Presentation & Viva Guide
## University Defense Walkthrough for Experimental Evaluation & Adaptation Gain

---

## 1. Opening Statement (15–30 Seconds)
> "Phase 4 delivers the empirical proof of our core thesis. By subjecting our adaptive framework to an 11-payload experimental matrix and a realistic dynamic mixed workload—80% small control frames and 20% bulk data—we demonstrated that Adaptive A-IPC achieves an Adaptation Gain ($G_{\text{adapt}}$) of 1.88x and cuts latency by 46.86% compared to the best statically chosen primitive."

---

## 2. Diagram Walkthrough (What to Point At)
1. Point to the **Mixed Workload Results Table**:
   - Highlight **Static Socket Throughput**: 284,909 msg/s.
   - Highlight **Adaptive A-IPC Throughput**: 536,143 msg/s.
   - Calculate live: $\frac{536,143}{284,909} = 1.88\times$.
2. Point to the **Static Shared Memory Row**:
   - Emphasize that static shared memory *fails* on mixed workloads because 20% of the messages (16 KB) exceed the 1024-byte slot boundary.
   - Conclude: *"This proves that runtime adaptation is not an optional optimization; it is mandatory for robust heterogeneous systems."*
3. Point to **Latency Reduction**:
   - Show how blended latency falls from 1754 ns to 932 ns (46.86% reduction).

---

## 3. Code Demonstration (Which Files to Show)
- Open [`bench/bench_matrix.c`](file:///bench/bench_matrix.c) and trace the mixed workload generation loop: show the 80/20 payload selection and timing calculation.
- Open [`results/mixed_workload_gain.csv`](file:///results/mixed_workload_gain.csv) to display the raw empirical dataset.

---

## 4. Test Demonstration (Safe Terminal Command)
```bash
# Verify Phase 4 matrix data and adaptation gain criteria
python scripts/verify_all_4_phases.py
```
- **Auditor Output:** Shows Phase 4 PASS with $G_{\text{adapt}} = 1.88\times$ (Gate $\ge 1.80\times$) and Latency Reduction $= 46.86\%$ (Gate $\ge 40.0\%$).

---

## 5. Research Comparison Key Talking Point
> "The original research proposal stated that the project would succeed if we proved $G_{\text{adapt}} > 1.0$. We did not merely beat 1.0; we achieved 1.88x, while proving that static shared memory suffers catastrophic buffer errors when payloads vary."

---

## 6. Time Budget Breakdown
- **Short Summary (1 minute):** Research question, 80/20 mixed workload model, and 1.88x gain finding.
- **Deep Walkthrough (3–5 minutes):** Walk through the 11-payload matrix sweep, analyze the crossover point at 1024 B, explain the mathematical definition of $G_{\text{adapt}}$, and show tail latency control.
