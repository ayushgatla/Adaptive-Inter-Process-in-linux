# Testing & Benchmarking: Test Environment Specification

## 1. Experimental Rigor in Systems Benchmarking

In systems programming, benchmark measurements are easily distorted by background noise: operating system thread migration, CPU power saving frequency drops (C-states), Turbo Boost jitter, and page faults.
To produce reliable, publishable results, all benchmarks in this project were conducted on a strictly controlled Linux test environment.

---

## 2. Hardware & Platform Specifications

| System Parameter | Benchmark Platform Specification | Academic Significance |
| :--- | :--- | :--- |
| **Processor Architecture**| x86_64 (Intel / AMD Multi-Core) | Hardware support for `alignas(64)` and `_mm_pause()` micro-spinning. |
| **Physical CPU Cores** | Multi-Core (at least 2 physical cores) | Ensures Producer and Consumer run on physically distinct cores. |
| **CPU Cache Hierarchy** | L1d Cache: 32–48 KB / core; L2: 512 KB–1 MB / core; L3: 16–32 MB Shared | FAS-IPC ring buffer (~1.1 MB) fits entirely into L3 cache. |
| **Cache Line Size** | Exactly 64 Bytes | Dictates the `alignas(64)` false-sharing padding in `fas_control_header_t`. |
| **Operating System** | Linux (Kernel version 6.x) | Modern kernel with red-black tree POSIX MQ and pipe optimizations. |
| **Compiler & Toolchain** | GCC (GNU Compiler Collection, C11 standard) | Invoked with `-O3` vectorization and strict error checks. |
| **POSIX Libraries** | `glibc` with `libpthread`, `librt`, `libm` | Required for `clock_gettime`, `shm_open`, `mq_open`, and futexes. |
| **Shared Memory Mount** | `/dev/shm` (Linux `tmpfs`) | High-speed RAM-backed virtual filesystem. |
| **Filesystem for Pipes** | `/tmp` | Used for FIFO nodes (`mkfifo`) and UNIX sockets (`AF_UNIX`). |

---

## 3. Compilation Configuration & Flags

All executables are compiled using the project's root [Makefile](file:///Makefile):

```makefile
CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -O3 -D_GNU_SOURCE -Iinclude
LDFLAGS ?= -pthread -lrt -lm
```

### Purpose of Compilation Flags:
1. `-Wall -Wextra -Werror`: Enforces absolute code quality. The codebase must compile with **zero warnings** treated as fatal errors.
2. `-O3`: Activates full compiler optimization, loop unrolling, register allocation, and inline function expansion.
3. `-D_GNU_SOURCE`: Exposes low-level Linux extensions (such as `sched_setaffinity`, `CPU_SET`, and `SYS_futex`).
4. `-Iinclude`: Includes the project's header directory.
5. `-pthread -lrt -lm`: Links the POSIX threading library, real-time clock extensions (`clock_gettime`), and mathematics library (`pow`, `log`).

---

## 4. CPU Core Pinning & Isolation Strategy

By default, the Linux Completely Fair Scheduler (CFS) dynamically moves threads across CPU cores to balance thermal load. When a process jumps to a new core, its L1 cache is cold, causing an immediate 500–1,000 ns latency spike.

In our benchmark harness ([src/aipc_bench.c](file:///src/aipc_bench.c#L18-L29)), every benchmark explicitly pins its threads:
- **Producer Process:** Strictly pinned to **CPU Core 1**.
- **Consumer Process:** Strictly pinned to **CPU Core 2**.

```c
int aipc_pin_cpu(int cpu_core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_core, &cpuset);
    return sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}
```

This guarantees:
1. Producer and Consumer never compete for execution time on the same logical CPU.
2. Inter-core communication tests the real hardware memory interconnect across physical core boundaries.
3. Cache lines remain hot throughout the 1,000,000 iteration test runs.
