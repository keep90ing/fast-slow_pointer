#ifndef BENCH_CSV_H
#define BENCH_CSV_H

struct runner_args;
struct list_node;
struct perf_counter_values;

int bench_csv_append_row(const struct runner_args *args,
                         const struct list_node *mid,
                         const struct perf_counter_values *perf_values,
                         double cache_miss_rate,
                         double l1_dcache_load_miss_rate,
                         double dtlb_load_miss_rate);

#endif
