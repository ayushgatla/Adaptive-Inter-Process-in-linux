#!/usr/bin/env bash
# scripts/setup_env.sh - Testbed Pre-flight Configuration for A-IPC

set -e

echo "=== Configuring Test Environment for A-IPC Benchmark ==="

# 1. Lock CPU Frequency Governor to Performance (if sudo without password works)
if command -v cpupower &> /dev/null; then
    sudo -n cpupower frequency-set -g performance 2>/dev/null || true
    echo "[OK] CPU governor checked (requires passwordless sudo for changes)"
else
    echo "[INFO] cpupower not found. Running with current system frequency governor."
fi

# 2. Adjust POSIX Message Queue Limits if privileged
if [ -f /proc/sys/fs/mqueue/msg_max ]; then
    sudo -n sysctl -w fs.mqueue.msg_max=1024 2>/dev/null || true
    sudo -n sysctl -w fs.mqueue.msgsize_max=8192 2>/dev/null || true
    echo "[OK] POSIX Message Queue limits checked"
fi

# 3. Clean up any stale POSIX shared memory segments and queues
rm -f /dev/shm/aipc_* 2>/dev/null || true
rm -f /dev/mqueue/aipc_* 2>/dev/null || true
echo "[OK] Cleaned stale /dev/shm and /dev/mqueue segments"

echo "=== System Ready for Phase 1 Baseline Execution ==="
