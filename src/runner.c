#include "create.h"
#include "fast_and_slow.h"
#include "single_pointer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum algo_mode {
    ALGO_SINGLE = 1,
    ALGO_FAST_SLOW = 2
};

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <mode> <count> <algo> <rounds>\n", prog);
    fprintf(stderr, "  mode: contiguous | sequential | random | 1 | 2 | 3\n");
    fprintf(stderr, "  count: number of nodes (> 0)\n");
    fprintf(stderr, "  algo: single | fastslow | 1 | 2\n");
    fprintf(stderr, "  rounds: run count (> 0)\n");
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
    int list_mode;
    int count;
    int algo_mode;
    int rounds;
    int i;
    clock_t start;
    clock_t end;
    struct list_node *head;
    struct list_node *mid = NULL;

    if (argc != 5) {
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

    srand((unsigned int)time(NULL));
    head = create_list(list_mode, count);
    if (!head) {
        fprintf(stderr, "Failed to create list.\n");
        return 1;
    }

    start = clock();
    for (i = 0; i < rounds; ++i) {
        if (algo_mode == ALGO_SINGLE)
            mid = single_pointer_middle_node(head);
        else
            mid = fast_and_slow_middle_node(head);
    }
    end = clock();

    printf("mode=%s, count=%d, algo=%s, rounds=%d\n",
           create_mode_name(list_mode), count, algo_mode_name(algo_mode), rounds);
    if (mid)
        printf("middle node: addr=%p, val=%d\n", (void *)mid, mid->val);
    printf("elapsed: %.6f sec\n", (double)(end - start) / CLOCKS_PER_SEC);

    destroy_list(head, list_mode);
    return 0;
}
