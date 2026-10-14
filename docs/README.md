# Adaptive Inter-Process Communication (A-IPC) in Linux
## Master Documentation Index & Technical Knowledge Base

Welcome to the official technical documentation for the **Adaptive Inter-Process Communication (A-IPC)** framework. This documentation suite provides an exhaustive, peer-review-grade breakdown of the architecture, implementation, research paper alignment, benchmark methodology, empirical results, and university viva defense strategy for the A-IPC project.

---

## 1. Documentation Structure & Sitemap

```
docs/
├── README.md                                    # Master Documentation Hub (You are here)
├── project-status.md                            # Comprehensive Repository Audit & Verification Status
├── 00-project-overview/                         # Executive Overview & Core Problem Formulation
│   ├── chapter.md                               # Chapter overview: System goals, background, and summary
│   ├── problem-statement.md                     # Formal problem statement & limitations of static IPC
│   ├── objectives-and-scope.md                  # Concrete engineering objectives, scope & non-goals
│   └── glossary.md                              # Formal systems & concurrency terminology glossary
├── 01-research-foundation/                      # Academic Literature Analysis & Alignment
│   ├── chapter.md                               # Chapter overview: Synthesis of literature & research questions
│   ├── paper-summary.md                         # Deep review of Danyliuk (2026), Krishnaveni (2016), etc.
│   ├── research-claims-and-evidence.md          # Scientific claims vs. empirical citations
│   ├── research-vs-project.md                   # Systematic comparison: Reproduction vs. Novel Adaptation
│   └── references.md                            # Formal academic bibliography
├── 02-phase-1-baselines/                        # Phase 1: Benchmark Harness & Classical Baselines
│   ├── chapter.md                               # Phase 1 Chapter: Core problem, starting state & workflow
│   ├── implementation-flow.md                   # Execution flow, data structures & system call analysis
│   ├── research-alignment.md                    # Danyliuk (2026) Table 1 baseline comparison
│   ├── tests-and-results.md                     # 1M round-trip measurement data & syscall validation
│   ├── presentation-guide.md                    # Viva defense walkthrough & script for Phase 1
│   └── faq.md                                   # Comprehensive Phase 1 technical FAQ
├── 03-phase-2-shared-memory-engine/             # Phase 2: Ultra-Low Latency FAS-IPC Engine
│   ├── chapter.md                               # Phase 2 Chapter: Cache-line isolation & AHSP synchronization
│   ├── implementation-flow.md                   # SPSC ring buffer, alignas(64), atomic acquire/release
│   ├── research-alignment.md                    # Beating Danyliuk's 850 ns baseline (< 400 ns target)
│   ├── tests-and-results.md                     # 5M stress test, zero-corruption, 0-syscall verification
│   ├── presentation-guide.md                    # Viva defense walkthrough & code inspection guide
│   └── faq.md                                   # Comprehensive Phase 2 technical FAQ
├── 04-phase-3-adaptive-dispatch/                # Phase 3: Workload Telemetry & Unified Channel
│   ├── chapter.md                               # Phase 3 Chapter: Telemetry, EWMA, & dynamic crossover
│   ├── implementation-flow.md                   # EWMA arrival tracker, cost evaluator, framed demuxing
│   ├── research-alignment.md                    # Alignment with dynamic workload adaptation theory
│   ├── tests-and-results.md                     # Telemetry convergence & dynamic payload routing tests
│   ├── presentation-guide.md                    # Viva defense walkthrough for adaptive routing
│   └── faq.md                                   # Comprehensive Phase 3 technical FAQ
├── 05-phase-4-experimental-evaluation/          # Phase 4: Full Matrix & Adaptation Gain Evaluation
│   ├── chapter.md                               # Phase 4 Chapter: Comprehensive 11-payload matrix sweep
│   ├── implementation-flow.md                   # Benchmark harness automation & multi-transport harness
│   ├── research-alignment.md                    # Empirical proof of Adaptation Gain hypothesis
│   ├── tests-and-results.md                     # Matrix results, G_adapt = 1.88x, 46.86% latency reduction
│   ├── presentation-guide.md                    # Viva defense walkthrough for empirical results
│   └── faq.md                                   # Comprehensive Phase 4 technical FAQ
├── 06-phase-5-analysis-and-defense/             # Phase 5: Visualizations & Defense Artifacts
│   ├── chapter.md                               # Phase 5 Chapter: Visualizations, crossover maps, defense
│   ├── implementation-flow.md                   # Automated Python/Matplotlib 300 DPI plotting pipeline
│   ├── research-alignment.md                    # Visual defense of research questions
│   ├── tests-and-results.md                     # Verification of Figure 1 through Figure 4 outputs
│   ├── presentation-guide.md                    # Guide to presenting charts to evaluators
│   └── faq.md                                   # Comprehensive Phase 5 technical FAQ
├── 07-cross-phase-traceability/                 # Traceability & System Synergy
│   ├── chapter.md                               # Master traceability overview
│   ├── phase-dependencies.md                    # End-to-end component dependencies & artifacts
│   ├── requirements-traceability.md             # Research Hypothesis -> Code -> Test -> Result Matrix
│   └── architecture-evolution.md                # Architectural progression from Phase 1 to Phase 5
├── 08-testing-and-reproducibility/              # Testing Rigor & Experimental Methodology
│   ├── chapter.md                               # Overview of reproducibility standards
│   ├── benchmark-methodology.md                 # Timing precision, CPU pinning, cache warmup, isolation
│   ├── correctness-and-stress-testing.md        # Data validation, race condition tests, memory safety
│   ├── reproduction-guide.md                    # Step-by-step instructions to rerun all experiments
│   └── threats-to-validity.md                   # Construct, internal, external, and conclusion validity
├── 09-results-and-comparisons/                  # Empirical Results Synthesis
│   ├── chapter.md                               # Results overview & high-level takeaways
│   ├── results-summary.md                       # Comprehensive data tables across all payloads
│   ├── paper-vs-project.md                      # Side-by-side comparison with Danyliuk & Krishnaveni
│   ├── performance-analysis.md                  # Micro-architectural analysis (cache, context switches)
│   └── missing-data-and-limitations.md          # Honest disclosure of edge cases & non-evaluated zones
├── 10-presentation-and-viva/                    # University Defense & Examination Toolkit
│   ├── presentation-outline.md                  # 10-Slide presentation deck structure & speaker notes
│   ├── demo-script.md                           # Live terminal demonstration script & fallback plans
│   ├── viva-question-bank.md                    # 30+ Challenging examiner questions with technical answers
│   └── claims-to-defend.md                      # Defensible claims vs. indefensible exaggerations
├── appendices/                                  # Operational Appendices & Reference Guides
│   ├── commands-and-build.md                    # Compilation, execution & profiling cheatsheet
│   ├── metric-definitions.md                    # Mathematical definitions of RTT, MPS, MB/s, G_adapt
│   ├── troubleshooting.md                       # Fixing POSIX mqueue limits, stale SHM, permissions
│   └── evidence-index.md                        # Master index linking every claim to exact lines & data
└── figures/                                     # Figure Catalog & Visualization Assets
    └── README.md                                # Catalog of generated figures with captions & data sources
```

