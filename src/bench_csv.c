#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "bench_csv.h"

#include "create.h"
#include "perf_counter.h"
#include "runner_args.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BENCH_LOG_DIR "./logs"
#define BENCH_CSV_PATH "./logs/bench.csv"

static const char *algo_mode_name(int algo_mode)
{
    if (algo_mode == RUNNER_ALGO_SINGLE)
        return "single_pointer";
    if (algo_mode == RUNNER_ALGO_FAST_SLOW)
        return "fast_and_slow";
    return "unknown";
}

static int ensure_log_dir_exists(void)
{
    if (mkdir(BENCH_LOG_DIR, 0755) == 0)
        return 0;
    if (errno == EEXIST)
        return 0;
    return -1;
}

int bench_csv_append_row(const struct runner_args *args, const struct list_node *mid,
                         const struct perf_counter_values *perf_values,
                         double cache_miss_rate,
                         double l1_dcache_load_miss_rate,
                         double dtlb_load_miss_rate)
{
    int saved_errno = 0;
    long file_size = 0;
    FILE *fp = NULL;
    const char *mode = NULL;
    const char *algo = NULL;

    if (!args || !perf_values) {
        errno = EINVAL;
        return -1;
    }

    if (ensure_log_dir_exists() < 0)
        return -1;

    fp = fopen(BENCH_CSV_PATH, "a+");
    if (!fp)
        return -1;

    if (fseek(fp, 0L, SEEK_END) < 0)
        goto fail;
    file_size = ftell(fp);
    if (file_size < 0)
        goto fail;

    if (file_size == 0) {
        if (fprintf(fp,
                    "mode,count,algo,seed,middle_addr,middle_val,"
                    "cpu_cycles,cache_miss_rate,l1_dcache_load_miss_rate,"
                    "l1_dcache_prefetches,dtlb_load_miss_rate\n") < 0)
            goto fail;
    }

    mode = create_mode_name(args->list_mode);
    algo = algo_mode_name(args->algo_mode);
    if (fprintf(fp, "%s,%d,%s,%u,%p,%d,%" PRIu64 ",%.6f,%.6f,",
                mode, args->count, algo, args->seed, (void *)mid,
                mid ? mid->val : -1, perf_values->cycles, cache_miss_rate,
                l1_dcache_load_miss_rate) < 0)
        goto fail;

    if (perf_values->has_l1_dcache_prefetches) {
        if (fprintf(fp, "%" PRIu64 ",", perf_values->l1_dcache_prefetches) < 0)
            goto fail;
    } else {
        if (fprintf(fp, "N/A,") < 0)
            goto fail;
    }

    if (perf_values->has_dtlb_load_miss_rate) {
        if (fprintf(fp, "%.6f\n", dtlb_load_miss_rate) < 0)
            goto fail;
    } else {
        if (fprintf(fp, "N/A\n") < 0)
            goto fail;
    }

    if (fclose(fp) != 0)
        return -1;
    return 0;

fail:
    saved_errno = errno;
    (void)fclose(fp);
    errno = saved_errno;
    return -1;
}
