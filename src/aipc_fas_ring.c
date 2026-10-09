#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "aipc_fas_ring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#include <immintrin.h>
#define AIPC_CPU_PAUSE() _mm_pause()
#elif defined(__aarch64__) || defined(__arm__)
#define AIPC_CPU_PAUSE() __asm__ __volatile__("yield" ::: "memory")
#else
#define AIPC_CPU_PAUSE() __asm__ __volatile__("" ::: "memory")
#endif

static inline int fas_futex_wake(atomic_uint_least32_t *uaddr, int val) {
    return (int)syscall(SYS_futex, (int *)uaddr, FUTEX_WAKE, val, NULL, NULL, 0);
}

static inline int fas_futex_wait(atomic_uint_least32_t *uaddr, int expected_val, const struct timespec *timeout) {
    return (int)syscall(SYS_futex, (int *)uaddr, FUTEX_WAIT, expected_val, timeout, NULL, 0);
}

void fas_ring_init(fas_ring_t *ring) {
    if (!ring) return;
    memset((void *)ring, 0, sizeof(fas_ring_t));
    atomic_init(&ring->tail, 0);
    ring->cached_head = 0;
    atomic_init(&ring->head, 0);
    ring->cached_tail = 0;
    atomic_init(&ring->futex_word, 0);
    atomic_init(&ring->waiting_readers, 0);
}

int fas_ring_send(fas_ring_t *ring, const void *data, uint32_t len) {
    if (!ring || !data || len > FAS_SLOT_MAX_SIZE) {
        return -1;
    }

    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = ring->cached_head;

    // Check full condition
    if ((tail - head) >= FAS_RING_SLOTS) {
        head = atomic_load_explicit(&ring->head, memory_order_acquire);
        ring->cached_head = head;
        while ((tail - head) >= FAS_RING_SLOTS) {
            AIPC_CPU_PAUSE();
            head = atomic_load_explicit(&ring->head, memory_order_acquire);
            ring->cached_head = head;
        }
    }

    uint32_t slot_idx = tail & (FAS_RING_SLOTS - 1);
    ring->slots[slot_idx].len = len;
    ring->slots[slot_idx].flags = 0;
    memcpy((void *)ring->slots[slot_idx].payload, data, len);

    // Release store commits the payload before advancing tail
    atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);

    // Full memory barrier prevents Store(tail)-Load(waiting_readers) reordering across cores
    atomic_thread_fence(memory_order_seq_cst);

    // Wake consumer ONLY if parked
    if (atomic_load_explicit(&ring->waiting_readers, memory_order_seq_cst) > 0) {
        atomic_fetch_add_explicit(&ring->futex_word, 1, memory_order_release);
        fas_futex_wake(&ring->futex_word, 1);
    }

    return 0;
}

int fas_ring_try_send(fas_ring_t *ring, const void *data, uint32_t len) {
    if (!ring || !data || len > FAS_SLOT_MAX_SIZE) {
        return -1;
    }

    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    uint32_t head = ring->cached_head;

    if ((tail - head) >= FAS_RING_SLOTS) {
        head = atomic_load_explicit(&ring->head, memory_order_acquire);
        ring->cached_head = head;
        if ((tail - head) >= FAS_RING_SLOTS) {
            return -1;
        }
    }

    uint32_t slot_idx = tail & (FAS_RING_SLOTS - 1);
    ring->slots[slot_idx].len = len;
    ring->slots[slot_idx].flags = 0;
    memcpy((void *)ring->slots[slot_idx].payload, data, len);

    atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);

    atomic_thread_fence(memory_order_seq_cst);

    if (atomic_load_explicit(&ring->waiting_readers, memory_order_seq_cst) > 0) {
        atomic_fetch_add_explicit(&ring->futex_word, 1, memory_order_release);
        fas_futex_wake(&ring->futex_word, 1);
    }

    return 0;
}

int fas_ring_recv(fas_ring_t *ring, void *buf, uint32_t max_len) {
    if (!ring || !buf || max_len == 0) {
        return -1;
    }

    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t tail = ring->cached_tail;

    if (head == tail) {
        tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
        ring->cached_tail = tail;
    }

    // Phase 1: Micro-spin using CPU pause instruction (bounded by FAS_AHSP_SPIN_LIMIT)
    int spin_count = 0;
    while (head == tail && spin_count < FAS_AHSP_SPIN_LIMIT) {
        AIPC_CPU_PAUSE();
        spin_count++;
        tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
        ring->cached_tail = tail;
    }

    // Phase 2: Kernel Futex park if channel remains idle
    if (head == tail) {
        atomic_fetch_add_explicit(&ring->waiting_readers, 1, memory_order_seq_cst);
        while (head == tail) {
            uint32_t expected_futex = atomic_load_explicit(&ring->futex_word, memory_order_acquire);
            tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
            ring->cached_tail = tail;
            if (head != tail) {
                break;
            }

            // Failsafe 50ms timeout prevents permanent deadlock if scheduler preempts across core boundary
            struct timespec ts = {.tv_sec = 0, .tv_nsec = 50000000};
            fas_futex_wait(&ring->futex_word, (int)expected_futex, &ts);
            tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
            ring->cached_tail = tail;
        }
        atomic_fetch_sub_explicit(&ring->waiting_readers, 1, memory_order_seq_cst);
    }

    uint32_t slot_idx = head & (FAS_RING_SLOTS - 1);
    uint32_t msg_len = ring->slots[slot_idx].len;
    uint32_t copy_len = (msg_len < max_len) ? msg_len : max_len;

    memcpy(buf, (const void *)ring->slots[slot_idx].payload, copy_len);
    atomic_store_explicit(&ring->head, head + 1, memory_order_release);

    return (int)copy_len;
}

