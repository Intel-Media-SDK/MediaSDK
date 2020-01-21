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

***********************************************************************************/

#include "cttmetrics.h"
#include "cttmetrics_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define US_IN_SEC 1000000                       // used by gettime() to return time in usec
#define MAX_RING_NUM 64                         // default support for up to 64 rings

static bool s_bInitialized = false;

static unsigned int SAMPLE_PERIOD_US = 500000;  // time period to collect samples in usec
static unsigned int SAMPLES_PER_PERIOD = 100;   // number of samples collected during sampling period
static int AFREQ_RID = -1;

typedef struct ring_info_t
{
    int id;         // ring buffer id, see ring_type_e (from driver)
    int size;       // size of ring buffer (from driver)
    int head;       // head pointer of ring buffer (from driver)
    int tail;       // tail pointer of ring buffer (from driver)
    int seqno;      // request sequence number, increases each time a batch buffer added to the ring (from driver)
    int jiffies;    // shows how many jiffies have expired since the oldest command was pushed to the ring (from driver)

    int ctt_id;     // ring id in the cttMetric enum
    int idle;       // counter for ring idle states increased during sampling period
    float busy;     // ring busy calculated during sampling period
} ring_info;

static ring_info rings[MAX_RING_NUM];
static int map_to_ring_id[MAX_RING_NUM] = {-1}; // maps metric index in GetValue to the ring id in ring_type_e
static int RING_NUM = 0;

const char DRI_DEBUGFS_PATH[] = "/sys/kernel/debug/dri";
const char STAT_FILE_NAME[] = "i915_ringstats";
char STAT_FILE[64] = {0};
char ACT_FREQ_FILE[64] = {0};

/*
    check that the file with stat info exist and accessible
*/
static cttStatus stat_file_check()
{
    unsigned int ringnum = 0;
    ssize_t ret_in;
    size_t len = 0;
    char* buffer = NULL;
    char* stat = NULL;
    const char delim[2] = ":";
    bool broken = false;

    cttStatus status = discover_path_to_gpu();
    if (CTT_ERR_NONE != status)
        return status;

    path_gen(STAT_FILE, sizeof(STAT_FILE), '/', DRI_DEBUGFS_PATH, CARD_N, STAT_FILE_NAME);
    FILE* fd = fopen(STAT_FILE, "r");
    if (NULL == fd)
    {
        memset(STAT_FILE, 0, sizeof(STAT_FILE));

        // Try to open stat file on 'renderD128' node
        path_gen(STAT_FILE, sizeof(STAT_FILE), '/', DRI_DEBUGFS_PATH, "128", STAT_FILE_NAME);
        fd = fopen(STAT_FILE, "r");
        if (NULL == fd)
        {
            return CTT_ERR_DRIVER_NO_INSTRUMENTATION;
        }
    }

    // process the file (may have multiple rings)
    while ((ret_in = getline(&buffer, &len, fd)) != -1)
    {
        // Get first Token
        stat = strtok(buffer, delim);
        if (NULL == stat) {broken = true; break;}

        if (strcmp(stat, "render ring")==0) {
            rings[ringnum].ctt_id = CTT_USAGE_RENDER;
        } else if (strcmp(stat, "blitter ring")==0) {
            rings[ringnum].ctt_id = CTT_USAGE_BLITTER;
        } else if (strcmp(stat, "bsd ring")==0) {
            rings[ringnum].ctt_id = CTT_USAGE_VIDEO;
        } else if (strcmp(stat, "video enhancement ring")==0) {
            rings[ringnum].ctt_id = CTT_USAGE_VIDEO_ENHANCEMENT;
        } else if (strcmp(stat, "bsd2 ring")==0 ||
                   strcmp(stat, "bds2 ring")==0) {
            rings[ringnum].ctt_id = CTT_USAGE_VIDEO2;
        } else {
            // Metric is not exposed to public interface
            continue;
        }

        stat = strtok(NULL, delim);
        if (NULL == stat) {broken = true; break;}
        rings[ringnum].id = atoi(stat);
        ringnum++;
    }

    free(buffer);
    fclose(fd);

    // WA: append frequency metric
    path_gen(ACT_FREQ_FILE, sizeof(ACT_FREQ_FILE), '/', "/sys/class/drm", CARD, "gt_act_freq_mhz");
    fd = fopen(ACT_FREQ_FILE, "r");
    if (NULL != fd) {
        AFREQ_RID = ringnum;
        rings[ringnum].id = ringnum;
        rings[ringnum].ctt_id = CTT_AVG_GT_FREQ;
        ringnum++;
        fclose(fd);
    } else {
        AFREQ_RID = -1;
    }

    RING_NUM = ringnum;

    if (true == broken)
        return CTT_ERR_NO_DATA;
    else
        return CTT_ERR_NONE;
}

