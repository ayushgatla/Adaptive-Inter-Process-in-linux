#ifndef AIPC_H
#define AIPC_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "aipc_selector.h"
#include "aipc_fas_ring.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Channel Endpoint Roles
 */
typedef enum {
    IPC_ROLE_PRODUCER = 0, // Initiating / Primary sender
    IPC_ROLE_CONSUMER = 1  // Responding / Primary receiver
} ipc_role_t;

/**
 * @brief Binary Message Framing Header for Streaming Transports (Pipe & Socket)
 *
 * Ensures discrete message boundaries and strict global ordering across
 * heterogeneous transport switching.
 */
#define AIPC_FRAME_MAGIC 0xA1PC0001U

typedef struct __attribute__((packed)) {
    uint32_t magic;         // Validation magic word (AIPC_FRAME_MAGIC)
    uint32_t payload_len;   // Payload length in bytes
    uint64_t seq_num;       // Sequential message index
    uint32_t transport_id;  // Source transport identifier
    uint32_t checksum;      // XOR integrity checksum
} aipc_frame_hdr_t;

/**
 * @brief Unified Adaptive IPC Channel Handle
 */
typedef struct ipc_channel {
    char                 channel_name[64];
    ipc_role_t           role;
    bool                 is_creator;
    ipc_transport_type_t current_transport;
    aipc_selector_t      selector;

    // Transport 1: Fast-Path Shared Memory (FAS-IPC)
    fas_channel_t        fas_chan;
    bool                 fas_enabled;

    // Transport 2: Medium-Payload Linux Pipe / FIFO
    int                  pipe_tx_fd;
    int                  pipe_rx_fd;
    char                 pipe_tx_path[128];
    char                 pipe_rx_path[128];
    bool                 pipe_enabled;

    // Transport 3: High-Bandwidth UNIX Domain Stream Socket (AF_UNIX)
    int                  sock_fd;
    int                  sock_listen_fd;
    char                 sock_path[128];
    bool                 sock_enabled;

    // Global Sequence Counters for In-Order Demuxing
    uint64_t             tx_seq;
    uint64_t             rx_seq;

    // Multiplexing Poll / Epoll Descriptor
    int                  epoll_fd;
} ipc_channel_t;

/* =========================================================================
 * Unified Public API (Application Interface Layer)
 * ========================================================================= */

/**
 * @brief Create or attach to an Adaptive IPC channel.
 *
 * Automatically provisions all underlying transports (FAS-IPC ring, Pipe,
 * and UNIX domain socket) and initializes the telemetry observer and selector.
 *
 * @param chan Pointer to channel handle
 * @param name Channel name identifier (e.g., "aipc_demo")
 * @param role IPC_ROLE_PRODUCER (creates resources) or IPC_ROLE_CONSUMER (connects)
 * @return 0 on success, negative error code on failure
 */
int ipc_create(ipc_channel_t *chan, const char *name, ipc_role_t role);

/**
 * @brief Transmit payload across dynamically selected optimal transport.
 *
 * Inspects payload size, updates EWMA telemetry, evaluates the crossover cost
 * model with hysteresis, and forwards data over the selected transport.
 *
 * @param chan Pointer to active channel handle
 * @param buf Pointer to data payload
 * @param len Size of data in bytes
 * @return 0 on success, negative error code on failure
 */
int ipc_send(ipc_channel_t *chan, const void *buf, size_t len);

/**
 * @brief Receive payload with coordinated multi-transport demuxing.
 *
 * Polling priority:
 *  1. Checks FAS-IPC lock-free ring with AHSP micro-spinning.
 *  2. Drains pending stream frames from Pipe and UNIX Socket.
 * Guarantees zero out-of-order delivery during rapid transport transitions.
 *
 * @param chan Pointer to active channel handle
 * @param buf Output buffer pointer
 * @param max_len Maximum bytes buffer can accept
 * @return Number of payload bytes received, or negative error code
 */
int ipc_receive(ipc_channel_t *chan, void *buf, size_t max_len);

/**
 * @brief Close local channel descriptors and unmap shared memory.
 */
int ipc_close(ipc_channel_t *chan);

/**
 * @brief Destroy channel and unlink all filesystem and shared memory primitives.
 */
int ipc_destroy(ipc_channel_t *chan);

/**
 * @brief Pin channel to a specific fixed transport (bypasses adaptive selector).
 */
void ipc_force_transport(ipc_channel_t *chan, ipc_transport_type_t transport);

/**
 * @brief Restore automatic adaptive transport selection.
 */
void ipc_reset_forced_transport(ipc_channel_t *chan);

/**
 * @brief Print detailed channel telemetry and transport switch statistics.
 */
void ipc_channel_print_stats(const ipc_channel_t *chan);

#ifdef __cplusplus
}
#endif

#endif // AIPC_H
