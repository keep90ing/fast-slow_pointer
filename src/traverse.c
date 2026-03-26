#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "create.h"
#include "fast_and_slow.h"
#include "runner_args.h"
#include "single_pointer.h"

#include <stdio.h>
#include <stdlib.h>

static const char *algo_mode_name(int algo_mode)
{
    if (algo_mode == RUNNER_ALGO_SINGLE)
        return "single_pointer";
    if (algo_mode == RUNNER_ALGO_FAST_SLOW)
        return "fast_and_slow";
    return "unknown";
}

int main(int argc, char **argv)
{
    struct runner_args args = { 0 };
    struct list_node *head = NULL;
    struct list_node *mid = NULL;

    if (runner_args_parse(argc, argv, &args) < 0) {
        runner_args_usage(argv[0]);
        return 1;
    }

    srand(args.seed);
    head = create_list(args.list_mode, args.count);
    if (!head) {
        fprintf(stderr, "Failed to create list.\n");
        return 1;
    }

    // 宣告一個大於 L3 Cache 的 64MB array
    size_t dummy_size = 64 * 1024 * 1024;
    char *dummy = malloc(dummy_size);
    for (size_t i = 0; i < dummy_size; i += 64) { // 每次跳一個 Cache Line (64 Bytes)
        dummy[i] = 1; // 強制寫入，迫使 CPU 將先前 Linked List 從 L1/L2/L3 evict
    }
    free(dummy);

    if (args.algo_mode == RUNNER_ALGO_SINGLE)
        mid = single_pointer_middle_node(head);
    else
        mid = fast_and_slow_middle_node(head);

    printf("mode=%s, count=%d, algo=%s\n", create_mode_name(args.list_mode),
           args.count, algo_mode_name(args.algo_mode));
    printf("seed=%u\n", args.seed);
    if (mid)
        printf("middle node: addr=%p, val=%d\n", (void *)mid, mid->val);

    free_list(head);
    return 0;
}
