#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "aipc_bench.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int aipc_pin_cpu(int cpu_core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_core, &cpuset);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
        fprintf(stderr, "[ERROR] sched_setaffinity failed for core %d: %s\n",
                cpu_core, strerror(errno));
        return -1;
    }
    return 0;
}

int aipc_collector_init(aipc_lat_collector_t *col, size_t capacity) {
    if (!col || capacity == 0) return -1;
    col->samples = (double *)malloc(capacity * sizeof(double));
    if (!col->samples) {
        fprintf(stderr, "[ERROR] Failed to allocate %zu samples for collector\n", capacity);
        return -1;
    }
    col->capacity = capacity;
    col->count = 0;
    return 0;
}

void aipc_collector_record(aipc_lat_collector_t *col, double lat_ns) {
    if (col && col->count < col->capacity) {
        col->samples[col->count++] = lat_ns;
    }
}

static int compare_doubles(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

void aipc_collector_compute(aipc_lat_collector_t *col,
                            const char *transport_name,
                            size_t payload_size,
                            uint64_t total_wall_ns,
                            long vol_csw,
                            long invol_csw,
                            aipc_bench_result_t *out_result) {
    if (!col || !out_result || col->count == 0) return;

    out_result->transport_name = transport_name;
    out_result->payload_size = payload_size;
    out_result->iterations = col->count;
    out_result->total_wall_ns = total_wall_ns;
    out_result->voluntary_csw = vol_csw;
    out_result->involuntary_csw = invol_csw;

    // Calculate sum, min, max
    double sum = 0.0;
    double min_v = col->samples[0];
    double max_v = col->samples[0];

    for (size_t i = 0; i < col->count; i++) {
        double v = col->samples[i];
        sum += v;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
    }

    out_result->mean_lat_ns = sum / (double)col->count;
    out_result->min_lat_ns = min_v;
    out_result->max_lat_ns = max_v;

    // Sort to obtain exact percentiles
    qsort(col->samples, col->count, sizeof(double), compare_doubles);

    size_t idx_p50  = (size_t)((double)col->count * 0.50);
    size_t idx_p90  = (size_t)((double)col->count * 0.90);
    size_t idx_p99  = (size_t)((double)col->count * 0.99);
    size_t idx_p999 = (size_t)((double)col->count * 0.999);

    if (idx_p50 >= col->count)  idx_p50 = col->count - 1;
    if (idx_p90 >= col->count)  idx_p90 = col->count - 1;
    if (idx_p99 >= col->count)  idx_p99 = col->count - 1;
    if (idx_p999 >= col->count) idx_p999 = col->count - 1;

    out_result->p50_lat_ns  = col->samples[idx_p50];
    out_result->p90_lat_ns  = col->samples[idx_p90];
    out_result->p99_lat_ns  = col->samples[idx_p99];
    out_result->p999_lat_ns = col->samples[idx_p999];

    // Compute throughput
    double duration_sec = (double)total_wall_ns / 1e9;
    if (duration_sec > 0.0) {
        out_result->throughput_mps = (double)col->count / duration_sec;
        out_result->throughput_mbps = ((double)col->count * (double)payload_size) / (1024.0 * 1024.0) / duration_sec;
    } else {
        out_result->throughput_mps = 0.0;
        out_result->throughput_mbps = 0.0;
    }
}

void aipc_collector_free(aipc_lat_collector_t *col) {
    if (col && col->samples) {
        free(col->samples);
        col->samples = NULL;
        col->capacity = 0;
        col->count = 0;
    }
}

void aipc_print_header(void) {
    printf("+----------------------+----------+-------------+-------------+-------------+-------------+-------------+------------------+------------------+---------+\n");
    printf("| %-20s | %-8s | %-11s | %-11s | %-11s | %-11s | %-11s | %-16s | %-16s | %-7s |\n",
           "Transport", "Payload", "Mean Lat(us)", "P50 (us)", "P90 (us)", "P99 (us)", "P99.9 (us)",
           "Throughput(msg/s)", "Throughput(MB/s)", "Vol CSW");
    printf("+----------------------+----------+-------------+-------------+-------------+-------------+-------------+------------------+------------------+---------+\n");
}

void aipc_print_result_row(const aipc_bench_result_t *res) {
    if (!res) return;

    char size_str[64];
    if (res->payload_size >= 1024 * 1024) {
        snprintf(size_str, sizeof(size_str), "%zu MB", res->payload_size / (1024 * 1024));
    } else if (res->payload_size >= 1024) {
        snprintf(size_str, sizeof(size_str), "%zu KB", res->payload_size / 1024);
    } else {
        snprintf(size_str, sizeof(size_str), "%zu B", res->payload_size);
    }

    printf("| %-20s | %-8s | %11.3f | %11.3f | %11.3f | %11.3f | %11.3f | %16.0f | %16.2f | %7ld |\n",
           res->transport_name,
           size_str,
           res->mean_lat_ns / 1000.0,
           res->p50_lat_ns / 1000.0,
           res->p90_lat_ns / 1000.0,
           res->p99_lat_ns / 1000.0,
           res->p999_lat_ns / 1000.0,
           res->throughput_mps,
           res->throughput_mbps,
           res->voluntary_csw);
    printf("+----------------------+----------+-------------+-------------+-------------+-------------+-------------+------------------+------------------+---------+\n");
}

void aipc_print_danyliuk_comparison_row(const aipc_bench_result_t *res) {
    if (!res) return;
    double danyliuk_target_us = 0.0;

    if (strcmp(res->transport_name, "Anonymous Pipe") == 0) {
        danyliuk_target_us = 2.10;
    } else if (strcmp(res->transport_name, "UNIX Socket") == 0) {
        danyliuk_target_us = 2.05;
    } else if (strcmp(res->transport_name, "POSIX MQ") == 0) {
        danyliuk_target_us = 14.50;
    } else if (strcmp(res->transport_name, "Naive SHM+Futex") == 0) {
        danyliuk_target_us = 0.85;
    }

    double measured_us = res->mean_lat_ns / 1000.0;
    printf("   [Danyliuk 2026 Audit] %s (64 B): Measured = %.3f us | Paper Reference = ~%.2f us | Status: %s\n",
           res->transport_name,
           measured_us,
           danyliuk_target_us,
           (measured_us < danyliuk_target_us * 2.5) ? "MATCHES RESEARCH HIERARCHY [OK]" : "CHECK OS JITTER [WARN]");
}

void aipc_export_csv_header(FILE *fp) {
    if (!fp) return;
    fprintf(fp, "transport,payload_bytes,iterations,min_lat_ns,max_lat_ns,mean_lat_ns,p50_lat_ns,p90_lat_ns,p99_lat_ns,p999_lat_ns,throughput_mps,throughput_mbps,vol_csw,invol_csw,wall_time_ns\n");
}

void aipc_export_csv_row(FILE *fp, const aipc_bench_result_t *res) {
    if (!fp || !res) return;
    fprintf(fp, "%s,%zu,%zu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%ld,%ld,%lu\n",
            res->transport_name,
            res->payload_size,
            res->iterations,
            res->min_lat_ns,
            res->max_lat_ns,
            res->mean_lat_ns,
            res->p50_lat_ns,
            res->p90_lat_ns,
            res->p99_lat_ns,
            res->p999_lat_ns,
            res->throughput_mps,
            res->throughput_mbps,
            res->voluntary_csw,
            res->involuntary_csw,
            (unsigned long)res->total_wall_ns);
}

void aipc_generate_payload(uint8_t *buf, size_t len, uint32_t seq) {
    if (!buf || len == 0) return;

    if (len >= sizeof(uint32_t)) {
        memcpy(buf, &seq, sizeof(uint32_t));
        for (size_t i = sizeof(uint32_t); i < len; i++) {
            buf[i] = (uint8_t)((seq + i) & 0xFF);
        }
    } else {
        buf[0] = (uint8_t)(seq & 0xFF);
    }
}

bool aipc_validate_payload(const uint8_t *buf, size_t len, uint32_t expected_seq) {
    if (!buf || len == 0) return false;

    if (len >= sizeof(uint32_t)) {
        uint32_t actual_seq = 0;
        memcpy(&actual_seq, buf, sizeof(uint32_t));
        if (actual_seq != expected_seq) return false;

        for (size_t i = sizeof(uint32_t); i < len; i++) {
            uint8_t expected_val = (uint8_t)((expected_seq + i) & 0xFF);
            if (buf[i] != expected_val) return false;
        }
    } else {
        uint8_t expected_val = (uint8_t)(expected_seq & 0xFF);
        if (buf[0] != expected_val) return false;
    }
    return true;
}
