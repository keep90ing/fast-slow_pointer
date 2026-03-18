#ifndef PERF_COUNTER_H
#define PERF_COUNTER_H

#include <stdint.h>

struct perf_counters {
    int leader_fd;
    int cycles_fd;
    int cache_refs_fd;
    int cache_misses_fd;
    int l1_dcache_loads_fd;
    int l1_dcache_load_misses_fd;
    int instructions_fd;
};

struct perf_counter_values {
    uint64_t task_clock_ns;
    uint64_t cycles;
    uint64_t cache_refs;
    uint64_t cache_misses;
    uint64_t l1_dcache_loads;
    uint64_t l1_dcache_load_misses;
    uint64_t instructions;
};

int perf_counters_open(struct perf_counters *counters);
void perf_counters_close(struct perf_counters *counters);
int perf_counters_reset(const struct perf_counters *counters);
int perf_counters_enable(const struct perf_counters *counters);
int perf_counters_disable(const struct perf_counters *counters);
int perf_counters_read(const struct perf_counters *counters,
                       struct perf_counter_values *values);

#endif