int fas_ring_try_recv(fas_ring_t *ring, void *buf, uint32_t max_len) {
    if (!ring || !buf || max_len == 0) {
        return -1;
    }

    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t tail = ring->cached_tail;

    if (head == tail) {
        tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
        ring->cached_tail = tail;
        if (head == tail) {
            return -1;
        }
    }

    uint32_t slot_idx = head & (FAS_RING_SLOTS - 1);
    uint32_t msg_len = ring->slots[slot_idx].len;
    uint32_t copy_len = (msg_len < max_len) ? msg_len : max_len;

    memcpy(buf, (const void *)ring->slots[slot_idx].payload, copy_len);
    atomic_store_explicit(&ring->head, head + 1, memory_order_release);

    return (int)copy_len;
}

bool fas_ring_is_empty(const fas_ring_t *ring) {
    if (!ring) return true;
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    return head == tail;
}

bool fas_ring_is_full(const fas_ring_t *ring) {
    if (!ring) return false;
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    return (tail - head) >= FAS_RING_SLOTS;
}

size_t fas_ring_count(const fas_ring_t *ring) {
    if (!ring) return 0;
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    return (size_t)(tail - head);
}

int fas_channel_create(fas_channel_t *chan, const char *shm_name, bool is_producer) {
    if (!chan || !shm_name) return -1;
    memset(chan, 0, sizeof(*chan));
    strncpy(chan->shm_name, shm_name, sizeof(chan->shm_name) - 1);
    chan->is_producer = is_producer;
    chan->is_creator = true;
    chan->shm_size = sizeof(fas_channel_shm_t);

    shm_unlink(shm_name);
    int fd = shm_open(shm_name, O_CREAT | O_RDWR | O_EXCL, 0666);
    if (fd < 0) {
        return -1;
    }

    if (ftruncate(fd, (off_t)chan->shm_size) != 0) {
        close(fd);
        shm_unlink(shm_name);
        return -1;
    }

    void *ptr = mmap(NULL, chan->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        close(fd);
        shm_unlink(shm_name);
        return -1;
    }

    chan->shm_fd = fd;
    chan->shm_map = (fas_channel_shm_t *)ptr;
    fas_ring_init(&chan->shm_map->p2c_ring);
    fas_ring_init(&chan->shm_map->c2p_ring);

    if (is_producer) {
        chan->tx_ring = &chan->shm_map->p2c_ring;
        chan->rx_ring = &chan->shm_map->c2p_ring;
    } else {
        chan->tx_ring = &chan->shm_map->c2p_ring;
        chan->rx_ring = &chan->shm_map->p2c_ring;
    }

    return 0;
}

int fas_channel_attach(fas_channel_t *chan, const char *shm_name, bool is_producer) {
    if (!chan || !shm_name) return -1;
    memset(chan, 0, sizeof(*chan));
    strncpy(chan->shm_name, shm_name, sizeof(chan->shm_name) - 1);
    chan->is_producer = is_producer;
    chan->is_creator = false;
    chan->shm_size = sizeof(fas_channel_shm_t);

    int fd = -1;
    for (int retry = 0; retry < 100; retry++) {
        fd = shm_open(shm_name, O_RDWR, 0666);
        if (fd >= 0) break;
        usleep(10000); // 10ms wait
    }
    if (fd < 0) {
        return -1;
    }

    void *ptr = mmap(NULL, chan->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        close(fd);
        return -1;
    }

    chan->shm_fd = fd;
    chan->shm_map = (fas_channel_shm_t *)ptr;

    if (is_producer) {
        chan->tx_ring = &chan->shm_map->p2c_ring;
        chan->rx_ring = &chan->shm_map->c2p_ring;
    } else {
        chan->tx_ring = &chan->shm_map->c2p_ring;
        chan->rx_ring = &chan->shm_map->p2c_ring;
    }

    return 0;
}

int fas_channel_send(fas_channel_t *chan, const void *data, uint32_t len) {
    if (!chan || !chan->tx_ring) return -1;
    return fas_ring_send(chan->tx_ring, data, len);
}

int fas_channel_try_send(fas_channel_t *chan, const void *data, uint32_t len) {
    if (!chan || !chan->tx_ring) return -1;
    return fas_ring_try_send(chan->tx_ring, data, len);
}

int fas_channel_recv(fas_channel_t *chan, void *buf, uint32_t max_len) {
    if (!chan || !chan->rx_ring) return -1;
    return fas_ring_recv(chan->rx_ring, buf, max_len);
}

int fas_channel_try_recv(fas_channel_t *chan, void *buf, uint32_t max_len) {
    if (!chan || !chan->rx_ring) return -1;
    return fas_ring_try_recv(chan->rx_ring, buf, max_len);
}

void fas_channel_close(fas_channel_t *chan) {
    if (!chan) return;
    if (chan->shm_map && chan->shm_map != MAP_FAILED) {
        munmap(chan->shm_map, chan->shm_size);
        chan->shm_map = NULL;
    }
    if (chan->shm_fd >= 0) {
        close(chan->shm_fd);
        chan->shm_fd = -1;
    }
}

void fas_channel_destroy(fas_channel_t *chan) {
    if (!chan) return;
    fas_channel_close(chan);
    if (chan->is_creator && chan->shm_name[0] != '\0') {
        shm_unlink(chan->shm_name);
    }
}
