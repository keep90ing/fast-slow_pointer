CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O0 -Iinclude
CCACHE_DISABLE ?= 1

BIN_DIR := bin
BUILD_DIR := build
SRC_DIR := src

RUNNER_BIN := $(BIN_DIR)/runner
TRAVERSE_BIN := $(BIN_DIR)/traverse
CHECK_BIN := $(BIN_DIR)/check
FISHER_YATES_BIN := $(BIN_DIR)/fisher_yates
FISHER_YATES_WITH_ARRAY_BIN := $(BIN_DIR)/fisher_yates_with_array
MERGE_SHUFFLE_BIN := $(BIN_DIR)/merge_shuffle
LEGACY_SHUFFLE_BENCH_BIN := $(BIN_DIR)/fisher_yates_vs_merge_shuffle

CREATE_OBJ := $(BUILD_DIR)/create.o
RUNNER_OBJ := $(BUILD_DIR)/runner.o
TRAVERSE_OBJ := $(BUILD_DIR)/traverse.o
CHECK_OBJ := $(BUILD_DIR)/check.o
FISHER_YATES_OBJ := $(BUILD_DIR)/fisher_yates.o
FISHER_YATES_WITH_ARRAY_OBJ := $(BUILD_DIR)/fisher_yates_with_array.o
MERGE_SHUFFLE_OBJ := $(BUILD_DIR)/merge_shuffle.o
SINGLE_OBJ := $(BUILD_DIR)/single_pointer.o
FAST_SLOW_OBJ := $(BUILD_DIR)/fast_and_slow.o
PERF_COUNTER_OBJ := $(BUILD_DIR)/perf_counter.o
RUNNER_ARGS_OBJ := $(BUILD_DIR)/runner_args.o

.PHONY: all runner traverse check fisher-yates fisher-yates-with-array fisher_yates_with_array merge-shuffle shuffle-bench clean

all: runner traverse check fisher-yates fisher-yates-with-array merge-shuffle

runner: $(RUNNER_BIN)

traverse: $(TRAVERSE_BIN)

check: $(CHECK_BIN)

fisher-yates: $(FISHER_YATES_BIN)

fisher-yates-with-array: $(FISHER_YATES_WITH_ARRAY_BIN)

fisher_yates_with_array: fisher-yates-with-array

merge-shuffle: $(MERGE_SHUFFLE_BIN)

shuffle-bench: fisher-yates fisher-yates-with-array merge-shuffle

$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

$(CREATE_OBJ): $(SRC_DIR)/create.c include/create.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(RUNNER_OBJ): $(SRC_DIR)/runner.c include/create.h include/single_pointer.h include/fast_and_slow.h include/perf_counter.h include/runner_args.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(TRAVERSE_OBJ): $(SRC_DIR)/traverse.c include/create.h include/single_pointer.h include/fast_and_slow.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(CHECK_OBJ): $(SRC_DIR)/check.c include/create.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(FISHER_YATES_OBJ): $(SRC_DIR)/fisher_yates.c include/create.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(FISHER_YATES_WITH_ARRAY_OBJ): $(SRC_DIR)/fisher_yates_with_array.c include/create.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(MERGE_SHUFFLE_OBJ): $(SRC_DIR)/merge_shuffle.c include/create.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(PERF_COUNTER_OBJ): $(SRC_DIR)/perf_counter.c include/perf_counter.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(RUNNER_ARGS_OBJ): $(SRC_DIR)/runner_args.c include/runner_args.h include/create.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(SINGLE_OBJ): $(SRC_DIR)/single_pointer.c include/single_pointer.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(FAST_SLOW_OBJ): $(SRC_DIR)/fast_and_slow.c include/fast_and_slow.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(RUNNER_BIN): $(RUNNER_OBJ) $(CREATE_OBJ) $(SINGLE_OBJ) $(FAST_SLOW_OBJ) $(PERF_COUNTER_OBJ) $(RUNNER_ARGS_OBJ) | $(BIN_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $^ -o $@

$(TRAVERSE_BIN): $(TRAVERSE_OBJ) $(CREATE_OBJ) $(SINGLE_OBJ) $(FAST_SLOW_OBJ) | $(BIN_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $^ -o $@

$(CHECK_BIN): $(CHECK_OBJ) $(CREATE_OBJ) | $(BIN_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $^ -o $@

$(FISHER_YATES_BIN): $(FISHER_YATES_OBJ) $(CREATE_OBJ) | $(BIN_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $^ -o $@

$(FISHER_YATES_WITH_ARRAY_BIN): $(FISHER_YATES_WITH_ARRAY_OBJ) $(CREATE_OBJ) | $(BIN_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $^ -o $@

$(MERGE_SHUFFLE_BIN): $(MERGE_SHUFFLE_OBJ) $(CREATE_OBJ) | $(BIN_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $^ -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)/create $(RUNNER_BIN) $(TRAVERSE_BIN) $(CHECK_BIN) $(FISHER_YATES_BIN) $(FISHER_YATES_WITH_ARRAY_BIN) $(MERGE_SHUFFLE_BIN) $(LEGACY_SHUFFLE_BENCH_BIN)
