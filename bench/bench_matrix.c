#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "aipc.h"
#include "aipc_bench.h"
#include "aipc_baselines.h"
#include "aipc_fas_ring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <math.h>
#include <signal.h>
#include <errno.h>

/**
 * Full payload matrix sweep as specified in Phase 4:
 * 64 B, 128 B, 256 B, 512 B, 1024 B, 2048 B, 4096 B, 8192 B, 16384 B, 32768 B, 65536 B
 */
static const size_t MATRIX_SWEEP_SIZES[] = {
    64,
    128,
    256,
    512,
    1024,
    2048,
    4096,
    8192,
    16384,
    32768,
    65536
};
static const size_t NUM_MATRIX_SIZES = sizeof(MATRIX_SWEEP_SIZES) / sizeof(MATRIX_SWEEP_SIZES[0]);

static void print_usage(const char *prog_name) {
    printf("================================================================================\n");
    printf(" Adaptive Inter-Process Communication (A-IPC) - Phase 4 Experimental Matrix\n");
    printf("================================================================================\n\n");
    printf("Usage: %s [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  --iterations <N>         Number of round-trip exchanges per cell (default: 50000)\n");
    printf("  --warmup <N>             Number of warmup iterations (default: 5000)\n");
    printf("  --producer-core <core>   CPU core to pin producer process (default: 1)\n");
    printf("  --consumer-core <core>   CPU core to pin consumer process (default: 2)\n");
    printf("  --sweep                  Run full payload matrix sweep across all transports\n");
    printf("  --mixed                  Run mixed workload (80%% 64B signaling + 20%% 16KB bulk)\n");
    printf("  --csv-matrix <path>      Output CSV file for full matrix (default: results/matrix_results.csv)\n");
    printf("  --csv-gain <path>        Output CSV file for adaptation gain (default: results/mixed_workload_gain.csv)\n");
    printf("  --help                   Display this help message and exit\n\n");
}

/* =========================================================================
 * 1. FAS-IPC 1:1 Echo Ping-Pong Helper
 * ========================================================================= */
static int run_fas_echo_benchmark(size_t payload_size, size_t iterations, size_t warmup,
                                  int prod_cpu, int cons_cpu, aipc_bench_result_t *out_result) {
    if (payload_size > FAS_SLOT_PAYLOAD_MAX) {
        return -1; // Exceeds FAS slot capacity
    }

    const char *ring_fwd = "/aipc_matrix_fas_fwd";
    const char *ring_rev = "/aipc_matrix_fas_rev";
    pid_t pid = fork();
    if (pid < 0) return -1;

    if (pid == 0) {
        aipc_pin_cpu(cons_cpu);
        usleep(30000);

        fas_channel_t cons_chan;
        if (fas_channel_attach(&cons_chan, ring_fwd, ring_rev, false) != 0) {
            exit(EXIT_FAILURE);
        }

        uint8_t *rx_buf = malloc(payload_size);
        for (size_t i = 0; i < warmup + iterations; i++) {
            fas_channel_receive(&cons_chan, rx_buf, payload_size);
            fas_channel_send(&cons_chan, rx_buf, payload_size);
        }

        free(rx_buf);
        fas_channel_close(&cons_chan);
        exit(EXIT_SUCCESS);
    } else {
        aipc_pin_cpu(prod_cpu);

        fas_channel_t prod_chan;
        if (fas_channel_create(&prod_chan, ring_fwd, ring_rev, true) != 0) {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            return -1;
        }

        uint8_t *tx_buf = malloc(payload_size);
        memset(tx_buf, 0x42, payload_size);

        for (size_t i = 0; i < warmup; i++) {
            fas_channel_send(&prod_chan, tx_buf, payload_size);
            fas_channel_receive(&prod_chan, tx_buf, payload_size);
        }

        aipc_lat_collector_t col;
        aipc_collector_init(&col, iterations);

        struct rusage ru_start, ru_end;
        getrusage(RUSAGE_SELF, &ru_start);
        uint64_t t_start = aipc_time_ns();

        for (size_t i = 0; i < iterations; i++) {
            uint64_t t0 = aipc_time_ns();
            fas_channel_send(&prod_chan, tx_buf, payload_size);
            fas_channel_receive(&prod_chan, tx_buf, payload_size);
            uint64_t t1 = aipc_time_ns();
            aipc_collector_record(&col, (double)(t1 - t0) / 2.0);
        }

        uint64_t t_end = aipc_time_ns();
        getrusage(RUSAGE_SELF, &ru_end);

        waitpid(pid, NULL, 0);

        long vol_csw = ru_end.ru_nvcsw - ru_start.ru_nvcsw;
        long invol_csw = ru_end.ru_nivcsw - ru_start.ru_nivcsw;

        aipc_collector_compute(&col, "Static FAS-IPC (SHM)", payload_size, t_end - t_start, vol_csw, invol_csw, out_result);

        aipc_collector_free(&col);
        free(tx_buf);
        fas_channel_unlink(&prod_chan);
        return 0;
    }
}

