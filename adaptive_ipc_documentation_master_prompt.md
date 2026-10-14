# Master Prompt: Generate Complete Phase-Wise Documentation for Adaptive IPC in Linux

Copy this entire prompt into your coding agent while it is opened at the root of the repository:

`https://github.com/ayushgatla/Adaptive-Inter-Process-in-linux`

---

## ROLE

Act as a senior Linux systems engineer, systems-performance researcher, technical writer, and university project-viva mentor.

Your task is to **inspect the existing repository first**, then create a complete, chapter-wise Markdown documentation directory that accurately explains the project’s evolution, implementation, research-paper alignment, benchmark methodology, results, limitations, and presentation strategy.

The project is an adaptive inter-process communication (IPC) framework for Linux. The intended research direction includes comparing standard IPC mechanisms with a shared-memory fast path and an adaptive transport-selection layer. Previously planned phases include:

1. Replication of baseline IPC benchmarks.
2. A high-speed shared-memory IPC engine.
3. Workload telemetry and adaptive transport selection.
4. An experimental matrix and adaptation-gain evaluation.
5. Visualizations, crossover analysis, and presentation/defense material.

Treat this list as **known project context, not proof that every phase is implemented**. Verify every status against the repository.

## CRITICAL RULES

1. **Inspect before writing.** Read `README.md`, the source tree, headers, `Makefile`, scripts, plans, result files, benchmark code, existing documentation, and relevant Git history. Identify available research-paper PDFs and other project resources. Do not assume that a file or feature exists just because it is mentioned here.
2. **Never fabricate progress or evidence.** Do not invent implementation details, completed phases, benchmark numbers, plots, paper claims, citations, test outcomes, or performance improvements.
3. **Separate four kinds of statements clearly:**
   - `Implemented and verified`
   - `Implemented but not yet verified`
   - `Planned / proposed`
   - `Unknown / requires evidence`
4. **Use exact source evidence.** When explaining implementation, cite repository-relative file paths and, where practical, function names and line ranges. Include commands that can reproduce a result.
5. **Research integrity is mandatory.** Do not claim that the project beats, matches, or reproduces a paper unless the comparison is methodologically valid and supported by data. Do not attribute a design to a paper unless the paper actually states it.
6. **Do not confuse targets with results.** For example, a target such as `<400 ns` latency is a target until measurements on identified hardware demonstrate it. Never turn a design goal into a reported achievement.
7. **Preserve the current implementation.** Documentation generation must not rewrite source code, change benchmark behavior, overwrite existing results, or delete files. Create a documentation-only directory unless the user explicitly authorizes code changes.
8. **Do not hide negative results.** Explain regressions, noisy measurements, unsupported claims, failed tests, missing evidence, and limitations honestly.
9. **Use consistent terminology.** Define IPC, latency, throughput, payload size, synchronization, spin-then-park, cache-line isolation, adaptive dispatch, crossover point, and adaptation gain.
10. **Make the output useful for a student project defense.** Explain both what the system does and why each engineering decision was made, including trade-offs and alternatives.
11. If a required paper or dataset is missing, record it as missing and explain what is needed. Do not silently substitute a different paper.
12. Avoid giant unstructured files. Keep one main index and separate chapters, appendices, figures, and data tables.

---

# STEP 1 — REPOSITORY AUDIT

Before generating the documentation, inspect the current repository and produce an internal inventory of:

- Existing directories and their purpose.
- Implemented IPC mechanisms and public APIs.
- Source files, headers, build configuration, benchmark executables, scripts, and test programs.
- Existing project plans and phase descriptions.
- Existing raw data, CSV/JSON output, logs, graphs, and generated reports.
- Research papers and supporting references available in the repository.
- Current build status and tests that can safely be run.
- Git history showing how the project evolved, where useful.
- Missing or ambiguous evidence that prevents a reliable conclusion.

Search for relevant symbols and implementation details, including—but not limited to—`fas_send`, `fas_recv`, `adz_send`, `adz_recv`, shared-memory setup, ring-buffer operations, atomics, futex calls, telemetry, EWMA sampling, dispatch decisions, payload thresholds, descriptor passing, and benchmark measurement code. These are investigation targets, not assumptions that the symbols or features exist.

