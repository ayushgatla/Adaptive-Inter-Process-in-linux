#!/usr/bin/env bash
# scripts/run_experiments.sh - Comprehensive Phase 4 Experimental Matrix & Adaptation Gain Runner
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

OUTPUT_DIR="results"
ITERATIONS=50000
WARMUP=5000
PROD_CORE=1
CONS_CORE=2

while [[ $# -gt 0 ]]; do
    case $1 in
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --iterations)
            ITERATIONS="$2"
            shift 2
            ;;
        --warmup)
            WARMUP="$2"
            shift 2
            ;;
        --producer-core)
            PROD_CORE="$2"
            shift 2
            ;;
        --consumer-core)
            CONS_CORE="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

mkdir -p "${OUTPUT_DIR}"

echo "================================================================================"
echo "      A-IPC TIER-1: PHASE 4 FULL EXPERIMENTAL MATRIX & ADAPTATION GAIN          "
echo "================================================================================"
echo "Date:             $(date)"
echo "Host:             $(uname -s -r -m 2>/dev/null || echo 'Non-POSIX Host')"
echo "Output Directory: ${OUTPUT_DIR}"
echo "Iterations:       ${ITERATIONS} per sweep cell"
echo "Warmup:           ${WARMUP} per sweep cell"
echo "CPU Pinning:      Producer=Core ${PROD_CORE} | Consumer=Core ${CONS_CORE}"
echo "================================================================================"

# 1. Testbed Environment Pre-flight
if [ -f "scripts/setup_env.sh" ]; then
    echo "[1/4] Running pre-flight testbed configuration..."
    bash scripts/setup_env.sh || true
fi

# 2. Build Bench Matrix Binary
echo ""
echo "[2/4] Compiling bench_matrix executable..."
make bench_matrix

# 3. Execute Full Experimental Matrix & Mixed Workloads
MATRIX_CSV="${OUTPUT_DIR}/matrix_results.csv"
GAIN_CSV="${OUTPUT_DIR}/mixed_workload_gain.csv"

echo ""
echo "[3/4] Executing benchmark matrix and mixed workload sweeps..."
./bench_matrix \
    --iterations "${ITERATIONS}" \
    --warmup "${WARMUP}" \
    --producer-core "${PROD_CORE}" \
    --consumer-core "${CONS_CORE}" \
    --sweep \
    --mixed \
    --csv-matrix "${MATRIX_CSV}" \
    --csv-gain "${GAIN_CSV}"

# 4. Optional: Syscall Auditing via perf if running with root/perf permissions on Linux
if command -v perf &> /dev/null; then
    echo ""
    echo "[4/4] Profiling system call count per operation using perf stat..."
    perf stat -e raw_syscalls:sys_enter ./bench_matrix --iterations 10000 --mixed 2>&1 | tee "${OUTPUT_DIR}/perf_matrix_syscalls.txt" || true
else
    echo ""
    echo "[4/4] perf not available or non-root. Skipping kernel trace capture."
fi

echo ""
echo "================================================================================"
echo "                       PHASE 4 BENCHMARK RUN COMPLETE                           "
echo "================================================================================"
echo "Generated Datasets:"
echo "  1. Full Matrix Results:   ${MATRIX_CSV}"
echo "  2. Adaptation Gain:       ${GAIN_CSV}"
echo "================================================================================"
