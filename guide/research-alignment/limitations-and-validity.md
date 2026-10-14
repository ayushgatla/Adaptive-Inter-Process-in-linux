# Research Alignment: Limitations & Threats to Validity

## 1. Introduction: The Academic Value of Frankness

In an honors university defense or scientific publication, professors respect an engineer who understands their system's physical boundaries far more than one who claims their system is universally perfect.
Every engineering choice involves a trade-off. This chapter outlines the architectural boundaries, hardware dependencies, and threats to validity of our research.

---

## 2. Fundamental System Limitations

### 1. Single-Producer Single-Consumer (SPSC) Boundary
- **Current Reality:** Our fast-path shared memory engine (FAS-IPC) is designed strictly for point-to-point communication between one Producer process and one Consumer process.
- **Why It Matters:** If multiple producers attempt to write to the FAS-IPC ring simultaneously without external locking, `tail` updates will collide, causing memory corruption.
- **Why We Chose This Trade-Off:** SPSC allows completely lock-free updates with simple atomic Acquire-Release operations. Supporting Multi-Producer (MPSC) requires atomic Compare-And-Swap (`atomic_compare_exchange_weak`) loops, which add bus contention and would increase 64-byte latency from 196 ns to ~350 ns.

### 2. Single-Host Scope (No Distributed IPC)
- **Current Reality:** A-IPC relies on POSIX shared memory (`/dev/shm`), named FIFOs, and UNIX domain sockets (`AF_UNIX`). It operates purely between processes sharing the same physical RAM.
- **Why It Matters:** A-IPC does not replace TCP/IP, gRPC, or RDMA for communication between different physical servers across a network.

### 3. Static Ring Sizing & Fixed Memory Footprint
- **Current Reality:** The FAS-IPC ring pre-allocates 1,024 slots of 1,024 bytes each (~1.1 MB per unidirectional ring, ~2.2 MB for duplex).
- **Why It Matters:** While 2.2 MB easily fits into modern L3 CPU caches, payloads exceeding 1,024 bytes cannot physically fit into a FAS-IPC slot and must be routed to pipes or sockets.

### 4. Workload Composition Dependency
- **Current Reality:** Our 1.88x Adaptation Gain was measured under an 80/20 mixed workload (80% 64 B + 20% 16 KB).
- **The Caveat:**
  - If an application's workload consists **100% of large 64 KB transfers**, A-IPC will route all traffic to pipes/sockets. In that scenario, A-IPC will not be faster than raw pipes—in fact, it will be ~12.5 nanoseconds slower due to selector evaluation!
  - If a workload consists **100% of 64-byte messages**, static FAS-IPC achieves 196.17 ns while adaptive A-IPC achieves 208.67 ns.
  - **Core Takeaway:** Adaptive A-IPC is specifically designed for dynamic, fluctuating workloads where static primitives degrade.

---

## 3. Threats to Experimental Validity

### A. Threats to Internal Validity (Measurement Accuracy)
1. **CPU Frequency Scaling & Jitter:**  
   Modern CPUs dynamically scale clock speeds (Intel SpeedStep / AMD Turbo Core). If a core drops to a lower frequency during an idle period, measured latency spikes artificially.  
   *Mitigation in our testbed:* We ran 10,000 warmup iterations to lock the CPU cores into high-performance P-states before recording timed samples.
2. **Operating System Scheduling Preemption:**  
   Even with CPU core pinning (`sched_setaffinity`), the Linux kernel periodically schedules timer interrupts or kernel worker threads on Core 1 and Core 2.  
   *Mitigation in our testbed:* We collected 1,000,000 samples and computed exact percentiles (P50, P90, P99, P99.9), confirming that kernel preemption affects less than 0.1% of samples.

---

### B. Threats to External Validity (Generalizability)
1. **Instruction Set Architecture (ISA) Dependency:**  
   The AHSP micro-spin optimization utilizes the x86 `_mm_pause()` intrinsic.  
   *Mitigation:* In [src/aipc_channel.c](file:///src/aipc_channel.c#L23-L30) and [src/aipc_fas_ring.c](file:///src/aipc_fas_ring.c#L18-L25), we implemented architecture-independent fallbacks:
   ```c
   #if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
   #include <immintrin.h>
   #define AIPC_CPU_PAUSE() _mm_pause()
   #elif defined(__aarch64__) || defined(__arm__)
   #define AIPC_CPU_PAUSE() __asm__ __volatile__("yield" ::: "memory")
   #else
   #define AIPC_CPU_PAUSE() __asm__ __volatile__("" ::: "memory")
   #endif
   ```
2. **Kernel Version & Distro Differences:**  
   Our tests ran on Linux 6.x. On older kernels (e.g., Linux 3.x or 4.x), POSIX Message Queue latencies are significantly worse (~14.5 μs in Danyliuk), making the relative advantage of our shared-memory fast path even more pronounced!

---

## 4. Defending Limitations in an Academic Viva

When an examiner challenges: *"Your system only works on a single machine and only supports one producer. Isn't that too limited?"*

> **Model Defense:**  
> *"That limitation is a deliberate architectural scope decision. In high-performance computing, financial trading engines, and robotics, single-host inter-process communication is the primary latency bottleneck.  
> By restricting our fast path to Single-Producer Single-Consumer (SPSC) and single-host RAM, we were able to eliminate CPU bus locks and kernel system calls completely, achieving sub-200 nanosecond latency.  
> Expanding to Multi-Producer (MPSC) or distributed network communication requires lock contention and network protocol headers that would destroy microsecond-level performance. We solved the hardest problem first—achieving physical cache-speed adaptation on a single host—and designed our architecture so MPSC can be layered on in future extensions."*