Do not run destructive commands. Do not require root privileges unless essential and explicitly justified. Do not run potentially unsafe or long-running tests without checking their effects first.

Create a brief audit document:

`docs/project-status.md`

It must contain:

- Repository inventory.
- Current verified implementation status.
- Existing benchmark and test evidence.
- Research materials located.
- Missing evidence and unresolved questions.
- A phase-status table with status, supporting files, tests, and next action.

---

# STEP 2 — CREATE THIS DOCUMENTATION DIRECTORY

Create the following structure, adapting names only when the existing repository strongly warrants it. Do not duplicate large existing files; link to them where appropriate.

```text
docs/
├── README.md
├── project-status.md
├── 00-project-overview/
│   ├── chapter.md
│   ├── problem-statement.md
│   ├── objectives-and-scope.md
│   └── glossary.md
├── 01-research-foundation/
│   ├── chapter.md
│   ├── paper-summary.md
│   ├── research-claims-and-evidence.md
│   ├── research-vs-project.md
│   └── references.md
├── 02-phase-1-baselines/
│   ├── chapter.md
│   ├── implementation-flow.md
│   ├── research-alignment.md
│   ├── tests-and-results.md
│   ├── presentation-guide.md
│   └── faq.md
├── 03-phase-2-shared-memory-engine/
│   ├── chapter.md
│   ├── implementation-flow.md
│   ├── research-alignment.md
│   ├── tests-and-results.md
│   ├── presentation-guide.md
│   └── faq.md
├── 04-phase-3-adaptive-dispatch/
│   ├── chapter.md
│   ├── implementation-flow.md
│   ├── research-alignment.md
│   ├── tests-and-results.md
│   ├── presentation-guide.md
│   └── faq.md
├── 05-phase-4-experimental-evaluation/
│   ├── chapter.md
│   ├── implementation-flow.md
│   ├── research-alignment.md
│   ├── tests-and-results.md
│   ├── presentation-guide.md
│   └── faq.md
├── 06-phase-5-analysis-and-defense/
│   ├── chapter.md
│   ├── implementation-flow.md
│   ├── research-alignment.md
│   ├── tests-and-results.md
│   ├── presentation-guide.md
│   └── faq.md
├── 07-cross-phase-traceability/
│   ├── chapter.md
│   ├── phase-dependencies.md
│   ├── requirements-traceability.md
│   └── architecture-evolution.md
├── 08-testing-and-reproducibility/
│   ├── chapter.md
│   ├── benchmark-methodology.md
│   ├── correctness-and-stress-testing.md
│   ├── reproduction-guide.md
│   └── threats-to-validity.md
├── 09-results-and-comparisons/
│   ├── chapter.md
│   ├── results-summary.md
│   ├── paper-vs-project.md
│   ├── performance-analysis.md
│   └── missing-data-and-limitations.md
├── 10-presentation-and-viva/
│   ├── presentation-outline.md
│   ├── demo-script.md
│   ├── viva-question-bank.md
│   └── claims-to-defend.md
├── appendices/
│   ├── commands-and-build.md
│   ├── metric-definitions.md
│   ├── troubleshooting.md
│   └── evidence-index.md
└── figures/
    └── README.md
```

If a phase is not implemented, **still create its chapter**, but document the design, prerequisites, current status, missing implementation, and next steps. Clearly label it as planned or incomplete. Never manufacture a completed implementation to fill the directory.

---

# STEP 3 — REQUIREMENTS FOR EVERY PHASE

Every phase must have a complete chapter and the supporting files listed above. The main `chapter.md` must be readable on its own and link to the detailed documents.

For each phase, document all of the following sections.

## A. Purpose and problem

- What problem does this phase solve?
- Why is it necessary for the overall IPC project?
- What are the measurable objectives?
- What are the inputs, outputs, assumptions, and constraints?
- What is explicitly out of scope?

## B. Starting state and work completed so far

