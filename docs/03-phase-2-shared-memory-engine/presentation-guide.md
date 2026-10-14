# Phase 2: Presentation & Viva Guide
## University Defense Walkthrough for the FAS-IPC Engine

---

## 1. Opening Statement (15–30 Seconds)
> "In Phase 2, we solved the latency bottleneck of classical IPC by engineering FAS-IPC—a lock-free, single-producer single-consumer shared-memory ring buffer. By strictly isolating mutable indices into 64-byte cache lines and introducing Adaptive Hybrid Spin-Park synchronization, we dropped 64-byte latency to 196.17 nanoseconds—beating Danyliuk's 850 nanosecond benchmark by 4.33x and eliminating kernel transitions entirely."

---

## 2. Diagram Walkthrough (What to Point At)
1. Point to **Cache Line Isolation Layout**:
   - Show how Cache Line 0 (offset 0) belongs strictly to the Producer (`tail`).
   - Show Cache Line 1 (offset 64) belongs strictly to the Consumer (`head`).
   - Explain: *"Under the MESI cache protocol, separating these variables prevents the cores from invalidating each other's L1 cache lines."*
2. Point to **AHSP Flowchart**:
   - Point to the 200 `_mm_pause()` loop. Explain that it takes ~250–350 ns, matching the core-to-core interconnect transit time.
   - Point to the futex fallback: *"If no data arrives, the process parks passively via private futex, preventing wasted CPU energy."*

---

## 3. Code Demonstration (Which Files to Show)
- Open [`include/aipc_fas_ring.h`](file:///include/aipc_fas_ring.h) and highlight `alignas(FAS_CACHE_LINE_SIZE)` and `FAS_SLOT_MAX_SIZE 1024`.
- Open [`src/aipc_fas_ring.c`](file:///src/aipc_fas_ring.c) and trace `fas_ring_recv()`: show the `_mm_pause()` spin loop transitioning to `SYS_futex` with `waiting_readers`.

---

## 4. Test Demonstration (Safe Terminal Command)
```bash
# Run structural alignment and 100k iteration latency test
./test_fas_ring --test-alignment
./test_fas_ring --iterations 100000 --payload 64
```
- **Fallback:** Display `results/fas_ipc_results.csv` showing 196.17 ns mean latency and 125 context switches per 1,000,000 operations.

---

## 5. Research Comparison Key Talking Point
> "Danyliuk reported that shared memory with futex achieved 850 ns. By eliminating false sharing and introducing AHSP micro-spinning, FAS-IPC achieves 196 ns—a 4.33x speedup—while reducing context switches by 99.98%."

---

## 6. Time Budget Breakdown
- **Short Summary (1 minute):** Architectural purpose, cache-line layout, AHSP concept, and 196 ns latency result.
- **Deep Walkthrough (3–5 minutes):** Walk through C11 atomic memory orders (release on write, acquire on read), bitmask indexing (`tail & 1023`), and explain why slots are padded to 1088 bytes.
