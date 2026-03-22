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

static void init_perf_counters(struct perf_counters *counters)
{
    counters->leader_fd = -1;
    counters->task_clock_fd = -1;
    counters->cycles_fd = -1;
    counters->cache_refs_fd = -1;
    counters->cache_misses_fd = -1;
    counters->l1_dcache_loads_fd = -1;
    counters->l1_dcache_load_misses_fd = -1;
    counters->l1_dcache_prefetches_fd = -1;
}

static int required_counters_ready(const struct perf_counters *counters)
{
    return counters && counters->leader_fd >= 0 && counters->task_clock_fd >= 0 &&
           counters->cycles_fd >= 0 && counters->cache_refs_fd >= 0 &&
           counters->cache_misses_fd >= 0 && counters->l1_dcache_loads_fd >= 0 &&
           counters->l1_dcache_load_misses_fd >= 0;
}

static void close_counter_fd(int *fd)
{
    if (!fd || *fd < 0)
        return;
    close(*fd);
    *fd = -1;
}

static uint64_t hw_cache_config(uint64_t cache_id, uint64_t op_id, uint64_t result_id)
{
    return cache_id | (op_id << 8) | (result_id << 16);
}

static int apply_ioctl_to_all(const struct perf_counters *counters, unsigned long req)
{
    /*
     * Keep task-clock/cycles/prefetch as independent events, and keep cache
     * ratio events in one atomic hardware group.
     */
    if (ioctl(counters->task_clock_fd, req, 0) < 0)
        return -1;
    if (ioctl(counters->cycles_fd, req, 0) < 0)
        return -1;
    if (counters->l1_dcache_prefetches_fd >= 0 &&
        ioctl(counters->l1_dcache_prefetches_fd, req, 0) < 0)
        return -1;
    return ioctl(counters->leader_fd, req, PERF_IOC_FLAG_GROUP);
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
    attr.exclude_guest = 1;
    attr.exclude_idle = 1;
    attr.read_format =
        PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;

    return perf_event_open(&attr, 0, -1, group_fd, PERF_FLAG_FD_CLOEXEC);
}

struct perf_read_value {
    uint64_t value;
    uint64_t time_enabled;
    uint64_t time_running;
};

static int read_counter_value(int fd, uint64_t *value)
{
    struct perf_read_value raw = { 0 };
    long double scaled = 0.0L;
    ssize_t read_len;

    if (!value) {
        errno = EINVAL;
        return -1;
    }

    do {
        read_len = read(fd, &raw, sizeof(raw));
    } while (read_len < 0 && errno == EINTR);

    if (read_len != (ssize_t)sizeof(raw)) {
        if (read_len >= 0)
            errno = EIO;
        return -1;
    }
    if (raw.time_running == 0 || raw.time_enabled == 0) {
        *value = raw.value;
        return 0;
    }

    if (raw.time_running >= raw.time_enabled) {
        *value = raw.value;
        return 0;
    }

    scaled = (long double)raw.value * (long double)raw.time_enabled /
             (long double)raw.time_running;
    if (scaled >= (long double)UINT64_MAX)
        *value = UINT64_MAX;
    else
        *value = (uint64_t)(scaled + 0.5L);
    return 0;
}

int perf_counters_open(struct perf_counters *counters)
{
    if (!counters) {
        errno = EINVAL;
        return -1;
    }

    init_perf_counters(counters);

    counters->task_clock_fd =
        open_counter(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK, -1, 1);
    if (counters->task_clock_fd < 0)
        goto fail;

    counters->cycles_fd =
        open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, -1, 1);
    if (counters->cycles_fd < 0)
        goto fail;

    counters->leader_fd =
        open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES, -1, 1);
    if (counters->leader_fd < 0)
        goto fail;
    counters->cache_refs_fd = counters->leader_fd;

    counters->cache_misses_fd =
        open_counter(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES,
                     counters->leader_fd, 0);
    if (counters->cache_misses_fd < 0)
        goto fail;

    counters->l1_dcache_loads_fd =
        open_counter(PERF_TYPE_HW_CACHE,
                     hw_cache_config(PERF_COUNT_HW_CACHE_L1D,
                                     PERF_COUNT_HW_CACHE_OP_READ,
                                     PERF_COUNT_HW_CACHE_RESULT_ACCESS),
                     counters->leader_fd, 0);
    if (counters->l1_dcache_loads_fd < 0)
        goto fail;

    counters->l1_dcache_load_misses_fd =
        open_counter(PERF_TYPE_HW_CACHE,
                     hw_cache_config(PERF_COUNT_HW_CACHE_L1D,
                                     PERF_COUNT_HW_CACHE_OP_READ,
                                     PERF_COUNT_HW_CACHE_RESULT_MISS),
                     counters->leader_fd, 0);
    if (counters->l1_dcache_load_misses_fd < 0)
        goto fail;

    /*
     * Prefetch events are optional across different PMUs. Keep runner
     * functional if this CPU/kernel cannot open them.
     */
    counters->l1_dcache_prefetches_fd =
        open_counter(PERF_TYPE_HW_CACHE,
                     hw_cache_config(PERF_COUNT_HW_CACHE_L1D,
                                     PERF_COUNT_HW_CACHE_OP_PREFETCH,
                                     PERF_COUNT_HW_CACHE_RESULT_ACCESS),
                     -1, 1);
    if (counters->l1_dcache_prefetches_fd < 0) {
        counters->l1_dcache_prefetches_fd = -1;
    }

    return 0;

fail:
    perf_counters_close(counters);
    return -1;
}

void perf_counters_close(struct perf_counters *counters)
{
    if (!counters)
        return;

    close_counter_fd(&counters->l1_dcache_prefetches_fd);
    close_counter_fd(&counters->l1_dcache_load_misses_fd);
    close_counter_fd(&counters->l1_dcache_loads_fd);
    close_counter_fd(&counters->cache_misses_fd);
    if (counters->cache_refs_fd != counters->leader_fd)
        close_counter_fd(&counters->cache_refs_fd);
    close_counter_fd(&counters->task_clock_fd);
    close_counter_fd(&counters->cycles_fd);
    close_counter_fd(&counters->leader_fd);
    counters->cache_refs_fd = -1;
}

int perf_counters_reset(const struct perf_counters *counters)
{
    if (!required_counters_ready(counters)) {
        errno = EINVAL;
        return -1;
    }

    return apply_ioctl_to_all(counters, PERF_EVENT_IOC_RESET);
}

int perf_counters_enable(const struct perf_counters *counters)
{
    if (!required_counters_ready(counters)) {
        errno = EINVAL;
        return -1;
    }

    return apply_ioctl_to_all(counters, PERF_EVENT_IOC_ENABLE);
}

int perf_counters_disable(const struct perf_counters *counters)
{
    if (!required_counters_ready(counters)) {
        errno = EINVAL;
        return -1;
    }

    return apply_ioctl_to_all(counters, PERF_EVENT_IOC_DISABLE);
}

int perf_counters_read(const struct perf_counters *counters,
                       struct perf_counter_values *values)
{
    if (!required_counters_ready(counters) || !values) {
        errno = EINVAL;
        return -1;
    }

    memset(values, 0, sizeof(*values));

    if (read_counter_value(counters->task_clock_fd, &values->task_clock_ns) < 0)
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

    if (counters->l1_dcache_prefetches_fd >= 0 &&
        read_counter_value(counters->l1_dcache_prefetches_fd,
                           &values->l1_dcache_prefetches) < 0)
        return -1;
    values->has_l1_dcache_prefetches = counters->l1_dcache_prefetches_fd >= 0;

    return 0;
}