Explain the state of the project **at the start of this phase** and what has actually changed.

Use a table:

| Item | Before this phase | Work in this phase | Verified current state | Evidence |
|---|---|---|---|---|

Link each implementation claim to source files, tests, commits, or output artifacts. Distinguish source-code existence from successful execution and validation.

## C. Project flow

Describe the complete flow from input to output:

1. Application or benchmark starts.
2. Configuration and resources are initialized.
3. Processes communicate using the relevant mechanism.
4. Synchronization, buffering, and error handling take place.
5. Metrics are collected.
6. Results are validated and stored.

Adapt these steps to the actual phase. Include a Mermaid flowchart where helpful, plus a prose walkthrough. The diagram must represent the real implementation, not just the proposed architecture.

## D. Architecture and code walkthrough

- Explain the relevant modules and their responsibilities.
- Trace important function calls in execution order.
- Explain data structures, synchronization, ownership, error paths, and cleanup.
- Identify relevant system calls and explain why they are used.
- Include repository-relative file paths and exact function names.
- Explain important trade-offs and rejected alternatives when evidence exists.

For concurrent code, discuss memory ordering, producer/consumer ownership, buffer-full and buffer-empty behavior, wake-up races, process termination, and correctness assumptions wherever applicable.

## E. Research-paper alignment

Compare the research concept with the actual application.

Use a table such as:

| Research-paper concept or claim | Exact paper evidence | Intended project design | Actual implementation | Difference | Reason / implication | Validation status |
|---|---|---|---|---|---|---|

Requirements:

- Identify the paper by title, authors, year, and source.
- Cite the relevant page, section, figure, table, or equation wherever available.
- Explain whether the project reproduces, adapts, extends, or does not implement each concept.
- Distinguish a paper's reported configuration from this project's configuration.
- Identify assumptions that cannot be reproduced on the available hardware or software.
- Explain why any design deviation matters for correctness, performance, or interpretation.
- If the paper does not support a claim, write `Not established by the available paper evidence`.
- Never describe a project idea as a novel research contribution without checking the relevant literature.

## F. Alignment with previous and next phases

Explain exactly how this phase connects to the project timeline.

Include:

- What artifacts or capabilities it receives from the previous phase.
- Which earlier design decisions it relies on.
- What new capability it contributes.
- What later phase depends on its output.
- What would break or become unreliable if this phase were skipped.
- What evidence is required to declare the phase complete.

Use a dependency table:

| Dependency | Produced by | Consumed by | Interface / artifact | Verified? |
|---|---|---|---|---|

## G. Testing and correctness

Document:

- Unit tests, integration tests, stress tests, and benchmark tests that exist.
- Commands used to run each test.
- Expected versus actual behavior.
- Data-integrity checks, ordering checks, loss/duplication checks, and process-failure checks where relevant.
- Test environment, compiler options, kernel version, CPU, affinity, and relevant system settings when known.
- Failures, untested edge cases, and limitations.

Use a table:

| Test ID | Purpose | Command | Expected result | Actual result | Evidence | Status |
|---|---|---|---|---|---|---|

Do not mark a test `PASS` unless it was run or its result is supported by existing trustworthy evidence. Otherwise use `NOT RUN`, `BLOCKED`, or `UNKNOWN`.

## H. Test results and research comparison

Explain what was measured, how it was measured, what the data means, and whether it supports the project's hypothesis.

Include:

- Raw-data locations and data formats.
- Measurement methodology and units.
- Warm-up policy, repetitions, sample count, and aggregation method, if known.
- Latency distribution, throughput, CPU usage, memory use, and context-switch/system-call measurements where available and relevant.
- Median and tail latency where data supports them.
- Variability and uncertainty where enough repetitions exist.
- Comparison against the paper's reported results only when workloads, units, and methodology are sufficiently comparable.
- A clear statement when comparison is directional rather than apples-to-apples.

Use a table:

| Workload / payload | IPC mechanism | Paper result | Project result | Unit | Environment / method differences | Comparable? |
|---|---|---:|---:|---|---|---|

