#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "aipc.h"
#include "aipc_bench.h"
#include "aipc_fas_ring.h"
#include "aipc_selector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#include <immintrin.h>
#define AIPC_CPU_PAUSE() _mm_pause()
#elif defined(__aarch64__) || defined(__arm__)
#define AIPC_CPU_PAUSE() __asm__ __volatile__("yield" ::: "memory")
#else
#define AIPC_CPU_PAUSE() __asm__ __volatile__("" ::: "memory")
#endif

static ssize_t write_exact(int fd, const void *buf, size_t count) {
    size_t written = 0;
    const char *p = (const char *)buf;
    while (written < count) {
        ssize_t n = write(fd, p + written, count - written);
        if (n > 0) {
            written += (size_t)n;
        } else if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        } else {
            return -1;
        }
    }
    return (ssize_t)written;
}

static ssize_t read_exact(int fd, void *buf, size_t count) {
    size_t read_bytes = 0;
    char *p = (char *)buf;
    while (read_bytes < count) {
        ssize_t n = read(fd, p + read_bytes, count - read_bytes);
        if (n > 0) {
            read_bytes += (size_t)n;
        } else if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        } else {
            return -1; // EOF
        }
    }
    return (ssize_t)read_bytes;
}

static void drain_bytes(int fd, size_t count) {
    char discard[4096];
    while (count > 0) {
        size_t chunk = (count < sizeof(discard)) ? count : sizeof(discard);
        if (read_exact(fd, discard, chunk) < 0) break;
        count -= chunk;
    }
}

int ipc_create(ipc_channel_t *chan, const char *name, ipc_role_t role) {
    if (!chan || !name) return -1;
    memset(chan, 0, sizeof(*chan));

    strncpy(chan->channel_name, name, sizeof(chan->channel_name) - 1);
    chan->role = role;
    chan->is_creator = (role == IPC_ROLE_PRODUCER);
    chan->pipe_tx_fd = -1;
    chan->pipe_rx_fd = -1;
    chan->sock_fd = -1;
    chan->sock_listen_fd = -1;
    chan->epoll_fd = -1;
    chan->tx_seq = 0;
    chan->rx_seq = 0;

    aipc_selector_init(&chan->selector);
    chan->current_transport = IPC_TRANSPORT_FAS_SHM;

    // Build resource paths
    char shm_name[128];
    snprintf(shm_name, sizeof(shm_name), "/aipc_%s_shm", name);

    char p2c_fifo[128];
    char c2p_fifo[128];
    snprintf(p2c_fifo, sizeof(p2c_fifo), "/tmp/aipc_%s_p2c.fifo", name);
    snprintf(c2p_fifo, sizeof(c2p_fifo), "/tmp/aipc_%s_c2p.fifo", name);

    snprintf(chan->sock_path, sizeof(chan->sock_path), "/tmp/aipc_%s_sock", name);

    // =========================================================================
    // 1. Fast-Path Shared Memory (FAS-IPC) Setup
    // =========================================================================
    if (chan->is_creator) {
        if (fas_channel_create(&chan->fas_chan, shm_name, true) == 0) {
            chan->fas_enabled = true;
        }
    } else {
        if (fas_channel_attach(&chan->fas_chan, shm_name, false) == 0) {
            chan->fas_enabled = true;
        }
    }

    // =========================================================================
    // 2. Linux Pipe / FIFO Setup
    // =========================================================================
    if (chan->is_creator) {
        unlink(p2c_fifo);
        unlink(c2p_fifo);
        mkfifo(p2c_fifo, 0666);
        mkfifo(c2p_fifo, 0666);

        strncpy(chan->pipe_tx_path, p2c_fifo, sizeof(chan->pipe_tx_path) - 1);
        strncpy(chan->pipe_rx_path, c2p_fifo, sizeof(chan->pipe_rx_path) - 1);

        chan->pipe_tx_fd = open(chan->pipe_tx_path, O_RDWR);
        chan->pipe_rx_fd = open(chan->pipe_rx_path, O_RDWR);
        if (chan->pipe_tx_fd >= 0 && chan->pipe_rx_fd >= 0) {
            chan->pipe_enabled = true;
        }
    } else {
        strncpy(chan->pipe_tx_path, c2p_fifo, sizeof(chan->pipe_tx_path) - 1);
        strncpy(chan->pipe_rx_path, p2c_fifo, sizeof(chan->pipe_rx_path) - 1);

        for (int retry = 0; retry < 50; retry++) {
            chan->pipe_rx_fd = open(chan->pipe_rx_path, O_RDWR);
            chan->pipe_tx_fd = open(chan->pipe_tx_path, O_RDWR);
            if (chan->pipe_rx_fd >= 0 && chan->pipe_tx_fd >= 0) {
                chan->pipe_enabled = true;
                break;
            }
            usleep(10000);
        }
    }

    // =========================================================================
    // 3. UNIX Domain Socket Setup
    // =========================================================================
    struct sockaddr_un sun_addr;
    memset(&sun_addr, 0, sizeof(sun_addr));
    sun_addr.sun_family = AF_UNIX;
    strncpy(sun_addr.sun_path, chan->sock_path, sizeof(sun_addr.sun_path) - 1);

    if (chan->is_creator) {
        unlink(chan->sock_path);
        chan->sock_listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (chan->sock_listen_fd >= 0) {
            if (bind(chan->sock_listen_fd, (struct sockaddr *)&sun_addr, sizeof(sun_addr)) == 0) {
                listen(chan->sock_listen_fd, 5);
                chan->sock_enabled = true;
            }
        }
    } else {
        chan->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (chan->sock_fd >= 0) {
            for (int retry = 0; retry < 50; retry++) {
                if (connect(chan->sock_fd, (struct sockaddr *)&sun_addr, sizeof(sun_addr)) == 0) {
                    chan->sock_enabled = true;
                    break;
                }
                usleep(10000);
            }
        }
    }

    return 0;
}

