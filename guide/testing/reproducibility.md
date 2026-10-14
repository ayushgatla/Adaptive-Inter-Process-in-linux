# Testing & Benchmarking: Master Reproducibility Guide

## 1. Reproducibility Philosophy

A core requirement of academic science is that any independent researcher must be able to download the code, execute the benchmarks, and obtain the same empirical conclusions.
This guide provides the complete, step-by-step instructions to compile the codebase, run all unit tests, execute benchmark sweeps, and regenerate all figures on a fresh Linux installation.

---

## 2. Prerequisites & System Setup

### Required Packages (Ubuntu / Debian / CentOS / RHEL)
On Ubuntu / Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential gcc make python3 python3-pip linux-tools-generic
pip3 install matplotlib numpy
```

On CentOS / RHEL / Fedora:
```bash
sudo dnf install -y gcc make python3 python3-pip perf
pip3 install matplotlib numpy
```

### Checking Kernel Memory Limits
Ensure that POSIX shared memory is mounted and has sufficient space:
```bash
df -h /dev/shm
```
(A-IPC requires less than 5 MB of shared memory).

---

## 3. Step-by-Step Build & Benchmark Execution

### Step 1: Clean and Build All Binaries
From the repository root directory:
```bash
make clean && make all
```
This compiles four standalone test binaries into the repository root:
- `bench_baselines`: Classical Linux IPC primitives benchmark (Phase 1).
- `test_fas_ring`: Standalone FAS-IPC shared memory ring buffer benchmark (Phase 2).
- `test_unified`: Unified channel and sequential demuxing correctness test (Phase 3).
- `bench_matrix`: 11-payload matrix sweep and mixed-workload evaluation (Phase 4).

---

### Step 2: Replicate Phase 1 Classical Baselines
Run 1,000,000 round-trip exchanges across Anonymous Pipes, UNIX Domain Sockets, POSIX Message Queues, and Naive Shared Memory at 64 bytes:
```bash
./bench_baselines --transport all --size 64 --iter 1000000 --out results/baseline_1m_64b.csv
```

---

### Step 3: Benchmark Phase 2 FAS-IPC Fast Path
Sweep FAS-IPC across small payload sizes (64 B, 128 B, 256 B, 512 B, 1,024 B):
```bash
./test_fas_ring --all --iter 1000000 --out results/fas_ipc_results.csv
```

---

### Step 4: Validate Phase 3 Unified Channel & Logic
Execute the functional order-validation and telemetry simulation:
```bash
./test_unified --msgs 500000 --validate-order
python3 scripts/test_phase3_logic.py
```

---

### Step 5: Execute Phase 4 Full Matrix & Mixed Workload Evaluation
Run the full 11-payload sweep across all transports:
```bash
./bench_matrix --mode all --out results/matrix_results.csv
```
Run the 80/20 mixed workload evaluation to measure Adaptation Gain:
```bash
./bench_matrix --mode mixed --out results/mixed_workload_gain.csv
```

---

### Step 6: Regenerate Publication Figures (Phase 5)
Generate all PNG and vector PDF charts in `results/plots/`:
```bash
python3 scripts/plot_results.py
```

---

### Step 7: Master Automated Verification (Phase 6)
To audit the entire multi-phase pipeline in a single command:
```bash
python3 scripts/verify_all_4_phases.py
```
Expected output:
```text
====================================================================================
     ALL 4 PHASES VERIFIED: 100% COMPLIANT, ALIGNED, AND READY FOR PHASE 5 & VIVA   
====================================================================================
```

---

## 4. Troubleshooting in Virtual Machines & Containers

### If Running in Docker or Kubernetes:
- Shared memory in Docker defaults to 64 MB. Run with `--shm-size=512m` or `--ipc=host`.
- Ensure CPU affinity permissions are granted: `--cap-add=SYS_NICE`.

### If Running in Windows Subsystem for Linux (WSL2):
- WSL2 supports POSIX shared memory and CPU core pinning.
- If CPU pinning fails due to hypervisor limitations, `aipc_pin_cpu()` gracefully falls back without crashing, though tail latency jitter may increase slightly.
