/***********************************************************************************

Copyright (C) 2014-2020 Intel Corporation.  All rights reserved.

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

File Name: cttmetrics.cpp

***********************************************************************************/

#include "cttmetrics.h"

#include <stdio.h>

struct CttMetricsCollector
{
    cttStatus (*Init)(const char *device);
    void (*Close)();
    cttStatus (*SetSamplePeriod)(unsigned int in_period);
    cttStatus (*SetSampleCount)(unsigned int in_num);
    cttStatus (*GetMetricCount)(unsigned int* out_count);
    cttStatus (*GetMetricInfo)(unsigned int count, cttMetric* out_metric_ids);
    cttStatus (*Subscribe)(unsigned int count, cttMetric* in_metric_ids);
    cttStatus (*GetValue)(unsigned int count, float* out_metric_values);
};

extern "C" {
    cttStatus CTTMetrics_Custom_Init(const char *device);
    void CTTMetrics_Custom_Close();
    cttStatus CTTMetrics_Custom_SetSamplePeriod(unsigned int in_period);
    cttStatus CTTMetrics_Custom_SetSampleCount(unsigned int in_num);
    cttStatus CTTMetrics_Custom_GetMetricCount(unsigned int* out_count);
    cttStatus CTTMetrics_Custom_GetMetricInfo(unsigned int count, cttMetric* out_metric_ids);
    cttStatus CTTMetrics_Custom_Subscribe(unsigned int count, cttMetric* in_metric_id);
    cttStatus CTTMetrics_Custom_GetValue(unsigned int count, float* out_metric_values);
}

extern "C" {
    cttStatus CTTMetrics_PMU_Init(const char *device);
    void CTTMetrics_PMU_Close();
    cttStatus CTTMetrics_PMU_SetSamplePeriod(unsigned int in_period);
    cttStatus CTTMetrics_PMU_SetSampleCount(unsigned int in_num);
    cttStatus CTTMetrics_PMU_GetMetricCount(unsigned int* out_count);
    cttStatus CTTMetrics_PMU_GetMetricInfo(unsigned int count, cttMetric* out_metric_ids);
    cttStatus CTTMetrics_PMU_Subscribe(unsigned int count, cttMetric* in_metric_id);
    cttStatus CTTMetrics_PMU_GetValue(unsigned int count, float* out_metric_values);
}

// List of collectors in the priority order. Library will try to inialize
// them one by one. First collector successfully initialized will be used.
static CttMetricsCollector g_Collectors[] =
{
    // This collector works thru i915 PMU API. It will work for:
    //  * User with root priviligies
    //  * Application with CAP_SYS_ADMIN capability (setcap cap_sys_admin+ep ./application)
    //  * Any user if /proc/sys/kernel/perf_event_paranoid is less than 1
    {
        CTTMetrics_PMU_Init,
        CTTMetrics_PMU_Close,
        CTTMetrics_PMU_SetSamplePeriod,
        CTTMetrics_PMU_SetSampleCount,
        CTTMetrics_PMU_GetMetricCount,
        CTTMetrics_PMU_GetMetricInfo,
        CTTMetrics_PMU_Subscribe,
        CTTMetrics_PMU_GetValue
    },
    // This collector requires custom (patched) i915 driver.
    // It will work only for user with root priviligies (it access debugfs).
    {
        CTTMetrics_Custom_Init,
        CTTMetrics_Custom_Close,
        CTTMetrics_Custom_SetSamplePeriod,
        CTTMetrics_Custom_SetSampleCount,
        CTTMetrics_Custom_GetMetricCount,
        CTTMetrics_Custom_GetMetricInfo,
        CTTMetrics_Custom_Subscribe,
        CTTMetrics_Custom_GetValue
    },
};
static CttMetricsCollector* g_SelectedCollector = NULL;

extern "C"
cttStatus CTTMetrics_Init(const char *device)
{
    cttStatus status = CTT_ERR_DRIVER_NO_INSTRUMENTATION;

    if (g_SelectedCollector)
        return CTT_ERR_ALREADY_INITIALIZED;

    for (size_t i=0; i < sizeof(g_Collectors)/sizeof(g_Collectors[0]); ++i) {
        status = g_Collectors[i].Init(device);
        if (status == CTT_ERR_NONE) {
            g_SelectedCollector = &g_Collectors[i];
            break;
        }
    }
    return status;
}

extern "C"
void CTTMetrics_Close()
{
    if (!g_SelectedCollector)
        return;
    g_SelectedCollector->Close();
    g_SelectedCollector = NULL;
}

extern "C"
cttStatus CTTMetrics_SetSamplePeriod(unsigned int in_period)
{
    if (!g_SelectedCollector)
        return CTT_ERR_NOT_INITIALIZED;
    return g_SelectedCollector->SetSamplePeriod(in_period);
}

extern "C"
cttStatus CTTMetrics_SetSampleCount(unsigned int in_num)
{
    if (!g_SelectedCollector)
        return CTT_ERR_NOT_INITIALIZED;
    return g_SelectedCollector->SetSampleCount(in_num);
}

extern "C"
cttStatus CTTMetrics_GetMetricCount(unsigned int* out_count)
{
    if (!g_SelectedCollector)
        return CTT_ERR_NOT_INITIALIZED;
    return g_SelectedCollector->GetMetricCount(out_count);
}

extern "C"
cttStatus CTTMetrics_GetMetricInfo(unsigned int count, cttMetric* out_metric_ids)
{
    if (!g_SelectedCollector)
        return CTT_ERR_NOT_INITIALIZED;
    return g_SelectedCollector->GetMetricInfo(count, out_metric_ids);
}

extern "C"
cttStatus CTTMetrics_Subscribe(unsigned int count, cttMetric* in_metric_ids)
{
    if (!g_SelectedCollector)
        return CTT_ERR_NOT_INITIALIZED;
    return g_SelectedCollector->Subscribe(count, in_metric_ids);
}

extern "C"
cttStatus CTTMetrics_GetValue(unsigned int count, float* out_metric_values)
{
    if (!g_SelectedCollector)
        return CTT_ERR_NOT_INITIALIZED;
    return g_SelectedCollector->GetValue(count, out_metric_values);
}