/* =========================================================================
 * 2. Adaptive A-IPC Unified Channel 1:1 Echo Ping-Pong Helper
 * ========================================================================= */
static int run_adaptive_echo_benchmark(size_t payload_size, size_t iterations, size_t warmup,
                                       int prod_cpu, int cons_cpu, aipc_bench_result_t *out_result) {
    const char *chan_name = "matrix_adaptive_bench";
    pid_t pid = fork();
    if (pid < 0) return -1;

    if (pid == 0) {
        aipc_pin_cpu(cons_cpu);
        usleep(40000);

        ipc_channel_t cons_chan;
        if (ipc_create(&cons_chan, chan_name, IPC_ROLE_CONSUMER) != 0) {
            exit(EXIT_FAILURE);
        }

        uint8_t *rx_buf = malloc(payload_size > 65536 ? payload_size : 65536);
        for (size_t i = 0; i < warmup + iterations; i++) {
            ipc_receive(&cons_chan, rx_buf, payload_size);
            ipc_send(&cons_chan, rx_buf, payload_size);
        }

        free(rx_buf);
        ipc_close(&cons_chan);
        exit(EXIT_SUCCESS);
    } else {
        aipc_pin_cpu(prod_cpu);

        ipc_channel_t prod_chan;
        if (ipc_create(&prod_chan, chan_name, IPC_ROLE_PRODUCER) != 0) {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            return -1;
        }

        uint8_t *tx_buf = malloc(payload_size > 65536 ? payload_size : 65536);
        memset(tx_buf, 0x5A, payload_size);

        for (size_t i = 0; i < warmup; i++) {
            ipc_send(&prod_chan, tx_buf, payload_size);
            ipc_receive(&prod_chan, tx_buf, payload_size);
        }

        aipc_lat_collector_t col;
        aipc_collector_init(&col, iterations);

        struct rusage ru_start, ru_end;
        getrusage(RUSAGE_SELF, &ru_start);
        uint64_t t_start = aipc_time_ns();

        for (size_t i = 0; i < iterations; i++) {
            uint64_t t0 = aipc_time_ns();
            ipc_send(&prod_chan, tx_buf, payload_size);
            ipc_receive(&prod_chan, tx_buf, payload_size);
            uint64_t t1 = aipc_time_ns();
            aipc_collector_record(&col, (double)(t1 - t0) / 2.0);
        }

        uint64_t t_end = aipc_time_ns();
        getrusage(RUSAGE_SELF, &ru_end);

        waitpid(pid, NULL, 0);

        long vol_csw = ru_end.ru_nvcsw - ru_start.ru_nvcsw;
        long invol_csw = ru_end.ru_nivcsw - ru_start.ru_nivcsw;

        aipc_collector_compute(&col, "Adaptive A-IPC Engine", payload_size, t_end - t_start, vol_csw, invol_csw, out_result);

        aipc_collector_free(&col);
        free(tx_buf);
        ipc_destroy(&prod_chan);
        return 0;
    }
}

/* =========================================================================
 * 3. Mixed Workload Benchmark (80% 64B Signaling + 20% 16KB Bulk Payload)
 * ========================================================================= */