/*
    parse file with statistic info from driver
*/
static cttStatus stat_file_parse()
{
    FILE* statfile_fd = NULL;
    ssize_t ret_in;
    size_t len = 0;
    char* buffer = NULL;
    char* stat = NULL;
    const char delim[2] = ":";
    int ringnum = 0;
    bool broken = false;

    statfile_fd = fopen(STAT_FILE, "r");

    if (NULL == statfile_fd)
        return CTT_ERR_DRIVER_NO_INSTRUMENTATION;

    // process the file (may have multiple rings)
    while ((ret_in = getline(&buffer, &len, statfile_fd)) != -1)
    {
        // Get first Token
        stat = strtok(buffer, delim);
        if (NULL == stat) {broken = true; break;}
        // Skipping the name - not much use to us..
        stat = strtok(NULL,  delim);
        if (NULL == stat) {broken = true; break;}
        rings[ringnum].id = atoi(stat);
        stat = strtok(NULL, delim);
        if (NULL == stat) {broken = true; break;}
        rings[ringnum].size = atoi(stat);
        stat = strtok(NULL, delim);
        if (NULL == stat) {broken = true; break;}
        rings[ringnum].head = atoi(stat);
        stat = strtok(NULL, delim);
        if (NULL == stat) {broken = true; break;}
        rings[ringnum].tail = atoi(stat);
        stat = strtok(NULL, delim);
        if (NULL == stat) {broken = true; break;}
        rings[ringnum].seqno= atoi(stat);
        stat = strtok(NULL, delim);
        if (NULL == stat) {broken = true; break;}
        rings[ringnum].jiffies= atoi(stat);

        ringnum++;
    }

    free(buffer);
    fclose(statfile_fd);

    if (true == broken)
        return CTT_ERR_NO_DATA;
    else
        return CTT_ERR_NONE;
}

// reset idle counter
static void ring_reset_idle(unsigned int ring_id)
{
    rings[ring_id].idle = 0;
}

// increase idle counter if conditions met
static void ring_sample(unsigned int ring_id)
{
    if ( (rings[ring_id].tail == rings[ring_id].head) ||
         (rings[ring_id].seqno == -1) )
    {
        rings[ring_id].idle++;
    }
}

static void calculate_busy(unsigned int ring_id, unsigned int samples_per_interval)
{
    if (0 == samples_per_interval)
        return;

    rings[ring_id].busy = 100.0 - (100.0 * rings[ring_id].idle) / samples_per_interval;
}

// get time in microseconds
static unsigned long gettime(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (t.tv_usec + (t.tv_sec * US_IN_SEC));
}

/*
    parsing ring buffer provided by driver and
    counting number of idle samples
*/
static cttStatus sample_busy_values()
{
    cttStatus status = CTT_ERR_NONE;
    unsigned long long t1, ti, tf;
    unsigned long long def_sleep = SAMPLE_PERIOD_US / SAMPLES_PER_PERIOD;
    int i, last_sample = SAMPLES_PER_PERIOD;

    unsigned int samples = 0;

    t1 = gettime();

    for (i=0; i<RING_NUM; i++) ring_reset_idle(i);

    for (samples = 0; samples < SAMPLES_PER_PERIOD; ++samples)
    {
        long long interval;
        ti = gettime();

        status = stat_file_parse();
        if (CTT_ERR_NONE != status)
            return status;

        for (i=0; i<RING_NUM; i++) ring_sample(i);

        tf = gettime();
        if (tf - t1 >= SAMPLE_PERIOD_US)
        {
            last_sample = samples+1;
            break;
        }
        interval = def_sleep - (tf - ti);
        if (interval > 0)
            usleep(interval);
    }

    for (i=0; i<RING_NUM; i++) calculate_busy(i, last_sample);

    return CTT_ERR_NONE;
}

static float get_busy_value(unsigned int ring_id)
{
    return rings[ring_id].busy;
}

