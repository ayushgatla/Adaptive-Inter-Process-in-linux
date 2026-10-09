#ifndef AIPC_BASELINES_H
#define AIPC_BASELINES_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AIPC_BASELINE_PIPE = 0,
    AIPC_BASELINE_SOCKET,
    AIPC_BASELINE_MQ,
    AIPC_BASELINE_NAIVE_SHM,
    AIPC_BASELINE_COUNT
} aipc_baseline_type_t;

const char *aipc_baseline_name(aipc_baseline_type_t type);
aipc_baseline_type_t aipc_baseline_from_string(const char *str);

/**
 * @brief Benchmark run configuration.
 */
typedef struct {
    aipc_baseline_type_t transport;
    size_t payload_size;
    size_t warmup_iterations;
    size_t measure_iterations;
    int    producer_cpu;
    int    consumer_cpu;
    bool   validate_data;
} aipc_bench_config_t;

/**
 * @brief Forward declaration of benchmark result from aipc_bench.h
 */
struct aipc_bench_result;

/**
 * @brief Execute an isolated 1:1 echo ping-pong benchmark for the chosen baseline.
 *
 * Spawns a child process (Consumer) pinned to consumer_cpu, while the parent
 * (Producer) pins to producer_cpu. Measures round-trip time (RTT), calculates
 * one-way latency as RTT / 2, and populates out_result.
 *
 * @param config Benchmark configuration
 * @param out_result Output benchmark metrics container
 * @return 0 on success, negative error code on failure
 */
int aipc_run_baseline_benchmark(const aipc_bench_config_t *config,
                                void *out_result);

#ifdef __cplusplus
}
#endif

#endif // AIPC_BASELINES_H
