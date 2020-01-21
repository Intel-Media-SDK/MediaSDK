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

File Name: cttmetrics_gtest.cpp

***********************************************************************************/

#include "cttmetrics.h"
#include "cttmetrics_utils.h"
#include "gtest/gtest.h"
#include "device_info.h"
#include "igt_load.h"

#ifndef I915_EXEC_BSD_RING1
    #define I915_EXEC_BSD_RING1 (1<<13)
#endif // !I915_EXEC_BSD_RING1

#ifndef I915_EXEC_BSD_RING2
    #define I915_EXEC_BSD_RING2 (2<<13)
#endif // !I915_EXEC_BSD_RING2

char GPU_MAX_FREQ_FILE_PATH [] = "/sys/class/drm/card0/gt_max_freq_mhz";
char GPU_MIN_FREQ_FILE_PATH [] = "/sys/class/drm/card0/gt_min_freq_mhz";
char GPU_BOOST_FREQ_FILE_PATH [] = "/sys/class/drm/card0/gt_boost_freq_mhz";
char GPU_RPn_FILE_PATH [] = {"/sys/class/drm/card0/gt_RPn_freq_mhz"};
char GPU_RP0_FILE_PATH [] = {"/sys/class/drm/card0/gt_RP0_freq_mhz"};
char I915_DRI_DIR[16] = "/dev/dri/card0";

uint16_t dev_id;
unsigned int num_slices;

unsigned translateCttToDRMEngineName(cttMetric metric, int gem_fd)
{
    switch (metric)
    {
        case CTT_USAGE_RENDER:
            return I915_EXEC_RENDER;
        case CTT_USAGE_BLITTER:
            return I915_EXEC_BLT;
        case CTT_USAGE_VIDEO:
            if (gem_has_bsd2(gem_fd))
                return I915_EXEC_BSD | I915_EXEC_BSD_RING1;
            else
                return I915_EXEC_BSD;
        case CTT_USAGE_VIDEO2:
            return I915_EXEC_BSD | I915_EXEC_BSD_RING2;
        case CTT_USAGE_VIDEO_ENHANCEMENT:
            return I915_EXEC_VEBOX;
        default:
            return 0;
    }
}

void getAndCheckAvailableMetrics(unsigned int* count, cttMetric* out_metric_ids)
{
    EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Init(NULL));
    EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_GetMetricCount(count));
    EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_GetMetricInfo(*count, out_metric_ids));

    if (num_slices > 1 && !IS_BROXTON(dev_id))
        EXPECT_EQ(*count, (unsigned int)CTT_MAX_METRIC_COUNT);
    else
        //GT2 systems, Broxton haven't VDBOX2
        EXPECT_EQ(*count, (unsigned int)(CTT_MAX_METRIC_COUNT - 1));

    CTTMetrics_Close();
}

int getGpuFrequency(char* path)
{
    int fd, res = -1;

    fd = open(path, O_RDONLY);
    if (fd >= 0)
    {
        res = read_freq(fd);
        close(fd);
    }

    return res;
}

int setGpuFrequency(int min, int max, int boost)
{
    int fd_min, fd_max, fd_boost, res = -1, res_min = -1, res_max = -1, res_boost = -1;

    if (!(min <= max && min <= boost && boost <= max))
        return res;

    std::string ss_min(std::to_string(min));
    std::string ss_max(std::to_string(max));
    std::string ss_boost(std::to_string(boost));

    fd_min = open(GPU_MIN_FREQ_FILE_PATH, O_WRONLY);
    fd_max = open(GPU_MAX_FREQ_FILE_PATH, O_WRONLY);
    fd_boost = open(GPU_BOOST_FREQ_FILE_PATH, O_WRONLY);

    if (fd_min >= 0 && fd_max >= 0)
    {
        if (min < getGpuFrequency(GPU_MIN_FREQ_FILE_PATH))
        {
            res_min = write(fd_min, ss_min.c_str(), ss_min.size());
            res_max = write(fd_max, ss_max.c_str(), ss_max.size());
            if (fd_boost >= 0) res_boost = write(fd_boost, ss_boost.c_str(), ss_boost.size());
        }
        else
        {
            if (fd_boost >= 0) res_boost = write(fd_boost, ss_boost.c_str(), ss_boost.size());
            res_max = write(fd_max, ss_max.c_str(), ss_max.size());
            res_min = write(fd_min, ss_min.c_str(), ss_min.size());
        }
    }

    if (res_min >= 0 && res_max >= 0)
    {
        res = 0;
        if (fd_boost >= 0) res = res_boost;
    }

    if (fd_min >= 0) close(fd_min);
    if (fd_max >= 0) close(fd_max);
    if (fd_boost >= 0) close(fd_boost);

    return res;
}

