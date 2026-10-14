# Phase 5: Frequently Asked Questions (FAQ)

---

### Q1: Why did you generate both PNG and vector PDF charts?
**Answer:** PNG files (rendered at 300 DPI) are optimized for web presentation, digital slide projections, and document embedding without high file sizes. Vector PDF files retain mathematical vector coordinates for text, axes, and lines, ensuring lossless rendering at arbitrary zoom levels for publication in academic conference papers and printed project reports.

### Q2: What exact data sources are used by `plot_results.py`?
**Answer:** The script reads exclusively from:
1. `results/matrix_results.csv`: Contains the 11-payload sweep across all 5 IPC configurations.
2. `results/mixed_workload_gain.csv`: Contains the 80/20 mixed workload evaluation data.
No hardcoded numerical arrays or synthetic data generators exist inside the plotting logic.

### Q3: What does the Cumulative Distribution Function (Figure 3) prove?
**Answer:** Average latency can be deceptive because a few severe tail outliers can ruin application determinism while leaving the arithmetic mean relatively low. Figure 3 plots latency across percentiles (P50 to P99.9). It visually proves that FAS-IPC remains tightly clustered under 330 ns at P99 and 571 ns at P99.9, whereas classical Pipes and Sockets experience tail spikes exceeding 15,000 ns due to kernel scheduler latency and context-switch jitter.

### Q4: How is the 2D Crossover Contour Map (Figure 2) interpreted?
**Answer:** Figure 2 plots payload size on the horizontal axis (64 B to 1 MB, log scale) and message rate on the vertical axis (100 to 1,000,000 msg/s, log scale). The color contours represent the optimal transport:
- **Emerald Green Zone ($\le 1024$ B):** FAS-IPC is optimal across all arrival rates.
- **Deep Blue Zone ($1024\text{ B} - 64\text{ KB}$):** Anonymous Pipe streaming is optimal.
- **Rust Orange Zone ($> 64\text{ KB}$):** UNIX Domain Sockets are optimal for bulk streaming.

### Q5: Why is a logarithmic scale used on the Y-axis of Figure 1?
**Answer:** Latencies range from 196 ns (FAS-IPC) to over 15,000 ns (POSIX Message Queues in literature). A linear axis would compress the sub-microsecond differences between 196 ns, 850 ns, and 1400 ns into an unreadable sliver at the bottom of the graph. A logarithmic scale clearly resolves differences across multiple orders of magnitude.

### Q6: What does the right panel of Figure 4 illustrate?
**Answer:** The right panel shows the **percentage latency reduction** relative to the Static Anonymous Pipe baseline (set to 0.00%):
- Static Socket achieves a 15.53% reduction.
- Static POSIX MQ experiences a 28.58% regression (-28.58%).
- Adaptive A-IPC delivers a **46.86% latency reduction**, cutting mean latency from ~1755 ns down to 932 ns.

### Q7: Can `plot_results.py` run on a machine without a graphical display (headless server)?
**Answer:** Yes. The script explicitly invokes `matplotlib.use("Agg")` before importing `pyplot`. This configures Matplotlib to use the headless Anti-Grain Geometry (Agg) rendering engine, allowing clean generation of PNGs and PDFs inside continuous integration (CI) servers, SSH terminal sessions, or Docker containers without an active X11/Wayland display server.

### Q8: How were the colors chosen for the figures?
**Answer:** The colors were selected using ColorBrewer qualitative palette guidelines to ensure strong visual contrast, accessibility for color-blind viewers, and legibility when printed on black-and-white laser printers.

### Q9: Why is there an "N/A" for Static FAS-IPC at payloads $> 1024$ B in the summary table?
**Answer:** The FAS-IPC ring buffer has a fixed physical slot size of 1024 bytes. Payloads exceeding 1024 B cannot physically fit into a slot. Rather than corrupting memory or splitting packets without framing, the low-level API returns an error. This highlights the exact reason why the Phase 3 unified channel and adaptive selector were engineered.

### Q10: Examiner Challenge: "If your Adaptive A-IPC latency at 64B is 208 ns and FAS-IPC is 196 ns, why not just use FAS-IPC exclusively?"
**Answer:** If an application solely and exclusively transmits 64-byte messages and never transfers larger data, Static FAS-IPC is indeed 12 ns faster. However, real-world systems are heterogeneous: they must handle occasional telemetry dumps, image frames, or configuration payloads. If a system bound exclusively to FAS-IPC attempts to send a 2 KB message, it crashes or drops the packet. Adaptive A-IPC sacrifices an imperceptible 12 ns of overhead to provide universal payload handling, yielding a **1.88x throughput advantage** when payloads vary.