extern "C"
cttStatus CTTMetrics_Custom_SetSampleCount(unsigned int in_num)
{
    if (false == s_bInitialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (in_num > 1000 || in_num < 1)
        return CTT_ERR_OUT_OF_RANGE;

    SAMPLES_PER_PERIOD = in_num;

    return CTT_ERR_NONE;
}

extern "C"
cttStatus CTTMetrics_Custom_SetSamplePeriod(unsigned int in_period)
{
    if (false == s_bInitialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (in_period > 1000 || in_period < 10)
        return CTT_ERR_OUT_OF_RANGE;

    SAMPLE_PERIOD_US = in_period * 1000;

    return CTT_ERR_NONE;
}

extern "C"
cttStatus CTTMetrics_Custom_GetMetricCount(unsigned int* out_count)
{
    if (false == s_bInitialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (NULL == out_count)
        return CTT_ERR_NULL_PTR;

    *out_count = (unsigned int)RING_NUM;

    return CTT_ERR_NONE;
}

extern "C"
cttStatus CTTMetrics_Custom_GetMetricInfo(unsigned int count, cttMetric* out_metric_ids)
{
    int i;

    if (false == s_bInitialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (NULL == out_metric_ids)
        return CTT_ERR_NULL_PTR;

    if ((int)count > RING_NUM)
        return CTT_ERR_OUT_OF_RANGE;

    for (i = 0; i < RING_NUM; ++i)
    {
        out_metric_ids[i] = (cttMetric)rings[i].ctt_id;
    }

    return CTT_ERR_NONE;
}

extern "C"
cttStatus CTTMetrics_Custom_Init(const char *device)
{
    cttStatus status = CTT_ERR_NONE;

    if (true == s_bInitialized)
        return CTT_ERR_ALREADY_INITIALIZED;

    // Only PMU path supports device specification
    if (device)
        return CTT_ERR_UNSUPPORTED;

    // check root priveledges
    if (0 != getuid())
        return CTT_ERR_NO_ROOT_PRIVILEDGES;

    memset(rings, 0, (size_t)MAX_RING_NUM * sizeof(struct ring_info_t));

    // check that driver exposes stat file
    status = stat_file_check();
    if (CTT_ERR_NONE != status)
        return status;

    s_bInitialized = true;

    return CTT_ERR_NONE;
}

extern "C"
cttStatus CTTMetrics_Custom_Subscribe(unsigned int count, cttMetric* in_metric_ids)
{
    if (false == s_bInitialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (NULL == in_metric_ids)
        return CTT_ERR_NULL_PTR;

    // one metric - one ring
    if ((int)count > RING_NUM)
        return CTT_ERR_OUT_OF_RANGE;

    // check metrics ids are correct and map metric index in value array to ring id
    int na_metric_cnt = 0;
    for (unsigned int i = 0; i < count; ++i)
    {
        map_to_ring_id[i] = -1;
        for (int j = 0; j < RING_NUM; ++j)
        {
            if (in_metric_ids[i] == rings[j].ctt_id)
            {
                map_to_ring_id[i] = j;
                break;
            }
        }

        if (map_to_ring_id[i] == -1) na_metric_cnt++;
    }

    if(na_metric_cnt)
        return CTT_WRN_METRIC_UNAVAILABLE;

    return CTT_ERR_NONE;
}


extern "C"
void CTTMetrics_Custom_Close()
{
    memset(rings, 0, (size_t)MAX_RING_NUM * sizeof(struct ring_info_t));
    s_bInitialized = false;
}

extern "C"
cttStatus CTTMetrics_Custom_GetValue(unsigned int count, float* out_metric_values)
{
    if (false == s_bInitialized)
        return CTT_ERR_NOT_INITIALIZED;

    if (NULL == out_metric_values)
        return CTT_ERR_NULL_PTR;

    if ((int)count > RING_NUM)
        return CTT_ERR_OUT_OF_RANGE;

    cttStatus status = sample_busy_values();
    if (CTT_ERR_NONE != status)
        return status;

    if (AFREQ_RID > 0) {

        path_gen(ACT_FREQ_FILE, sizeof(ACT_FREQ_FILE), '/', "/sys/class/drm", CARD, "gt_act_freq_mhz");
        FILE* fd = fopen(ACT_FREQ_FILE, "r");
        if(fd) {
            if(fscanf(fd,"%f",&(rings[AFREQ_RID].busy))!=1)
                rings[AFREQ_RID].busy = 0.0;
            fclose(fd);
        } else {
            rings[AFREQ_RID].busy = 0.0;
        }
    }

    float busy = 0.0;
    for (unsigned int i = 0; i < count; ++i)
    {
        if (-1 != map_to_ring_id[i])
        {
            busy = get_busy_value(map_to_ring_id[i]);
        }
        else
        {
            busy = 0.0; // not subscribed/unavailable metrics are always idle
        }
        out_metric_values[i] = busy;
    }

    return CTT_ERR_NONE;
}
