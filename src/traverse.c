#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "create.h"
#include "fast_and_slow.h"
#include "single_pointer.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum algo_mode {
    ALGO_SINGLE = 1,
    ALGO_FAST_SLOW = 2
};

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
    fprintf(stderr, "  mode: sequential | random | 1 | 2\n");
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
    int list_mode, count, algo_mode, rounds;
    unsigned int seed;
    int use_fixed_seed = 0;
    int i;
    struct timespec start_ts,end_ts;
    double elapsed_sec = 0.0;
    struct list_node *head, *mid = NULL;

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

    if (clock_gettime(CLOCK_MONOTONIC, &start_ts) < 0) {
        perror("clock_gettime(start) failed");
        free_list(head);
        return 1;
    }

    for (i = 0; i < rounds; ++i) {
        if (algo_mode == ALGO_SINGLE)
            mid = single_pointer_middle_node(head);
        else
            mid = fast_and_slow_middle_node(head);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end_ts) < 0) {
        perror("clock_gettime(end) failed");
        free_list(head);
        return 1;
    }

    elapsed_sec = (double)(end_ts.tv_sec - start_ts.tv_sec) +
                  (double)(end_ts.tv_nsec - start_ts.tv_nsec) / 1e9;

    printf("mode=%s, count=%d, algo=%s, rounds=%d\n",
           create_mode_name(list_mode), count, algo_mode_name(algo_mode), rounds);
    if (use_fixed_seed)
        printf("seed=%u\n", seed);
    if (mid)
        printf("middle node: addr=%p, val=%d\n", (void *)mid, mid->val);
        
    printf("elapsed(sec): %.6f\n", elapsed_sec);

    free_list(head);
    return 0;
}
