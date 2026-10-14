# Academic References & Bibliography

This document lists the formal academic citations, research papers, textbooks, and technical specifications that provide the scientific foundation for the **Adaptive Inter-Process Communication (A-IPC)** framework.

---

## 1. Primary Comparative Literature

1. **Danyliuk, I. M. (2026)**  
   *Classical Interprocess Communication Mechanisms in Linux OS: Review and Latency Analysis for HPC Scenarios*.  
   **Journal:** SWorldJournal, Issue 37, Part 2, pp. 28–36.  
   **DOI:** [10.30888/2663-5712.2026-37-02-028](https://doi.org/10.30888/2663-5712.2026-37-02-028)  
   **Local Repository File:** [Resources/year_2026_benchmark.pdf](file:///Resources/year_2026_benchmark.pdf)  
   *Role in Project:* Primary comparative benchmark reference for 64-byte small-message latency across Anonymous Pipes, UNIX Domain Sockets, POSIX Message Queues, and Naive Shared Memory.

2. **Krishnaveni, S., & Ruby, D. (2016)**  
   *Comparing and Evaluating the Performance of Inter Process Communication Models in Linux Environment*.  
   **Journal:** International Journal of Trend in Research and Development (IJTRD), Special Issue NCSMAC-2016, ISSN: 2394-9333, pp. 51–55.  
   **Local Repository File:** [Resources/year_2016.pdf](file:///Resources/year_2016.pdf)  
   *Role in Project:* Reference for bulk payload scaling up to 99 MB, demonstrating that streaming sockets outperform pipes and queues on massive multi-megabyte transfers.

3. **Original Problem Statement & Solution Proposal (2024)**  
   *Adaptive Inter-Process Communication in Linux: High-Performance Runtime Selection Framework*.  
   **Local Repository File:** [Resources/initial_problem_and_sol.pdf](file:///Resources/initial_problem_and_sol.pdf)  
   *Role in Project:* Foundational problem formulation and research question establishing the need for runtime mechanism adaptation.

---

## 2. Operating Systems & Linux Systems Programming

4. **Kerrisk, M. (2010)**  
   *The Linux Programming Interface: A Linux and UNIX System Programming Handbook*.  
   No Starch Press, San Francisco, CA. ISBN: 978-1-59327-220-3.  
   *Chapters Referenced:* Chapters 43–44 (Pipes and FIFOs), 49–54 (Shared Memory and Memory Mappings), 52 (POSIX Message Queues), and 56–61 (UNIX Domain Sockets).

5. **Silberschatz, A., Galvin, P. B., & Gagne, G. (2018)**  
   *Operating System Concepts* (10th Edition).  
   John Wiley & Sons. ISBN: 978-1-119-32091-3.  
   *Topics Referenced:* Chapter 3 (Inter-Process Communication, Shared Memory vs Message Passing), Chapter 5 (CPU Scheduling and Context Switches).

6. **Love, R. (2013)**  
   *Linux System Programming: Talking Directly to the Kernel and C Library* (2nd Edition).  
   O'Reilly Media. ISBN: 978-1-449-33953-1.  
   *Topics Referenced:* Page caching, memory alignment, system call overhead, and IPC semantics.

---

## 3. Concurrency, Memory Models & Lock-Free Data Structures

7. **Hennessy, J. L., & Patterson, D. A. (2017)**  
   *Computer Architecture: A Quantitative Approach* (6th Edition).  
   Morgan Kaufmann. ISBN: 978-0-128-11905-1.  
   *Topics Referenced:* Multi-core cache coherence, the MESI protocol, and false sharing.

8. **Lamport, L. (1977)**  
   *Proving the Correctness of Multiprocess Programs*.  
   IEEE Transactions on Software Engineering, Vol. SE-3, No. 2, pp. 125–143.  
   *Role in Project:* Foundational mathematical proof of Single-Producer Single-Consumer (SPSC) lock-free ring buffer correctness without mutual exclusion locks.

9. **Thompson, M., Barker, D., & Stewart, P. (2011)**  
   *LMAX Disruptor: High Performance Alternative to Bounded Queues for Exchanging Data Between Concurrent Threads*.  
   LMAX Technical Whitepaper.  
   *Role in Project:* Design insights into mechanical sympathy, CPU cache-line padding, and false sharing elimination.

10. **ISO/IEC 9899:2011 (C11 Standard)**  
    *Information Technology — Programming Languages — C*.  
    International Organization for Standardization.  
    *Sections Referenced:* `<stdatomic.h>` (Acquire-Release memory order semantics) and `<stdalign.h>` (`alignas(64)` memory alignment).

11. **Franke, H., Russell, R., & Kirkwood, M. (2002)**  
    *Fuss, Futexes and Furwocks: Fast Userlevel Locking in Linux*.  
    Ottawa Linux Symposium (OLS 2002), pp. 479–495.  
    *Role in Project:* Reference for `SYS_futex` user-space hybrid synchronization.
