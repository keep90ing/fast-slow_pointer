#include "runner_args.h"

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

static int parse_algo_mode(const char *algo_str)
{
    if (!strcmp(algo_str, "1") || !strcmp(algo_str, "single") ||
        !strcmp(algo_str, "single_pointer"))
        return RUNNER_ALGO_SINGLE;
    if (!strcmp(algo_str, "2") || !strcmp(algo_str, "fastslow") ||
        !strcmp(algo_str, "fast_and_slow"))
        return RUNNER_ALGO_FAST_SLOW;
    return 0;
}

void runner_args_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <mode> <count> <algo> [seed]\n", prog);
    fprintf(stderr, "  mode: sequential | random | 1 | 2\n");
    fprintf(stderr, "  count: number of nodes (> 0)\n");
    fprintf(stderr, "  algo: single | fastslow | 1 | 2\n");
    fprintf(stderr, "  seed: optional unsigned integer for deterministic random mode\n");
}

int runner_args_parse(int argc, char **argv, struct runner_args *out)
{
    if (!out || !argv)
        return -1;

    if (argc != 4 && argc != 5)
        return -1;

    out->list_mode = create_parse_mode(argv[1]);
    if (!out->list_mode)
        return -1;

    if (create_parse_count(argv[2], &out->count) < 0)
        return -1;

    out->algo_mode = parse_algo_mode(argv[3]);
    if (!out->algo_mode)
        return -1;

    out->use_fixed_seed = 0;
    if (argc == 5) {
        if (parse_seed(argv[4], &out->seed) < 0)
            return -1;
        out->use_fixed_seed = 1;
    } else {
        out->seed = (unsigned int)time(NULL);
    }

    return 0;
}

const char *runner_algo_mode_name(int mode)
{
    if (mode == RUNNER_ALGO_SINGLE)
        return "single_pointer";
    if (mode == RUNNER_ALGO_FAST_SLOW)
        return "fast_and_slow";
    return "unknown";
}
