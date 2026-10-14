#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "aipc_selector.h"
#include "aipc_bench.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

const char *aipc_transport_type_name(ipc_transport_type_t transport) {
    switch (transport) {
        case IPC_TRANSPORT_FAS_SHM:  return "FAS-IPC (Lock-Free SHM)";
        case IPC_TRANSPORT_PIPE:     return "Anonymous Pipe / FIFO";
        case IPC_TRANSPORT_SOCKET:   return "UNIX Domain Socket";
        case IPC_TRANSPORT_POSIX_MQ: return "POSIX Message Queue";
        default:                     return "Unknown Transport";
    }
}

void aipc_selector_init(aipc_selector_t *sel) {
    if (!sel) return;
    memset(sel, 0, sizeof(*sel));
    sel->current_transport = IPC_TRANSPORT_FAS_SHM;
    sel->stats.ewma_rate_msg_per_sec = 10000.0; // Reasonable initial prior
    sel->stats.last_timestamp_ns = 0;
    sel->consecutive_stable_count = 0;
    sel->total_switches = 0;
    sel->force_transport = false;
    sel->forced_transport = IPC_TRANSPORT_FAS_SHM;
}

void aipc_selector_record_event(aipc_selector_t *sel, size_t msg_len, size_t backlog) {
    if (!sel) return;

    uint64_t now = aipc_time_ns();
    sel->stats.msg_count++;
    sel->stats.byte_count += msg_len;
    sel->stats.last_msg_size = msg_len;
    sel->stats.current_backlog = backlog;

    if (sel->stats.last_timestamp_ns > 0) {
        uint64_t delta_ns = now - sel->stats.last_timestamp_ns;
        if (delta_ns > 0) {
            double inst_rate = 1000000000.0 / (double)delta_ns;
            // Cap extreme outliers due to timer resolution
            if (inst_rate > 50000000.0) inst_rate = 50000000.0;

            // EWMA formula: S_t = alpha * Y_t + (1 - alpha) * S_{t-1}
            sel->stats.ewma_rate_msg_per_sec = 
                (AIPC_EWMA_ALPHA * inst_rate) + 
                ((1.0 - AIPC_EWMA_ALPHA) * sel->stats.ewma_rate_msg_per_sec);

            // Compute bandwidth in MB/s
            double inst_bw_mbps = (inst_rate * (double)msg_len) / (1024.0 * 1024.0);
            sel->stats.ewma_bandwidth_mbps =
                (AIPC_EWMA_ALPHA * inst_bw_mbps) +
                ((1.0 - AIPC_EWMA_ALPHA) * sel->stats.ewma_bandwidth_mbps);
        }
    }
    sel->stats.last_timestamp_ns = now;
}

double aipc_selector_evaluate_cost(ipc_transport_type_t transport,
                                   size_t msg_len,
                                   double arrival_rate,
                                   size_t backlog) {
    (void)arrival_rate;

    switch (transport) {
        case IPC_TRANSPORT_FAS_SHM: {
            // Hard constraint: FAS-IPC slots are strictly bounded to 1024 bytes
            if (msg_len > AIPC_CROSSOVER_SMALL_MAX) {
                return 1e12; // Prohibitive penalty
            }
            // Ultra-low base latency (<200 ns) + low memory copy overhead, zero syscalls
            double base_latency = 180.0 + (0.12 * (double)msg_len);
            double backlog_penalty = (double)backlog * 4.0;
            return base_latency + backlog_penalty;
        }

        case IPC_TRANSPORT_PIPE: {
            // Pipe buffer is 64 KB; ideal for medium streaming payloads
            if (msg_len > AIPC_CROSSOVER_PIPE_MAX) {
                return 1e9;
            }
            // ~1400 ns baseline syscall + copy overhead
            double base_latency = 1350.0 + (0.04 * (double)msg_len);
            double backlog_penalty = (double)backlog * 1.5;
            return base_latency + backlog_penalty;
        }

        case IPC_TRANSPORT_SOCKET: {
            // Full duplex, high-bandwidth streaming fallback
            // ~1450 ns baseline syscall overhead, highly efficient for large chunks (>64 KB)
            double base_latency = 1450.0 + (0.025 * (double)msg_len);
            double backlog_penalty = (double)backlog * 1.0;
            return base_latency + backlog_penalty;
        }

        case IPC_TRANSPORT_POSIX_MQ: {
            // POSIX MQ has substantial kernel queue management overhead (~15 us)
            double base_latency = 12000.0 + (0.25 * (double)msg_len);
            return base_latency + ((double)backlog * 10.0);
        }

        default:
            return 1e12;
    }
}

