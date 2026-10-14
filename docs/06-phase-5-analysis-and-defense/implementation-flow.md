# Phase 5: Deep Implementation Flow
## Matplotlib Pipeline, Vector Rendering, and Aesthetic Configuration

---

## 1. Automated Python Plotting Pipeline Architecture

The plotting pipeline is implemented in [`scripts/plot_results.py`](file:///scripts/plot_results.py). It operates entirely autonomously from the command line, accepting raw CSV paths and output directory arguments:
```bash
python3 scripts/plot_results.py --input results/matrix_results.csv --gain results/mixed_workload_gain.csv --outdir results/plots
```

### Aesthetic Configuration & Typography:
To produce publication-standard academic figures suitable for IEEE/ACM proceedings or university honors theses, `plot_results.py` enforces standardized typographical constraints:
```python
plt.rcParams.update({
    "font.family": "sans-serif",
    "font.sans-serif": ["DejaVu Sans", "Helvetica", "Arial"],
    "font.size": 11,
    "axes.titlesize": 13,
    "axes.labelsize": 12,
    "xtick.labelsize": 10,
    "ytick.labelsize": 10,
    "legend.fontsize": 10,
    "figure.titlesize": 14,
    "figure.dpi": 300,
    "savefig.dpi": 300,
    "savefig.bbox": "tight"
})
```

### Color Palette Architecture:
To ensure accessibility and contrast across both digital projectors and monochrome printouts, a curated color palette is enforced:
- **Anonymous Pipe:** Deep Blue (`#2b5c8f`)
- **UNIX Socket:** Rust Orange (`#d95f02`)
- **POSIX MQ:** Purple (`#7570b3`)
- **Naive SHM:** Brown (`#8c564b`)
- **FAS-IPC Fast Path:** Emerald Green (`#1b9e77`)
- **Adaptive A-IPC:** Vibrant Crimson/Magenta (`#e7298a`)
- **Danyliuk (2026) Literature:** Charcoal Gray (`#666666`)

---

## 2. Mathematical Construction of Plots

### Figure 1: Grouped Logarithmic Latency Bars
Plotted with message sizes 64 B to 1024 B on the X-axis and latency (nanoseconds, log scale) on the Y-axis. Danyliuk's literature numbers are placed side-by-side with our local measured Pipe, Socket, POSIX MQ, and FAS-IPC data.

### Figure 2: Empirical 2D Crossover Contour Map
Constructed over a 2D meshgrid spanning:
- Payload size $X \in [64, 10^6]$ bytes (logarithmic progression)
- Message arrival rate $Y \in [10^2, 10^6]$ msg/s (logarithmic progression)
Zoned regions:
1. $X \le 1024$ B $\to$ Zone 1 (FAS-IPC Shared Memory)
2. $1024\text{ B} < X \le 65536\text{ B}$ $\to$ Zone 2 (Anonymous Pipe Streaming)
3. $X > 65536\text{ B}$ $\to$ Zone 3 (UNIX Domain Socket)

### Figure 3: Tail Latency CDF (Cumulative Distribution Function)
Plotted by computing the empirical CDF:
$$F(x) = \frac{1}{N} \sum_{i=1}^N \mathbf{1}_{X_i \le x}$$
Evaluated across percentiles: Min, P50, P90, P99, P99.9, and Max. Plotted on a semi-log scale to highlight tail elongation.

### Figure 4: Adaptation Gain & Latency Reduction
A dual-panel visualization:
- Left Panel: Message throughput (msg/s) comparing Static Pipe, Static Socket, Static MQ, and Adaptive A-IPC, with a horizontal reference line indicating the 1.88x gain.
- Right Panel: Latency reduction percentage relative to Static Pipe (0.00%), showing Static Socket (+15.53%), Static MQ (-28.58%), and Adaptive A-IPC (**+46.86%**).
