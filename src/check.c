#include "create.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    fprintf(stderr, "Usage: %s <mode> <count> [seed]\n", prog);
    fprintf(stderr, "  mode: sequential | random | 1 | 2\n");
    fprintf(stderr, "  count: number of nodes (> 0)\n");
    fprintf(stderr, "  seed: optional unsigned integer, only used for random mode\n");
}

int main(int argc, char **argv)
{
    int mode;
    int count;
    int index = 0;
    unsigned int seed;
    int use_fixed_seed = 0;
    struct list_node *head;
    struct list_node *cur;

    if (argc != 3 && argc != 4) {
        usage(argv[0]);
        return 1;
    }

    mode = create_parse_mode(argv[1]);
    if (!mode) {
        usage(argv[0]);
        return 1;
    }

    if (create_parse_count(argv[2], &count) < 0) {
        usage(argv[0]);
        return 1;
    }

    if (argc == 4) {
        if (parse_seed(argv[3], &seed) < 0) {
            usage(argv[0]);
            return 1;
        }
        use_fixed_seed = 1;
    } else {
        seed = (unsigned int)time(NULL);
    }

    srand(seed);
    head = create_list(mode, count);
    if (!head) {
        fprintf(stderr, "Failed to create list.\n");
        return 1;
    }

    printf("mode=%s, count=%d\n", create_mode_name(mode), count);
    if (mode == MODE_RANDOM && use_fixed_seed)
        printf("seed=%u\n", seed);

    cur = head;
    while (cur) {
        if (cur->next) {
            printf("[%d] val=%-2d (%p) -> (%p)\n",
                   index, cur->val, (void *)cur, (void *)cur->next);
        } else {
            printf("[%d] val=%-2d (%p) -> NULL\n",
                   index, cur->val, (void *)cur);
        }
        cur = cur->next;
        ++index;
    }

    free_list(head);
    return 0;
}
