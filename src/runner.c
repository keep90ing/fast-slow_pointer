#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "create.h"
#include "fast_and_slow.h"
#include "single_pointer.h"

#include <errno.h>
#include <inttypes.h>
#include <linux/perf_event.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

enum algo_mode {
    ALGO_SINGLE = 1,
    ALGO_FAST_SLOW = 2
};

struct perf_counters {
    int leader_fd;
    int cache_refs_fd;
    int cache_misses_fd;
    int instructions_fd;
};

static int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                           int group_fd, unsigned long flags)
{
    return (int)syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

static int open_hw_counter(uint64_t config, int group_fd, int disabled)
{
    struct perf_event_attr attr;

    memset(&attr, 0, sizeof(attr));
    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = config;
    attr.disabled = disabled;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.exclude_idle = 1;

    return perf_event_open(&attr, 0, -1, group_fd, 0);
}

static void close_perf_counters(struct perf_counters *counters)
{
    if (counters->instructions_fd >= 0)
        close(counters->instructions_fd);
    if (counters->cache_misses_fd >= 0)
        close(counters->cache_misses_fd);
    if (counters->cache_refs_fd >= 0)
        close(counters->cache_refs_fd);
    if (counters->leader_fd >= 0)
        close(counters->leader_fd);
}

static int open_perf_counters(struct perf_counters *counters)
{
    counters->leader_fd = -1;
    counters->cache_refs_fd = -1;
    counters->cache_misses_fd = -1;
    counters->instructions_fd = -1;

    counters->leader_fd =
        open_hw_counter(PERF_COUNT_HW_CPU_CYCLES, -1, 1);
    if (counters->leader_fd < 0)
        return -1;

    counters->cache_refs_fd = open_hw_counter(PERF_COUNT_HW_CACHE_REFERENCES,
                                              counters->leader_fd, 0);
    if (counters->cache_refs_fd < 0)
        return -1;

    counters->cache_misses_fd = open_hw_counter(PERF_COUNT_HW_CACHE_MISSES,
                                                counters->leader_fd, 0);
    if (counters->cache_misses_fd < 0)
        return -1;

    counters->instructions_fd = open_hw_counter(PERF_COUNT_HW_INSTRUCTIONS,
                                                counters->leader_fd, 0);
    if (counters->instructions_fd < 0)
        return -1;

    return 0;
}

static int read_counter_value(int fd, uint64_t *value)
{
    ssize_t read_len = read(fd, value, sizeof(*value));
    return (read_len == (ssize_t)sizeof(*value)) ? 0 : -1;
}

static int parse_seed(const char *seed_str, unsigned int *out)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(seed_str, &end, 10);
    if (errno || *seed_str == '\0' || *end != '\0')
        return -1;
    if (value > UINT32_MAX)
        return -1;

    *out = (unsigned int)value;
    return 0;
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <mode> <count> <algo> <rounds> [seed]\n", prog);
    fprintf(stderr, "  mode: contiguous | sequential | random | 1 | 2 | 3\n");
    fprintf(stderr, "  count: number of nodes (> 0)\n");
    fprintf(stderr, "  algo: single | fastslow | 1 | 2\n");
    fprintf(stderr, "  rounds: run count (> 0)\n");
    fprintf(stderr, "  seed: optional unsigned integer for deterministic random mode\n");
}

static int parse_algo_mode(const char *algo_str)
{
    if (!strcmp(algo_str, "1") || !strcmp(algo_str, "single") ||
        !strcmp(algo_str, "single_pointer"))
        return ALGO_SINGLE;
    if (!strcmp(algo_str, "2") || !strcmp(algo_str, "fastslow") ||
        !strcmp(algo_str, "fast_and_slow"))
        return ALGO_FAST_SLOW;
    return 0;
}

static const char *algo_mode_name(int mode)
{
    if (mode == ALGO_SINGLE)
        return "single_pointer";
    if (mode == ALGO_FAST_SLOW)
        return "fast_and_slow";
    return "unknown";
}