// cttMetricsRobustness test set is designed to check API

// Tests singleMetricReport, allMetricReport, fewMetricReport
// check 0% load on gpu engines (tests don't generate the load)

TEST(cttMetricsRobustness, singleMetricReport)
{
    // INITIALIZATION

    const unsigned int test_metric_cnt = 1;
    const float epsilon = 1.0f;
    const float value = 0.0f;

    unsigned int i915_metric_cnt = 0;
    unsigned int num_repeats = 5;

    cttMetric metric_all_ids[CTT_MAX_METRIC_COUNT] = {CTT_WRONG_METRIC_ID};
    float metric_values[test_metric_cnt];
    memset(metric_values, 0, (size_t)test_metric_cnt * sizeof(float));

    // TEST

    getAndCheckAvailableMetrics(&i915_metric_cnt, metric_all_ids);

    for (unsigned int i915_metric_num = 0;i915_metric_num < i915_metric_cnt;i915_metric_num++)
    {
        EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Init(NULL));
        EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Subscribe(test_metric_cnt, &metric_all_ids[i915_metric_num]));

        for (unsigned int repeat = 0;repeat < num_repeats;repeat++)
        {
            EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_GetValue(test_metric_cnt, metric_values));

            if (metric_all_ids[i915_metric_num] == CTT_AVG_GT_FREQ) continue;

            EXPECT_GE(metric_values[0], value) << "metric_values[0] : " << metric_values[0];
            EXPECT_LE(metric_values[0], value + epsilon) << "metric_values[0] : " << metric_values[0];
        }

        CTTMetrics_Close();
    }
}

TEST(cttMetricsRobustness, allMetricReport)
{
    // INITIALIZATION

    const float epsilon = 1.0f;
    const float value = 0.0f;

    unsigned int i915_metric_cnt = 0;
    unsigned int num_repeats = 5;

    cttMetric metric_all_ids[CTT_MAX_METRIC_COUNT] = {CTT_WRONG_METRIC_ID};

    // TEST

    getAndCheckAvailableMetrics(&i915_metric_cnt, metric_all_ids);

    float metric_values[i915_metric_cnt];
    memset(metric_values, 0, (size_t)i915_metric_cnt * sizeof(float));

    EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Init(NULL));
    EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Subscribe(i915_metric_cnt, metric_all_ids));

    for (unsigned int repeat = 0;repeat < num_repeats;repeat++)
    {
        EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_GetValue(i915_metric_cnt, metric_values));

        for (unsigned int i915_metric_num = 0;i915_metric_num < i915_metric_cnt;i915_metric_num++)
        {
            if (metric_all_ids[i915_metric_num] == CTT_AVG_GT_FREQ) continue;

            EXPECT_GE(metric_values[i915_metric_num], value) << "metric_values[i915_metric_num] : " << metric_values[i915_metric_num];
            EXPECT_LE(metric_values[i915_metric_num], value + epsilon) << "metric_values[i915_metric_num] : " << metric_values[i915_metric_num];
        }
    }

    CTTMetrics_Close();
}

