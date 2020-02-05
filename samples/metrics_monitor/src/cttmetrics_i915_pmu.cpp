/***********************************************************************************

Copyright (C) 2018-2020 Intel Corporation.  All rights reserved.

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
#include <sys/sysmacros.h>
#include <time.h>
#include <unistd.h>
#include <i915_drm.h>
#include <xf86drm.h>

static bool has_param(int fd, int param)
{
    drm_i915_getparam_t gp;
    int tmp = 0;

    memset(&gp, 0, sizeof(gp));
    gp.value = &tmp;
    gp.param = param;

    if (drmIoctl(fd, DRM_IOCTL_I915_GETPARAM, &gp))
    {
        return false;
    }

    return tmp > 0;
}

static bool is_engine_config(uint64_t config)
{
    return config < __I915_PMU_OTHER(0);
}

static uint8_t get_engine_sample(uint64_t config)
{
    return config & I915_PMU_SAMPLE_MASK;
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

static char *bus_address(int i915, char *path, int pathlen)
{
    struct stat st = {};
    if (fstat(i915, &st) || !S_ISCHR(st.st_mode))
        return NULL;

    snprintf(path, pathlen, "/sys/dev/char/%d:%d",
         major(st.st_rdev), minor(st.st_rdev));

    int len = -1;
    int dir = open(path, O_RDONLY);
    if (dir != -1) {
        len = readlinkat(dir, "device", path, pathlen - 1);
        close(dir);
    }
    if (len < 0)
        return NULL;

    path[len] = '\0';

    /* strip off the relative path */
    char * s = strrchr(path, '/');
    if (s)
        memmove(path, s + 1, len - (s - path) + 1);

    return path;
}

static const char *i915_perf_device(int i915, char *buf, long unsigned int buflen)
{
    static const char * prefix = "i915_";
    const long unsigned int plen = strlen(prefix);

    if (!buf || buflen < plen)
        return NULL;

    memcpy(buf, prefix, plen);

    if (!bus_address(i915, buf + plen, buflen - plen) ||
        strcmp(buf + plen, "0000:00:02.0") == 0) /* legacy name for igfx */
        buf[plen - 1] = '\0';

    /* Convert all colons in the address to '_' */
    for (char *s = buf; *s; s++)
        if (*s == ':')
            *s = '_';

    return buf;
}

static uint64_t i915_type_id(const char *device)
{
    if (!device)
        return 0;

    char buf[120];
    snprintf(buf, sizeof(buf), "/sys/bus/event_source/devices/%s/type", device);

    int fd = open(buf, O_RDONLY);
    if (fd < 0)
        return 0;

    ssize_t ret = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (ret < 1)
        return 0;

    buf[ret] = '\0';

    return strtoull(buf, NULL, 0);
}

