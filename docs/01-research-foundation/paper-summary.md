# Academic Paper Summaries
## Detailed Analysis of Foundational Research Papers

---

## 1. Danyliuk, I. M. (2026)

- **Full Citation:** Danyliuk, I. M. (2026), *Classical Interprocess Communication Mechanisms in Linux OS: Review and Latency Analysis for HPC Scenarios*, SWorldJournal, Issue 37 / Part 2, pp. 40–45. DOI: `10.30888/2663-5712.2026-37-02-028`.
- **Repository Location:** `Resources/year_2026_benchmark.pdf`
- **Focus:** Evaluation of five classical Linux IPC mechanisms for high-performance computing (HPC) scenarios under small payload sizes (64 to 1024 bytes).

### Key Architectural Findings:
1. **Latency Hierarchy (Table 1):**
   - **Shared Memory with Futex:** Demonstrates the lowest latency (~0.5–1.0 μs average, ~850 ns at 64 B; P99 ~1.5–3.0 μs). Once memory is mapped, access occurs purely in user space without kernel mediation, assuming no lock contention.
   - **Anonymous Pipes:** Demonstrates moderate latency (~2.0 μs average; P99 ~5.0–8.0 μs). Incurs 2 system calls (`write`/`read`) and 2 context switches per exchange.
   - **UNIX Domain Sockets:** Exhibits performance very close to pipes (~2.0 μs average; P99 ~5.0–8.0 μs). Efficient stream communication inside the kernel without network stack overhead.
   - **POSIX Message Queues:** Exhibits the highest latency (~12.0–18.0 μs average; P99 ~25.0–35.0 μs). Kernel priority queue insertion and message copying induce substantial algorithmic overhead.
2. **Impact of Spectre and Meltdown Mitigations (Section 4):**
   - Public disclosure of Spectre and Meltdown (2018) prompted kernel mitigations including Kernel Page-Table Isolation (KPTI) and retpoline.
   - Pre-mitigation system calls (e.g., `getpid`) consumed ~45–90 ns; post-mitigation cost rose to ~100–230 ns per syscall.
   - For primitives requiring 2 system calls per round-trip exchange, this introduces an unavoidable **200–400 ns latency penalty** purely for crossing the user/kernel privilege boundary.
   - Shared memory is fundamentally immune to this penalty because steady-state data transfer executes without kernel entry.

---

## 2. Krishnaveni, S., & Ruby, D. (2016)

- **Full Citation:** Krishnaveni, S., & Ruby, D. (2016), *Comparing and Evaluating the Performance of Inter Process Communication Models in Linux Environment*, International Journal of Trend in Research and Development (IJTRD), Special Issue NCSMAC-2016, ISSN: 2394-9333, pp. 51–55.
- **Repository Location:** `Resources/year_2016.pdf`
- **Focus:** Performance evaluation of IPC mechanisms across scaling data sizes from 32 bytes up to 99 Megabytes on Unix/Linux systems.

### Key Architectural Findings:
1. **Bulk Payload Transfer Behavior (Table 1 & Figures 1–5):**
   - Evaluated data transfer timings for payloads of 32 B, 306 B, 3 KB, 30 KB, 270.4 KB, 1.6 MB, 33 MB, and 99 MB.
   - For small payloads (32 B – 30 KB), Pipes and Message Queues execute quickly ($\le 1$ second).
   - For bulk payloads (99 MB), dramatic performance divergences emerge:
     - **Anonymous Pipe:** Required 161 seconds (stalled by frequent buffer saturation and blocking on the 64 KB kernel pipe buffer).
     - **System V Message Queue:** Required 220 seconds (degraded by discrete message packetization limits and queue management).
     - **Streaming Socket:** Required only **6 seconds** (benefiting from large kernel socket buffers and efficient TCP/UNIX streaming pipelines).
     - **Datagram Socket:** Required **2 seconds** for raw data transit.
2. **Implication for A-IPC Architecture:**
   - Demonstrates that while shared memory and pipes excel for small-to-medium messages, **UNIX Domain Sockets are indispensable for bulk streaming payloads**, validating their inclusion as the high-payload tier in A-IPC's multi-transport engine.

---

## 3. Immich, Bhagavatula, & Pendse (2003)

- **Full Citation:** Immich, P. K., Bhagavatula, R. S., & Pendse, R. (2003), *Performance analysis of five interprocess communication mechanisms across UNIX operating systems*, Journal of Systems and Software, 68(1), pp. 27–43.
- **Relevance:** Cited by Danyliuk (2026) as the seminal methodology establishing isolated 1:1 producer-consumer ping-pong echo benchmarking.
- **Key Contribution:** Proved that IPC performance varies drastically across operating system implementations, buffer sizes, and memory allocation schemes, disproving the notion of universal hardware rankings.
