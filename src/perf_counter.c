#define _GNU_SOURCE

#include "perf_counter.h"

#include <errno.h>
#include <linux/perf_event.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

static int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                           int group_fd, unsigned long flags)
{
    return (int)syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

static uint64_t hw_cache_config(uint64_t cache_id, uint64_t op_id, uint64_t result_id)
{
    return cache_id | (op_id << 8) | (result_id << 16);
}

static int apply_ioctl_to_all(const struct perf_counters *counters, unsigned long req)
{
    if (ioctl(counters->leader_fd, req, 0) < 0)
        return -1;
    if (ioctl(counters->cycles_fd, req, 0) < 0)
        return -1;
    if (ioctl(counters->cache_refs_fd, req, 0) < 0)
        return -1;
    if (ioctl(counters->cache_misses_fd, req, 0) < 0)
        return -1;
    if (ioctl(counters->l1_dcache_loads_fd, req, 0) < 0)
        return -1;
    if (ioctl(counters->l1_dcache_load_misses_fd, req, 0) < 0)
        return -1;
    if (ioctl(counters->instructions_fd, req, 0) < 0)
        return -1;
    return 0;
}

static int open_counter(uint32_t type, uint64_t config, int group_fd, int disabled)
{
    struct perf_event_attr attr;

    memset(&attr, 0, sizeof(attr));
    attr.type = type;
    attr.size = sizeof(attr);
    attr.config = config;
    attr.disabled = disabled;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.exclude_idle = 1;

    return perf_event_open(&attr, 0, -1, group_fd, 0);
}

static int read_counter_value(int fd, uint64_t *value)
{
    ssize_t read_len = read(fd, value, sizeof(*value));
    return (read_len == (ssize_t)sizeof(*value)) ? 0 : -1;
}

int perf_counters_open(struct perf_counters *counters)
{
    if (!counters) {
        errno = EINVAL;
        return -1;
    }

    counters->leader_fd = -1;
    counters->cycles_fd = -1;
    counters->cache_refs_fd = -1;
    counters->cache_misses_fd = -1;
    counters->l1_dcache_loads_fd = -1;
    counters->l1_dcache_load_misses_fd = -1;
    counters->instructions_fd = -1;

    counters->leader_fd =
        open_counter(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK, -1, 1);
    if (counters->leader_fd < 0)
        goto fail;

    counters->cycles_fd =
        open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, -1, 1);
    if (counters->cycles_fd < 0)
        goto fail;

    counters->cache_refs_fd =
        open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES, -1, 1);
    if (counters->cache_refs_fd < 0)
        goto fail;

    counters->cache_misses_fd =
        open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, -1, 1);
    if (counters->cache_misses_fd < 0)
        goto fail;

    counters->l1_dcache_loads_fd =
        open_counter(PERF_TYPE_HW_CACHE,
                     hw_cache_config(PERF_COUNT_HW_CACHE_L1D,
                                     PERF_COUNT_HW_CACHE_OP_READ,
                                     PERF_COUNT_HW_CACHE_RESULT_ACCESS),
                     -1, 1);
    if (counters->l1_dcache_loads_fd < 0)
        goto fail;

    counters->l1_dcache_load_misses_fd =
        open_counter(PERF_TYPE_HW_CACHE,
                     hw_cache_config(PERF_COUNT_HW_CACHE_L1D,
                                     PERF_COUNT_HW_CACHE_OP_READ,
                                     PERF_COUNT_HW_CACHE_RESULT_MISS),
                     -1, 1);
    if (counters->l1_dcache_load_misses_fd < 0)
        goto fail;

    counters->instructions_fd =
        open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, -1, 1);
    if (counters->instructions_fd < 0)
        goto fail;

    return 0;

fail:
    perf_counters_close(counters);
    return -1;
}

void perf_counters_close(struct perf_counters *counters)
{
    if (!counters)
        return;

    if (counters->instructions_fd >= 0)
        close(counters->instructions_fd);
    if (counters->l1_dcache_load_misses_fd >= 0)
        close(counters->l1_dcache_load_misses_fd);
    if (counters->l1_dcache_loads_fd >= 0)
        close(counters->l1_dcache_loads_fd);
    if (counters->cache_misses_fd >= 0)
        close(counters->cache_misses_fd);
    if (counters->cache_refs_fd >= 0)
        close(counters->cache_refs_fd);
    if (counters->cycles_fd >= 0)
        close(counters->cycles_fd);
    if (counters->leader_fd >= 0)
        close(counters->leader_fd);

    counters->leader_fd = -1;
    counters->cycles_fd = -1;
    counters->cache_refs_fd = -1;
    counters->cache_misses_fd = -1;
    counters->l1_dcache_loads_fd = -1;
    counters->l1_dcache_load_misses_fd = -1;
    counters->instructions_fd = -1;
}

int perf_counters_reset(const struct perf_counters *counters)
{
    if (!counters || counters->leader_fd < 0 || counters->cycles_fd < 0 ||
        counters->cache_refs_fd < 0 || counters->cache_misses_fd < 0 ||
        counters->l1_dcache_loads_fd < 0 || counters->l1_dcache_load_misses_fd < 0 ||
        counters->instructions_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    return apply_ioctl_to_all(counters, PERF_EVENT_IOC_RESET);
}

int perf_counters_enable(const struct perf_counters *counters)
{
    if (!counters || counters->leader_fd < 0 || counters->cycles_fd < 0 ||
        counters->cache_refs_fd < 0 || counters->cache_misses_fd < 0 ||
        counters->l1_dcache_loads_fd < 0 || counters->l1_dcache_load_misses_fd < 0 ||
        counters->instructions_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    return apply_ioctl_to_all(counters, PERF_EVENT_IOC_ENABLE);
}

int perf_counters_disable(const struct perf_counters *counters)
{
    if (!counters || counters->leader_fd < 0 || counters->cycles_fd < 0 ||
        counters->cache_refs_fd < 0 || counters->cache_misses_fd < 0 ||
        counters->l1_dcache_loads_fd < 0 || counters->l1_dcache_load_misses_fd < 0 ||
        counters->instructions_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    return apply_ioctl_to_all(counters, PERF_EVENT_IOC_DISABLE);
}

int perf_counters_read(const struct perf_counters *counters,
                       struct perf_counter_values *values)
{
    if (!counters || !values) {
        errno = EINVAL;
        return -1;
    }

    memset(values, 0, sizeof(*values));

    if (read_counter_value(counters->leader_fd, &values->task_clock_ns) < 0)
        return -1;
    if (read_counter_value(counters->cycles_fd, &values->cycles) < 0)
        return -1;
    if (read_counter_value(counters->cache_refs_fd, &values->cache_refs) < 0)
        return -1;
    if (read_counter_value(counters->cache_misses_fd, &values->cache_misses) < 0)
        return -1;
    if (read_counter_value(counters->l1_dcache_loads_fd,
                           &values->l1_dcache_loads) < 0)
        return -1;
    if (read_counter_value(counters->l1_dcache_load_misses_fd,
                           &values->l1_dcache_load_misses) < 0)
        return -1;
    if (read_counter_value(counters->instructions_fd, &values->instructions) < 0)
        return -1;

    return 0;
}
