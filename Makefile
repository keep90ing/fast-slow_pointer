CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O0 -Iinclude
CCACHE_DISABLE ?= 1

BIN_DIR := bin
BUILD_DIR := build
SRC_DIR := src

RUNNER_BIN := $(BIN_DIR)/runner
CHECK_BIN := $(BIN_DIR)/check

CREATE_OBJ := $(BUILD_DIR)/create.o
RUNNER_OBJ := $(BUILD_DIR)/runner.o
CHECK_OBJ := $(BUILD_DIR)/check.o
SINGLE_OBJ := $(BUILD_DIR)/single_pointer.o
FAST_SLOW_OBJ := $(BUILD_DIR)/fast_and_slow.o

.PHONY: all runner check clean

all: runner check

runner: $(RUNNER_BIN)

check: $(CHECK_BIN)

$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

$(CREATE_OBJ): $(SRC_DIR)/create.c include/create.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(RUNNER_OBJ): $(SRC_DIR)/runner.c include/create.h include/single_pointer.h include/fast_and_slow.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(CHECK_OBJ): $(SRC_DIR)/check.c include/create.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(SINGLE_OBJ): $(SRC_DIR)/single_pointer.c include/single_pointer.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(FAST_SLOW_OBJ): $(SRC_DIR)/fast_and_slow.c include/fast_and_slow.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(RUNNER_BIN): $(RUNNER_OBJ) $(CREATE_OBJ) $(SINGLE_OBJ) $(FAST_SLOW_OBJ) | $(BIN_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $^ -o $@

$(CHECK_BIN): $(CHECK_OBJ) $(CREATE_OBJ) | $(BIN_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $^ -o $@

clean:
	rm -rf $(BUILD_DIR) $(RUNNER_BIN) $(CHECK_BIN)