---

## 2. Quick Status Dashboard

| Phase | Core Capability | Key Metric / Result | Implementation Status | Verification Status |
| :---: | :--- | :--- | :---: | :---: |
| **Phase 1** | Classical Baselines (Pipe, Socket, MQ, Naive SHM) | Replicated Danyliuk (2026): ~1.3–1.6 μs latency, 2 syscalls/op | Complete (`aipc_baselines.c`) | **Verified** (`baseline_1m_64b.csv`) |
| **Phase 2** | FAS-IPC Lock-Free Shared Memory Fast Path | **196.17 ns** mean latency at 64B, 0 syscalls/op, 2.43M msg/s | Complete (`aipc_fas_ring.c`) | **Verified** (`fas_ipc_results.csv`) |
| **Phase 3** | Workload Telemetry & Dynamic Crossover Channel | EWMA tracking ($\alpha=0.125$), dynamic crossover, 0 msg loss | Complete (`aipc_selector.c`, `aipc_channel.c`) | **Verified** (`test_unified.c`, `test_phase3_logic.py`) |
| **Phase 4** | Experimental Evaluation & Adaptation Gain | **$G_{\text{adapt}} = 1.88\times$**, 46.86% latency reduction on mixed workload | Complete (`bench_matrix.c`) | **Verified** (`matrix_results.csv`, `mixed_workload_gain.csv`) |
| **Phase 5** | Academic Visualizations & Defense Documentation | 4 Publication-grade 300 DPI figures & Defense Summary | Complete (`plot_results.py`) | **Verified** (`results/plots/`, `summary_table.md`) |

---

## 3. How to Navigate This Documentation

- **For University Project Evaluators / Examiners**:
  Start with [docs/10-presentation-and-viva/claims-to-defend.md](file:///docs/10-presentation-and-viva/claims-to-defend.md) and [docs/09-results-and-comparisons/paper-vs-project.md](file:///docs/09-results-and-comparisons/paper-vs-project.md) to inspect our scientific honesty, research alignment, and verified empirical gains.
- **For Systems Engineers / Kernel Developers**:
  Dive into [docs/03-phase-2-shared-memory-engine/implementation-flow.md](file:///docs/03-phase-2-shared-memory-engine/implementation-flow.md) to review our cache-line isolation (`alignas(64)`), C11 memory ordering, and Adaptive Hybrid Spin-Park (AHSP) futex logic.
- **For Benchmark Reproducibility**:
  Follow [docs/08-testing-and-reproducibility/reproduction-guide.md](file:///docs/08-testing-and-reproducibility/reproduction-guide.md) to rebuild from source and replicate all 1,000,000-iteration sweeps.
