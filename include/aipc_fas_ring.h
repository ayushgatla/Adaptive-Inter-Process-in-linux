#ifndef AIPC_FAS_RING_H
#define AIPC_FAS_RING_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdalign.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FAS-IPC Architecture Dimensions
 *
 * FAS_CACHE_LINE_SIZE: Strict 64-byte alignment eliminates false sharing (MESI).
 * FAS_RING_SLOTS: Power-of-two slot count (1024) allows fast bitwise wrapping.
 * FAS_SLOT_MAX_SIZE: Maximum payload per fast-path slot (1024 bytes).
 * FAS_AHSP_SPIN_LIMIT: 200 iterations of _mm_pause() (~250-350 ns) before futex parking.
 */
#define FAS_CACHE_LINE_SIZE 64
#define FAS_RING_SLOTS      1024
#define FAS_SLOT_MAX_SIZE   1024
#define FAS_AHSP_SPIN_LIMIT 200

/**
 * @brief FAS-IPC Ring Buffer Slot Layout.
 *
 * Padded to an exact multiple of 64 bytes (1088 bytes = 17 * 64 bytes)
 * so every slot starts on an isolated cache line boundary.
 */
typedef struct {
    uint32_t len;
    uint32_t flags;
    uint8_t  payload[FAS_SLOT_MAX_SIZE];
    uint8_t  _pad[56]; // 4 + 4 + 1024 + 56 = 1088 bytes (17 * 64)
} fas_slot_t;

/**
 * @brief FAS-IPC Control Header Structure.
 *
 * Exactly 3 separate 64-byte cache lines (192 bytes total):
 *  - Cache Line 0 (Producer / Core C1): tail, cached_head, pad0
 *  - Cache Line 1 (Consumer / Core C2): head, cached_tail, pad1
 *  - Cache Line 2 (Sync Coordination): futex_word, waiting_readers, pad2
 */
typedef struct {
    // Cache Line 0: Written by PRODUCER ONLY
    alignas(FAS_CACHE_LINE_SIZE) atomic_uint_least32_t tail;
    uint32_t cached_head;
    uint8_t  pad0[FAS_CACHE_LINE_SIZE - sizeof(atomic_uint_least32_t) - sizeof(uint32_t)];

    // Cache Line 1: Written by CONSUMER ONLY
    alignas(FAS_CACHE_LINE_SIZE) atomic_uint_least32_t head;
    uint32_t cached_tail;
    uint8_t  pad1[FAS_CACHE_LINE_SIZE - sizeof(atomic_uint_least32_t) - sizeof(uint32_t)];

    // Cache Line 2: Synchronization & Futex coordination
    alignas(FAS_CACHE_LINE_SIZE) atomic_uint_least32_t futex_word;
    atomic_uint_least32_t waiting_readers;
    uint8_t  pad2[FAS_CACHE_LINE_SIZE - (2 * sizeof(atomic_uint_least32_t))];
} fas_control_header_t;

/**
 * @brief Single-Producer Single-Consumer (SPSC) Cache-Isolated Ring Buffer.
 */
typedef struct fas_ring {
    union {
        fas_control_header_t ctrl;
        struct {
            // Cache Line 0
            alignas(FAS_CACHE_LINE_SIZE) atomic_uint_least32_t tail;
            uint32_t cached_head;
            uint8_t  pad0[FAS_CACHE_LINE_SIZE - sizeof(atomic_uint_least32_t) - sizeof(uint32_t)];

            // Cache Line 1
            alignas(FAS_CACHE_LINE_SIZE) atomic_uint_least32_t head;
            uint32_t cached_tail;
            uint8_t  pad1[FAS_CACHE_LINE_SIZE - sizeof(atomic_uint_least32_t) - sizeof(uint32_t)];

            // Cache Line 2
            alignas(FAS_CACHE_LINE_SIZE) atomic_uint_least32_t futex_word;
            atomic_uint_least32_t waiting_readers;
            uint8_t  pad2[FAS_CACHE_LINE_SIZE - (2 * sizeof(atomic_uint_least32_t))];
        };
    };

    // Payload storage starts at offset 192 (Cache Line 3)
    alignas(FAS_CACHE_LINE_SIZE) fas_slot_t slots[FAS_RING_SLOTS];
} fas_ring_t;

/**
 * @brief Duplex (Bidirectional) Shared-Memory Layout.
 * Contains two independent SPSC rings:
 *  - p2c_ring: Producer -> Consumer channel
 *  - c2p_ring: Consumer -> Producer channel (echo / reply)
 */