TEST(cttMetricsRobustness, fewMetricReport)
{
    // INITIALIZATION

    const unsigned int test_metric_cnt = 2;
    const float epsilon = 1.0f;
    const float value = 0.0f;

    unsigned int i915_metric_cnt = 0;
    unsigned int num_repeats = 5;

    cttMetric metric_all_ids[CTT_MAX_METRIC_COUNT] = {CTT_WRONG_METRIC_ID};
    cttMetric test_metric_values[test_metric_cnt];

    float metric_values[test_metric_cnt];
    memset(metric_values, 0, (size_t)test_metric_cnt * sizeof(float));

    // TEST

    getAndCheckAvailableMetrics(&i915_metric_cnt, metric_all_ids);

    for (unsigned int i = 0;i < i915_metric_cnt;i++)
    {
        for (unsigned int j = 0;j < i915_metric_cnt;j++)
        {
            if (i != j)
            {
                test_metric_values[0] = metric_all_ids[i];
                test_metric_values[1] = metric_all_ids[j];

                EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Init(NULL));
                EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Subscribe(test_metric_cnt, test_metric_values));

                for (unsigned int repeat = 0;repeat < num_repeats;repeat++)
                {
                    EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_GetValue(test_metric_cnt, metric_values));

                    for (unsigned int k = 0;k < 2;k++)
                    {
                        if (test_metric_values[k] == CTT_AVG_GT_FREQ) continue;

                        EXPECT_GE(metric_values[k], value) << "metric_values[k] : " << metric_values[k];
                        EXPECT_LE(metric_values[k], value + epsilon) << "metric_values[k] : " << metric_values[k];
                    }
                }

                CTTMetrics_Close();
            }
        }
    }
}

TEST(cttMetricsRobustness, setSampleCount)
{
    // INITIALIZATION

    cttStatus sts = CTT_ERR_NONE;

    const unsigned int step = 100;
    const unsigned int limit = 2000;
    const unsigned int num_samples_min = 1;
    const unsigned int num_samples_max = 1000;

    unsigned int num_samples = 0;

    // TEST

    while (num_samples < limit)
    {
        EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Init(NULL));

        sts = CTTMetrics_SetSampleCount(num_samples);

        if (num_samples < num_samples_min || num_samples > num_samples_max)
            EXPECT_EQ(CTT_ERR_OUT_OF_RANGE, sts) << "num_samples : " << num_samples;
        else
            EXPECT_EQ(CTT_ERR_NONE, sts);

        CTTMetrics_Close();

        num_samples += step;
    }
}

TEST(cttMetricsRobustness, setSamplePeriod)
{
    // INITIALIZATION

    cttStatus sts = CTT_ERR_NONE;

    const unsigned int step = 100;
    const unsigned int limit = 2000;
    const unsigned int period_ms_min = 10;
    const unsigned int period_ms_max = 1000;

    unsigned int period_ms = 0;

    // TEST

    while (period_ms < limit)
    {
        EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Init(NULL));

        sts = CTTMetrics_SetSamplePeriod(period_ms);

        if (period_ms < period_ms_min || period_ms > period_ms_max)
            EXPECT_EQ(CTT_ERR_OUT_OF_RANGE, sts) << "period_ms : " << period_ms;
        else
            EXPECT_EQ(CTT_ERR_NONE, sts);

        CTTMetrics_Close();

        period_ms += step;
    }
}

// cttMetricsFrequencyReport test set is designed to check frequency reporting correctness

