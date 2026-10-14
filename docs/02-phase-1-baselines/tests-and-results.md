# Phase 1: Tests and Results
## Empirical Measurements, Data Integrity, and Statistical Breakdown

---

## 1. Test Execution Register

| Test ID | Test Target | Command | Expected Result | Actual Measured Result | Evidence File | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :---: |
| **TEST-P1-01** | Anonymous Pipe Echo | `./bench_baselines --transport pipe --payload 64 --iterations 1000000` | Latency ~1.2–2.0 μs, zero corruption | Mean: 1404.53 ns, P50: 1369.50 ns, 0 corruptions | `results/baseline_1m_64b.csv` | **PASS** |
| **TEST-P1-02** | UNIX Socket Echo | `./bench_baselines --transport socket --payload 64 --iterations 1000000` | Latency ~1.2–2.0 μs, zero corruption | Mean: 1370.62 ns, P50: 1309.50 ns, 0 corruptions | `results/baseline_1m_64b.csv` | **PASS** |
| **TEST-P1-03** | POSIX MQ Echo | `./bench_baselines --transport mq --payload 64 --iterations 1000000` | Latency ~1.5–3.0 μs, zero corruption | Mean: 1530.48 ns, P50: 1490.50 ns, 0 corruptions | `results/baseline_1m_64b.csv` | **PASS** |
| **TEST-P1-04** | Naive SHM+Futex Echo | `./bench_baselines --transport naive_shm --payload 64 --iterations 1000000` | Latency ~1.0–2.0 μs with futex contention | Mean: 1451.05 ns, P50: 1432.00 ns, 0 corruptions | `results/baseline_1m_64b.csv` | **PASS** |
| **TEST-P1-05** | Syscall Profiling | `perf stat -e raw_syscalls:sys_enter ./bench_baselines ...` | ~2 syscalls per exchange operation | Exactly 2.00 syscalls/op recorded | `results/matrix_results.csv` | **PASS** |

---

## 2. Statistical Breakdown at 64 Bytes (1M Iterations)

The table below presents the exact empirical metrics extracted from `results/baseline_1m_64b.csv`:

| Transport | Min (ns) | P50 (ns) | P90 (ns) | P99 (ns) | P99.9 (ns) | Max (ns) | Mean (ns) | Throughput (msg/s) | Bandwidth (MB/s) | Vol CSW | Invol CSW |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Anonymous Pipe** | 906.50 | 1369.50 | 1434.00 | 1818.00 | 7672.00 | 426,957.00 | 1404.53 | 348,756.23 | 21.29 | 999,986 | 3 |
| **UNIX Socket** | 1060.50 | 1309.50 | 1689.50 | 1975.50 | 3760.00 | 558,146.50 | 1370.62 | 358,298.17 | 21.87 | 1,000,520 | 9 |
| **POSIX MQ** | 1164.50 | 1490.50 | 1573.50 | 1985.00 | 10,394.50 | 597,686.00 | 1530.48 | 321,361.37 | 19.61 | 999,988 | 4 |
| **Naive SHM+Futex** | 107.00 | 1432.00 | 1469.00 | 2039.00 | 11,651.00 | 778,558.00 | 1451.05 | 339,294.27 | 20.71 | 981,337 | 6 |

---

## 3. Data Integrity & Verification

Data integrity was validated on every exchange using `aipc_generate_payload()` and `aipc_validate_payload()` in `src/aipc_bench.c`. Across all 4,000,000 recorded iterations, zero data byte corruption or out-of-order sequence transitions were observed.
