CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O2 -Iinclude
CCACHE_DISABLE ?= 1

BIN_DIR := bin
BUILD_DIR := build
SRC_DIR := src

CREATE_BIN := $(BIN_DIR)/create
RUNNER_BIN := $(BIN_DIR)/runner

CREATE_MAIN_OBJ := $(BUILD_DIR)/create_main.o
CREATE_LIB_OBJ := $(BUILD_DIR)/create_lib.o
RUNNER_OBJ := $(BUILD_DIR)/runner.o
SINGLE_OBJ := $(BUILD_DIR)/single_pointer.o
FAST_SLOW_OBJ := $(BUILD_DIR)/fast_and_slow.o

.PHONY: all create runner clean

all: create runner

create: $(CREATE_BIN)

runner: $(RUNNER_BIN)

$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

$(CREATE_MAIN_OBJ): $(SRC_DIR)/create.c include/create.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(CREATE_LIB_OBJ): $(SRC_DIR)/create.c include/create.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -DCREATE_NO_MAIN -c $< -o $@

$(RUNNER_OBJ): $(SRC_DIR)/runner.c include/create.h include/single_pointer.h include/fast_and_slow.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(SINGLE_OBJ): $(SRC_DIR)/single_pointer.c include/single_pointer.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(FAST_SLOW_OBJ): $(SRC_DIR)/fast_and_slow.c include/fast_and_slow.h include/list.h | $(BUILD_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $(CFLAGS) -c $< -o $@

$(CREATE_BIN): $(CREATE_MAIN_OBJ) | $(BIN_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $< -o $@

$(RUNNER_BIN): $(RUNNER_OBJ) $(CREATE_LIB_OBJ) $(SINGLE_OBJ) $(FAST_SLOW_OBJ) | $(BIN_DIR)
	CCACHE_DISABLE=$(CCACHE_DISABLE) $(CC) $^ -o $@

clean:
	rm -rf $(BUILD_DIR) $(CREATE_BIN) $(RUNNER_BIN)