TEST(cttMetricsFrequencyReport, setAndCheckFrequency)
{
    // INITIALIZATION

    const unsigned int test_metric_cnt = 1;
    const float epsilon = 20.0f;

    cttMetric metrics_ids [] = {CTT_AVG_GT_FREQ};
    unsigned int num_repeats = 2;
    unsigned int rp_n_freq = 0, rp_0_freq = 0;

    float metric_values[test_metric_cnt];
    memset(metric_values, 0, (size_t)test_metric_cnt * sizeof(float));

    // TEST

    rp_n_freq = getGpuFrequency(GPU_RPn_FILE_PATH);
    rp_0_freq = getGpuFrequency(GPU_RP0_FILE_PATH);

    ASSERT_GT(rp_n_freq, 0u) << "rp_n_freq : " << rp_n_freq;
    ASSERT_GT(rp_0_freq, rp_n_freq) << "rp_0_freq : " << rp_0_freq << " ; rp_n_freq : " << rp_n_freq;

    for (unsigned int rp_freq = rp_n_freq;rp_freq <= rp_0_freq;rp_freq += 50)
    {
        EXPECT_NE(setGpuFrequency(rp_freq, rp_freq, rp_freq), -1) << "rp_freq : " << rp_freq;

        int gem_fd = open(I915_DRI_DIR, O_RDWR);
        if (gem_fd >= 0)
        {
            EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Init(NULL));
            EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Subscribe(test_metric_cnt, metrics_ids));

            for (unsigned int repeat = 0;repeat < num_repeats;repeat++)
            {
                igt_spin_t* spin = igt_spin_batch_new(gem_fd, 0, I915_EXEC_RENDER, 0);
                if (!spin)
                {
                    EXPECT_NE(spin, nullptr);
                    break;
                }

                EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_GetValue(test_metric_cnt, metric_values)) << "rp_freq : " << rp_freq;

                igt_spin_batch_free(gem_fd, spin);

                if (rp_freq >= (rp_0_freq - 50*num_slices) && rp_freq <= rp_0_freq && metric_values[0] < (rp_freq - epsilon) && IS_SKYLAKE(dev_id))
                {
                    int rp_freq_skl = rp_0_freq - 50*num_slices;
                    EXPECT_GE(metric_values[0], rp_freq_skl - epsilon) << "rp_freq_skl : " << rp_freq_skl;
                }
                else
                {
                    EXPECT_GE(metric_values[0], rp_freq - epsilon) << "rp_freq : " << rp_freq;
                }

                EXPECT_LE(metric_values[0], rp_freq + epsilon) << "rp_freq : " << rp_freq;
            }

            CTTMetrics_Close();
            close(gem_fd);
        }
    }
}

// cttMetricsEngineLoadReport test set is designed to check load value on gpu engine
// check 100% load on gpu engines (tests generate the load)

TEST(cttMetricsEngineLoadReport, setAndCheckFullLoadOnSingleEngine)
{
    // INITIALIZATION

    const unsigned int test_metric_cnt = 1;
    const float epsilon = 2.0f;
    const float value = 100.0f;

    unsigned int i915_metric_cnt = 0;
    unsigned int num_repeats = 5;

    cttMetric metric_all_ids[CTT_MAX_METRIC_COUNT] = {CTT_WRONG_METRIC_ID};
    float metric_values[test_metric_cnt];
    memset(metric_values, 0, (size_t)test_metric_cnt * sizeof(float));

    // TEST

    getAndCheckAvailableMetrics(&i915_metric_cnt, metric_all_ids);

    for (unsigned int i915_metric_num = 0;i915_metric_num < i915_metric_cnt;i915_metric_num++)
    {
        if (metric_all_ids[i915_metric_num] == CTT_AVG_GT_FREQ) continue;

        EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Init(NULL));
        EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Subscribe(test_metric_cnt, &metric_all_ids[i915_metric_num]));

        int gem_fd = open(I915_DRI_DIR, O_RDWR);
        if (gem_fd >= 0)
        {
            for (unsigned int repeat = 0;repeat < num_repeats;repeat++)
            {
                igt_spin_t* spin = igt_spin_batch_new(gem_fd, 0, translateCttToDRMEngineName(metric_all_ids[i915_metric_num], gem_fd), 0);
                if (!spin)
                {
                    EXPECT_NE(spin, nullptr);
                    break;
                }

                EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_GetValue(test_metric_cnt, metric_values));

                igt_spin_batch_free(gem_fd, spin);

                EXPECT_GE(metric_values[0], value - epsilon) << "metric_values[0] : " << metric_values[0];
                EXPECT_LE(metric_values[0], value + epsilon) << "metric_values[0] : " << metric_values[0];
            }
            close(gem_fd);
        }

        CTTMetrics_Close();
    }
}

