#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "aipc_baselines.h"
#include "aipc_bench.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <mqueue.h>

#define NAIVE_SHM_MAX_PAYLOAD 65536

typedef struct {
    // Naive unpadded contiguous layout triggering false sharing
    volatile int p2c_ready;
    volatile int c2p_ready;
    uint32_t     len;
    uint8_t      buffer[NAIVE_SHM_MAX_PAYLOAD];
} naive_shm_channel_t;

const char *aipc_baseline_name(aipc_baseline_type_t type) {
    switch (type) {
        case AIPC_BASELINE_PIPE:      return "Anonymous Pipe";
        case AIPC_BASELINE_SOCKET:    return "UNIX Socket";
        case AIPC_BASELINE_MQ:        return "POSIX MQ";
        case AIPC_BASELINE_NAIVE_SHM: return "Naive SHM+Futex";
        default:                      return "Unknown";
    }
}

aipc_baseline_type_t aipc_baseline_from_string(const char *str) {
    if (!str) return AIPC_BASELINE_PIPE;
    if (strcasecmp(str, "pipe") == 0) return AIPC_BASELINE_PIPE;
    if (strcasecmp(str, "socket") == 0) return AIPC_BASELINE_SOCKET;
    if (strcasecmp(str, "mq") == 0) return AIPC_BASELINE_MQ;
    if (strcasecmp(str, "shm_naive") == 0 || strcasecmp(str, "shm") == 0) return AIPC_BASELINE_NAIVE_SHM;
    return AIPC_BASELINE_PIPE;
}

