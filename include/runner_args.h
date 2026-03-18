#ifndef RUNNER_ARGS_H
#define RUNNER_ARGS_H

enum runner_algo_mode {
    RUNNER_ALGO_SINGLE = 1,
    RUNNER_ALGO_FAST_SLOW = 2
};

struct runner_args {
    int list_mode;
    int count;
    int algo_mode;
    unsigned int seed;
    int use_fixed_seed;
};

void runner_args_usage(const char *prog);
int runner_args_parse(int argc, char **argv, struct runner_args *out);
const char *runner_algo_mode_name(int mode);

#endif