typedef struct {
    fas_ring_t p2c_ring;
    fas_ring_t c2p_ring;
} fas_channel_shm_t;

/**
 * @brief User-facing Duplex FAS-IPC Channel Handle.
 */
typedef struct {
    char               shm_name[64];
    int                shm_fd;
    size_t             shm_size;
    fas_channel_shm_t *shm_map;
    fas_ring_t        *tx_ring;  // Active transmission ring for this role
    fas_ring_t        *rx_ring;  // Active reception ring for this role
    bool               is_producer;
    bool               is_creator;
} fas_channel_t;

/* =========================================================================
 * Low-Level SPSC Ring API (Direct Ring Operations)
 * ========================================================================= */

/**
 * @brief Zero-initialize a ring buffer in shared or local memory.
 */
void fas_ring_init(fas_ring_t *ring);

/**
 * @brief Enqueue a message into the ring buffer (producer fast-path).
 *
 * If the ring is full, spins until a slot frees up.
 * In steady-state streaming, incurs 0 system calls.
 * If waiting_readers > 0, wakes the parked reader via futex.
 *
 * @param ring Pointer to SPSC ring buffer
 * @param data Message payload pointer
 * @param len Message payload size (must be <= FAS_SLOT_MAX_SIZE)
 * @return 0 on success, negative error code on invalid parameters
 */
int fas_ring_send(fas_ring_t *ring, const void *data, uint32_t len);

/**
 * @brief Non-blocking enqueue. Returns immediately with -1 if ring is full.
 */
int fas_ring_try_send(fas_ring_t *ring, const void *data, uint32_t len);

/**
 * @brief Dequeue a message from the ring buffer using AHSP (consumer fast-path).
 *
 * Adaptive Hybrid Spin-Park (AHSP):
 *  1. Micro-spins using _mm_pause() for up to FAS_AHSP_SPIN_LIMIT (200) iterations.
 *  2. If still empty, registers waiting_readers and parks via SYS_futex.
 *
 * @param ring Pointer to SPSC ring buffer
 * @param buf Output buffer pointer
 * @param max_len Maximum bytes to receive
 * @return Number of payload bytes received, or negative error code
 */
int fas_ring_recv(fas_ring_t *ring, void *buf, uint32_t max_len);

/**
 * @brief Non-blocking dequeue. Returns immediately with -1 if ring is empty.
 */
int fas_ring_try_recv(fas_ring_t *ring, void *buf, uint32_t max_len);

/**
 * @brief Query ring state metrics.
 */
bool   fas_ring_is_empty(const fas_ring_t *ring);
bool   fas_ring_is_full(const fas_ring_t *ring);
size_t fas_ring_count(const fas_ring_t *ring);

/* =========================================================================
 * Duplex Shared-Memory Channel Lifecycle & Data Transfer API
 * ========================================================================= */

/**
 * @brief Create and initialize a new duplex POSIX shared memory channel.
 *
 * @param chan Pointer to channel handle
 * @param shm_name Shared memory segment name (e.g. "/aipc_fas_shm")
 * @param is_producer True if this endpoint acts as primary producer
 * @return 0 on success, negative on error
 */
int fas_channel_create(fas_channel_t *chan, const char *shm_name, bool is_producer);

/**
 * @brief Attach to an existing duplex POSIX shared memory channel.
 */
int fas_channel_attach(fas_channel_t *chan, const char *shm_name, bool is_producer);

/**
 * @brief Send payload over the channel's active transmit ring.
 */
int fas_channel_send(fas_channel_t *chan, const void *data, uint32_t len);

/**
 * @brief Non-blocking send over the channel's active transmit ring.
 */
int fas_channel_try_send(fas_channel_t *chan, const void *data, uint32_t len);

/**
 * @brief Receive payload from the channel's active receive ring with AHSP.
 */
int fas_channel_recv(fas_channel_t *chan, void *buf, uint32_t max_len);

/**
 * @brief Non-blocking receive from the channel's active receive ring.
 */
int fas_channel_try_recv(fas_channel_t *chan, void *buf, uint32_t max_len);

/**
 * @brief Unmap shared memory segment and close descriptor.
 */
void fas_channel_close(fas_channel_t *chan);

/**
 * @brief Close and unlink the shared memory segment.
 */
void fas_channel_destroy(fas_channel_t *chan);

#ifdef __cplusplus
}
#endif

#endif // AIPC_FAS_RING_H
