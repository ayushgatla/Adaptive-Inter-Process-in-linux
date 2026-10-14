# Adaptive Linux IPC: Complete Research & Development Guide

Welcome to the official, evidence-based research and development guide for the **Adaptive Inter-Process Communication (A-IPC)** framework in Linux.

This guide provides a comprehensive, chapter-by-chapter explanation of the project: why it was created, how inter-process communication works in Linux, how our custom C11 engine is constructed, how our work compares with published academic literature, and how to defend the project in an academic examination (viva voce).

---

## 1. Quick Project Executive Summary

In Linux, when two separate programs (processes) need to talk to each other, developers typically hardcode one communication mechanism:
- **Pipes or FIFOs** (good for streaming byte streams),
- **UNIX Domain Sockets** (great for two-way client-server streams),
- **POSIX Message Queues** (good for discrete priority messages, but slow), or
- **Shared Memory** (extremely fast because data isn't copied through the kernel, but complex to synchronize safely).

**The Core Problem:** No single IPC mechanism is optimal across all workload patterns. Small signaling messages (e.g., 64 bytes) suffer massive latency overheads when forced through kernel system calls (~1.3 μs to ~14.5 μs). Conversely, fixed-size lock-free shared memory queues cannot accept large bulk data transfers (e.g., 64 KB or 1 MB) without excessive memory consumption or packet dropping.

**Our Solution (A-IPC):**
We designed and implemented a unified user-space C11 library that wraps multiple Linux IPC transports behind a single API (`ipc_send()`, `ipc_receive()`).
1. **Fast-Path Engine (FAS-IPC):** An ultra-fast lock-free Single-Producer Single-Consumer (SPSC) shared memory circular ring buffer with 64-byte cache line alignment and Adaptive Hybrid Spin-Park (AHSP) synchronization. It achieves **196.17 ns** mean latency for 64-byte payloads—eliminating kernel system calls (**0 syscalls/op** during streaming).
2. **Bulk Streaming Fallback:** Seamless dynamic hand-off to Linux Anonymous Pipes / FIFOs and UNIX Domain Sockets for large payloads (up to 64 KB and beyond).
3. **Telemetry & Dynamic Crossover Engine:** An Exponentially Weighted Moving Average (EWMA) tracker and cost model with hysteresis dampening that selects the winning transport per message with < 15 ns overhead.
4. **Empirical Validation:** Under a dynamic mixed workload (80% small 64 B control packets + 20% large 16 KB bulk packets), A-IPC delivers an **Adaptation Gain ($G_{\text{adapt}}$) of 1.88x** and a **46.86% latency reduction** over the best static baseline.

---

## 2. Master Navigation Index

### Fundamental Foundations
- [00. Project Overview](00-project-overview.md) — High-level summary, executive problem definition, and terminology primer.
- [01. Problem Statement & Motivation](01-problem-statement-and-motivation.md) — Why static IPC fails modern workloads and the core research question.
- [02. Linux IPC Fundamentals](02-linux-ipc-fundamentals.md) — Deep dive into pipes, sockets, message queues, and shared memory with trade-offs.
- [03. Existing System & Project Architecture](03-existing-system-and-project-architecture.md) — Layered architecture, C11 data structures, and framing.
- [04. Research Paper Analysis](04-research-paper-analysis.md) — Critical study of Danyliuk (2026) and Krishnaveni & Ruby (2016).
- [05. Research Gap & Our Contribution](05-research-gap-and-our-contribution.md) — What published literature missed and what our implementation contributes.
- [06. Development Timeline & Master Phase Map](06-development-timeline-and-phase-map.md) — Chronological evolution, commit trace, and status verification.

---

### Phase Chapters (`guide/phases/`)
Every phase chapter follows a standardized 11-section format: Summary, Starting Point, Project Flow, Implementation Walkthrough, Paper Alignment, Inter-phase Synergy, Testing Evidence, Results Interpretation, 10-Question FAQ, Viva Presentation Pitch, and Exit Criteria.

- [Phases Index](phases/README.md) — Master index and overview of all development phases.
- [Phase 1: Baseline Harness & Classical Replication](phases/phase-01-baseline-and-foundation.md) — Local replication of Danyliuk (2026) classical IPC primitives.
- [Phase 2: Ultra-Low Latency FAS-IPC Shared Memory Engine](phases/phase-02-ipc-implementation.md) — Lock-free SPSC ring, cache isolation, and AHSP synchronization.
- [Phase 3: Workload Telemetry & Adaptive Selection Engine](phases/phase-03-adaptive-selection.md) — EWMA arrival tracking, crossover cost evaluator, and framed demuxing.
- [Phase 4: Matrix Sweeps & Adaptation Gain Validation](phases/phase-04-benchmarking-and-validation.md) — 11-payload sweep (64 B to 64 KB) and mixed workload validation ($G_{\text{adapt}} = 1.88\times$).
- [Phase 5: Performance Analysis & Micro-Architectural Profiling](phases/phase-05-analysis-and-optimization.md) — Cache-line validation, MESI state analysis, and CPU context switch audits.
- [Phase 6: Final Evaluation, Packaging & Viva Readiness](phases/phase-06-final-evaluation-and-presentation.md) — Academic defense artifacts, reproducible benchmarks, and packaging.

---

### Architectural Specifications (`guide/architecture/`)
- [Architecture Overview](architecture/README.md) — Overview of architectural specifications.
- [System Architecture](architecture/system-architecture.md) — Comprehensive block diagram, component interactions, and API definitions.
- [Data Flow](architecture/data-flow.md) — End-to-end path from `ipc_send()` to `ipc_receive()` across fast-path and stream transports.
- [IPC Selection Logic](architecture/ipc-selection-logic.md) — Crossover thresholds, EWMA formula, cost function equations, and hysteresis dampening.
- [Error Handling & Resource Cleanup](architecture/error-handling-and-resource-cleanup.md) — POSIX shared memory unlinking, FIFO draining, socket cleanup, and signal safety.

---

### Research Alignment & Academic Defense (`guide/research-alignment/`)
- [Research Alignment Overview](research-alignment/README.md) — Overview of academic alignment.
- [Paper vs Implementation](research-alignment/paper-versus-implementation.md) — Direct side-by-side comparison with Danyliuk (2026) and Krishnaveni & Ruby (2016).
- [Research Gap & Novelty](research-alignment/research-gap-and-novelty.md) — Novelty boundaries, honest limitations, and scientific differentiation.
- [Feature Alignment Matrix](research-alignment/feature-alignment-matrix.md) — Line-item matrix tracking claims vs verified code reality.
- [Limitations & Validity](research-alignment/limitations-and-validity.md) — Threats to validity, hardware dependencies, and boundary conditions.

---

### Testing, Benchmarking & Reproducibility (`guide/testing/`)
- [Testing Overview](testing/README.md) — Overview of testing methodology and test suites.
- [Test Environment](testing/test-environment.md) — Hardware specs, Linux kernel settings, CPU core pinning, and isolation.
- [Test Cases & Correctness](testing/test-cases-and-correctness.md) — Data integrity checks, sequence verification, boundary fuzzing, and demux order.
- [Benchmark Methodology](testing/benchmark-methodology.md) — Ping-pong round-trip timing, monotonic nanosecond clocks, and percentile calculation.
- [Results & Comparison](testing/results-and-comparison.md) — Tabular empirical findings across 11 payload sizes and mixed workloads.
- [Plotting & Visualization](testing/plotting-and-visualization.md) — Python scripts, generated charts (Figures 1-4), and CDF interpretations.
- [Reproducibility Guide](testing/reproducibility.md) — Step-by-step instructions to compile, run tests, and regenerate all results.

---

### Viva Voce & Presentation Preparation (`guide/viva/`)
- [Viva Overview](viva/README.md) — Master guide for academic oral examination.
- [Questions & Answers (30 Master Questions)](viva/questions-and-answers.md) — Beginner, intermediate, and advanced viva questions with model answers.
- [Phase-Wise FAQ](viva/phase-wise-faq.md) — Consolidated FAQ covering every implementation phase.
- [Research Defense](viva/research-defense.md) — How to counter critical professor challenges regarding novelty, overhead, and complexity.
- [Presentation Script](viva/presentation-script.md) — 30-second elevator pitch, 2-minute project summary, and 10-minute slide deck script.

---

### Roadmap & Future Extensions (`guide/roadmap/`)
- [Completed Work](roadmap/completed-work.md) — Audit of completed, tested, and verified components.
- [Pending Work](roadmap/pending-work.md) — Current limitations and items remaining for full enterprise hardening.
- [Next Steps](roadmap/next-steps.md) — Planned Tier-2 extensions (e.g., io_uring integration, multi-producer rings).

---

### General Documentation & References
- [Assets & Diagrams Index](assets/README.md) — Visual charts, Mermaid diagram descriptions, and plot links.
- [Academic References](references.md) — Complete bibliography with full citations, DOIs, and local file links.

---

## 3. Current Implementation Status Matrix

| Component / Subsystem | Implementation Status | Test Status | Evidence File / Location | Verified Performance |
| :--- | :---: | :---: | :--- | :--- |
| **Classical Baselines Harness** | Implemented | Tested | [baseline_1m_64b.csv](file:///results/baseline_1m_64b.csv) | Pipe: 1404 ns, Sock: 1370 ns, MQ: 1530 ns |
| **FAS-IPC SPSC Ring Engine** | Implemented | Tested | [fas_ipc_results.csv](file:///results/fas_ipc_results.csv) | 64 B: **196.17 ns**, 0 syscalls, 125 CSW/1M |
| **Workload Telemetry (EWMA)** | Implemented | Tested | [aipc_selector.c](file:///src/aipc_selector.c#L34-L63) | < 15 ns overhead, converged in < 30 samples |
| **Dynamic Crossover Logic** | Implemented | Tested | [matrix_results.csv](file:///results/matrix_results.csv) | Fast-path <= 1024 B, Pipe <= 64 KB, Sock > 64 KB |
| **Framed Demuxing & Ordering** | Implemented | Tested | [test_unified.c](file:///bench/test_unified.c) | Zero corruption, strict sequential ordering |
| **Full Payload Matrix (11 sizes)** | Implemented | Tested | [matrix_results.csv](file:///results/matrix_results.csv) | Evaluated across 64 B to 64 KB |
| **Mixed Workload Adaptation** | Implemented | Tested | [mixed_workload_gain.csv](file:///results/mixed_workload_gain.csv) | **1.88x Gain**, **46.86% Latency Reduction** |
| **Multi-Producer (MPSC/MPMC)** | Planned | Unverified | Roadmap only | Deferred to Tier-2 |
| **Kernel Module / eBPF Bypass** | Planned | Unverified | Roadmap only | Out of project scope (User-space design) |

---

## 4. How to Update This Guide

When future contributors add features or execute new benchmarks:
1. Do not overwrite historical measurement logs in `results/`. Always append or create versioned files (e.g., `results/matrix_results_v2.csv`).
2. Update the corresponding phase chapter in `guide/phases/` and update the status column in the tables.
3. Re-run `scripts/verify_all_4_phases.py` to confirm cross-phase regression safety before publishing new results.