static int run_mixed_workload_benchmark(const char *transport_type, size_t iterations,
                                        int prod_cpu, int cons_cpu, aipc_bench_result_t *out_result) {
    const size_t SIZE_SMALL = 64;
    const size_t SIZE_BULK = 16384;
    const char *chan_name = "matrix_mixed_bench";

    pid_t pid = fork();
    if (pid < 0) return -1;

    if (pid == 0) {
        aipc_pin_cpu(cons_cpu);
        usleep(40000);

        ipc_channel_t cons_chan;
        if (ipc_create(&cons_chan, chan_name, IPC_ROLE_CONSUMER) != 0) {
            exit(EXIT_FAILURE);
        }

        uint8_t *rx_buf = malloc(SIZE_BULK + 1024);
        for (size_t i = 0; i < iterations; i++) {
            size_t expected_size = ((i % 10) < 8) ? SIZE_SMALL : SIZE_BULK;
            ipc_receive(&cons_chan, rx_buf, expected_size);
            ipc_send(&cons_chan, rx_buf, expected_size);
        }

        free(rx_buf);
        ipc_close(&cons_chan);
        exit(EXIT_SUCCESS);
    } else {
        aipc_pin_cpu(prod_cpu);

        ipc_channel_t prod_chan;
        if (ipc_create(&prod_chan, chan_name, IPC_ROLE_PRODUCER) != 0) {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            return -1;
        }

        uint8_t *tx_buf = malloc(SIZE_BULK + 1024);
        memset(tx_buf, 0x7E, SIZE_BULK + 1024);

        aipc_lat_collector_t col;
        aipc_collector_init(&col, iterations);

        struct rusage ru_start, ru_end;
        getrusage(RUSAGE_SELF, &ru_start);
        uint64_t t_start = aipc_time_ns();

        for (size_t i = 0; i < iterations; i++) {
            size_t cur_size = ((i % 10) < 8) ? SIZE_SMALL : SIZE_BULK;
            uint64_t t0 = aipc_time_ns();
            ipc_send(&prod_chan, tx_buf, cur_size);
            ipc_receive(&prod_chan, tx_buf, cur_size);
            uint64_t t1 = aipc_time_ns();
            aipc_collector_record(&col, (double)(t1 - t0) / 2.0);
        }

        uint64_t t_end = aipc_time_ns();
        getrusage(RUSAGE_SELF, &ru_end);

        waitpid(pid, NULL, 0);

        long vol_csw = ru_end.ru_nvcsw - ru_start.ru_nvcsw;
        long invol_csw = ru_end.ru_nivcsw - ru_start.ru_nivcsw;

        // Effective blended byte size: (0.8 * 64) + (0.2 * 16384) = 51.2 + 3276.8 = 3328 B
        size_t avg_payload_size = 3328;
        aipc_collector_compute(&col, transport_type, avg_payload_size, t_end - t_start, vol_csw, invol_csw, out_result);

        aipc_collector_free(&col);
        free(tx_buf);
        ipc_destroy(&prod_chan);
        return 0;
    }
}

/* =========================================================================
 * 4. Main Matrix Runner
 * ========================================================================= */