ipc_transport_type_t aipc_selector_select(aipc_selector_t *sel, size_t msg_len) {
    if (!sel) return IPC_TRANSPORT_FAS_SHM;

    // Fast-path bypass if explicitly pinned
    if (__builtin_expect(sel->force_transport, 0)) {
        return sel->forced_transport;
    }

    // Direct crossover evaluation
    ipc_transport_type_t best_transport;
    if (msg_len <= AIPC_CROSSOVER_SMALL_MAX) {
        best_transport = IPC_TRANSPORT_FAS_SHM;
    } else if (msg_len <= AIPC_CROSSOVER_PIPE_MAX) {
        best_transport = IPC_TRANSPORT_PIPE;
    } else {
        best_transport = IPC_TRANSPORT_SOCKET;
    }

    // Check if immediate switch is required due to hard boundary
    if (best_transport != sel->current_transport) {
        bool must_switch_immediately = false;

        // FAS-IPC cannot physically transport messages > 1024 bytes
        if (sel->current_transport == IPC_TRANSPORT_FAS_SHM && msg_len > AIPC_CROSSOVER_SMALL_MAX) {
            must_switch_immediately = true;
        }
        // Pipe cannot comfortably stream single messages > 64 KB
        else if (sel->current_transport == IPC_TRANSPORT_PIPE && msg_len > AIPC_CROSSOVER_PIPE_MAX) {
            must_switch_immediately = true;
        }
        // If message is small, returning to FAS-IPC offers massive (>5x) speedup
        else if (best_transport == IPC_TRANSPORT_FAS_SHM && msg_len <= AIPC_CROSSOVER_SMALL_MAX) {
            must_switch_immediately = true;
        }

        if (must_switch_immediately) {
            sel->current_transport = best_transport;
            sel->consecutive_stable_count = 0;
            sel->total_switches++;
            return sel->current_transport;
        }

        // Otherwise apply hysteresis dampening across Pipe and Socket
        double current_cost = aipc_selector_evaluate_cost(sel->current_transport,
                                                          msg_len,
                                                          sel->stats.ewma_rate_msg_per_sec,
                                                          sel->stats.current_backlog);
        double candidate_cost = aipc_selector_evaluate_cost(best_transport,
                                                            msg_len,
                                                            sel->stats.ewma_rate_msg_per_sec,
                                                            sel->stats.current_backlog);

        // Require at least AIPC_HYSTERESIS_MARGIN (15%) improvement and minimum window
        if (candidate_cost < current_cost * (1.0 - AIPC_HYSTERESIS_MARGIN) &&
            sel->consecutive_stable_count >= AIPC_HYSTERESIS_MIN_WINDOW) {
            sel->current_transport = best_transport;
            sel->consecutive_stable_count = 0;
            sel->total_switches++;
        } else {
            sel->consecutive_stable_count++;
        }
    } else {
        sel->consecutive_stable_count++;
    }

    return sel->current_transport;
}

void aipc_selector_set_forced_transport(aipc_selector_t *sel, ipc_transport_type_t transport) {
    if (!sel) return;
    sel->force_transport = true;
    sel->forced_transport = transport;
    sel->current_transport = transport;
}

void aipc_selector_clear_forced_transport(aipc_selector_t *sel) {
    if (!sel) return;
    sel->force_transport = false;
}