static int perf_i915_open(int gem_fd, int config, int group, int read_format)
{
    char buf[80];

    struct perf_event_attr attr = {};
    attr.type = i915_type_id(i915_perf_device(gem_fd, buf, sizeof(buf)));
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

struct i915_pmu_collector_ctx_t
{
    bool initialized;
    unsigned int sample_period_us;
    unsigned int metrics_count;

    unsigned int user_idx_map[CTT_MAX_METRIC_COUNT];
    unsigned int pm_idx_map[CTT_MAX_METRIC_COUNT];
    cttMetric metrics[CTT_MAX_METRIC_COUNT];

    pmu_metrics pm;

    int gem_fd;
    bool has_semaphores;
    bool has_preemption;
};

/* Order of metrics in configs[] is important:
  - don't mix samples of different engines;
  - don't mix global metrics and engine metrics; */
static int perf_init(struct i915_pmu_collector_ctx_t *ctx, int num_configs, int configs[])
{
    int i, res = -1;

    memset(&ctx->pm, 0, sizeof(struct pmu_metrics));
    ctx->pm.fd = -1;
    ctx->pm.read_format =
        PERF_FORMAT_TOTAL_TIME_ENABLED |
        PERF_FORMAT_GROUP;
    ctx->pm.groups = (struct metrics_group*)calloc(num_configs, sizeof(struct metrics_group));
    if (!ctx->pm.groups)
        return -1;

    if (!num_configs)
        return -1;

    ctx->pm.num_groups = 0;

    int prev_config_id = is_engine_config(configs[0]) ? get_engine_id(configs[0]) : configs[0];

    /* Code block below will put per engine configs in the same groups,
     * all global metrics in their own groups */
    for (i = 0; i < num_configs; ++i) {

        bool is_sema_or_wait_config = is_engine_config(configs[i]) && (I915_SAMPLE_BUSY != get_engine_sample(configs[i]));
        /* Skip SEMA/WAIT configs if semaphores are not enabled in i915 scheduler or they're enabled but preemption is missing */
        if (is_sema_or_wait_config && (!ctx->has_semaphores || !ctx->has_preemption))
            continue;

        int config_id = is_engine_config(configs[i]) ? get_engine_id(configs[i]) : configs[i];
        if (prev_config_id != config_id) {
            ctx->pm.num_groups++;
            prev_config_id = config_id;
        }

        if (ctx->pm.fd < 0)
            res = ctx->pm.fd = perf_i915_open(ctx->gem_fd, configs[i], -1, ctx->pm.read_format);
        else
            res = perf_i915_open(ctx->gem_fd, configs[i], ctx->pm.fd, ctx->pm.read_format);
        if (res >= 0) {
            if (is_engine_config(configs[i])) {
                int sample_type = I915_PMU_SAMPLE_MASK & configs[i];
                ctx->pm.groups[ctx->pm.num_groups].num_metrics++;
                ctx->pm.groups[ctx->pm.num_groups].metrics[sample_type].config = configs[i];
            }
            else {
                ctx->pm.groups[ctx->pm.num_groups].num_metrics = 1;
                ctx->pm.groups[ctx->pm.num_groups].metrics[0].config = configs[i]; // For global, let's use metrics[0] field
            }

            ctx->pm.num_metrics++;
        }
    }

    if (res >= 0)
        ctx->pm.num_groups++; /* For last config in the list */

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

static i915_pmu_collector_ctx_t g_ctx = {
    .initialized  = false,
};

extern "C"
cttStatus CTTMetrics_PMU_Init(const char *device)
{
    /* When adding new condigs, don't mix metrics from different groups!*/
    int configs[] = {
        /* Render engine metrics */
        I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_RENDER, 0),
        I915_PMU_ENGINE_WAIT(I915_ENGINE_CLASS_RENDER, 0),
        I915_PMU_ENGINE_SEMA(I915_ENGINE_CLASS_RENDER, 0),
        /* VDBOX metrics */
        I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO, 0),
        I915_PMU_ENGINE_WAIT(I915_ENGINE_CLASS_VIDEO, 0),
        I915_PMU_ENGINE_SEMA(I915_ENGINE_CLASS_VIDEO, 0),
        /* 2nd VDBOX metrics */
        I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO, 1),
        I915_PMU_ENGINE_WAIT(I915_ENGINE_CLASS_VIDEO, 1),
        I915_PMU_ENGINE_SEMA(I915_ENGINE_CLASS_VIDEO, 1),
        /* Blitter metrics */
        I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_COPY, 0),
        I915_PMU_ENGINE_WAIT(I915_ENGINE_CLASS_COPY, 0),
        I915_PMU_ENGINE_SEMA(I915_ENGINE_CLASS_COPY, 0),
        /* VEBOX metrics */
        I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO_ENHANCE, 0),
        I915_PMU_ENGINE_WAIT(I915_ENGINE_CLASS_VIDEO_ENHANCE, 0),
        I915_PMU_ENGINE_SEMA(I915_ENGINE_CLASS_VIDEO_ENHANCE, 0),
        /* Global metrics */
        I915_PMU_ACTUAL_FREQUENCY,
    };
    int num_configs = sizeof(configs)/sizeof(configs[0]);

    if (g_ctx.initialized)
        return CTT_ERR_ALREADY_INITIALIZED;

    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.sample_period_us = 500*1000;

    g_ctx.gem_fd = device ? open(device, O_RDWR) : drmOpenWithType("i915", NULL, DRM_NODE_RENDER);

    if (g_ctx.gem_fd >= 0) {
        g_ctx.has_semaphores = has_param(g_ctx.gem_fd, I915_PARAM_HAS_SEMAPHORES);
        g_ctx.has_preemption = has_param(g_ctx.gem_fd, I915_SCHEDULER_CAP_PREEMPTION);
    } else
        return CTT_ERR_DRIVER_NOT_FOUND;

    if (0 == perf_init(&g_ctx, num_configs, configs)) {
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
        return CTT_ERR_DRIVER_NO_INSTRUMENTATION;
    } else {
        g_ctx.initialized = true;
        return CTT_ERR_NONE;
    }
}

extern "C"
void CTTMetrics_PMU_Close()
{
    if (!g_ctx.initialized)
        return;

    if (g_ctx.pm.fd != -1) {
        perf_close(&g_ctx.pm);
    }
    if (g_ctx.gem_fd != -1) {
        close(g_ctx.gem_fd);
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
    double time;

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
            time = (double)perf_elapsed_time(&group->metrics[I915_SAMPLE_BUSY]);

            double value_sema = 0, value_wait = 0;
            double time_sema = 0, time_wait = 0;
            bool is_sema_and_wait_available = is_engine_config(group->metrics[I915_SAMPLE_BUSY].config) && (3 == group->num_metrics);

            if (is_sema_and_wait_available) {
                value_sema = perf_elapsed(&group->metrics[I915_SAMPLE_SEMA]);
                time_sema = (double)perf_elapsed_time(&group->metrics[I915_SAMPLE_SEMA]);

                value_wait = perf_elapsed(&group->metrics[I915_SAMPLE_WAIT]);
                time_wait = (double)perf_elapsed_time(&group->metrics[I915_SAMPLE_WAIT]);
            }

            if (g_ctx.metrics[metric_idx] == CTT_AVG_GT_FREQ) {
                value *= 1000000000; // to compensate time in ns
            }
            else {
                value *= 100; // we need %
                value_sema *= 100;
                value_wait *= 100;
            }

            value /= time;
            if (is_sema_and_wait_available) {
                value_sema /= time_sema;
                value_wait /= time_wait;
                // due to sampling nature of sema counter, very frequently we get (sema value) > (busy value)
                // and there is no other decisions besides of clamping sema to busy
                value_sema = (value_sema > value) ? value : value_sema;

                value -= value_sema + value_wait; // Substract sema and wait from busy metric
            }
        } else {
            value = 0.0; // not subscribed/unavailable metrics are always idle
        }

        out_metric_values[i] = value;
    }

    return CTT_ERR_NONE;
}
