# Systems Troubleshooting & Environment Diagnostic Guide

---

## 1. POSIX Message Queue Limit Errors (`EMFILE` / `EINVAL`)

### Symptom:
`mq_open()` fails with `EMFILE` ("Too many open files") or `EINVAL` ("Invalid argument").
### Root Cause:
Linux enforces default system-wide limits in `/proc/sys/fs/mqueue/`:
- `msg_max`: Defaults to 10 messages per queue.
- `msgsize_max`: Defaults to 8192 bytes.
If the benchmark requests `mq_maxmsg = 100` or payloads $> 8192$ B without root adjustment, the kernel rejects the call.
### Solution:
Elevate limits using `sysctl` or `sudo`:
```bash
sudo sysctl -w fs.mqueue.msg_max=1024
sudo sysctl -w fs.mqueue.msgsize_max=65536
```

---

## 2. Stale POSIX Shared Memory Segments (`EEXIST` / Bus Errors)

### Symptom:
`shm_open()` returns `EEXIST` when creating a new channel, or crashes with `SIGBUS` during `mmap()`.
### Root Cause:
If a previous test run was abruptly killed via `Ctrl+C` before `ipc_destroy()` or `shm_unlink()` was called, the shared memory file remains resident in the Linux `/dev/shm/` tmpfs filesystem.
### Solution:
Unlink stale POSIX shared memory segments:
```bash
rm -f /dev/shm/aipc_*
```
In `src/aipc_channel.c`, `ipc_create()` explicitly unlinks stale segments before creating new ones if `is_creator` is true.

---

## 3. High Tail Latency Jitter (CPU Governor Throttling)

### Symptom:
P99.9 latency spikes beyond 50 μs during baseline or FAS-IPC tests.
### Root Cause:
The Linux CPU frequency governor (e.g., `powersave` or `ondemand`) dynamically downclocks idle cores to save energy. When a message arrives, the processor takes microseconds to ramp up frequency.
### Solution:
Lock the CPU governor to high-performance mode:
```bash
sudo cpupower frequency-set -g performance
```
Verify CPU clock stability via:
```bash
watch -n 1 "grep 'cpu MHz' /proc/cpuinfo"
```

---

## 4. Permission Denied on FIFO Pipes or UNIX Sockets

### Symptom:
`mkfifo()` or `bind()` fails with `EACCES` ("Permission denied").
### Root Cause:
Existing socket or FIFO endpoints in `/tmp/` owned by another user or stale process.
### Solution:
Clean up temporary files:
```bash
rm -f /tmp/aipc_*
```