Use `Not reported`, `Not measured`, or `Not comparable` instead of inventing values.

Explain how to interpret the results without overstating them. If the paper's raw data or full methodology is unavailable, say so and limit the conclusions accordingly.

## I. Limitations and next actions

List:

- Current limitations.
- Unverified assumptions.
- Performance bottlenecks or regressions.
- Missing tests or data.
- Specific actions required before the next phase can rely on this phase.

Prioritize actions as `P0` (blocking correctness), `P1` (blocking reliable evaluation), or `P2` (improvement / polish).

## J. Presentation guide

For every phase, include a concise guide explaining how to present the work in a university project review or viva:

1. **Opening:** one or two sentences describing the phase's purpose.
2. **Diagram walkthrough:** what to point at and explain first, second, and third.
3. **Code demonstration:** which source file or function to open and what to show.
4. **Test demonstration:** exact safe command, expected output shape, and what evidence to display.
5. **Research comparison:** the most defensible difference between the paper and the application.
6. **Result interpretation:** what the data proves, what it suggests, and what it does not prove.
7. **Transition:** how this phase leads to the next one.
8. **Time budget:** a suggested short explanation (about 1 minute) and a deeper explanation (about 3–5 minutes).

Do not promise output that has not been verified. Where a demo is not currently runnable, provide a clearly marked fallback using recorded evidence.

## K. FAQ with answers

Create a phase-specific `faq.md` containing at least 10 useful questions and answers, provided the available evidence supports meaningful answers. Cover:

- Basic concept and motivation.
- Architecture and execution flow.
- Why this design was chosen over alternatives.
- Relevant Linux system calls and synchronization.
- How correctness is established.
- How performance is measured.
- What the research paper does differently.
- How this phase connects to previous and next phases.
- Limitations and failure scenarios.
- Likely examiner challenges and honest responses.

Answers must be technically accurate and explain the reasoning, not just provide one-line definitions. If an answer depends on missing evidence, say exactly what remains unknown and how to verify it. Add a few challenging follow-up questions that test understanding.

---

# STEP 4 — PHASE-SPECIFIC CONTENT

Use the following as a guide. Verify actual implementation before writing any statement as fact.

## Phase 1 — Baseline IPC benchmarking

Investigate whether the repository implements and measures:

- Pipes.
- UNIX domain sockets.
- POSIX message queues.
- Naive shared memory.
- Any baseline implementation or synchronization mechanism used by the selected research paper.

Explain why baselines are needed, how benchmark fairness is established, and how the project's measurements are compared with the paper.

Questions to answer:

- Are message sizes, process placement, synchronization, and timing methodology equivalent?
- Are setup and teardown costs included or excluded?
- Does the benchmark measure one-way or round-trip latency?
- How are throughput and latency calculated?
- What evidence demonstrates that baseline trends are reproduced?

## Phase 2 — High-speed shared-memory engine

Investigate the actual shared-memory implementation. Potential concepts to verify include:

- `shm_open` and `mmap`.
- A bounded power-of-two ring buffer.
- Atomic producer and consumer indices.
- Cache-line separation or alignment of hot metadata.
- Futex-based waiting.
- Adaptive hybrid spin-then-park (AHSP).
- Producer/consumer CPU affinity.

For each feature, explain the actual code path, correctness requirements, measured benefit, and trade-off. If any feature is only planned, mark it as such.

Potential evaluation criteria include message integrity, ordering, wraparound, full/empty behavior, process crashes, latency, throughput, and CPU consumption.

A latency target such as `<400 ns` must remain a target unless the repository contains valid measurements that support it on a specified machine.

## Phase 3 — Telemetry and adaptive transport selection

Investigate whether the application actually measures workload characteristics and dynamically selects a transport.

Potential concepts to verify include:

- Sampling and EWMA telemetry.
- A cost evaluator.
- A dynamic crossover threshold.
- Branchless or low-overhead dispatch.
- A shared-memory fast path for small payloads.
- A bulk-transfer path for large payloads.
- UNIX domain socket setup or descriptor passing.

