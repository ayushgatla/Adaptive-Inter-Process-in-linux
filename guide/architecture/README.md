# System Architecture: Master Overview

Welcome to the Architectural Specifications section of the **Adaptive Inter-Process Communication (A-IPC)** framework.

This directory documents the structural design, data flow, mathematical algorithms, and resource lifecycle management that govern our high-performance C11 runtime engine.

---

## Architectural Documentation Index

1. [System Architecture](system-architecture.md)  
   Comprehensive specification of the four system layers (Application Layer, Public API Layer, Adaptive Selection Core, and Multi-Transport Plane). Details all C11 struct definitions, alignment primitives (`alignas(64)`), and component interactions.

2. [Data Flow](data-flow.md)  
   Step-by-step trace of data packets traveling from the sender's memory buffer, through telemetry recording, dynamic transport selection, binary framing (`aipc_frame_hdr_t`), and memory bus / kernel transfer, to the receiver's memory buffer.

3. [IPC Selection Logic](ipc-selection-logic.md)  
   Mathematical breakdown of the Exponentially Weighted Moving Average (EWMA) telemetry tracker, the multi-attribute heuristic cost function, crossover thresholds, and the 15% hysteresis dampening filter that prevents transport flapping.

4. [Error Handling & Resource Cleanup](error-handling-and-resource-cleanup.md)  
   In-depth explanation of how A-IPC guarantees crash safety, prevents zombie POSIX shared memory segments in `/dev/shm`, cleans up temporary FIFOs and sockets in `/tmp`, and ensures graceful recovery if an underlying transport reaches capacity.
