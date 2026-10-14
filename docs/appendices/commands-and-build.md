# Operational Commands & Build Cheatsheet
## Build Targets, Execution Options, and Profiling Workflows

---

## 1. Compilation Targets (Makefile)

The unified POSIX build system is defined in [`Makefile`](file:///Makefile). Targets are compiled using:
```bash
CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -O3 -D_GNU_SOURCE -Iinclude
LDFLAGS ?= -pthread -lrt -lm
```

### Build Commands:
```bash
# Build all benchmark and test executables
make all

# Clean build artifacts and binaries
make clean

# Build individual targets:
make bench_baselines  # Phase 1: Classical OS primitives benchmark
make test_fas_ring    # Phase 2: FAS-IPC lock-free ring unit & stress test
make test_unified     # Phase 3: Unified channel & multiplexing test
make bench_matrix     # Phase 4: Full matrix & adaptation gain benchmark
```

---

## 2. Command-Line Options for Executables

### A. `bench_baselines` (Phase 1)
```bash
./bench_baselines [OPTIONS]
  --transport <pipe|socket|mq|naive_shm|all>  Select IPC transport (default: all)
  --payload <bytes>                          Payload size in bytes (default: 64)
  --iterations <count>                       Exchange count (default: 1000000)
  --sweep                                    Execute full payload sweep (64B to 64KB)
  --warmup <count>                           Warmup iterations discarded (default: 50000)
  --producer-cpu <id>                        Producer core affinity (default: 1)
  --consumer-cpu <id>                        Consumer core affinity (default: 2)
  --csv <filepath>                           Export output to CSV file
```

### B. `test_fas_ring` (Phase 2)
```bash
./test_fas_ring [OPTIONS]
  --test-alignment                           Execute structural cache-alignment assertions
  --payload <bytes>                          Fast-path payload size (<= 1024)
  --iterations <count>                       Exchange count (default: 1000000)
  --sweep                                    Sweep 64B to 1024B
  --stress                                   Run 5,000,000 iteration stress test
  --validate-data                            Validate 100% byte integrity
```

### C. `test_unified` (Phase 3)
```bash
./test_unified [OPTIONS]
  --test-basic                               Test basic loopback on unified API
  --test-telemetry                           Verify EWMA convergence and overhead
  --test-mixed-sizes                         Send rapid interleaved payload sequence
  --count <count>                            Total messages in sequence (default: 100000)
```

### D. `bench_matrix` (Phase 4)
```bash
./bench_matrix [OPTIONS]
  --sweep                                    Execute 11-payload full matrix sweep
  --test-mixed-workload                      Execute 80/20 mixed workload evaluation
  --iterations <count>                       Exchanges per test point (default: 100000)
  --output <csv_path>                        Path to export matrix CSV data
```

---

## 3. Profiling Workflows via `perf`

### Profile Raw System Calls:
```bash
perf stat -e raw_syscalls:sys_enter,raw_syscalls:sys_exit ./bench_matrix --payload 64 --iterations 100000
```

### Profile Cache Misses & CPU Cycles:
```bash
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,context-switches ./test_fas_ring --payload 64 --iterations 1000000
```