If a threshold around `32 KB`, a sampling rate of `1 in 128`, or a large-message mechanism is only a proposed design, document it as a hypothesis rather than an implemented fact.

Explain how the selector avoids unstable switching, what happens when estimates are wrong, and how message order, ownership, and lossless delivery are maintained when switching transports. If the current implementation does not guarantee these properties, identify the gap.

## Phase 4 — Experimental matrix and adaptation-gain evaluation

Document the actual test matrix, including payload sizes, message rates, concurrency levels, transport mechanisms, and system conditions represented in the available experiments.

Investigate whether the project measures:

- Latency and throughput.
- CPU consumption.
- Context switches and system calls.
- Adaptation overhead.
- Selection accuracy.
- Workload changes and crossover behavior.
- Correctness while switching between transports.

Define every metric mathematically or operationally. For adaptation gain, specify the reference strategy, workload, and calculation. Do not invent a formula and claim it is the project's established metric; propose one explicitly if none exists and label it as proposed.

Discuss repeated trials, outliers, uncertainty, CPU affinity, NUMA placement where relevant, background load, and other threats to validity.

## Phase 5 — Visualizations and defense artifacts

Inspect existing analysis scripts and figures. Document or propose, as appropriate:

- Latency-versus-payload plots.
- Throughput-versus-payload plots.
- Latency CDFs.
- CPU-cost comparisons.
- Crossover maps.
- Adaptive-selection behavior under changing workloads.
- Summary tables comparing standard IPC, the paper baseline, the project's fast path, and the adaptive implementation.

Every figure must identify units, environment, workload, sample size or repetition count where available, and data source. Never generate a graph from fabricated or manually guessed values.

Explain how to present each graph, what conclusion it supports, and what alternative interpretation or limitation must be acknowledged.

---

# STEP 5 — CROSS-PHASE TRACEABILITY

Create a master phase-dependency diagram and a traceability matrix.

The matrix must connect:

`Research problem → Requirement → Design decision → Source implementation → Test → Measured result → Conclusion → Presentation claim`

Use a table:

| ID | Requirement / hypothesis | Design decision | Source evidence | Test evidence | Result | Claim allowed? |
|---|---|---|---|---|---|---|

For every important project claim, the reader should be able to navigate to its supporting code, test, data, and research evidence.

Flag any claim that has no supporting implementation or measurement. This is especially important for statements such as:

- “The new IPC engine is faster.”
- “The adaptive selector chooses the best transport.”
- “The project reproduces the research paper.”
- “Switching transports does not lose or reorder messages.”
- “The method reduces system calls or CPU usage.”

Claims must be scoped to the exact workloads and conditions supported by evidence.

---

# STEP 6 — BENCHMARK METHODOLOGY AND RESULT REPORTING

Create a reproducibility guide that enables another student to rerun the experiments.

Document:

- Required compiler, libraries, kernel features, and build tools.
- Build and execution commands.
- CPU and operating-system details.
- CPU affinity and NUMA policy, if used.
- Payload sizes and workload-generation procedure.
- Warm-up and repetition policy.
- Timing source and timer overhead considerations.
- Data-output format and output locations.
- Correctness checks performed alongside performance tests.
- How to regenerate plots from raw data.
- How to compare project results with the paper.
- Known sources of measurement bias.

Prefer raw machine-readable output such as CSV or JSON when supported by the existing tooling. Preserve original result files. Do not silently edit historical data.

If the benchmark harness is missing a required feature, document the gap and propose a concrete implementation plan instead of pretending the experiment is reproducible.

For comparisons, use identical units and clearly identify differences in hardware, kernel, compiler, workload, synchronization, and measurement methodology. If the environments differ, explain the effect this may have on interpretation.

---

# STEP 7 — RESEARCH PAPER AND PROJECT ALIGNMENT

Create one central `research-vs-project.md` or `paper-vs-project.md` that gives a whole-project comparison.

It must distinguish:

1. **Direct reproduction:** what is implemented and evaluated using sufficiently comparable conditions.
2. **Adaptation:** what has been modified from the paper and why.
3. **Extension:** what new behavior or architecture is proposed or implemented beyond the paper.
4. **Unimplemented concepts:** what remains planned.
5. **Unverified claims:** what lacks sufficient evidence.

