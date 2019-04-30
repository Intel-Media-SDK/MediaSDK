/***********************************************************************************

Copyright (C) 2018 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***********************************************************************************/

#include "cttmetrics_utils.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

enum drm_i915_gem_engine_class {
    I915_ENGINE_CLASS_RENDER        = 0,
    I915_ENGINE_CLASS_COPY          = 1,
    I915_ENGINE_CLASS_VIDEO         = 2,
    I915_ENGINE_CLASS_VIDEO_ENHANCE = 3,

    I915_ENGINE_CLASS_INVALID       = -1
};

enum drm_i915_pmu_engine_sample {
    I915_SAMPLE_BUSY = 0,
    I915_SAMPLE_WAIT = 1,
    I915_SAMPLE_SEMA = 2
};

#define I915_PMU_SAMPLE_BITS (4)
#define I915_PMU_SAMPLE_MASK (0xf)
#define I915_PMU_SAMPLE_INSTANCE_BITS (8)
#define I915_PMU_CLASS_SHIFT \
    (I915_PMU_SAMPLE_BITS + I915_PMU_SAMPLE_INSTANCE_BITS)

#define __I915_PMU_ENGINE(class, instance, sample) \
    ((class) << I915_PMU_CLASS_SHIFT | \
    (instance) << I915_PMU_SAMPLE_BITS | \
    (sample))

#define I915_PMU_ENGINE_BUSY(class, instance) \
    __I915_PMU_ENGINE(class, instance, I915_SAMPLE_BUSY)

#define I915_PMU_ENGINE_WAIT(class, instance) \
    __I915_PMU_ENGINE(class, instance, I915_SAMPLE_WAIT)

#define I915_PMU_ENGINE_SEMA(class, instance) \
    __I915_PMU_ENGINE(class, instance, I915_SAMPLE_SEMA)

#define __I915_PMU_OTHER(x) (__I915_PMU_ENGINE(0xff, 0xff, 0xf) + 1 + (x))

#define I915_PMU_ACTUAL_FREQUENCY	__I915_PMU_OTHER(0)
#define I915_PMU_REQUESTED_FREQUENCY	__I915_PMU_OTHER(1)

#define I915_PMU_LAST I915_PMU_REQUESTED_FREQUENCY

static bool is_engine_config(uint64_t config)
{
    return config < __I915_PMU_OTHER(0);
}

static int get_engine_id(uint64_t config)
{
    return config & 0xffff0;
}

