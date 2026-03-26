#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "bench_csv.h"
#include "create.h"
#include "fast_and_slow.h"
#include "perf_counter.h"
#include "runner_args.h"
#include "single_pointer.h"

#include <stdio.h>
#include <stdlib.h>

static double percent_rate(uint64_t numerator, uint64_t denominator)
{
    if (denominator == 0)
        return 0.0;
    return 100.0 * (double)numerator / (double)denominator;
}

int main(int argc, char **argv)
{
    int ret = 1;
    int perf_enabled = 0;
    struct runner_args args = { 0 };
    double cache_miss_rate = 0.0;
    double l1_dcache_load_miss_rate = 0.0;
    double dtlb_load_miss_rate = 0.0;
    struct list_node *head = NULL;
    struct list_node *mid = NULL;
    struct perf_counters counters = { .leader_fd = -1,
                                      .cycles_fd = -1,
                                      .cache_refs_fd = -1,
                                      .cache_misses_fd = -1,
                                      .l1_dcache_loads_fd = -1,
                                      .l1_dcache_load_misses_fd = -1,
                                      .l1_dcache_prefetches_fd = -1,
                                      .dtlb_loads_fd = -1,
                                      .dtlb_load_misses_fd = -1 };
    struct perf_counter_values perf_values = { 0 };

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

    // 宣告一個大於 L3 Cache 的 64MB array
    size_t dummy_size = 64 * 1024 * 1024;
    char *dummy = malloc(dummy_size);
    for (size_t i = 0; i < dummy_size; i += 64) { // 每次跳一個 Cache Line (64 Bytes)
        dummy[i] = 1; // 強制寫入，迫使 CPU 將先前 Linked List 從 L1/L2/L3 evict
    }
    free(dummy);

    if (perf_counters_enable(&counters) < 0) {
        perror("PERF_EVENT_IOC_ENABLE failed");
        goto cleanup;
    }
    perf_enabled = 1;

    if (args.algo_mode == RUNNER_ALGO_SINGLE)
        mid = single_pointer_middle_node(head);
    else
        mid = fast_and_slow_middle_node(head);

    if (perf_counters_disable(&counters) < 0) {
        perror("PERF_EVENT_IOC_DISABLE failed");
        goto cleanup;
    }
    perf_enabled = 0;

    if (perf_counters_read(&counters, &perf_values) < 0) {
        perror("read perf counters failed");
        goto cleanup;
    }

    cache_miss_rate = percent_rate(perf_values.cache_misses, perf_values.cache_refs);
    l1_dcache_load_miss_rate = percent_rate(perf_values.l1_dcache_load_misses,
                                            perf_values.l1_dcache_loads);
    dtlb_load_miss_rate = percent_rate(perf_values.dtlb_load_misses,
                                       perf_values.dtlb_loads);

    if (bench_csv_append_row(&args, mid, &perf_values, cache_miss_rate,
                             l1_dcache_load_miss_rate,
                             dtlb_load_miss_rate) < 0) {
        perror("failed to append CSV output");
        goto cleanup;
    }

    ret = 0;

cleanup:
    /* Always release resources, including partially-initialized counters. */
    if (perf_enabled)
        (void)perf_counters_disable(&counters);
    perf_counters_close(&counters);
    free_list(head);
    return ret;
}