static void ensure_socket_connected(ipc_channel_t *chan) {
    if (!chan || !chan->sock_enabled) return;
    if (chan->sock_fd >= 0) return;

    if (chan->is_creator && chan->sock_listen_fd >= 0) {
        struct pollfd pfd;
        pfd.fd = chan->sock_listen_fd;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 100) > 0 && (pfd.revents & POLLIN)) {
            int conn_fd = accept(chan->sock_listen_fd, NULL, NULL);
            if (conn_fd >= 0) {
                chan->sock_fd = conn_fd;
            }
        }
    }
}

int ipc_send(ipc_channel_t *chan, const void *buf, size_t len) {
    if (!chan || !buf || len == 0) {
        return -1;
    }

    // 1. Workload Telemetry Update
    aipc_selector_record_event(&chan->selector, len, 0);

    // 2. Dynamic Transport Selection (Crossover Logic + Hysteresis)
    ipc_transport_type_t target = aipc_selector_select(&chan->selector, len);
    chan->current_transport = target;

    // 3. Global Sequence Allocation
    uint64_t seq = chan->tx_seq++;

    // 4. Dispatch to Selected Transport
    switch (target) {
        case IPC_TRANSPORT_FAS_SHM: {
            if (__builtin_expect(chan->fas_enabled && chan->fas_chan.tx_ring != NULL, 1)) {
                return fas_ring_send_flags(chan->fas_chan.tx_ring, buf, (uint32_t)len, (uint32_t)seq);
            }
            // Fallback to Pipe if FAS-IPC unavailable
            target = IPC_TRANSPORT_PIPE;
            /* fallthrough */
        }

        case IPC_TRANSPORT_PIPE: {
            if (chan->pipe_enabled && chan->pipe_tx_fd >= 0) {
                aipc_frame_hdr_t hdr;
                hdr.magic = AIPC_FRAME_MAGIC;
                hdr.payload_len = (uint32_t)len;
                hdr.seq_num = seq;
                hdr.transport_id = IPC_TRANSPORT_PIPE;
                hdr.checksum = 0;

                if (write_exact(chan->pipe_tx_fd, &hdr, sizeof(hdr)) < 0) {
                    return -1;
                }
                if (write_exact(chan->pipe_tx_fd, buf, len) < 0) {
                    return -1;
                }
                return 0;
            }
            // Fallback to Socket if Pipe unavailable
            target = IPC_TRANSPORT_SOCKET;
            /* fallthrough */
        }

        case IPC_TRANSPORT_SOCKET: {
            ensure_socket_connected(chan);
            if (chan->sock_enabled && chan->sock_fd >= 0) {
                aipc_frame_hdr_t hdr;
                hdr.magic = AIPC_FRAME_MAGIC;
                hdr.payload_len = (uint32_t)len;
                hdr.seq_num = seq;
                hdr.transport_id = IPC_TRANSPORT_SOCKET;
                hdr.checksum = 0;

                if (write_exact(chan->sock_fd, &hdr, sizeof(hdr)) < 0) {
                    return -1;
                }
                if (write_exact(chan->sock_fd, buf, len) < 0) {
                    return -1;
                }
                return 0;
            }
            return -1;
        }

        default:
            return -1;
    }
}

