# Viva Voce Master Guide: Presentation Scripts & Pitch Deck

## 1. Overview of Presentation Formats

This guide provides three tailored scripts for presenting the **Adaptive Inter-Process Communication (A-IPC)** framework:
1. **The 30-Second Elevator Pitch:** For rapid high-level introductions.
2. **The 2-Minute Executive Summary:** For introductory viva questions (*"Tell us about your project"*).
3. **The 10-Minute Honors Slide Deck Script:** Slide-by-slide script for final project presentations and capstone defenses.

---

## 2. The 30-Second Elevator Pitch

> *"In Linux, software processes traditionally hardcode a single communication primitive—like Pipes or Shared Memory. But small messages suffer from high kernel system call overheads (~1.4 μs), while shared memory cannot handle large bulk data.  
> We engineered Adaptive A-IPC: a C11 user-space framework that dynamically routes messages based on real-time traffic. We built a custom lock-free shared memory engine that achieves **196 nanoseconds** latency for small packets, and seamlessly falls back to pipes for bulk streaming.  
> Under realistic mixed traffic, our system achieves an **Adaptation Gain of 1.88x** and cuts communication latency by **46.86%** over the best static Linux baseline."*

---

## 3. The 2-Minute Executive Summary

> *"Good morning, respected professors and examiners. Today I am presenting our capstone research project: **Adaptive Inter-Process Communication in Linux**.  
> 
> When two separate programs run on the same computer, they must communicate. Developers typically face a difficult trade-off:  
> If they choose Anonymous Pipes or UNIX Domain Sockets, small signaling messages pay a heavy penalty: every single transfer forces two operating system system calls and a process context switch, wasting over 1,300 nanoseconds.  
> If they choose Shared Memory, data transfer is fast, but fixed-size memory slots cannot accommodate large, bursty data chunks without dropping packets or consuming massive amounts of RAM.  
> 
> Existing research—including Danyliuk (2026)—evaluated these mechanisms only as rigid, static choices. We asked a fundamental research question: *Can a system observe traffic in real time and dynamically dispatch each message to the optimal transport?*  
> 
> To answer this, we built A-IPC across four engineering phases:  
> First, in Phase 1, we replicated Danyliuk's classical baselines locally, confirming the ~1,400 ns kernel bottleneck and capturing over 999,000 context switches.  
> Second, in Phase 2, we designed **FAS-IPC**, a lock-free SPSC circular ring buffer. By padding Producer and Consumer indices to separate 64-byte cache lines to eliminate false sharing, and implementing our Adaptive Hybrid Spin-Park algorithm, we achieved a mean latency of **196.17 nanoseconds** at 64 bytes—running **4.33x faster** than Danyliuk's 850 ns benchmark with zero system calls.  
> Third, in Phase 3, we implemented an EWMA traffic observer that evaluates transport costs in under 15 nanoseconds, and created a binary framing protocol that guarantees in-order packet delivery across transport switches.  
> Finally, in Phase 4, we evaluated the system under a realistic 80/20 mixed workload. While static shared memory crashed on large packets and static pipes suffered high latency, our Adaptive engine achieved 932 nanoseconds mean latency. This delivered an **Adaptation Gain of 1.88x**—an 88% throughput increase—and a **46.86% latency cut** over the best static baseline.  
> 
> All code compiles with zero warnings, and every empirical claim is 100% reproducible via our automated verification suite."*

---

## 4. The 10-Minute Slide Deck Presentation Script

### Slide 1: Title & Team
- **Visual:** Project Title: *Adaptive Inter-Process Communication (A-IPC) in Linux: Runtime Workload-Aware Selection of IPC Mechanisms*.
- **Script:** *"Respected members of the evaluation panel, welcome to the final presentation of our research project on Adaptive Linux IPC. My name is [Student Name], and today I will show you how our system breaks through the traditional latency trade-offs of Linux inter-process communication."*

---

### Slide 2: The Problem: The Static Primitive Compromise
- **Visual:** Diagram of Process A and Process B, showing the dichotomy between small signaling packets and bulk data streams.
- **Script:** *"In modern operating systems, processes run in isolated address spaces. When they communicate, developers statically hardcode an IPC mechanism at compile time.  
This introduces a severe compromise: small signaling messages (like heartbeats or RPC headers) suffer massive system call and context-switching overheads in pipes and sockets (~1.4 μs). Meanwhile, fixed-size lock-free shared memory cannot handle large, bursty data transfers without packet loss or excessive RAM bloat."*

---

### Slide 3: Research Gap & Literature Context
- **Visual:** Side-by-side comparison of Danyliuk (2026) and Krishnaveni & Ruby (2016).
- **Script:** *"Our research directly examines published literature. Danyliuk (2026) evaluated 64-byte latency in HPC clusters, showing that naive shared memory achieved 850 ns while pipes took 2.1 μs. However, Danyliuk only evaluated 64-byte messages in static isolation. Krishnaveni (2016) evaluated bulk streaming up to 99 MB, but ignored micro-latency.  
The gap was clear: no published framework bridges micro-signaling and bulk streaming dynamically at runtime."*

