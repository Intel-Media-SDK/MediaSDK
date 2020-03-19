/* ****************************************************************************** *\

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

File Name: cttmetrics_sample.cpp

\* ****************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "cttmetrics.h"

#define MIN_PERIOD_MS  10
#define MAX_PERIOD_MS  1000
#define DEFAULT_PERIOD_MS  500

#define MIN_NUMSAMPLES  1
#define MAX_NUMSAMPLES  1000
#define DEFAULT_NUMSAMPLES 100

volatile sig_atomic_t run = 1;

void signal_handler(int signo)
{
    if (SIGINT == signo)
    {
        run = 0;
    }
}

static void
usage(const char *appname)
{
    printf("metrics_monitor - Monitors GPU usage per engine\n"
        "\n"
        "Usage: %s [OPTION]\n"
        "\n"
        "\tThe following parameters are optional:\n\n"
        "\t[-h]          Show this help text.\n"
        "\t[-s <num>]    Number of metric samples to collect during sampling period(valid range %u..%u, default %u).\n"
        "\t[-p <ms>]     Sampling period in milliseconds(valid range %u..%u, default %u).\n"
        "\t[-d <path>]   Path to gfx device (like /dev/dri/card* or /dev/dri/renderD*).\n"
        "\t              If device is not set, the tool uses i915 render node device with smallest number."
        "\n",
        appname, MIN_NUMSAMPLES, MAX_NUMSAMPLES, DEFAULT_NUMSAMPLES, MIN_PERIOD_MS, MAX_PERIOD_MS, DEFAULT_PERIOD_MS);
}

int main(int argc, char *argv[])
{
    cttStatus status = CTT_ERR_NONE;
    cttMetric metrics_ids[] = {CTT_USAGE_RENDER, CTT_USAGE_VIDEO, CTT_USAGE_VIDEO_ENHANCEMENT, CTT_AVG_GT_FREQ, CTT_USAGE_VIDEO2};
    unsigned int metric_cnt = sizeof(metrics_ids)/sizeof(metrics_ids[0]);

    unsigned int num_samples = DEFAULT_NUMSAMPLES;
    unsigned int period_ms = DEFAULT_PERIOD_MS;
    char* device_path = NULL;
    int ch;

    /* Parse options */
    while ((ch = getopt(argc, argv, "d:s:p:h")) != -1) {
        switch (ch) {
        case 'd':
            device_path = optarg;
            break;
        case 's':
            num_samples = atoi(optarg);
            if (num_samples < MIN_NUMSAMPLES || num_samples > MAX_NUMSAMPLES) {
                fprintf(stderr, "%u is an invalid number of metric samples\n\n", num_samples);
                usage(argv[0]);
                exit(1);
            }
            break;
        case 'p':
            period_ms = atoi(optarg);
            if (period_ms < MIN_PERIOD_MS || period_ms > MAX_PERIOD_MS) {
                fprintf(stderr, "%u is an invalid number sampling period\n\n", period_ms);
                usage(argv[0]);
                exit(1);
            }
            break;
        case 'h':
            usage(argv[0]);
            exit(0);
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    signal(SIGINT, signal_handler);

    status = CTTMetrics_Init(device_path);
    if (CTT_ERR_NONE != status)
    {
        fprintf(stderr, "ERROR: Failed to initialize metrics monitor, error code %d\n", (int)status);
        return 1;
    }

    unsigned int metric_all_cnt = 0;
    status = CTTMetrics_GetMetricCount(&metric_all_cnt);
    if (CTT_ERR_NONE != status)
    {
        fprintf(stderr, "ERROR: Failed to get number of metrics available, error code %d\n", (int)status);
        return 1;
    }

    cttMetric metric_all_ids[CTT_MAX_METRIC_COUNT] = {CTT_WRONG_METRIC_ID};
    status = CTTMetrics_GetMetricInfo(metric_all_cnt, metric_all_ids);
    if (CTT_ERR_NONE != status)
    {
        fprintf(stderr, "ERROR: Failed to get metrics info, error code %d\n", (int)status);
        return 1;
    }

    unsigned int i;
    bool isVideo2 = false;
    for (i = 0; i < metric_all_cnt; ++i)
    {
        if (CTT_USAGE_VIDEO2 == metric_all_ids[i])
          isVideo2 = true;
    }

    if (false == isVideo2)
        metric_cnt = metric_cnt - 1; // exclude video2 usage metric

    bool isFreq = false;
    for (i = 0; i < metric_all_cnt; ++i)
    {
        if (CTT_AVG_GT_FREQ == metric_all_ids[i])
          isFreq = true;
    }

    if (false == isFreq)
        metric_cnt = metric_cnt - 1; // exclude average frequency metric

    status = CTTMetrics_Subscribe(metric_cnt, metrics_ids);
    if (CTT_ERR_NONE != status)
    {
        fprintf(stderr, "ERROR: Failed to subscribe for metrics, error code %d\n", (int)status);
        return 1;
    }

    status = CTTMetrics_SetSampleCount(num_samples);
    if (CTT_ERR_NONE != status)
    {
        fprintf(stderr, "ERROR: Failed to set number of samples, error code %d\n", (int)status);
        return 1;
    }

    status = CTTMetrics_SetSamplePeriod(period_ms);
    if (CTT_ERR_NONE != status)
    {
        fprintf(stderr, "ERROR: Failed to set measure interval, error code %d\n", (int)status);
        return 1;
    }

    float metric_values[metric_cnt];
    memset(metric_values, 0, (size_t)metric_cnt * sizeof(float));

    while(run)
    {
        status = CTTMetrics_GetValue(metric_cnt, metric_values);
        if (CTT_ERR_NONE != status)
        {
            fprintf(stderr, "ERROR: Failed to get metrics, error code %d\n", status);
            return 1;
        }
        printf("RENDER usage: %3.2f,\tVIDEO usage: %3.2f,\tVIDEO_E usage: %3.2f", metric_values[0], metric_values[1], metric_values[2]);

        if (true == isVideo2)
            printf("\tVIDEO2 usage: %3.2f", metric_values[4]);

        if (true == isFreq)
            printf("\tGT Freq: %4.2f", metric_values[3]);

        printf("\n");
    }

    CTTMetrics_Close();

    return 0;
}
