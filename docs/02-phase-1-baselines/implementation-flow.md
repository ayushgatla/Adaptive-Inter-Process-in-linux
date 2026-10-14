# Phase 1: Deep Implementation Flow
## Technical Architecture, Concurrency Control, and System Call Analysis

---

## 1. System Call Architecture & Tracing

In Phase 1, each classical Linux IPC mechanism is implemented in its canonical POSIX form. The table below traces the exact system call sequence executed during an exchange:

| Mechanism | Sender System Calls | Receiver System Calls | Kernel Transitions / Round-Trip | Kernel Overhead Source |
| :--- | :--- | :--- | :---: | :--- |
| **Anonymous Pipe** | `write(fd, buf, len)` | `read(fd, buf, len)` | **4 transitions (2 syscalls/direction)** | `copy_from_user`, circular pipe buffer lock, wait queue wakeups, `copy_to_user`. |
| **UNIX Domain Socket** | `send(fd, buf, len, 0)` | `recv(fd, buf, len, 0)` | **4 transitions (2 syscalls/direction)** | `sk_buff` allocation, socket buffer lock, scheduler context switch, memory copies. |
| **POSIX Message Queue** | `mq_send(mqd, buf, len, prio)` | `mq_receive(mqd, buf, len, &prio)` | **4 transitions (2 syscalls/direction)** | Priority queue sorting (red-black tree), quota checking, signal/wait queue traversal. |
| **Naive SHM + Futex** | Memory store + `syscall(SYS_futex, ..., FUTEX_WAKE, 1)` | `syscall(SYS_futex, ..., FUTEX_WAIT, val, NULL)` | **Variable (2–4 transitions when contended)** | Futex hash bucket lock, kernel thread sleep/wakeup queue traversal. |

---

## 2. Memory Ordering & Concurrency Control

In `src/aipc_baselines.c`, synchronization for naive shared memory is implemented as follows:
```c
// Producer writes data then signals consumer
memcpy(shm->buffer, data, len);
atomic_store_explicit(&shm->ready_flag, 1, memory_order_release);
syscall(SYS_futex, (int *)&shm->ready_flag, FUTEX_WAKE, 1, NULL, NULL, 0);

// Consumer waits on futex if not yet set
while (atomic_load_explicit(&shm->ready_flag, memory_order_acquire) == 0) {
    syscall(SYS_futex, (int *)&shm->ready_flag, FUTEX_WAIT, 0, NULL, NULL, 0);
}
```

### Why Naive SHM Performs Poorly (~1.45 μs)
1. **False Sharing:** In `naive_shm_t`, `ready_flag` and `buffer` reside within adjacent memory addresses in the same 64-byte cache line. When the producer writes to `buffer`, the CPU invalidates the consumer's L1 cache line containing `ready_flag`, causing repeated cache misses.
2. **Immediate Futex Entry:** Without a micro-spin phase, if the consumer is descheduled or slightly delayed, it immediately makes a `SYS_futex` system call, forcing a voluntary context switch into the kernel. This defeats the primary benefit of shared memory.

---

## 3. Statistical Collection & Percentile Calculations

To prevent timing distortions during benchmarking, `src/aipc_bench.c` employs an offline percentile sorting engine:
1. Dynamic allocation of `capacity` samples of `double` (one-way nanoseconds).
2. During the 1,000,000-iteration loop, `aipc_time_ns()` is invoked immediately before sending and after receiving:
   $$\text{lat\_ns} = \frac{t_{\text{end}} - t_{\text{start}}}{2.0}$$
3. Samples are stored sequentially into `col->samples[i]` with minimal cache disturbance.
4. After measurement concludes, the array is sorted via standard C library `qsort()` using an `ascending_double` comparator.
5. Exact percentiles are computed:
   - $P_{50} = \text{samples}[\lfloor 0.50 \times N \rfloor]$
   - $P_{90} = \text{samples}[\lfloor 0.90 \times N \rfloor]$
   - $P_{99} = \text{samples}[\lfloor 0.99 \times N \rfloor]$
   - $P_{99.9} = \text{samples}[\lfloor 0.999 \times N \rfloor]$
