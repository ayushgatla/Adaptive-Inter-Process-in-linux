# Reproduction Guide
## Step-by-Step Instructions to Rebuild, Verify, and Evaluate A-IPC

---

## 1. System Requirements & Prerequisites

To execute the native C11 benchmarks and reproduction scripts, the target environment must satisfy:
- **Operating System:** Linux x86-64 (Ubuntu 20.04+, Debian 11+, RHEL 8+, or WSL2).
- **Compiler:** GCC 9.0+ or Clang 11+ with C11 support (`-Wall -Wextra -Werror -O3 -pthread -lrt -lm`).
- **Python Runtime:** Python 3.8+ with `matplotlib`, `numpy`, and `pandas`.
- **System Utilities:** `cpupower` (recommended for CPU governor locking), `perf` (optional, for syscall profiling).

---

## 2. Step-by-Step Reproduction Instructions

### Step 1: Clone Repository & Tune System Environment
```bash
git clone https://github.com/ayushgatla/Adaptive-Inter-Process-in-linux.git
cd Adaptive-Inter-Process-in-linux

# Set executable permissions on scripts
chmod +x scripts/*.sh scripts/*.py

# Run pre-flight environment configuration (CPU governor & POSIX limits)
./scripts/setup_env.sh
```

### Step 2: Compile All Benchmark Executables
```bash
make clean
make all
```
*Expected Output:* Compiles `bench_baselines`, `test_fas_ring`, `test_unified`, and `bench_matrix` with zero warnings under `-Wall -Wextra -Werror`.

### Step 3: Run Phase 1 Classical Baselines
```bash
# Replicate 64B Danyliuk baseline (1,000,000 iterations)
./bench_baselines --payload 64 --iterations 1000000

# Run full baseline sweep across 64B to 64KB
./bench_baselines --sweep --iterations 100000
```

### Step 4: Run Phase 2 FAS-IPC Alignment & Stress Tests
```bash
# Verify structural 64-byte cache alignments
./test_fas_ring --test-alignment

# Run 1M iteration 64B fast-path latency test
./test_fas_ring --iterations 1000000 --payload 64

# Run 5M iteration concurrency stress test
./test_fas_ring --stress --iterations 5000000
```

### Step 5: Run Phase 3 Unified Channel & Demuxing Tests
```bash
# Run Python unit audit of EWMA logic and crossover boundaries
python3 scripts/test_phase3_logic.py

# Run C unified channel rapid interleaved size test
./test_unified --test-mixed-sizes --count 100000
```

### Step 6: Run Phase 4 Full Experimental Matrix
```bash
# Execute full matrix and dynamic mixed workload evaluation
./bench_matrix --sweep --iterations 100000
./bench_matrix --test-mixed-workload --iterations 500000
```

### Step 7: Run Phase 5 Automated Visualization Pipeline
```bash
python3 scripts/plot_results.py --input results/matrix_results.csv --gain results/mixed_workload_gain.csv --outdir results/plots
```
*Expected Output:* Generates all 4 PNG and vector PDF charts in `results/plots/` and updates `results/summary_table.md`.

### Step 8: Execute Master Cross-Phase Verification Suite
```bash
python3 scripts/verify_all_4_phases.py
```
*Expected Output:* Prints `ALL 4 PHASES VERIFIED: 100% COMPLIANT, ALIGNED, AND READY FOR PHASE 5`.