---

### Slide 4: System Architecture & Public API
- **Visual:** The 4-layer system diagram from [guide/03-existing-system-and-project-architecture.md](file:///guide/03-existing-system-and-project-architecture.md).
- **Script:** *"We designed A-IPC as a user-space C11 framework with four layers:  
At the top is our clean public API (`ipc_send()`, `ipc_receive()`).  
Beneath it sits our Workload Observer and Adaptive Selector.  
At the execution plane, we synthesize two complementary engines: our custom fast-path shared memory ring (FAS-IPC) for signaling, and streaming Linux Pipes and UNIX Domain Sockets for bulk transfers."*

---

### Slide 5: The FAS-IPC Shared Memory Fast Path
- **Visual:** Memory map of `fas_control_header_t` showing 64-byte `alignas(64)` cache lines and AHSP state diagram.
- **Script:** *"To achieve true hardware speed for small payloads, we built FAS-IPC. We tackled three micro-architectural bottlenecks:  
First, we eliminated false sharing by padding the Producer's `tail` and Consumer's `head` to distinct 64-byte cache lines using C11 `alignas(64)`.  
Second, we enforced Acquire-Release atomic memory barriers to prevent out-of-order memory reordering.  
Third, we created Adaptive Hybrid Spin-Park: the receiver micro-spins for 200 iterations (~300 ns) using `_mm_pause()`, absorbing streaming traffic in user space with zero system calls, only sleeping via `SYS_futex` when the channel is truly idle.  
Result: 196.17 nanoseconds mean latency—4.33x faster than Danyliuk's benchmark."*

---

### Slide 6: Telemetry, Crossover & Framing
- **Visual:** EWMA formula ($S_t = \alpha Y_t + (1 - \alpha) S_{t-1}$) and the 24-byte `aipc_frame_hdr_t` structure.
- **Script:** *"To make intelligent decisions without slowing down data transfer, our selector calculates an Exponentially Weighted Moving Average of arrival rate in under 15 nanoseconds.  
It applies hard physical crossover boundaries at 1,024 bytes and enforces a 15% hysteresis margin to eliminate route flapping.  
Crucially, we solved the problem of cross-transport reordering. By prepending a 24-byte binary framing header with global sequence numbers, `ipc_receive()` guarantees strict sequential delivery across transport switches."*

---

### Slide 7: Matrix Benchmark Sweep (Figure 1 & Figure 2)
- **Visual:** Display [Figure 1 (Danyliuk Comparison)](file:///results/plots/fig1_danyliuk_comparison.png) and [Figure 2 (Crossover Map)](file:///results/plots/fig2_crossover_map.png).
- **Script:** *"We swept the system across 11 payload sizes from 64 B to 64 KB. Figure 1 shows our 4.33x latency speedup at 64 bytes. Figure 2 shows the empirical crossover contour: payloads up to 1 KB run at sub-370 ns with 0 system calls, and payloads above 1 KB smoothly hand off to streaming pipes without dropping a single packet."*

---

### Slide 8: The Core Proof: Mixed Workload & Adaptation Gain (Figure 4)
- **Visual:** Display [Figure 4 (Adaptation Gain)](file:///results/plots/fig4_adaptation_gain.png).
- **Script:** *"This slide presents our core scientific proof. We simulated a realistic production workload comprising 80% small control packets (64 B) and 20% large data chunks (16 KB).  
Static shared memory dropped packets on 16 KB bursts. Static pipes suffered an average latency of 2,077 nanoseconds.  
Our Adaptive A-IPC engine achieved a mean latency of 932.59 nanoseconds and 536,143 messages per second.  
This delivers an **Adaptation Gain ($G_{\text{adapt}}$) of 1.88x** and a **46.86% latency reduction** over the best static baseline."*

---

### Slide 9: Limitations & Academic Honesty
- **Visual:** Table of system boundaries: Single-Host, SPSC Scope, Fixed Ring Sizing.
- **Script:** *"To maintain academic rigor, we openly identify our system boundaries:  
FAS-IPC is currently Single-Producer Single-Consumer (SPSC); expanding to Multi-Producer (MPSC) with Compare-And-Swap loops is planned for Tier-2.  
The framework operates on a single physical host and does not replace distributed network protocols.  
And if a workload is 100% static, a static primitive suffices. Our framework is purpose-built for dynamic, mixed traffic."*

---

### Slide 10: Conclusion & Live Demonstration
- **Visual:** Summary box showing key metrics: 196.17 ns latency, 99.98% kernel bypass, 1.88x Adaptation Gain.
- **Script:** *"In conclusion, A-IPC proves that workload-aware runtime adaptation provides a superior alternative to compile-time static binding in Linux IPC.  
I invite the panel to observe our master verification suite running live in the terminal right now. Thank you, and I look forward to your questions."*
