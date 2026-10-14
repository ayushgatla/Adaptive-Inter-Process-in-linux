# Visual Assets & Artifacts Index

This directory documents all visual figures, architectural diagrams, and empirical plots created and referenced throughout the **Adaptive Inter-Process Communication (A-IPC)** guide.

---

## 1. Generated Publication Figures (`results/plots/`)

All publication plots were generated from verified raw CSV datasets by [scripts/plot_results.py](file:///scripts/plot_results.py). Each figure is available in both web-ready raster format (PNG at 300 DPI) and print-ready vector format (PDF).

| Figure | Title | Description | Relative Link (PNG) | Relative Link (PDF) |
| :---: | :--- | :--- | :---: | :---: |
| **Fig 1** | **Danyliuk Latency Comparison** | Bar chart comparing 64-byte latencies between Danyliuk (2026) Table 1 and our implementations. Shows FAS-IPC running 4.33x faster. | [fig1_danyliuk_comparison.png](file:///results/plots/fig1_danyliuk_comparison.png) | [fig1_danyliuk_comparison.pdf](file:///results/plots/fig1_danyliuk_comparison.pdf) |
| **Fig 2** | **2D Crossover Contour Map** | 2D color contour mapping payload size vs arrival rate, showing the vertical crossover boundary at 1,024 bytes. | [fig2_crossover_map.png](file:///results/plots/fig2_crossover_map.png) | [fig2_crossover_map.pdf](file:///results/plots/fig2_crossover_map.pdf) |
| **Fig 3** | **Tail Latency Percentile CDF** | Cumulative distribution function plotting P50 to P99.9 latency, demonstrating deterministic sub-323 ns P99 delivery. | [fig3_tail_latency_cdf.png](file:///results/plots/fig3_tail_latency_cdf.png) | [fig3_tail_latency_cdf.pdf](file:///results/plots/fig3_tail_latency_cdf.pdf) |
| **Fig 4** | **Adaptation Gain & Latency Reduction** | Dual-axis bar chart showing our 1.88x Adaptation Gain and 46.86% latency cut under 80/20 mixed traffic. | [fig4_adaptation_gain.png](file:///results/plots/fig4_adaptation_gain.png) | [fig4_adaptation_gain.pdf](file:///results/plots/fig4_adaptation_gain.pdf) |

---

## 2. Key Mermaid Diagrams in the Guide

The guide contains structured Mermaid diagrams illustrating the actual codebase:
1. **High-Level System Architecture:** [guide/00-project-overview.md](file:///guide/00-project-overview.md#3-high-level-system-architecture) and [guide/architecture/system-architecture.md](file:///guide/architecture/system-architecture.md#1-architectural-overview).
2. **Linux IPC Trade-off Quadrant Chart:** [guide/01-problem-statement-and-motivation.md](file:///guide/01-problem-statement-and-motivation.md#2-the-core-problem-the-myth-of-the-best-ipc-mechanism).
3. **Micro-Architectural Kernel vs User Space IPC:** [guide/02-linux-ipc-fundamentals.md](file:///guide/02-linux-ipc-fundamentals.md#2-detailed-breakdown-of-linux-ipc-primitives).
4. **Phase 1 Echo Benchmark Sequence:** [guide/phases/phase-01-baseline-and-foundation.md](file:///guide/phases/phase-01-baseline-and-foundation.md#c-project-flow).
5. **Phase 2 FAS-IPC Cache-Isolated Sequence:** [guide/phases/phase-02-ipc-implementation.md](file:///guide/phases/phase-02-ipc-implementation.md#c-project-flow).
6. **Phase 3 Unified Dispatcher Pipeline:** [guide/phases/phase-03-adaptive-selection.md](file:///guide/phases/phase-03-adaptive-selection.md#c-project-flow).
7. **Phase 4 Matrix Benchmark Pipeline:** [guide/phases/phase-04-benchmarking-and-validation.md](file:///guide/phases/phase-04-benchmarking-and-validation.md#c-project-flow).
8. **End-to-End Data Flow Sequence:** [guide/architecture/data-flow.md](file:///guide/architecture/data-flow.md#1-end-to-end-data-flow-overview).
9. **Crossover & Hysteresis State Machine:** [guide/architecture/ipc-selection-logic.md](file:///guide/architecture/ipc-selection-logic.md#4-crossover-thresholds--hysteresis-dampening).
10. **Resource Lifecycle Teardown Flow:** [guide/architecture/error-handling-and-resource-cleanup.md](file:///guide/architecture/error-handling-and-resource-cleanup.md#2-resource-lifecycle-management).
