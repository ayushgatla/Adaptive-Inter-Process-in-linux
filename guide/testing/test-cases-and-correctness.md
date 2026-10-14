# Testing & Benchmarking: Test Cases & Functional Correctness

## 1. Overview of Correctness Verification

Performance benchmarking is meaningless if data is corrupted in transit.
To ensure that our high-speed optimizations never sacrifice reliability, the A-IPC test suite subjects the framework to six comprehensive functional tests:
1. Zero-Corruption Payload Validation
2. Multi-Transport Sequential Ordering
3. Buffer Truncation & Stream Draining
4. Ring Full Backpressure
5. Idle Channel Futex Wakeup
6. Clean Teardown & Resource Leaks

---

## 2. Test Case Suite

### Test Case 1: Zero-Corruption Payload Validation
- **Objective:** Verify that every single byte sent by the Producer arrives byte-for-byte identical at the Consumer without memory corruption.
- **Implementation:** [src/aipc_bench.c](file:///src/aipc_bench.c#L88-L96):
  - `aipc_generate_payload(buf, len, seq)`: Fills buffer with a deterministic mathematical pattern based on message sequence:
    $$\text{buf}[i] = ((\text{seq} \times 17) + i) \pmod{256}$$
  - `aipc_validate_payload(buf, len, seq)`: Inspects incoming bytes against the formula.
- **Pass Criteria:** 1,000,000 consecutive messages validated with **0 byte mismatches**.
- **Result:** **PASSED.**

---

### Test Case 2: Multi-Transport Sequential Ordering
- **Objective:** Ensure that when messages rapidly transition between Shared Memory, Pipes, and Sockets, they are received in exact chronological order without packet reordering.
- **Implementation:** [bench/test_unified.c](file:///bench/test_unified.c):
  - Transmits 500,000 mixed messages where payload sizes alternate between 64 B (FAS-IPC), 4 KB (Pipe), and 16 KB (Pipe).
  - Consumer asserts that `hdr.seq_num == expected_seq++`.
- **Pass Criteria:** Zero sequence jumps, zero inversions, zero missing packets.
- **Result:** **PASSED.** (Verified over 500,000 dynamic transitions).

---

### Test Case 3: Buffer Truncation & Stream Draining
- **Objective:** Test behavior when the receiver calls `ipc_receive()` with a buffer smaller than the incoming message payload (`max_len < len`).
- **Implementation:**
  - Sender transmits a 16 KB message over a Pipe.
  - Receiver supplies an 8 KB buffer (`max_len = 8192`).
  - Verifies that `ipc_receive()` returns 8,192 bytes and automatically calls `drain_bytes()` in [src/aipc_channel.c](file:///src/aipc_channel.c#L66-L73) to purge the remaining 8,192 bytes from the pipe.
- **Pass Criteria:** Next message header retains perfect 24-byte alignment with magic `0xA1PC0001`.
- **Result:** **PASSED.**

---

### Test Case 4: Ring Full Backpressure Handling
- **Objective:** Verify that when the 1,024-slot FAS-IPC shared memory ring fills to capacity, the Producer micro-spins and waits rather than overwriting unread slots.
- **Implementation:**
  - Producer writes 1,024 messages into the ring while Consumer is intentionally delayed.
  - Producer attempts to send message 1,025.
  - Verifies that the Producer blocks until the Consumer drains at least one slot.
- **Pass Criteria:** Zero buffer overruns, zero data loss.
- **Result:** **PASSED.**

---

### Test Case 5: Idle Channel Futex Wakeup
- **Objective:** Verify that when traffic ceases and the Consumer parks in a `SYS_futex` system call, a subsequent message wakes the Consumer promptly.
- **Implementation:**
  - Producer sends 10 messages, then sleeps for 500 milliseconds (causing Consumer to spin 200 cycles and park in futex).
  - Producer sends message 11.
  - Verifies that `fas_futex_wake()` wakes the Consumer and message 11 is received within < 50 μs.
- **Pass Criteria:** Consumer wakes reliably without hanging or timing out.
- **Result:** **PASSED.**

---

### Test Case 6: Clean Teardown & OS Resource Leak Audit
- **Objective:** Verify that shutting down the system leaves no lingering files or memory segments in Linux.
- **Implementation:**
  - Inspect `/dev/shm` and `/tmp` before running tests.
  - Run `bench_matrix` and `test_unified`.
  - Call `ipc_destroy()`.
  - Assert that `/dev/shm/aipc_*` and `/tmp/aipc_*` are deleted.
- **Pass Criteria:** Zero residual files in `/dev/shm` or `/tmp`.
- **Result:** **PASSED.**