static ssize_t write_all(int fd, const void *buf, size_t count) {
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

static ssize_t read_all(int fd, void *buf, size_t count) {
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

/* ========================================================================= */
/* 1. Linux Anonymous Pipe Echo Implementation                               */
/* ========================================================================= */
static int run_pipe_bench(const aipc_bench_config_t *config, aipc_bench_result_t *res) {
    int p2c[2]; // Producer -> Consumer
    int c2p[2]; // Consumer -> Producer

    if (pipe(p2c) != 0 || pipe(c2p) != 0) {
        perror("[ERROR] pipe creation failed");
        return -1;
    }

    size_t total_iterations = config->warmup_iterations + config->measure_iterations;
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] fork failed");
        return -1;
    }

    if (pid == 0) {
        // Consumer process
        close(p2c[1]);
        close(c2p[0]);

        if (aipc_pin_cpu(config->consumer_cpu) != 0) {
            _exit(1);
        }

        uint8_t *echo_buf = (uint8_t *)malloc(config->payload_size);
        if (!echo_buf) _exit(2);

        for (size_t i = 0; i < total_iterations; i++) {
            if (read_all(p2c[0], echo_buf, config->payload_size) != (ssize_t)config->payload_size) {
                free(echo_buf);
                _exit(3);
            }
            if (write_all(c2p[1], echo_buf, config->payload_size) != (ssize_t)config->payload_size) {
                free(echo_buf);
                _exit(4);
            }
        }

        free(echo_buf);
        close(p2c[0]);
        close(c2p[1]);
        _exit(0);
    }

    // Producer process
    close(p2c[0]);
    close(c2p[1]);

    if (aipc_pin_cpu(config->producer_cpu) != 0) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return -1;
    }

    uint8_t *send_buf = (uint8_t *)malloc(config->payload_size);
    uint8_t *recv_buf = (uint8_t *)malloc(config->payload_size);
    if (!send_buf || !recv_buf) {
        free(send_buf);
        free(recv_buf);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return -1;
    }

    aipc_generate_payload(send_buf, config->payload_size, 0x12345678);

    // Warm-up loop
    for (size_t i = 0; i < config->warmup_iterations; i++) {
        if (write_all(p2c[1], send_buf, config->payload_size) != (ssize_t)config->payload_size ||
            read_all(c2p[0], recv_buf, config->payload_size) != (ssize_t)config->payload_size) {
            perror("[ERROR] Pipe warmup communication failure");
            free(send_buf);
            free(recv_buf);
            waitpid(pid, NULL, 0);
            return -1;
        }
    }

    // Measurement loop
    aipc_lat_collector_t col;
    if (aipc_collector_init(&col, config->measure_iterations) != 0) {
        free(send_buf);
        free(recv_buf);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return -1;
    }

    struct rusage ru_before, ru_after;
    getrusage(RUSAGE_SELF, &ru_before);
    uint64_t t_wall_start = aipc_time_ns();

    for (size_t i = 0; i < config->measure_iterations; i++) {
        if (config->validate_data) {
            aipc_generate_payload(send_buf, config->payload_size, (uint32_t)i);
        }

        uint64_t t_start = aipc_time_ns();
        if (write_all(p2c[1], send_buf, config->payload_size) != (ssize_t)config->payload_size ||
            read_all(c2p[0], recv_buf, config->payload_size) != (ssize_t)config->payload_size) {
            perror("[ERROR] Pipe measurement communication failure");
            break;
        }
        uint64_t t_end = aipc_time_ns();

        double lat_ns = (double)(t_end - t_start) / 2.0;
        aipc_collector_record(&col, lat_ns);

        if (config->validate_data) {
            if (!aipc_validate_payload(recv_buf, config->payload_size, (uint32_t)i)) {
                fprintf(stderr, "[ERROR] Pipe data validation failed at iteration %zu\n", i);
                break;
            }
        }
    }

    uint64_t t_wall_end = aipc_time_ns();
    getrusage(RUSAGE_SELF, &ru_after);

    close(p2c[1]);
    close(c2p[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    long vol_csw = ru_after.ru_nvcsw - ru_before.ru_nvcsw;
    long invol_csw = ru_after.ru_nivcsw - ru_before.ru_nivcsw;
    uint64_t total_wall_ns = t_wall_end - t_wall_start;

    aipc_collector_compute(&col, aipc_baseline_name(AIPC_BASELINE_PIPE),
                           config->payload_size, total_wall_ns,
                           vol_csw, invol_csw, res);

    aipc_collector_free(&col);
    free(send_buf);
    free(recv_buf);

    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : -1;
}

/* ========================================================================= */
/* 2. UNIX Domain Socket (AF_UNIX) Implementation                            */
/* ========================================================================= */
static int run_socket_bench(const aipc_bench_config_t *config, aipc_bench_result_t *res) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        perror("[ERROR] socketpair failed");
        return -1;
    }

    size_t total_iterations = config->warmup_iterations + config->measure_iterations;
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] fork failed");
        return -1;
    }

    if (pid == 0) {
        // Consumer process
        close(sv[0]);

        if (aipc_pin_cpu(config->consumer_cpu) != 0) {
            _exit(1);
        }

        uint8_t *echo_buf = (uint8_t *)malloc(config->payload_size);
        if (!echo_buf) _exit(2);

        for (size_t i = 0; i < total_iterations; i++) {
            if (read_all(sv[1], echo_buf, config->payload_size) != (ssize_t)config->payload_size) {
                free(echo_buf);
                _exit(3);
            }
            if (write_all(sv[1], echo_buf, config->payload_size) != (ssize_t)config->payload_size) {
                free(echo_buf);
                _exit(4);
            }
        }

        free(echo_buf);
        close(sv[1]);
        _exit(0);
    }

    // Producer process
    close(sv[1]);

    if (aipc_pin_cpu(config->producer_cpu) != 0) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return -1;
    }

    uint8_t *send_buf = (uint8_t *)malloc(config->payload_size);
    uint8_t *recv_buf = (uint8_t *)malloc(config->payload_size);
    if (!send_buf || !recv_buf) {
        free(send_buf);
        free(recv_buf);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return -1;
    }

    aipc_generate_payload(send_buf, config->payload_size, 0x12345678);

    // Warm-up loop
    for (size_t i = 0; i < config->warmup_iterations; i++) {
        if (write_all(sv[0], send_buf, config->payload_size) != (ssize_t)config->payload_size ||
            read_all(sv[0], recv_buf, config->payload_size) != (ssize_t)config->payload_size) {
            perror("[ERROR] Socket warmup communication failure");
            free(send_buf);
            free(recv_buf);
            waitpid(pid, NULL, 0);
            return -1;
        }
    }

    // Measurement loop
    aipc_lat_collector_t col;
    if (aipc_collector_init(&col, config->measure_iterations) != 0) {
        free(send_buf);
        free(recv_buf);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return -1;
    }

    struct rusage ru_before, ru_after;
    getrusage(RUSAGE_SELF, &ru_before);
    uint64_t t_wall_start = aipc_time_ns();

    for (size_t i = 0; i < config->measure_iterations; i++) {
        if (config->validate_data) {
            aipc_generate_payload(send_buf, config->payload_size, (uint32_t)i);
        }

        uint64_t t_start = aipc_time_ns();
        if (write_all(sv[0], send_buf, config->payload_size) != (ssize_t)config->payload_size ||
            read_all(sv[0], recv_buf, config->payload_size) != (ssize_t)config->payload_size) {
            perror("[ERROR] Socket measurement communication failure");
            break;
        }
        uint64_t t_end = aipc_time_ns();

        double lat_ns = (double)(t_end - t_start) / 2.0;
        aipc_collector_record(&col, lat_ns);

        if (config->validate_data) {
            if (!aipc_validate_payload(recv_buf, config->payload_size, (uint32_t)i)) {
                fprintf(stderr, "[ERROR] Socket data validation failed at iteration %zu\n", i);
                break;
            }
        }
    }

    uint64_t t_wall_end = aipc_time_ns();
    getrusage(RUSAGE_SELF, &ru_after);

    close(sv[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    long vol_csw = ru_after.ru_nvcsw - ru_before.ru_nvcsw;
    long invol_csw = ru_after.ru_nivcsw - ru_before.ru_nivcsw;
    uint64_t total_wall_ns = t_wall_end - t_wall_start;

    aipc_collector_compute(&col, aipc_baseline_name(AIPC_BASELINE_SOCKET),
                           config->payload_size, total_wall_ns,
                           vol_csw, invol_csw, res);

    aipc_collector_free(&col);
    free(send_buf);
    free(recv_buf);

    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : -1;
}

/* ========================================================================= */
/* 3. POSIX Message Queue (mq_open, mq_send, mq_receive) Implementation      */
/* ========================================================================= */
static int run_mq_bench(const aipc_bench_config_t *config, aipc_bench_result_t *res) {
    if (config->payload_size > 8192) {
        printf("   [SKIP] POSIX MQ: Payload %zu B exceeds Linux kernel default limit (fs.mqueue.msgsize_max = 8192 B)\n",
               config->payload_size);
        return -2; // Signal skipped
    }

    char mq_name_p2c[64];
    char mq_name_c2p[64];
    pid_t my_pid = getpid();
    snprintf(mq_name_p2c, sizeof(mq_name_p2c), "/aipc_mq_p2c_%d", (int)my_pid);
    snprintf(mq_name_c2p, sizeof(mq_name_c2p), "/aipc_mq_c2p_%d", (int)my_pid);

    // Clean up any stale queues
    mq_unlink(mq_name_p2c);
    mq_unlink(mq_name_c2p);

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = (long)config->payload_size;

    mqd_t mq_p2c = mq_open(mq_name_p2c, O_CREAT | O_RDWR, 0666, &attr);
    if (mq_p2c == (mqd_t)-1) {
        perror("[ERROR] mq_open p2c failed");
        return -1;
    }

    mqd_t mq_c2p = mq_open(mq_name_c2p, O_CREAT | O_RDWR, 0666, &attr);
    if (mq_c2p == (mqd_t)-1) {
        perror("[ERROR] mq_open c2p failed");
        mq_close(mq_p2c);
        mq_unlink(mq_name_p2c);
        return -1;
    }

    size_t total_iterations = config->warmup_iterations + config->measure_iterations;
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] fork failed");
        mq_close(mq_p2c);
        mq_close(mq_c2p);
        mq_unlink(mq_name_p2c);
        mq_unlink(mq_name_c2p);
        return -1;
    }

    if (pid == 0) {
        // Consumer process
        if (aipc_pin_cpu(config->consumer_cpu) != 0) {
            _exit(1);
        }

        uint8_t *echo_buf = (uint8_t *)malloc(config->payload_size);
        if (!echo_buf) _exit(2);

        for (size_t i = 0; i < total_iterations; i++) {
            ssize_t n = mq_receive(mq_p2c, (char *)echo_buf, config->payload_size, NULL);
            if (n < 0) {
                free(echo_buf);
                _exit(3);
            }
            if (mq_send(mq_c2p, (const char *)echo_buf, config->payload_size, 0) != 0) {
                free(echo_buf);
                _exit(4);
            }
        }

        free(echo_buf);
        mq_close(mq_p2c);
        mq_close(mq_c2p);
        _exit(0);
    }

    // Producer process
    if (aipc_pin_cpu(config->producer_cpu) != 0) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        mq_close(mq_p2c);
        mq_close(mq_c2p);
        mq_unlink(mq_name_p2c);
        mq_unlink(mq_name_c2p);
        return -1;
    }

    uint8_t *send_buf = (uint8_t *)malloc(config->payload_size);
    uint8_t *recv_buf = (uint8_t *)malloc(config->payload_size);
    if (!send_buf || !recv_buf) {
        free(send_buf);
        free(recv_buf);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        mq_close(mq_p2c);
        mq_close(mq_c2p);
        mq_unlink(mq_name_p2c);
        mq_unlink(mq_name_c2p);
        return -1;
    }

    aipc_generate_payload(send_buf, config->payload_size, 0x12345678);

    // Warm-up loop
    for (size_t i = 0; i < config->warmup_iterations; i++) {
        if (mq_send(mq_p2c, (const char *)send_buf, config->payload_size, 0) != 0 ||
            mq_receive(mq_c2p, (char *)recv_buf, config->payload_size, NULL) < 0) {
            perror("[ERROR] POSIX MQ warmup communication failure");
            break;
        }
    }

    // Measurement loop
    aipc_lat_collector_t col;
    if (aipc_collector_init(&col, config->measure_iterations) != 0) {
        free(send_buf);
        free(recv_buf);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        mq_close(mq_p2c);
        mq_close(mq_c2p);
        mq_unlink(mq_name_p2c);
        mq_unlink(mq_name_c2p);
        return -1;
    }

    struct rusage ru_before, ru_after;
    getrusage(RUSAGE_SELF, &ru_before);
    uint64_t t_wall_start = aipc_time_ns();

    for (size_t i = 0; i < config->measure_iterations; i++) {
        if (config->validate_data) {
            aipc_generate_payload(send_buf, config->payload_size, (uint32_t)i);
        }

        uint64_t t_start = aipc_time_ns();
        if (mq_send(mq_p2c, (const char *)send_buf, config->payload_size, 0) != 0 ||
            mq_receive(mq_c2p, (char *)recv_buf, config->payload_size, NULL) < 0) {
            perror("[ERROR] POSIX MQ measurement communication failure");
            break;
        }
        uint64_t t_end = aipc_time_ns();

        double lat_ns = (double)(t_end - t_start) / 2.0;
        aipc_collector_record(&col, lat_ns);

        if (config->validate_data) {
            if (!aipc_validate_payload(recv_buf, config->payload_size, (uint32_t)i)) {
                fprintf(stderr, "[ERROR] POSIX MQ data validation failed at iteration %zu\n", i);
                break;
            }
        }
    }

    uint64_t t_wall_end = aipc_time_ns();
    getrusage(RUSAGE_SELF, &ru_after);

    mq_close(mq_p2c);
    mq_close(mq_c2p);
    mq_unlink(mq_name_p2c);
    mq_unlink(mq_name_c2p);

    int status = 0;
    waitpid(pid, &status, 0);

    long vol_csw = ru_after.ru_nvcsw - ru_before.ru_nvcsw;
    long invol_csw = ru_after.ru_nivcsw - ru_before.ru_nivcsw;
    uint64_t total_wall_ns = t_wall_end - t_wall_start;

    aipc_collector_compute(&col, aipc_baseline_name(AIPC_BASELINE_MQ),
                           config->payload_size, total_wall_ns,
                           vol_csw, invol_csw, res);

    aipc_collector_free(&col);
    free(send_buf);
    free(recv_buf);

    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : -1;
}

