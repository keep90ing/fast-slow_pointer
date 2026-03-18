#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "create.h"
#include "fast_and_slow.h"
#include "perf_counter.h"
#include "runner_args.h"
#include "single_pointer.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int ret = 1;
    struct runner_args args = { 0 };
    double elapsed_sec = 0.0;
    struct list_node *head, *mid = NULL;
    struct perf_counters counters = { .leader_fd = -1,
                                      .cycles_fd = -1,
                                      .cache_refs_fd = -1,
                                      .cache_misses_fd = -1,
                                      .l1_dcache_loads_fd = -1,
                                      .l1_dcache_load_misses_fd = -1,
                                      .instructions_fd = -1 };
    struct perf_counter_values perf_values = { 0 };
    double cache_miss_rate = 0.0, l1_dcache_load_miss_rate = 0.0, ipc = 0.0;

    /* Parse CLI arguments into a single config struct. */
    if (runner_args_parse(argc, argv, &args) < 0) {
        runner_args_usage(argv[0]);
        return 1;
    }

    /* Build the test list once; traversal is measured after this point. */
    srand(args.seed);
    head = create_list(args.list_mode, args.count);
    if (!head) {
        fprintf(stderr, "Failed to create list.\n");
        return 1;
    }

    /* Open grouped perf counters so we can enable/disable them together. */
    if (perf_counters_open(&counters) < 0) {
        perror("Failed to open perf counters");
        fprintf(stderr,
                "Hint: check perf_event permission (e.g. perf_event_paranoid).\n");
        goto cleanup;
    }

    if (perf_counters_reset(&counters) < 0) {
        perror("PERF_EVENT_IOC_RESET failed");
        goto cleanup;
    }

    if (perf_counters_enable(&counters) < 0) {
        perror("PERF_EVENT_IOC_ENABLE failed");
        goto cleanup;
    }

    /* Measure only one traversal of the selected algorithm. */
    if (args.algo_mode == RUNNER_ALGO_SINGLE)
        mid = single_pointer_middle_node(head);
    else
        mid = fast_and_slow_middle_node(head);

    if (perf_counters_disable(&counters) < 0) {
        perror("PERF_EVENT_IOC_DISABLE failed");
        goto cleanup;
    }

    if (perf_counters_read(&counters, &perf_values) < 0) {
        perror("read perf counters failed");
        goto cleanup;
    }

    /* Derive user-facing metrics from raw perf counters. */
    elapsed_sec = (double)perf_values.task_clock_ns / 1e9;
    if (perf_values.cache_refs > 0)
        cache_miss_rate = 100.0 * (double)perf_values.cache_misses /
                          (double)perf_values.cache_refs;
    if (perf_values.l1_dcache_loads > 0)
        l1_dcache_load_miss_rate =
            100.0 * (double)perf_values.l1_dcache_load_misses /
            (double)perf_values.l1_dcache_loads;
    if (perf_values.cycles > 0)
        ipc = (double)perf_values.instructions / (double)perf_values.cycles;

    /* Print benchmark summary and perf statistics. */
    printf("mode=%s, count=%d, algo=%s\n",
           create_mode_name(args.list_mode), args.count,
           runner_algo_mode_name(args.algo_mode));
    if (args.use_fixed_seed)
        printf("seed=%u\n", args.seed);
    if (mid)
        printf("middle node: addr=%p, val=%d\n", (void *)mid, mid->val);

    printf("task-clock(ns): %" PRIu64 "\n", perf_values.task_clock_ns);
    printf("elapsed(sec): %.6f\n", elapsed_sec);
    printf("cpu-cycles: %" PRIu64 "\n", perf_values.cycles);
    printf("cache-references: %" PRIu64 "\n", perf_values.cache_refs);
    printf("cache-misses: %" PRIu64 "\n", perf_values.cache_misses);
    printf("cache-miss-rate: %.2f%%\n", cache_miss_rate);
    printf("L1-dcache-loads: %" PRIu64 "\n", perf_values.l1_dcache_loads);
    printf("L1-dcache-load-misses: %" PRIu64 "\n",
           perf_values.l1_dcache_load_misses);
    printf("L1-dcache-load-miss-rate: %.2f%%\n", l1_dcache_load_miss_rate);
    printf("instructions: %" PRIu64 "\n", perf_values.instructions);
    printf("IPC: %.2f\n", ipc);

    ret = 0;

cleanup:
    /* Always release resources, including partially-initialized counters. */
    perf_counters_close(&counters);
    free_list(head);
    return ret;
}
