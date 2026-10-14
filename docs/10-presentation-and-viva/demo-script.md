# Live Project Demonstration Script
## Step-by-Step Terminal Execution & Fallback Protocols for Examination

---

## 1. Pre-Flight Preparation (Before the Review Begins)

Ensure the Linux terminal or WSL2 environment is open at the root of the repository:
```bash
cd /path/to/Adaptive-Inter-Process-in-linux
pwd
# Confirm git branch is main and clean
git status
```

---

## 2. Live Terminal Demonstration Sequence

### Step 1: Clean Compilation & Zero-Warning Audit (30 Seconds)
**Examiner Message:** *"Show that the codebase builds cleanly without suppressing compiler warnings."*
**Terminal Command:**
```bash
make clean && make all
```
**Expected Output Pattern:**
- Compiles `aipc_bench.o`, `aipc_baselines.o`, `aipc_fas_ring.o`, `aipc_selector.o`, `aipc_channel.o`, and all test binaries with `-Wall -Wextra -Werror -O3`. Zero warnings, zero errors.

### Step 2: Structural Cache Alignment Validation (30 Seconds)
**Examiner Message:** *"Prove that your memory structures eliminate false sharing."*
**Terminal Command:**
```bash
./test_fas_ring --test-alignment
```
**Expected Output Pattern:**
```
[CHECK] Verifying Cache-Line Alignments...
[CHECK] offsetof(tail) = 0 (aligned to 64)
[CHECK] offsetof(head) = 64 (aligned to 64)
[CHECK] offsetof(futex) = 128 (aligned to 64)
[CHECK] offsetof(slots) = 192 (aligned to 64)
[PASS] All structural alignment assertions verified.
```

### Step 3: Fast-Path Sub-200 ns Demonstration (45 Seconds)
**Examiner Message:** *"Demonstrate that FAS-IPC achieves sub-400 ns latency on 64B messages."*
**Terminal Command:**
```bash
./test_fas_ring --iterations 100000 --payload 64
```
**Expected Output Pattern:**
- Output displays `Mean Latency: ~196 - 210 ns`, `P50: ~190 ns`, `P99: ~320 ns`, `Throughput: > 2.3 M msg/s`, and `Voluntary CSW: < 50`.
- Highlight to examiners: *"Notice that across 100,000 exchanges, context switches were near zero, proving that AHSP micro-spinning absorbed transfers in userspace."*

### Step 4: Rapid Dynamic Transport Transition Test (45 Seconds)
**Examiner Message:** *"Prove that dynamically switching transports does not drop or reorder messages."*
**Terminal Command:**
```bash
./test_unified --test-mixed-sizes --count 50000
```
**Expected Output Pattern:**
- Output displays `Exchanging 50,000 interleaved messages (64B -> 4KB -> 64KB -> 256B)`.
- Ends with: `[PASS] 50000/50000 messages verified in sequence with zero corruption.`

### Step 5: Automated Master Verification Suite (30 Seconds)
**Examiner Message:** *"Run your formal verification suite covering all 4 phases."*
**Terminal Command:**
```bash
python3 scripts/verify_all_4_phases.py
```
**Expected Output Pattern:**
- Audits Phase 1, Phase 2, Phase 3, Phase 4, and Interphase Synergy.
- Prints `ALL 4 PHASES VERIFIED: 100% COMPLIANT, ALIGNED, AND READY FOR PHASE 5`.

---

## 3. Fallback Protocols (For Non-Linux Host or Live System Glitch)

If presenting on a Windows host without native Linux ELF execution, or if live system benchmarks experience unexpected thermal jitter:

### Fallback Protocol A: Python Logic & Telemetry Audit
Execute the pure Python telemetry and crossover audit:
```bash
python scripts/test_phase3_logic.py
```
- Proves that the EWMA tracker converges within 30 samples and verifies dynamic crossover classification.

### Fallback Protocol B: Empirical Data Inspection
Open and display the committed CSV datasets and Markdown defense tables:
- Display [`results/baseline_1m_64b.csv`](file:///results/baseline_1m_64b.csv) for classical baselines.
- Display [`results/fas_ipc_results.csv`](file:///results/fas_ipc_results.csv) for sub-200 ns FAS-IPC.
- Display [`results/mixed_workload_gain.csv`](file:///results/mixed_workload_gain.csv) for the 1.88x gain.
- Open [`results/summary_table.md`](file:///results/summary_table.md) to walk through the compiled defense table.

### Fallback Protocol C: High-Resolution Visualizations
Open and display the 300 DPI figures in `results/plots/`:
- [`fig1_danyliuk_comparison.png`](file:///results/plots/fig1_danyliuk_comparison.png)
- [`fig2_crossover_map.png`](file:///results/plots/fig2_crossover_map.png)
- [`fig3_tail_latency_cdf.png`](file:///results/plots/fig3_tail_latency_cdf.png)
- [`fig4_adaptation_gain.png`](file:///results/plots/fig4_adaptation_gain.png)