static inline int
perf_event_open(struct perf_event_attr *attr,
    pid_t pid,
    int cpu,
    int group_fd,
    unsigned long flags)
{
#ifndef __NR_perf_event_open
#if defined(__i386__)
#define __NR_perf_event_open 336
#elif defined(__x86_64__)
#define __NR_perf_event_open 298
#else
#define __NR_perf_event_open 0
#endif
#endif
    attr->size = sizeof(*attr);
    return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

static uint64_t elapsed_ns(const struct timespec *start, const struct timespec *end)
{
    return ((end->tv_sec - start->tv_sec)*1e9 +
        (end->tv_nsec - start->tv_nsec));
}

static uint64_t i915_type_id(void)
{
    char buf[1024];
    int fd, n;

    fd = open("/sys/bus/event_source/devices/i915/type", 0);
    if (fd < 0) {
        n = -1;
    } else {
        n = read(fd, buf, sizeof(buf)-1);
        close(fd);
    }
    if (n < 0)
        return 0;

    buf[n] = '\0';
    return strtoull(buf, 0, 0);
}

static int perf_i915_open(int config, int group, int read_format)
{
    struct perf_event_attr attr;

    memset(&attr, 0, sizeof (attr));

    attr.type = i915_type_id();
    if (attr.type == 0)
        return -ENOENT;
    attr.config = config;

    attr.read_format = read_format;
    if (group != -1)
        attr.read_format &= ~PERF_FORMAT_GROUP;

    return perf_event_open(&attr, -1, 0, group, 0);
}

struct metric {
    int config;
    struct {
        uint64_t value;
        uint64_t time;
    } start;
    struct {
        uint64_t value;
        uint64_t time;
    } end;
};

#define I915_ENGINE_SAMPLE_COUNT (I915_SAMPLE_SEMA + 1)

struct metrics_group {
     metric metrics[I915_ENGINE_SAMPLE_COUNT]; /* 0 - I915_SAMPLE_BUSY/Global PMU, 1 - I915_SAMPLE_WAIT, 2 - I915_SAMPLE_SEMA */
     uint64_t num_metrics;
};

struct pmu_metrics {
    int fd;
    int read_format;
    uint64_t num_metrics;
    uint64_t num_groups;
    struct metrics_group* groups;
};

/* Order of metrics in configs[] is important:
  - don't mix samples of different engines;
  - don't mix global metrics and engine metrics; */
static int perf_init(struct pmu_metrics *pm, int num_configs, int configs[])
{
    int i, res;

    memset(pm, 0, sizeof(struct pmu_metrics));
    pm->fd = -1;
    pm->read_format =
        PERF_FORMAT_TOTAL_TIME_ENABLED |
        PERF_FORMAT_GROUP;
    pm->groups = (struct metrics_group*)calloc(num_configs, sizeof(struct metrics_group));
    if (!pm->groups)
        return -1;

    if (!num_configs)
        return -1;

    pm->num_groups = 0;

    int prev_config_id = is_engine_config(configs[0]) ? get_engine_id(configs[0]) : configs[0];

    /* Code block below will put per engine configs in the same groups,
     * all global metrics in their own groups */
    for (i = 0; i < num_configs; ++i) {
        int config_id = is_engine_config(configs[i]) ? get_engine_id(configs[i]) : configs[i];
        if (prev_config_id != config_id) {
            pm->num_groups++;
            prev_config_id = config_id;
        }

        if (pm->fd < 0)
            res = pm->fd = perf_i915_open(configs[i], -1, pm->read_format);
        else
            res = perf_i915_open(configs[i], pm->fd, pm->read_format);
        if (res >= 0) {
            if (is_engine_config(configs[i])) {
                int sample_type = I915_PMU_SAMPLE_MASK & configs[i];
                pm->groups[pm->num_groups].num_metrics++;
                pm->groups[pm->num_groups].metrics[sample_type].config = configs[i];
            }
            else {
                pm->groups[pm->num_groups].num_metrics = 1;
                pm->groups[pm->num_groups].metrics[0].config = configs[i]; // For global, let's use metrics[0] field
            }

            pm->num_metrics++;
        }
    }

    if (res >= 0)
        pm->num_groups++; /* For last config in the list */

    return 0;
}

static void perf_close(struct pmu_metrics *pm)
{
    if (pm->fd != -1 ) { close(pm->fd); pm->fd = -1; }
    if (pm->groups) { free(pm->groups); pm->groups= NULL; }
}

/* see 'man 2 perf_event_open' */
struct perf_read_format {
    uint64_t nr_values;     /* The number of events */
    uint64_t time_enabled;  /* if PERF_FORMAT_TOTAL_TIME_ENABLED */
    struct {
        uint64_t value;     /* The value of the event */
    } values[1024];
};

static int perf_read(struct pmu_metrics *pm)
{
    int read_format =
        PERF_FORMAT_TOTAL_TIME_ENABLED |
        PERF_FORMAT_GROUP;
    struct perf_read_format data;
    ssize_t len;

    if (pm->fd < 0)
        return -1;

    if (pm->read_format != read_format)
        return -1;

    len = read(pm->fd, &data, sizeof(data));
    if (len < 0) {
        return -1;
    }

    if (pm->num_metrics != data.nr_values)
        return -1;

    for (uint64_t i = 0, k = 0; i < pm->num_groups; ++i) {
        for (uint64_t j = 0; j < pm->groups[i].num_metrics; ++j) {
            pm->groups[i].metrics[j].start.value = pm->groups[i].metrics[j].end.value;
            pm->groups[i].metrics[j].start.time = pm->groups[i].metrics[j].end.time;
            pm->groups[i].metrics[j].end.value = data.values[k++].value;
            pm->groups[i].metrics[j].end.time = data.time_enabled;
        }
    }

    return 0;
}

static uint64_t perf_elapsed(struct metric* m)
{
    return m->end.value - m->start.value;
}

static uint64_t perf_elapsed_time(struct metric* m)
{
    return m->end.time - m->start.time;
}

struct i915_pmu_collector_ctx_t
{
    bool initialized;
    unsigned int sample_period_us;
    unsigned int metrics_count;

    unsigned int user_idx_map[CTT_MAX_METRIC_COUNT];
    unsigned int pm_idx_map[CTT_MAX_METRIC_COUNT];
    cttMetric metrics[CTT_MAX_METRIC_COUNT];

    pmu_metrics pm;
};

static i915_pmu_collector_ctx_t g_ctx = {
    .initialized  = false,
};

extern "C"
cttStatus CTTMetrics_PMU_Init()
{
    /* When adding new condigs, don't mix metrics from different groups!*/
    int configs[] = {
        /* Render engine metrics */
        I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_RENDER, 0), /*I915_PMU_ENGINE_WAIT(I915_ENGINE_CLASS_RENDER, 0), I915_PMU_ENGINE_SEMA(I915_ENGINE_CLASS_RENDER, 0),*/
        /* VDBOX metrics */
        I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO, 0),  /*I915_PMU_ENGINE_WAIT(I915_ENGINE_CLASS_VIDEO, 0), I915_PMU_ENGINE_SEMA(I915_ENGINE_CLASS_VIDEO, 0),*/
        /* 2nd VDBOX metrics */
        I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO, 1),  /*I915_PMU_ENGINE_WAIT(I915_ENGINE_CLASS_VIDEO, 1), I915_PMU_ENGINE_SEMA(I915_ENGINE_CLASS_VIDEO, 1),*/
        /* Blitter metrics */
        I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_COPY, 0),   /*I915_PMU_ENGINE_WAIT(I915_ENGINE_CLASS_COPY, 0), I915_PMU_ENGINE_SEMA(I915_ENGINE_CLASS_COPY, 0),*/
        /* VEBOX metrics */
        I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO_ENHANCE, 0), /*I915_PMU_ENGINE_WAIT(I915_ENGINE_CLASS_VIDEO_ENHANCE, 0), I915_PMU_ENGINE_SEMA(I915_ENGINE_CLASS_VIDEO_ENHANCE, 0),*/
        /* Global metrics */
        I915_PMU_ACTUAL_FREQUENCY,
    };
    int num_configs = sizeof(configs)/sizeof(configs[0]);

    if (g_ctx.initialized)
        return CTT_ERR_ALREADY_INITIALIZED;

    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.sample_period_us = 500*1000;

    cttStatus status = discover_path_to_gpu();
    if (CTT_ERR_NONE != status)
        return status;

    if (0 == perf_init(&g_ctx.pm, num_configs, configs)) {
        for (unsigned int i = 0; i < g_ctx.pm.num_groups; ++i) {
            g_ctx.pm_idx_map[g_ctx.metrics_count] = i;
            switch (g_ctx.pm.groups[i].metrics[0].config) {
                case I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_RENDER, 0):
                    g_ctx.metrics[g_ctx.metrics_count++] = CTT_USAGE_RENDER;
                    break;
                case I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO, 0):
                    g_ctx.metrics[g_ctx.metrics_count++] = CTT_USAGE_VIDEO;
                    break;
                case I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO, 1):
                    g_ctx.metrics[g_ctx.metrics_count++] = CTT_USAGE_VIDEO2;
                    break;
                case I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_COPY, 0):
                    g_ctx.metrics[g_ctx.metrics_count++] = CTT_USAGE_BLITTER;
                    break;
                case I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO_ENHANCE, 0):
                    g_ctx.metrics[g_ctx.metrics_count++] = CTT_USAGE_VIDEO_ENHANCEMENT;
                    break;
                case I915_PMU_ACTUAL_FREQUENCY:
                    g_ctx.metrics[g_ctx.metrics_count++] = CTT_AVG_GT_FREQ;
                    break;
                default:
                    break; // if we are here - that's a bug
            }
        }
    }

    if (!g_ctx.metrics_count) {
        status = CTT_ERR_DRIVER_NO_INSTRUMENTATION;
    } else {
        status = CTT_ERR_NONE;
        g_ctx.initialized = true;
    }

    return status;
}

