# Chapter 0: Project Overview
## Adaptive Inter-Process Communication (A-IPC) in Linux

---

## 1. Executive Summary

Modern multi-core Linux systems rely heavily on inter-process communication (IPC) for distributed computation, microservice coordination, and high-performance computing (HPC) workflows. Even standardized distributed libraries such as OpenMPI fall back to local Linux OS IPC primitives when communicating between processes on the same physical host.

However, Linux provides multiple distinct IPC mechanisms—including **Anonymous Pipes**, **UNIX Domain Sockets**, **POSIX Message Queues**, and **Shared Memory**. Each primitive operates under entirely different kernel buffering semantics, synchronization mechanisms, and performance profiles. Contemporary software architectures statically select an IPC mechanism at compile time or initial design. As application workloads shift across differing payload distributions (e.g., small high-frequency signaling messages versus large bulk data transfers), static IPC selections incur severe latency penalties, excessive kernel context switches, or buffer contention.

**Adaptive Inter-Process Communication (A-IPC)** resolves this fundamental bottleneck. It introduces a unified, transport-agnostic communication abstraction (`ipc_send`, `ipc_receive`) backed by:
1. **A Zero-Syscall Lock-Free Fast Path (FAS-IPC)** utilizing 64-byte cache line isolation (`alignas(64)`) and Adaptive Hybrid Spin-Park (AHSP) synchronization for payloads $\le 1024$ bytes.
2. **A Workload Telemetry Engine** monitoring payload size and exponentially smoothed arrival rates ($\alpha = 0.125$).
3. **An Adaptive Selector & Dispatcher** routing data dynamically across shared memory, pipes, and domain sockets based on empirical crossover thresholds.

---

## 2. Core Research Question

> *Can runtime workload-aware selection of Linux IPC mechanisms reduce communication latency and CPU cost while increasing throughput compared with any statically selected IPC mechanism?*

Grounded in empirical evaluation, our research answers this question affirmatively. Across dynamic, multi-modal workloads (80% 64-byte signaling + 20% 16-kilobyte bulk payload), Adaptive A-IPC delivers an **Adaptation Gain ($G_{\text{adapt}}$) of $1.88\times$** and a **46.86% latency reduction** compared to the best static primitive, while completely eliminating kernel transitions on small messages.

---

## 3. High-Level System Architecture

The following diagram illustrates the layered architecture of the A-IPC framework:

```mermaid
flowchart TD
    subgraph USER_SPACE["Application & Interface Layer"]
        AppProducer["Producer Process"]
        AppConsumer["Consumer Process"]
        APISend["ipc_send(chan, data, len)"]
        APIRecv["ipc_receive(chan, buf, max_len)"]
    end

    subgraph ADAPTIVE_CORE["Telemetry & Dynamic Selection Engine"]
        Observer["Workload Observer\n• Payload Size Inspector\n• EWMA Arrival Rate Tracker (alpha=0.125)\n• Backlog Estimator"]
        Selector{"Adaptive Selector\n• Dynamic Crossover Logic\n• Heuristic Cost Function\n• Anti-Flapping Hysteresis (15%)"}
    end

    subgraph TRANSPORT_PLANE["Multi-Transport Execution Plane"]
        direction TB
        FAS_IPC["Fast Path: FAS-IPC Shared Memory\n• 64B Cache Line Isolated SPSC Ring\n• AHSP: 200 _mm_pause() -> futex park\n• Payload: <= 1024 B | 0 Syscalls/op"]
        PIPE_TR["Streaming Path: Linux Anonymous Pipe\n• Kernel Circular Buffer (64 KB)\n• Payload: 1025 B – 65536 B | 2 Syscalls/op"]
        SOCK_TR["Bulk Path: UNIX Domain Socket (AF_UNIX)\n• Bidirectional Stream Socket\n• Payload: > 65536 B | High Bandwidth"]
    end

    AppProducer --> APISend
    APISend --> Observer
    Observer --> Selector
    Selector -- "<= 1024 B" --> FAS_IPC
    Selector -- "1025 B - 64 KB" --> PIPE_TR
    Selector -- "> 64 KB" --> SOCK_TR

    FAS_IPC --> APIRecv
    PIPE_TR --> APIRecv
    SOCK_TR --> APIRecv
    APIRecv --> AppConsumer
```

---

## 4. Chapter Directory & Supporting Documents

This chapter introduces the project foundations through the following detailed documents:

1. [Problem Statement](file:///docs/00-project-overview/problem-statement.md): Formal analysis of Linux IPC trade-offs and the flaw of static compile-time selection.
2. [Objectives and Scope](file:///docs/00-project-overview/objectives-and-scope.md): Explicit project deliverables, engineering constraints, and out-of-scope boundaries.
3. [Glossary](file:///docs/00-project-overview/glossary.md): Comprehensive definitions of operating systems, micro-architectural, and benchmarking terms.