int ipc_receive(ipc_channel_t *chan, void *buf, size_t max_len) {
    if (!chan || !buf || max_len == 0) {
        return -1;
    }

    uint64_t target_seq = chan->rx_seq;
    int spin_count = 0;

    while (1) {
        // =====================================================================
        // Step 1: Fast-Path Check (FAS-IPC Shared Memory Ring)
        // =====================================================================
        if (chan->fas_enabled && chan->fas_chan.rx_ring) {
            fas_ring_t *ring = chan->fas_chan.rx_ring;
            uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
            uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

            if (head != tail) {
                uint32_t slot_idx = head & (FAS_RING_SLOTS - 1);
                fas_slot_t *slot = &ring->slots[slot_idx];

                if (slot->flags == (uint32_t)target_seq) {
                    uint32_t msg_len = slot->len;
                    uint32_t copy_len = (msg_len < max_len) ? msg_len : (uint32_t)max_len;
                    memcpy(buf, (const void *)slot->payload, copy_len);

                    atomic_store_explicit(&ring->head, head + 1, memory_order_release);
                    chan->rx_seq++;
                    return (int)copy_len;
                }
            }
        }

        // =====================================================================
        // Step 2: Stream Transport Check (Pipe / FIFO)
        // =====================================================================
        if (chan->pipe_enabled && chan->pipe_rx_fd >= 0) {
            struct pollfd pfd;
            pfd.fd = chan->pipe_rx_fd;
            pfd.events = POLLIN;

            if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
                aipc_frame_hdr_t hdr;
                if (read_exact(chan->pipe_rx_fd, &hdr, sizeof(hdr)) == (ssize_t)sizeof(hdr)) {
                    if (hdr.magic == AIPC_FRAME_MAGIC) {
                        uint32_t to_read = (hdr.payload_len < max_len) ? hdr.payload_len : (uint32_t)max_len;
                        if (read_exact(chan->pipe_rx_fd, buf, to_read) < 0) {
                            return -1;
                        }
                        if (hdr.payload_len > to_read) {
                            drain_bytes(chan->pipe_rx_fd, hdr.payload_len - to_read);
                        }
                        chan->rx_seq++;
                        return (int)to_read;
                    }
                }
            }
        }

        // =====================================================================
        // Step 3: Stream Transport Check (UNIX Domain Socket)
        // =====================================================================
        ensure_socket_connected(chan);
        if (chan->sock_enabled && chan->sock_fd >= 0) {
            struct pollfd sfd;
            sfd.fd = chan->sock_fd;
            sfd.events = POLLIN;

            if (poll(&sfd, 1, 0) > 0 && (sfd.revents & POLLIN)) {
                aipc_frame_hdr_t hdr;
                if (read_exact(chan->sock_fd, &hdr, sizeof(hdr)) == (ssize_t)sizeof(hdr)) {
                    if (hdr.magic == AIPC_FRAME_MAGIC) {
                        uint32_t to_read = (hdr.payload_len < max_len) ? hdr.payload_len : (uint32_t)max_len;
                        if (read_exact(chan->sock_fd, buf, to_read) < 0) {
                            return -1;
                        }
                        if (hdr.payload_len > to_read) {
                            drain_bytes(chan->sock_fd, hdr.payload_len - to_read);
                        }
                        chan->rx_seq++;
                        return (int)to_read;
                    }
                }
            }
        }

        // =====================================================================
        // Step 4: Hybrid Micro-Spin and Backoff
        // =====================================================================
        if (spin_count < FAS_AHSP_SPIN_LIMIT) {
            AIPC_CPU_PAUSE();
            spin_count++;
        } else {
            // Channel remains idle: bounded 5ms wait across active file descriptors
            struct pollfd fds[2];
            nfds_t nfds = 0;
            if (chan->pipe_rx_fd >= 0) {
                fds[nfds].fd = chan->pipe_rx_fd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
            if (chan->sock_fd >= 0) {
                fds[nfds].fd = chan->sock_fd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
            if (nfds > 0) {
                poll(fds, nfds, 5); // 5ms sleep prevents burning 100% CPU on idle
            } else {
                usleep(1000);
            }
            spin_count = 0;
        }
    }
}

int ipc_close(ipc_channel_t *chan) {
    if (!chan) return -1;

    if (chan->fas_enabled) {
        fas_channel_close(&chan->fas_chan);
        chan->fas_enabled = false;
    }
    if (chan->pipe_tx_fd >= 0) {
        close(chan->pipe_tx_fd);
        chan->pipe_tx_fd = -1;
    }
    if (chan->pipe_rx_fd >= 0) {
        close(chan->pipe_rx_fd);
        chan->pipe_rx_fd = -1;
    }
    if (chan->sock_fd >= 0) {
        close(chan->sock_fd);
        chan->sock_fd = -1;
    }
    if (chan->sock_listen_fd >= 0) {
        close(chan->sock_listen_fd);
        chan->sock_listen_fd = -1;
    }
    if (chan->epoll_fd >= 0) {
        close(chan->epoll_fd);
        chan->epoll_fd = -1;
    }
    return 0;
}

int ipc_destroy(ipc_channel_t *chan) {
    if (!chan) return -1;
    ipc_close(chan);

    if (chan->is_creator) {
        fas_channel_destroy(&chan->fas_chan);
        if (chan->pipe_tx_path[0]) unlink(chan->pipe_tx_path);
        if (chan->pipe_rx_path[0]) unlink(chan->pipe_rx_path);
        if (chan->sock_path[0]) unlink(chan->sock_path);
    }
    return 0;
}

void ipc_force_transport(ipc_channel_t *chan, ipc_transport_type_t transport) {
    if (!chan) return;
    aipc_selector_set_forced_transport(&chan->selector, transport);
    chan->current_transport = transport;
}

void ipc_reset_forced_transport(ipc_channel_t *chan) {
    if (!chan) return;
    aipc_selector_clear_forced_transport(&chan->selector);
}

void ipc_channel_print_stats(const ipc_channel_t *chan) {
    if (!chan) return;
    printf("================================================================================\n");
    printf(" A-IPC Channel Telemetry & Statistics: [%s]\n", chan->channel_name);
    printf("================================================================================\n");
    printf(" Total Messages Transmitted: %lu\n", (unsigned long)chan->selector.stats.msg_count);
    printf(" Total Bytes Transferred:   %lu (%.2f MB)\n",
           (unsigned long)chan->selector.stats.byte_count,
           (double)chan->selector.stats.byte_count / (1024.0 * 1024.0));
    printf(" EWMA Arrival Rate:         %.2f msgs/sec\n", chan->selector.stats.ewma_rate_msg_per_sec);
    printf(" EWMA Bandwidth:            %.2f MB/sec\n", chan->selector.stats.ewma_bandwidth_mbps);
    printf(" Total Dynamic Switches:    %lu\n", (unsigned long)chan->selector.total_switches);
    printf(" Active Transport:          %s\n", aipc_transport_type_name(chan->current_transport));
    printf("================================================================================\n");
}
