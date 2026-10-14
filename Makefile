CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -O3 -D_GNU_SOURCE -Iinclude
LDFLAGS ?= -pthread -lrt -lm

SRC_DIR = src
INC_DIR = include
BENCH_DIR = bench
BUILD_DIR = build
RESULTS_DIR = results

BASE_OBJS = $(BUILD_DIR)/aipc_bench.o
BASELINE_OBJS = $(BASE_OBJS) $(BUILD_DIR)/aipc_baselines.o
FAS_OBJS = $(BASE_OBJS) $(BUILD_DIR)/aipc_fas_ring.o
UNIFIED_OBJS = $(BASE_OBJS) $(BUILD_DIR)/aipc_fas_ring.o $(BUILD_DIR)/aipc_selector.o $(BUILD_DIR)/aipc_channel.o
MATRIX_OBJS = $(UNIFIED_OBJS) $(BUILD_DIR)/aipc_baselines.o $(BUILD_DIR)/bench_matrix.o

TARGETS = bench_baselines test_fas_ring test_unified bench_matrix

all: $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(RESULTS_DIR):
	mkdir -p $(RESULTS_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(BENCH_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

bench_baselines: $(BASELINE_OBJS) $(BUILD_DIR)/bench_baselines.o | $(RESULTS_DIR)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

test_fas_ring: $(FAS_OBJS) $(BUILD_DIR)/test_fas_ring.o | $(RESULTS_DIR)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

test_unified: $(UNIFIED_OBJS) $(BUILD_DIR)/test_unified.o | $(RESULTS_DIR)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

bench_matrix: $(MATRIX_OBJS) | $(RESULTS_DIR)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGETS)

.PHONY: all clean