TEST(cttMetricsEngineLoadReport, setAndCheckFullLoadOnFewEngines)
{
    // INITIALIZATION

    const unsigned int test_metric_cnt = 2;
    const float epsilon = 2.0f;
    const float value = 100.0f;

    unsigned int i915_metric_cnt = 0;
    unsigned int num_repeats = 5;

    cttMetric metric_all_ids[CTT_MAX_METRIC_COUNT] = {CTT_WRONG_METRIC_ID};
    cttMetric test_metric_values[test_metric_cnt];

    float metric_values[test_metric_cnt];
    memset(metric_values, 0, (size_t)test_metric_cnt * sizeof(float));

    // TEST

    getAndCheckAvailableMetrics(&i915_metric_cnt, metric_all_ids);

    for (unsigned int i = 0;i < i915_metric_cnt;i++)
    {
        if (metric_all_ids[i] == CTT_AVG_GT_FREQ) continue;

        for (unsigned int j = 0;j < i915_metric_cnt;j++)
        {
            if (i == j || metric_all_ids[j] == CTT_AVG_GT_FREQ) continue;

            test_metric_values[0] = metric_all_ids[i];
            test_metric_values[1] = metric_all_ids[j];

            EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Init(NULL));
            EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_Subscribe(test_metric_cnt, test_metric_values));

            int gem_fd = open(I915_DRI_DIR, O_RDWR);
            if (gem_fd >= 0)
            {
                for (unsigned int repeat = 0;repeat < num_repeats;repeat++)
                {
                    igt_spin_t* spin_1 = igt_spin_batch_new(gem_fd, 0, translateCttToDRMEngineName(metric_all_ids[i], gem_fd), 0);
                    igt_spin_t* spin_2 = igt_spin_batch_new(gem_fd, 0, translateCttToDRMEngineName(metric_all_ids[j], gem_fd), 0);
                    if (!spin_1 || !spin_2)
                    {
                        EXPECT_NE(spin_1, nullptr);
                        EXPECT_NE(spin_2, nullptr);
                        break;
                    }

                    EXPECT_EQ(CTT_ERR_NONE, CTTMetrics_GetValue(test_metric_cnt, metric_values));

                    igt_spin_batch_free(gem_fd, spin_1);
                    igt_spin_batch_free(gem_fd, spin_2);

                    EXPECT_GE(metric_values[0], value - epsilon) << "metric_values[0] : " << metric_values[0];
                    EXPECT_LE(metric_values[0], value + epsilon) << "metric_values[0] : " << metric_values[0];

                    EXPECT_GE(metric_values[1], value - epsilon) << "metric_values[1] : " << metric_values[1];
                    EXPECT_LE(metric_values[1], value + epsilon) << "metric_values[1] : " << metric_values[1];
                }
                close(gem_fd);
            }

            CTTMetrics_Close();
        }
    }
}

int main(int argc, char **argv)
{
    int res = 0, min_freq = 0, max_freq = 0, boost_freq = 0;

    // get current frequency
    min_freq = getGpuFrequency(GPU_MIN_FREQ_FILE_PATH);
    max_freq = getGpuFrequency(GPU_MAX_FREQ_FILE_PATH);
    boost_freq = getGpuFrequency(GPU_BOOST_FREQ_FILE_PATH);

    // get device info
    dev_id = intel_get_drm_devid();
    num_slices = intel_gt(dev_id);

    ::testing::InitGoogleTest(&argc, argv);
    res = RUN_ALL_TESTS();

    // revert frequency to original values
    if (boost_freq == -1) boost_freq = max_freq;
    setGpuFrequency(min_freq, max_freq, boost_freq);

    return res;
}