extern "C"
void CTTMetrics_PMU_Close()
{
    if (!g_ctx.initialized)
        return;

    if (g_ctx.pm.fd != -1) {
        perf_close(&g_ctx.pm);
    }
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.initialized = false;
}

extern "C"
cttStatus CTTMetrics_PMU_SetSamplePeriod(unsigned int in_period)
{
    if (!g_ctx.initialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (in_period > 1000 || in_period < 10)
        return CTT_ERR_OUT_OF_RANGE;

    g_ctx.sample_period_us = in_period * 1000;

    return CTT_ERR_NONE;
}

extern "C"
cttStatus CTTMetrics_PMU_SetSampleCount(unsigned int in_num)
{
    if (!g_ctx.initialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (in_num > 1000 || in_num < 1)
        return CTT_ERR_OUT_OF_RANGE;

    // i915 PMU does not support interrupt sampling, but it supports
    // arbitary (any) user space sampling. The above check is just
    // to comply to metrics_monitor definition
    return CTT_ERR_NONE;
}

extern "C"
cttStatus CTTMetrics_PMU_GetMetricCount(unsigned int* out_count)
{
    if (!g_ctx.initialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (!out_count)
        return CTT_ERR_NULL_PTR;

    *out_count = g_ctx.metrics_count;
    return CTT_ERR_NONE;
}

extern "C"
cttStatus CTTMetrics_PMU_GetMetricInfo(unsigned int count, cttMetric* out_metric_ids)
{
    if (!g_ctx.initialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (!out_metric_ids)
        return CTT_ERR_NULL_PTR;

    if (count > g_ctx.metrics_count)
        return CTT_ERR_OUT_OF_RANGE;

    for (unsigned int i=0; i < g_ctx.metrics_count; ++i) {
        out_metric_ids[i] = g_ctx.metrics[i];
    }

    return CTT_ERR_NONE;
}

extern "C"
cttStatus CTTMetrics_PMU_Subscribe(unsigned int count, cttMetric* in_metric_ids)
{
    if (!g_ctx.initialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (!in_metric_ids)
        return CTT_ERR_NULL_PTR;

    if (count > g_ctx.metrics_count)
        return CTT_ERR_OUT_OF_RANGE;

    unsigned int na_metric_cnt = 0;
    for (unsigned int i = 0; i < count; ++i) {
        g_ctx.user_idx_map[i] = g_ctx.metrics_count;

        for (unsigned int j = 0; j < g_ctx.metrics_count; ++j) {
            if (in_metric_ids[i] == g_ctx.metrics[j]) {
                g_ctx.user_idx_map[i] = j;
                break;
            }
        }
        if (g_ctx.user_idx_map[i] == g_ctx.metrics_count) ++na_metric_cnt;
    }

    return (na_metric_cnt)? CTT_WRN_METRIC_UNAVAILABLE: CTT_ERR_NONE;
}

extern "C"
cttStatus CTTMetrics_PMU_GetValue(unsigned int count, float* out_metric_values)
{
    if (!g_ctx.initialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (!out_metric_values)
        return CTT_ERR_NULL_PTR;

    if (count > g_ctx.metrics_count)
        return CTT_ERR_OUT_OF_RANGE;

    unsigned int metric_idx, pm_metric_idx;
    double value;

    if (g_ctx.pm.num_groups && 0 != perf_read(&g_ctx.pm))
        return CTT_ERR_DRIVER_NO_INSTRUMENTATION;

    usleep(g_ctx.sample_period_us);

    if (g_ctx.pm.num_groups && 0 != perf_read(&g_ctx.pm))
        return CTT_ERR_DRIVER_NO_INSTRUMENTATION;

    metrics_group* group = NULL;

    for (unsigned int i = 0; i < count; ++i) {
        metric_idx = g_ctx.user_idx_map[i];

        if (g_ctx.metrics_count != metric_idx) {
            pm_metric_idx = g_ctx.pm_idx_map[metric_idx];
            group = &g_ctx.pm.groups[pm_metric_idx];

            value = perf_elapsed(&group->metrics[I915_SAMPLE_BUSY]);
            if (g_ctx.metrics[metric_idx] == CTT_AVG_GT_FREQ) {
                value *= 1000000000; // to compensate time in ns
            }
            else {
                value *= 100; // we need %
            }
            value /= (double)perf_elapsed_time(&group->metrics[I915_SAMPLE_BUSY]);
        } else {
            value = 0.0; // not subscribed/unavailable metrics are always idle
        }

        out_metric_values[i] = value;
    }

    return CTT_ERR_NONE;
}
