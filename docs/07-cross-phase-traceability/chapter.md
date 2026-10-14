# Chapter 7: Cross-Phase Traceability
## Complete Requirements Traceability & Architectural Evolution

---

## 1. Executive Summary

A critical criterion of research integrity and high-distinction engineering is **end-to-end traceability**. Every claim made in the project proposal, presentation, or thesis must be traceable forward to concrete source code, reproducible test harnesses, empirical data files, and valid conclusions. Conversely, every piece of source code in the repository must be traceable backward to a specific research requirement or literature observation.

This chapter establishes full bidirectional traceability across the 5 project phases:
1. **Master Phase Dependencies**: How each phase consumes artifacts from preceding phases and delivers capabilities to subsequent phases.
2. **Requirements Traceability Matrix**: Mapping:
   $$\text{Research Hypothesis} \longrightarrow \text{Requirement} \longrightarrow \text{Design Decision} \longrightarrow \text{Source Code} \longrightarrow \text{Test Evidence} \longrightarrow \text{Result} \longrightarrow \text{Presentation Claim}$$
3. **Architecture Evolution**: How the codebase grew systematically from an isolated benchmark harness in Phase 1 to a unified adaptive runtime framework in Phase 5.

---

## 2. Master System Dependency Diagram

```mermaid
flowchart TD
    subgraph P1["Phase 1: Classical Baselines & Timing Harness"]
        P1_Code["include/aipc_bench.h\nsrc/aipc_bench.c\nsrc/aipc_baselines.c"]
        P1_Data["results/baseline_1m_64b.csv\n(Pipe: 1404 ns, Socket: 1370 ns, MQ: 1530 ns)"]
    end

    subgraph P2["Phase 2: Lock-Free FAS-IPC Engine"]
        P2_Code["include/aipc_fas_ring.h\nsrc/aipc_fas_ring.c\nbench/test_fas_ring.c"]
        P2_Data["results/fas_ipc_results.csv\n(64B: 196.17 ns, 0 syscalls/op, 125 CSW)"]
    end

    subgraph P3["Phase 3: Workload Telemetry & Unified Channel"]
        P3_Code["include/aipc.h\ninclude/aipc_selector.h\nsrc/aipc_selector.c\nsrc/aipc_channel.c"]
        P3_Logic["EWMA arrival tracker (alpha=0.125)\nCrossover selector (<=1KB, 1-64KB, >64KB)\nFramed Demuxing (aipc_frame_hdr_t)"]
    end

    subgraph P4["Phase 4: Full Matrix & Adaptation Gain Evaluation"]
        P4_Code["bench/bench_matrix.c\nscripts/run_experiments.sh"]
        P4_Data["results/matrix_results.csv (11 payloads)\nresults/mixed_workload_gain.csv (G_adapt = 1.88x)"]
    end

    subgraph P5["Phase 5: Automated Visualization & Defense"]
        P5_Code["scripts/plot_results.py"]
        P5_Plots["results/plots/fig[1-4]_*.{png,pdf}\nresults/summary_table.md"]
    end

    P1_Code -->|Provides CLOCK_MONOTONIC_RAW & CPU pinning| P2_Code
    P1_Code -->|Provides streaming Pipe & Socket fallbacks| P3_Code
    P2_Code -->|Provides <= 1024 B sub-200 ns fast path| P3_Code
    P3_Code & P3_Logic -->|Powers Adaptive A-IPC Engine row| P4_Code
    P1_Data & P2_Data & P4_Data -->|Provides raw empirical datasets| P5_Code
    P5_Code --> P5_Plots
```

---

## 3. Chapter Structure

- [Phase Dependencies](file:///docs/07-cross-phase-traceability/phase-dependencies.md): Explicit interfaces, inputs, and outputs across all phases.
- [Requirements Traceability Matrix](file:///docs/07-cross-phase-traceability/requirements-traceability.md): The exhaustive claim-to-code traceability matrix.
- [Architecture Evolution](file:///docs/07-cross-phase-traceability/architecture-evolution.md): Historical evolution and design rationale across the project lifecycle.
