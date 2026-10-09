#ifndef AIPC_BENCH_H
#define AIPC_BENCH_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <sched.h>
#include <sys/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief High-precision raw monotonic clock reading in nanoseconds.
 *        CLOCK_MONOTONIC_RAW is unaffected by NTP frequency adjustments.
 */
static inline uint64_t aipc_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Pin calling thread/process to a specific CPU core.
 * @param cpu_core Logical CPU core index (0 to num_cpus-1)
 * @return 0 on success, -1 on failure
 */
int aipc_pin_cpu(int cpu_core);

/**
 * @brief Benchmark statistical result container.
 */
typedef struct {
    const char *transport_name;
    size_t      payload_size;
    size_t      iterations;
    double      min_lat_ns;
    double      max_lat_ns;
    double      mean_lat_ns;
    double      p50_lat_ns;
    double      p90_lat_ns;
    double      p99_lat_ns;
    double      p999_lat_ns;
    double      throughput_mps;   // Messages per second
    double      throughput_mbps;  // Megabytes per second (MiB/s)
    long        voluntary_csw;    // ru_nvcsw
    long        involuntary_csw;  // ru_nivcsw
    uint64_t    total_wall_ns;
} aipc_bench_result_t;

/**
 * @brief Dynamic latency sample collector for calculating exact percentiles.
 */
typedef struct {
    double *samples;  // One-way latency samples in nanoseconds (RTT / 2.0)
    size_t  capacity;
    size_t  count;
} aipc_lat_collector_t;

int  aipc_collector_init(aipc_lat_collector_t *col, size_t capacity);
void aipc_collector_record(aipc_lat_collector_t *col, double lat_ns);
void aipc_collector_compute(aipc_lat_collector_t *col,
                            const char *transport_name,
                            size_t payload_size,
                            uint64_t total_wall_ns,
                            long vol_csw,
                            long invol_csw,
                            aipc_bench_result_t *out_result);
void aipc_collector_free(aipc_lat_collector_t *col);

/**
 * @brief Formatting and printing utilities for benchmark outputs.
 */
void aipc_print_header(void);
void aipc_print_result_row(const aipc_bench_result_t *res);
void aipc_print_danyliuk_comparison_row(const aipc_bench_result_t *res);
void aipc_export_csv_header(FILE *fp);
void aipc_export_csv_row(FILE *fp, const aipc_bench_result_t *res);

/**
 * @brief Payload generation and validation for zero-corruption data integrity check.
 */
void aipc_generate_payload(uint8_t *buf, size_t len, uint32_t seq);
bool aipc_validate_payload(const uint8_t *buf, size_t len, uint32_t expected_seq);

#ifdef __cplusplus
}
#endif

#endif // AIPC_BENCH_H
