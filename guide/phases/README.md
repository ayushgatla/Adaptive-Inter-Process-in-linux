# Implementation Phases: Master Overview

Welcome to the development phases directory of the **Adaptive Inter-Process Communication (A-IPC)** project.

Each phase chapter in this directory documents a distinct milestone of our research, implementation, benchmarking, and validation journey. Every chapter follows a rigorous, standardized 11-section structure:
1. **Phase Summary** — Everyday explanation of the phase, problem solved, and status.
2. **Starting Point** — Repository state, preconditions, and prior dependencies.
3. **Project Flow** — End-to-end execution path with code snippets and Mermaid diagrams.
4. **Implementation Walkthrough** — Detailed examination of C11 source functions and data structures.
5. **Research-Paper Alignment** — 6-column comparison matrix contrasting our implementation with published literature.
6. **Connection with Previous Phases** — Inter-phase synergy and dependency progression.
7. **Testing & Evidence** — Real test commands, expected vs. observed results, and raw artifact links.
8. **Results Interpretation** — Hardware-level explanation of measured latencies, throughput, and context switches.
9. **Phase-Wise FAQ** — At least 10 in-depth questions and answers designed for academic defense.
10. **Viva Presentation Script** — 30-second pitch, 2-minute summary, code demonstrations, and defense strategy.
11. **Phase Exit Criteria** — Strict checklist proving completion with verified evidence.

---

## Master Phase Directory

- [Phase 1: Baseline Harness & Classical Replication](phase-01-baseline-and-foundation.md)  
  *Status: Implemented & Tested.*  
  Constructs the nanosecond-precision benchmark harness and replicates the 64-byte latency findings of Danyliuk (2026) for Anonymous Pipes, UNIX Domain Sockets, POSIX Message Queues, and Naive Shared Memory.

- [Phase 2: Ultra-Low Latency FAS-IPC Shared Memory Engine](phase-02-ipc-implementation.md)  
  *Status: Implemented & Tested.*  
  Designs and implements our custom lock-free SPSC circular ring buffer with 64-byte cache-line isolation and Adaptive Hybrid Spin-Park (AHSP) futex synchronization, achieving **196.17 ns** mean latency (4.33x faster than Danyliuk).

- [Phase 3: Workload Telemetry & Adaptive Selection Engine](phase-03-adaptive-selection.md)  
  *Status: Implemented & Tested.*  
  Builds the unified application channel (`ipc_send()`, `ipc_receive()`), lightweight EWMA arrival tracking (< 15 ns cost), dynamic crossover logic, and sequential binary frame demuxing.

- [Phase 4: Full Matrix Sweeps & Adaptation Gain Validation](phase-04-benchmarking-and-validation.md)  
  *Status: Implemented & Tested.*  
  Executes full-spectrum matrix sweeps across 11 payload sizes (64 B to 64 KB) and proves the core research thesis under an 80/20 mixed workload: **$G_{\text{adapt}} = 1.88\times$** and **46.86% latency reduction**.

- [Phase 5: Performance Analysis & Micro-Architectural Profiling](phase-05-analysis-and-optimization.md)  
  *Status: Implemented & Tested.*  
  Profiles hardware-level CPU cache states (MESI protocol), context switch suppression, and generates publication-grade visual charts (Figures 1 to 4).

- [Phase 6: Final Evaluation, Packaging & Viva Readiness](phase-06-final-evaluation-and-presentation.md)  
  *Status: Completed & Documented.*  
  Prepares comprehensive oral defense scripts, reproducible test commands (`scripts/verify_all_4_phases.py`), and master packaging for examiners.
