CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -O3 -D_GNU_SOURCE -Iinclude
LDFLAGS ?= -pthread -lrt

SRC_DIR = src
INC_DIR = include
BENCH_DIR = bench
BUILD_DIR = build
RESULTS_DIR = results

OBJS = $(BUILD_DIR)/aipc_bench.o $(BUILD_DIR)/aipc_baselines.o

all: bench_baselines

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(RESULTS_DIR):
	mkdir -p $(RESULTS_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bench_baselines.o: $(BENCH_DIR)/bench_baselines.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

bench_baselines: $(OBJS) $(BUILD_DIR)/bench_baselines.o | $(RESULTS_DIR)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR) bench_baselines

.PHONY: all clean