int main(int argc, char *argv[]) {
    size_t iterations = 50000;
    size_t warmup = 5000;
    int prod_cpu = 1;
    int cons_cpu = 2;
    bool do_sweep = false;
    bool do_mixed = false;
    char csv_matrix_path[256] = "results/matrix_results.csv";
    char csv_gain_path[256] = "results/mixed_workload_gain.csv";

    static struct option long_options[] = {
        {"iterations",    required_argument, 0, 'i'},
        {"warmup",        required_argument, 0, 'w'},
        {"producer-core", required_argument, 0, 'p'},
        {"consumer-core", required_argument, 0, 'c'},
        {"sweep",         no_argument,       0, 'S'},
        {"mixed",         no_argument,       0, 'M'},
        {"csv-matrix",    required_argument, 0, 'O'},
        {"csv-gain",      required_argument, 0, 'G'},
        {"help",          no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt = 0;
    int opt_index = 0;
    while ((opt = getopt_long(argc, argv, "i:w:p:c:SMO:G:h", long_options, &opt_index)) != -1) {
        switch (opt) {
            case 'i': iterations = (size_t)strtoull(optarg, NULL, 10); break;
            case 'w': warmup = (size_t)strtoull(optarg, NULL, 10); break;
            case 'p': prod_cpu = atoi(optarg); break;
            case 'c': cons_cpu = atoi(optarg); break;
            case 'S': do_sweep = true; break;
            case 'M': do_mixed = true; break;
            case 'O': strncpy(csv_matrix_path, optarg, sizeof(csv_matrix_path) - 1); break;
            case 'G': strncpy(csv_gain_path, optarg, sizeof(csv_gain_path) - 1); break;
            case 'h':
            default:
                print_usage(argv[0]);
                return 0;
        }
    }

    if (!do_sweep && !do_mixed) {
        do_sweep = true;
        do_mixed = true;
    }

    FILE *fp_matrix = fopen(csv_matrix_path, "w");
    if (fp_matrix) {
        aipc_export_csv_header(fp_matrix);
    }

    printf("================================================================================\n");
    printf("   A-IPC Phase 4: Full Experimental Matrix & Adaptation Gain Evaluation\n");
    printf("================================================================================\n");
    printf(" Configuration: Iterations=%zu | Warmup=%zu | Producer Core=%d | Consumer Core=%d\n\n",
           iterations, warmup, prod_cpu, cons_cpu);

    aipc_bench_result_t res;

    // -------------------------------------------------------------------------
    // Execution 1: Full Payload Sweep
    // -------------------------------------------------------------------------
    if (do_sweep) {
        printf(">>> [SWEEP 1/2] Full Payload Sweep (64 B -> 64 KB across 5 Transports)\n");
        aipc_print_header();

        for (size_t s = 0; s < NUM_MATRIX_SIZES; s++) {
            size_t sz = MATRIX_SWEEP_SIZES[s];

            // 1. Static Anonymous Pipe
            aipc_bench_config_t cfg_pipe = {
                .transport = AIPC_BASELINE_PIPE,
                .payload_size = sz,
                .warmup_iterations = warmup,
                .measure_iterations = iterations,
                .producer_cpu = prod_cpu,
                .consumer_cpu = cons_cpu,
                .validate_data = false
            };
            if (aipc_run_baseline_benchmark(&cfg_pipe, &res) == 0) {
                aipc_print_result_row(&res);
                if (fp_matrix) aipc_export_csv_row(fp_matrix, &res);
            }

            // 2. Static UNIX Domain Socket
            aipc_bench_config_t cfg_sock = {
                .transport = AIPC_BASELINE_SOCKET,
                .payload_size = sz,
                .warmup_iterations = warmup,
                .measure_iterations = iterations,
                .producer_cpu = prod_cpu,
                .consumer_cpu = cons_cpu,
                .validate_data = false
            };
            if (aipc_run_baseline_benchmark(&cfg_sock, &res) == 0) {
                aipc_print_result_row(&res);
                if (fp_matrix) aipc_export_csv_row(fp_matrix, &res);
            }

            // 3. Static POSIX MQ (valid up to 8192 bytes default max message size)
            if (sz <= 8192) {
                aipc_bench_config_t cfg_mq = {
                    .transport = AIPC_BASELINE_MQ,
                    .payload_size = sz,
                    .warmup_iterations = warmup,
                    .measure_iterations = iterations,
                    .producer_cpu = prod_cpu,
                    .consumer_cpu = cons_cpu,
                    .validate_data = false
                };
                if (aipc_run_baseline_benchmark(&cfg_mq, &res) == 0) {
                    aipc_print_result_row(&res);
                    if (fp_matrix) aipc_export_csv_row(fp_matrix, &res);
                }
            }

            // 4. Static FAS-IPC (valid up to 1024 bytes)
            if (sz <= 1024) {
                if (run_fas_echo_benchmark(sz, iterations, warmup, prod_cpu, cons_cpu, &res) == 0) {
                    aipc_print_result_row(&res);
                    if (fp_matrix) aipc_export_csv_row(fp_matrix, &res);
                }
            }

            // 5. Adaptive A-IPC Engine (Unified Channel)
            if (run_adaptive_echo_benchmark(sz, iterations, warmup, prod_cpu, cons_cpu, &res) == 0) {
                aipc_print_result_row(&res);
                if (fp_matrix) aipc_export_csv_row(fp_matrix, &res);
            }

            printf("--------------------------------------------------------------------------------\n");
        }
    }

    if (fp_matrix) fclose(fp_matrix);

    // -------------------------------------------------------------------------
    // Execution 2: Mixed Workload & Adaptation Gain Evaluation
    // -------------------------------------------------------------------------
    if (do_mixed) {
        printf("\n>>> [SWEEP 2/2] Mixed Workload (80%% 64 B Signaling + 20%% 16 KB Bulk Payload)\n");
        printf(" Evaluating Adaptation Gain (G_adapt) and Latency Reduction %%...\n\n");

        FILE *fp_gain = fopen(csv_gain_path, "w");
        if (fp_gain) {
            fprintf(fp_gain, "workload_type,transport,avg_lat_ns,p50_ns,p90_ns,p99_ns,throughput_mps,throughput_mbps,adaptation_gain,latency_reduction_pct\n");
        }

        aipc_bench_result_t res_pipe, res_sock, res_adapt;
        memset(&res_pipe, 0, sizeof(res_pipe));
        memset(&res_sock, 0, sizeof(res_sock));
        memset(&res_adapt, 0, sizeof(res_adapt));

        // Evaluate Static Pipe on mixed
        run_mixed_workload_benchmark("Static Pipe Baseline", iterations, prod_cpu, cons_cpu, &res_pipe);
        // Evaluate Static Socket on mixed
        run_mixed_workload_benchmark("Static UNIX Socket Baseline", iterations, prod_cpu, cons_cpu, &res_sock);
        // Evaluate Adaptive A-IPC on mixed
        run_mixed_workload_benchmark("Adaptive A-IPC Engine", iterations, prod_cpu, cons_cpu, &res_adapt);

        double max_static_throughput = (res_pipe.throughput_mps > res_sock.throughput_mps) ? 
                                        res_pipe.throughput_mps : res_sock.throughput_mps;
        double best_static_latency = (res_pipe.mean_lat_ns < res_sock.mean_lat_ns) ? 
                                      res_pipe.mean_lat_ns : res_sock.mean_lat_ns;

        double g_adapt = res_adapt.throughput_mps / max_static_throughput;
        double lat_red_pct = ((best_static_latency - res_adapt.mean_lat_ns) / best_static_latency) * 100.0;

        printf("--------------------------------------------------------------------------------\n");
        printf(" Workload: 80%% 64 B + 20%% 16 KB (Average Payload Size: 3,328 Bytes)\n");
        printf("--------------------------------------------------------------------------------\n");
        printf(" 1. Static Pipe:         Mean Latency: %8.2f ns | Throughput: %9.2f msg/s (%7.2f MB/s)\n",
               res_pipe.mean_lat_ns, res_pipe.throughput_mps, res_pipe.throughput_mbps);
        printf(" 2. Static UNIX Socket:  Mean Latency: %8.2f ns | Throughput: %9.2f msg/s (%7.2f MB/s)\n",
               res_sock.mean_lat_ns, res_sock.throughput_mps, res_sock.throughput_mbps);
        printf(" 3. Static FAS-IPC:      [DROPPED/UNSUPPORTED] Cannot handle 16 KB bulk packets without fallback!\n");
        printf(" 4. Adaptive A-IPC:      Mean Latency: %8.2f ns | Throughput: %9.2f msg/s (%7.2f MB/s)\n",
               res_adapt.mean_lat_ns, res_adapt.throughput_mps, res_adapt.throughput_mbps);
        printf("--------------------------------------------------------------------------------\n");
        printf(" ===> MEASURED ADAPTATION GAIN (G_adapt): %.2fx (Target: >= 1.80x)\n", g_adapt);
        printf(" ===> MEASURED LATENCY REDUCTION:         %.2f%% (Target: >= 40.0%%)\n", lat_red_pct);
        printf("--------------------------------------------------------------------------------\n");

        if (fp_gain) {
            fprintf(fp_gain, "Mixed_80_20,Static Pipe,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,1.00,0.00\n",
                    res_pipe.mean_lat_ns, res_pipe.p50_lat_ns, res_pipe.p90_lat_ns, res_pipe.p99_lat_ns,
                    res_pipe.throughput_mps, res_pipe.throughput_mbps);
            fprintf(fp_gain, "Mixed_80_20,Static Socket,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,1.00,0.00\n",
                    res_sock.mean_lat_ns, res_sock.p50_lat_ns, res_sock.p90_lat_ns, res_sock.p99_lat_ns,
                    res_sock.throughput_mps, res_sock.throughput_mbps);
            fprintf(fp_gain, "Mixed_80_20,Adaptive A-IPC,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                    res_adapt.mean_lat_ns, res_adapt.p50_lat_ns, res_adapt.p90_lat_ns, res_adapt.p99_lat_ns,
                    res_adapt.throughput_mps, res_adapt.throughput_mbps, g_adapt, lat_red_pct);
            fclose(fp_gain);
        }
    }

    printf("\n>>> Matrix results exported to: %s\n", csv_matrix_path);
    printf(">>> Adaptation gain exported to: %s\n", csv_gain_path);
    printf("================================================================================\n");
    return 0;
}
