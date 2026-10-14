#ifndef AIPC_SELECTOR_H
#define AIPC_SELECTOR_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC Transport Type Identifiers
 */
typedef enum {
    IPC_TRANSPORT_FAS_SHM = 0, // High-speed lock-free shared memory (FAS-IPC)
    IPC_TRANSPORT_PIPE    = 1, // Linux Pipe (Anonymous / Named FIFO)
    IPC_TRANSPORT_SOCKET  = 2, // UNIX Domain Stream Socket (AF_UNIX)
    IPC_TRANSPORT_POSIX_MQ = 3, // POSIX Message Queue
    IPC_TRANSPORT_COUNT   = 4
} ipc_transport_type_t;

/**
 * @brief Dynamic Crossover Thresholds and Policy Constants
 */
#define AIPC_EWMA_ALPHA              0.125   // Smoothing factor (1/8) for arrival rate
#define AIPC_CROSSOVER_SMALL_MAX     1024    // Upper limit for FAS-IPC fast path (1 KB)
#define AIPC_CROSSOVER_PIPE_MAX      65536   // Upper limit for Pipe streaming (64 KB)
#define AIPC_HYSTERESIS_MARGIN       0.15    // 15% superiority required to switch transports
#define AIPC_HYSTERESIS_MIN_WINDOW   16      // Minimum sample window before switching

/**
 * @brief Runtime Workload Observer Statistics Container
 */
typedef struct {
    uint64_t msg_count;              // Total messages observed
    uint64_t byte_count;             // Total bytes transferred
    uint64_t last_timestamp_ns;      // Timestamp of last recorded event
    double   ewma_rate_msg_per_sec;  // Smoothed arrival rate (messages/sec)
    double   ewma_bandwidth_mbps;    // Smoothed throughput (MB/s)
    size_t   last_msg_size;          // Size of most recent message
    size_t   current_backlog;        // Channel backlog estimate
} aipc_workload_stats_t;

/**
 * @brief Adaptive Selector State Engine
 */
typedef struct {
    aipc_workload_stats_t stats;
    ipc_transport_type_t  current_transport;
    uint32_t              consecutive_stable_count; // Samples with current transport
    double                current_cost_score;
    uint64_t              total_switches;           // Total dynamic switches executed
    bool                  force_transport;          // If true, bypass dynamic selection
    ipc_transport_type_t  forced_transport;
} aipc_selector_t;

/**
 * @brief Initialize selector engine with default thresholds.
 */
void aipc_selector_init(aipc_selector_t *sel);

/**
 * @brief Update workload telemetry with an observed message event.
 *
 * Updates message count, byte count, and Exponentially Weighted Moving
 * Average (EWMA) arrival rate. Designed to incur < 15 ns overhead.
 *
 * @param sel Pointer to selector state
 * @param msg_len Message payload length in bytes
 * @param backlog Estimated channel backlog
 */
void aipc_selector_record_event(aipc_selector_t *sel, size_t msg_len, size_t backlog);

/**
 * @brief Multi-attribute cost function evaluator.
 *
 * Computes heuristic cost score:
 *   Score = w_lat * Latency(T, len) + w_cpu * Syscall_Cost(T) + Backlog_Penalty(T, backlog)
 * Lower score is better.
 *
 * @param transport Candidate transport mechanism
 * @param msg_len Message payload length in bytes
 * @param arrival_rate Current EWMA arrival rate (msg/sec)
 * @param backlog Current channel backlog
 * @return Computed heuristic cost score (arbitrary relative units)
 */
double aipc_selector_evaluate_cost(ipc_transport_type_t transport,
                                   size_t msg_len,
                                   double arrival_rate,
                                   size_t backlog);

/**
 * @brief Select optimal transport for the given payload size and workload state.
 *
 * Applies crossover rules and hysteresis dampening to prevent rapid flapping.
 *
 * @param sel Pointer to selector state
 * @param msg_len Size of message to transmit
 * @return Optimal transport type to route message
 */
ipc_transport_type_t aipc_selector_select(aipc_selector_t *sel, size_t msg_len);

/**
 * @brief Force selector to always use a specific transport (disables dynamic switching).
 */
void aipc_selector_set_forced_transport(aipc_selector_t *sel, ipc_transport_type_t transport);

/**
 * @brief Re-enable dynamic selection after being forced.
 */
void aipc_selector_clear_forced_transport(aipc_selector_t *sel);

/**
 * @brief Get human-readable name of transport.
 */
const char *aipc_transport_type_name(ipc_transport_type_t transport);

#ifdef __cplusplus
}
#endif

#endif // AIPC_SELECTOR_H