int main(int argc, char **argv)
{
    int ret = 1;
    int list_mode;
    int count;
    int algo_mode;
    int rounds;
    unsigned int seed;
    int use_fixed_seed = 0;
    int i;
    struct timespec start_ts;
    struct timespec end_ts;
    double elapsed_sec = 0.0;
    struct list_node *head;
    struct list_node *mid = NULL;
    struct perf_counters counters;
    uint64_t cache_refs = 0;
    uint64_t cache_misses = 0;
    uint64_t cycles = 0;
    uint64_t instructions = 0;
    double cache_miss_rate = 0.0;
    double ipc = 0.0;

    if (argc != 5 && argc != 6) {
        usage(argv[0]);
        return 1;
    }

    list_mode = create_parse_mode(argv[1]);
    if (!list_mode) {
        usage(argv[0]);
        return 1;
    }

    if (create_parse_count(argv[2], &count) < 0) {
        usage(argv[0]);
        return 1;
    }

    algo_mode = parse_algo_mode(argv[3]);
    if (!algo_mode) {
        usage(argv[0]);
        return 1;
    }

    if (create_parse_count(argv[4], &rounds) < 0) {
        usage(argv[0]);
        return 1;
    }

    if (argc == 6) {
        if (parse_seed(argv[5], &seed) < 0) {
            usage(argv[0]);
            return 1;
        }
        use_fixed_seed = 1;
    } else {
        seed = (unsigned int)time(NULL);
    }

    srand(seed);
    head = create_list(list_mode, count);
    if (!head) {
        fprintf(stderr, "Failed to create list.\n");
        return 1;
    }

    if (open_perf_counters(&counters) < 0) {
        perror("Failed to open perf counters");
        fprintf(stderr,
                "Hint: check perf_event permission (e.g. perf_event_paranoid).\n");
        goto cleanup;
    }

    if (ioctl(counters.leader_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) < 0) {
        perror("PERF_EVENT_IOC_RESET failed");
        goto cleanup;
    }

    if (ioctl(counters.leader_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) < 0) {
        perror("PERF_EVENT_IOC_ENABLE failed");
        goto cleanup;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &start_ts) < 0) {
        perror("clock_gettime(start) failed");
        goto cleanup;
    }

    for (i = 0; i < rounds; ++i) {
        if (algo_mode == ALGO_SINGLE)
            mid = single_pointer_middle_node(head);
        else
            mid = fast_and_slow_middle_node(head);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end_ts) < 0) {
        perror("clock_gettime(end) failed");
        goto cleanup;
    }

    if (ioctl(counters.leader_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) < 0) {
        perror("PERF_EVENT_IOC_DISABLE failed");
        goto cleanup;
    }

    if (read_counter_value(counters.cache_refs_fd, &cache_refs) < 0) {
        perror("read cache-references failed");
        goto cleanup;
    }
    if (read_counter_value(counters.cache_misses_fd, &cache_misses) < 0) {
        perror("read cache-misses failed");
        goto cleanup;
    }
    if (read_counter_value(counters.leader_fd, &cycles) < 0) {
        perror("read cycles failed");
        goto cleanup;
    }
    if (read_counter_value(counters.instructions_fd, &instructions) < 0) {
        perror("read instructions failed");
        goto cleanup;
    }

    elapsed_sec = (double)(end_ts.tv_sec - start_ts.tv_sec) +
                  (double)(end_ts.tv_nsec - start_ts.tv_nsec) / 1e9;
    if (cache_refs > 0)
        cache_miss_rate = 100.0 * (double)cache_misses / (double)cache_refs;
    if (cycles > 0)
        ipc = (double)instructions / (double)cycles;

    printf("mode=%s, count=%d, algo=%s, rounds=%d\n",
           create_mode_name(list_mode), count, algo_mode_name(algo_mode), rounds);
    if (use_fixed_seed)
        printf("seed=%u\n", seed);
    if (mid)
        printf("middle node: addr=%p, val=%d\n", (void *)mid, mid->val);
    printf("elapsed(sec): %.6f\n", elapsed_sec);
    printf("cache-references:u: %" PRIu64 "\n", cache_refs);
    printf("cache-misses:u: %" PRIu64 "\n", cache_misses);
    printf("cache-miss-rate: %.2f%%\n", cache_miss_rate);
    printf("IPC: %.2f\n", ipc);

    ret = 0;

cleanup:
    close_perf_counters(&counters);
    destroy_list(head, list_mode);
    return ret;
}
