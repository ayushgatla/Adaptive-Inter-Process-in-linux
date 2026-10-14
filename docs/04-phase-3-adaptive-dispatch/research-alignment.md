# Phase 3: Research Alignment
## Alignment with Runtime Workload Adaptation Theory

---

## 1. Compliance with Foundational Research Statement (`initial_problem_and_sol.pdf`)

The table below audits the Phase 3 implementation against the explicit requirements set forth in the project proposal document:

| Research Proposal Requirement | Proposal Citation | Proposed Architectural Strategy | Actual Repository Implementation | Alignment Status |
| :--- | :--- | :--- | :--- | :---: |
| **Unified Common Interface** | Sections 1 & 9 | `ipc_create()`, `ipc_send()`, `ipc_receive()`, `ipc_close()` hiding transport mechanics. | Implemented in `include/aipc.h` and `src/aipc_channel.c`. | **100% Aligned** |
| **Multi-Metric Telemetry** | Section 2 | Track message size, message rate, and queue backlog rather than frequency alone. | `aipc_selector_record_event()` in `src/aipc_selector.c` tracks size, byte counts, and EWMA rate. | **100% Aligned** |
| **EWMA Smoothing Formula** | Section 12 | Exponential moving average ($S_t = \alpha Y_t + (1-\alpha) S_{t-1}$) with $\alpha \approx 0.1$. | `AIPC_EWMA_ALPHA = 0.125` ($\frac{1}{8}$) implemented in `include/aipc_selector.h`. | **100% Aligned** |
| **Multi-Attribute Cost Function** | Section 13 | Score based on normalized latency, CPU cost, and queue pressure; select `argmin(Score)`. | `aipc_selector_evaluate_cost()` in `src/aipc_selector.c`. | **100% Aligned** |
| **Anti-Flapping Hysteresis** | Sections 14 & 15 | Require $\ge 15\%$ improvement and minimum decision window before triggering transport change. | `AIPC_HYSTERESIS_MARGIN = 0.15` and `AIPC_HYSTERESIS_MIN_WINDOW = 16` in `src/aipc_selector.c`. | **100% Aligned** |
| **Safe Transport Migration & Draining** | Section 14 | Prevent message loss when switching from Pipe to Socket or Shared Memory. | Coordinated demuxing with discrete `aipc_frame_hdr_t` framing headers in `src/aipc_channel.c`. | **100% Aligned** |

---

## 2. Analysis of Design Synthesis

Phase 3 synthesizes the individual findings of Danyliuk (2026) and Krishnaveni & Ruby (2016):
- Danyliuk demonstrated that shared memory is unbeatable for $\le 1024$ bytes but provides no native scaling for variable-length streams.
- Krishnaveni demonstrated that streaming sockets scale efficiently up to multi-megabytes, while pipes and message queues degrade severely.
- Phase 3 unites these mechanisms into a single runtime brain that dispatches traffic according to the empirical strengths of each primitive.
