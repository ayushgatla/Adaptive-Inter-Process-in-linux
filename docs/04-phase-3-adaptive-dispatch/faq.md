# Phase 3: Frequently Asked Questions (FAQ)

---

### Q1: Why is an Exponentially Weighted Moving Average (EWMA) used for arrival rate tracking?
**Answer:** Workload arrival rates can fluctuate erratically between microsecond bursts and idle intervals. A simple arithmetic mean across all history is overly sluggish and fails to respond to phase shifts, while instantaneous rate ($\frac{1}{\Delta t}$) is excessively noisy. EWMA ($\alpha = 0.125$) provides an optimal balance: it reacts quickly to sustained shifts while smoothing transient spikes, and its power-of-two coefficient ($\frac{1}{8}$) compiles to fast CPU bit shifts without expensive division.

### Q2: What is the exact purpose of the hysteresis filter?
**Answer:** If two transports have nearly identical performance scores around a crossover boundary, minute workload fluctuations can cause the selector to switch back and forth on every message (thrashing). Each switch incurs cache line churn and demuxing checks. A-IPC's hysteresis filter enforces two stability rules:
1. The target transport's score must be at least 15% better than the current transport.
2. The current transport must remain active for a minimum stability window of 16 consecutive samples.

### Q3: Why does Phase 3 define an upper crossover limit of 65,536 bytes (64 KB) for pipes?
**Answer:** In the Linux kernel, the default circular buffer capacity of an anonymous pipe is exactly 65,536 bytes (64 KB). Attempting to write more than 64 KB in a single transfer causes the sending process to block until the reader drains the pipe, or requires partial writes that increase context switching. UNIX domain stream sockets possess dynamic socket buffers that scale more effectively for bulk data beyond 64 KB.

### Q4: How does A-IPC ensure message boundaries on streaming transports like Pipes and Sockets?
**Answer:** Linux pipes and stream sockets are continuous byte streams without inherent message boundaries. If a sender writes 100 bytes and then 200 bytes, a reader may receive 300 bytes in a single `read()`. A-IPC prepends each message with a 24-byte packed binary framing header (`aipc_frame_hdr_t`), which embeds the `payload_len`, sequence index, validation magic word (`0xA1PC0001U`), and an XOR checksum. The receiver reads the fixed header first to know exactly how many payload bytes to dequeue.

### Q5: How is in-order delivery guaranteed when switching transports dynamically?
**Answer:** Every outgoing message is assigned a monotonically increasing 64-bit sequence counter (`tx_seq`). In the receiver's `ipc_receive()` loop, incoming messages are unpacked and their sequence numbers are validated against expected `rx_seq`. If a transition from Pipe to FAS-IPC occurs, the demuxer drains any remaining in-flight frames from the pipe descriptor before reading subsequent messages from the shared memory ring buffer.

### Q6: What is the computational overhead of the adaptive selector in `ipc_send()`?
**Answer:** Updating the telemetry observer and executing the crossover branch checks takes approximately **8 to 14 nanoseconds** on modern processors. Compared to the ~196 ns latency of FAS-IPC and ~1400 ns latency of Pipes/Sockets, a 10 ns selection cost represents less than 5% of fast-path transit time, making the runtime decision easily affordable.

### Q7: Can an application manually force A-IPC to use a specific transport?
**Answer:** Yes. The API provides `ipc_force_transport(chan, transport_type)` and `ipc_reset_forced_transport(chan)`. This allows developers or benchmark suites to disable the adaptive selector and pin the channel to an "Always-Pipe", "Always-Socket", or "Always-SHM" configuration for benchmarking comparisons.

### Q8: How are pipes and sockets initialized in `ipc_create()`?
**Answer:** When `ipc_create()` is invoked with `IPC_ROLE_PRODUCER`, it provisions the FAS-IPC shared memory segment via `shm_open()`, creates FIFO named pipes via `mkfifo()`, and binds a UNIX domain stream socket in `/tmp/aipc_sock_*`. When the consumer calls `ipc_create()` with `IPC_ROLE_CONSUMER`, it attaches to the existing shared memory mapping, opens the pipe descriptors, and connects to the UNIX domain socket.

### Q9: What happens if an application attempts to send a 2 MB message?
**Answer:** The selector immediately detects `len > 65536`. It directs the payload to the UNIX domain socket transport. The sender streams the 24-byte `aipc_frame_hdr_t` followed by the 2 MB payload through the socket. The receiver reads the frame header, allocates or prepares the receiving buffer, and streams the 2 MB payload into user memory without memory corruption.

### Q10: Examiner Challenge: "What happens if a burst of 64B messages arrives while a large 16KB message is still being read from the pipe?"
**Answer:** The demuxer prioritizes completing the active stream message currently framed on the descriptor. Once the stream frame is completely read and validated, the demuxer returns the bulk payload. On the subsequent `ipc_receive()` call, it checks the FAS-IPC ring, immediately dequeuing the arriving 64B fast-path messages with microsecond-level latency.