Include a research-contribution statement with conservative wording. Clearly distinguish engineering integration from a novel research contribution.

For each paper claim, record the page/section/table, the paper's experimental conditions, the corresponding project evidence, and whether the comparison is valid.

Do not claim superiority from a single favorable result. Explain workload-specific trade-offs and report regressions as well as improvements.

---

# STEP 8 — PRESENTATION, DEMO, AND VIVA PREPARATION

Create a presentation outline sized for a university technical project defense. Recommend a logical narrative:

1. Problem and motivation.
2. Existing Linux IPC mechanisms and their trade-offs.
3. Research paper and baseline.
4. Overall architecture.
5. Phase-wise implementation.
6. Important design choices and synchronization.
7. Benchmark methodology.
8. Results and comparison.
9. Limitations and future work.
10. Conclusion and defensible contributions.

For each slide, include:

- Slide title.
- Key message.
- Suggested diagram, table, or graph.
- Evidence/source file to cite.
- Brief speaker notes.
- Likely question from the examiner.

Create a demo script with exact commands drawn from the repository. Include expected output patterns only when grounded in existing behavior. Add a fallback plan for unavailable hardware, a failed live benchmark, or an incomplete phase.

Create a viva question bank with answers covering:

- IPC fundamentals.
- Linux processes and system calls.
- Shared memory and synchronization.
- Atomics and memory ordering.
- Cache coherence and false sharing.
- Futexes and spin-then-park behavior.
- Benchmark design and statistical interpretation.
- Adaptive decision-making and crossover thresholds.
- Research-paper methodology.
- Correctness, failure handling, and limitations.
- Novelty, contributions, and future work.

Answers should be consistent with the implementation and the paper. Include “What evidence would you show?” for challenging claims.

---

# STEP 9 — DOCUMENT QUALITY REQUIREMENTS

- Write in clear technical English appropriate for a B.Tech CSE project report and viva.
- Explain advanced concepts without oversimplifying them.
- Use consistent headings, terminology, tables, and relative links.
- Add Mermaid diagrams where they genuinely clarify architecture or flow.
- Keep figures and tables numbered when referenced from other chapters.
- Use citations for research-paper claims and source-code references for implementation claims.
- Add a glossary and define units.
- Ensure relative Markdown links work.
- Do not copy long passages from research papers; summarize and cite them.
- Avoid repetitive filler and unsupported adjectives such as “revolutionary,” “optimal,” or “state-of-the-art.”
- Clearly label proposed designs and unimplemented features.
- Make each phase's FAQ directly relevant to that phase.
- Keep the top-level `docs/README.md` as a navigation index with a concise status summary.
- Include an evidence index linking every major claim to code, tests, raw data, or paper pages.

---

# STEP 10 — FINAL VALIDATION

After writing the documentation:

1. List every generated file.
2. Check all relative Markdown links.
3. Check that every phase has all required files, including a phase-specific FAQ with answers.
4. Check that the phase status matches the inspected source and test evidence.
5. Check every benchmark number against its raw output or an identified source.
6. Check every research-paper statement against the actual paper.
7. Search for unsupported claims such as `proved`, `guaranteed`, `zero-loss`, `faster`, `best`, and `reproduced`; qualify them where evidence is insufficient.
8. Verify that targets are not presented as measured results.
9. Verify that no source code or original result file was modified.
10. Produce a final summary stating:
    - What documentation was created.
    - Which phases are verified complete, partial, planned, or unknown.
    - Which tests were actually run.
    - Which research comparisons are valid and which are not.
    - What evidence is missing.
    - The highest-priority next steps.

## FINAL INSTRUCTION

Generate the documentation from the **current repository state**, not from assumptions about what the project ought to contain. The goal is a traceable, honest, reproducible account of the work completed so far, how it aligns with the research, how each phase builds on earlier phases, what the experiments demonstrate, and how to defend the project confidently without exaggerating its results.