/* ========================================================================= */
/* 4. Naive Shared Memory with Futex (Unpadded, False-Sharing Baseline)      */
/* ========================================================================= */
static int run_naive_shm_bench(const aipc_bench_config_t *config, aipc_bench_result_t *res) {
    if (config->payload_size > NAIVE_SHM_MAX_PAYLOAD) {
        fprintf(stderr, "[ERROR] Payload size %zu exceeds naive SHM max payload %d\n",
                config->payload_size, NAIVE_SHM_MAX_PAYLOAD);
        return -1;
    }

    char shm_name[64];
    pid_t my_pid = getpid();
    snprintf(shm_name, sizeof(shm_name), "/aipc_naive_shm_%d", (int)my_pid);

    shm_unlink(shm_name);

    int fd = shm_open(shm_name, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("[ERROR] shm_open failed");
        return -1;
    }

    if (ftruncate(fd, sizeof(naive_shm_channel_t)) != 0) {
        perror("[ERROR] ftruncate failed");
        close(fd);
        shm_unlink(shm_name);
        return -1;
    }

    naive_shm_channel_t *shm = (naive_shm_channel_t *)mmap(
        NULL, sizeof(naive_shm_channel_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        perror("[ERROR] mmap failed");
        close(fd);
        shm_unlink(shm_name);
        return -1;
    }

    memset((void *)shm, 0, sizeof(naive_shm_channel_t));

    size_t total_iterations = config->warmup_iterations + config->measure_iterations;
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] fork failed");
        munmap(shm, sizeof(naive_shm_channel_t));
        close(fd);
        shm_unlink(shm_name);
        return -1;
    }

    if (pid == 0) {
        // Consumer process
        if (aipc_pin_cpu(config->consumer_cpu) != 0) {
            _exit(1);
        }

        for (size_t i = 0; i < total_iterations; i++) {
            // Wait for producer
            while (__atomic_load_n(&shm->p2c_ready, __ATOMIC_ACQUIRE) == 0) {
                syscall(SYS_futex, (int *)&shm->p2c_ready, FUTEX_WAIT, 0, NULL, NULL, 0);
            }
            __atomic_store_n(&shm->p2c_ready, 0, __ATOMIC_RELEASE);

            // In echo test: read message and signal back
            // Signal back to producer
            __atomic_store_n(&shm->c2p_ready, 1, __ATOMIC_RELEASE);
            syscall(SYS_futex, (int *)&shm->c2p_ready, FUTEX_WAKE, 1, NULL, NULL, 0);
        }

        munmap(shm, sizeof(naive_shm_channel_t));
        close(fd);
        _exit(0);
    }

    // Producer process
    if (aipc_pin_cpu(config->producer_cpu) != 0) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        munmap(shm, sizeof(naive_shm_channel_t));
        close(fd);
        shm_unlink(shm_name);
        return -1;
    }

    uint8_t *send_buf = (uint8_t *)malloc(config->payload_size);
    uint8_t *recv_buf = (uint8_t *)malloc(config->payload_size);
    if (!send_buf || !recv_buf) {
        free(send_buf);
        free(recv_buf);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        munmap(shm, sizeof(naive_shm_channel_t));
        close(fd);
        shm_unlink(shm_name);
        return -1;
    }

    aipc_generate_payload(send_buf, config->payload_size, 0x12345678);

    // Warm-up loop
    for (size_t i = 0; i < config->warmup_iterations; i++) {
        memcpy((void *)shm->buffer, send_buf, config->payload_size);
        shm->len = (uint32_t)config->payload_size;

        __atomic_store_n(&shm->p2c_ready, 1, __ATOMIC_RELEASE);
        syscall(SYS_futex, (int *)&shm->p2c_ready, FUTEX_WAKE, 1, NULL, NULL, 0);

        while (__atomic_load_n(&shm->c2p_ready, __ATOMIC_ACQUIRE) == 0) {
            syscall(SYS_futex, (int *)&shm->c2p_ready, FUTEX_WAIT, 0, NULL, NULL, 0);
        }
        __atomic_store_n(&shm->c2p_ready, 0, __ATOMIC_RELEASE);

        memcpy(recv_buf, (const void *)shm->buffer, config->payload_size);
    }

    // Measurement loop
    aipc_lat_collector_t col;
    if (aipc_collector_init(&col, config->measure_iterations) != 0) {
        free(send_buf);
        free(recv_buf);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        munmap(shm, sizeof(naive_shm_channel_t));
        close(fd);
        shm_unlink(shm_name);
        return -1;
    }

    struct rusage ru_before, ru_after;
    getrusage(RUSAGE_SELF, &ru_before);
    uint64_t t_wall_start = aipc_time_ns();

    for (size_t i = 0; i < config->measure_iterations; i++) {
        if (config->validate_data) {
            aipc_generate_payload(send_buf, config->payload_size, (uint32_t)i);
        }

        uint64_t t_start = aipc_time_ns();

        memcpy((void *)shm->buffer, send_buf, config->payload_size);
        shm->len = (uint32_t)config->payload_size;

        __atomic_store_n(&shm->p2c_ready, 1, __ATOMIC_RELEASE);
        syscall(SYS_futex, (int *)&shm->p2c_ready, FUTEX_WAKE, 1, NULL, NULL, 0);

        while (__atomic_load_n(&shm->c2p_ready, __ATOMIC_ACQUIRE) == 0) {
            syscall(SYS_futex, (int *)&shm->c2p_ready, FUTEX_WAIT, 0, NULL, NULL, 0);
        }
        __atomic_store_n(&shm->c2p_ready, 0, __ATOMIC_RELEASE);

        memcpy(recv_buf, (const void *)shm->buffer, config->payload_size);

        uint64_t t_end = aipc_time_ns();

        double lat_ns = (double)(t_end - t_start) / 2.0;
        aipc_collector_record(&col, lat_ns);

        if (config->validate_data) {
            if (!aipc_validate_payload(recv_buf, config->payload_size, (uint32_t)i)) {
                fprintf(stderr, "[ERROR] Naive SHM data validation failed at iteration %zu\n", i);
                break;
            }
        }
    }

    uint64_t t_wall_end = aipc_time_ns();
    getrusage(RUSAGE_SELF, &ru_after);

    int status = 0;
    waitpid(pid, &status, 0);

    munmap(shm, sizeof(naive_shm_channel_t));
    close(fd);
    shm_unlink(shm_name);

    long vol_csw = ru_after.ru_nvcsw - ru_before.ru_nvcsw;
    long invol_csw = ru_after.ru_nivcsw - ru_before.ru_nivcsw;
    uint64_t total_wall_ns = t_wall_end - t_wall_start;

    aipc_collector_compute(&col, aipc_baseline_name(AIPC_BASELINE_NAIVE_SHM),
                           config->payload_size, total_wall_ns,
                           vol_csw, invol_csw, res);

    aipc_collector_free(&col);
    free(send_buf);
    free(recv_buf);

    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : -1;
}

int aipc_run_baseline_benchmark(const aipc_bench_config_t *config, void *out_result) {
    if (!config || !out_result) return -1;
    aipc_bench_result_t *res = (aipc_bench_result_t *)out_result;

    switch (config->transport) {
        case AIPC_BASELINE_PIPE:
            return run_pipe_bench(config, res);
        case AIPC_BASELINE_SOCKET:
            return run_socket_bench(config, res);
        case AIPC_BASELINE_MQ:
            return run_mq_bench(config, res);
        case AIPC_BASELINE_NAIVE_SHM:
            return run_naive_shm_bench(config, res);
        default:
            fprintf(stderr, "[ERROR] Unsupported baseline transport: %d\n", config->transport);
            return -1;
    }
}
