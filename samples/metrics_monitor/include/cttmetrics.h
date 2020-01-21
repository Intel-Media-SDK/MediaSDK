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

File Name: cttmetrics.h

\* ****************************************************************************** */

#ifndef __CTT_METRICS__
#define __CTT_METRICS__

#ifdef __cplusplus
extern "C"
{
#endif

/*
    Metrics IDs
*/
typedef enum
{
    CTT_WRONG_METRIC_ID         = -1,
    CTT_USAGE_RENDER            = 0, // EUs
    CTT_USAGE_VIDEO             = 1, // VDBOX
    CTT_USAGE_BLITTER           = 2, // Blitter
    CTT_USAGE_VIDEO_ENHANCEMENT = 3, // VEBOX
    CTT_USAGE_VIDEO2            = 4, // VDBOX2
    CTT_AVG_GT_FREQ             = 5, // Average GT frequency
    CTT_MAX_METRIC_COUNT = CTT_AVG_GT_FREQ+1
} cttMetric;

/*
    Error codes
*/
typedef enum
{
    /* warnings */
    CTT_WRN_METRIC_UNAVAILABLE          = 1,    /* unavailable metrics in subscription list */

    /* no error */
    CTT_ERR_NONE                        = 0,    /* no error */

    /* reserved for unexpected errors */
    CTT_ERR_UNKNOWN                     = -1,   /* unknown error. */

    /* error codes <0 */
    CTT_ERR_NULL_PTR                    = -2,  /* null pointer */
    CTT_ERR_UNSUPPORTED                 = -3,  /* feature not supported */
    CTT_ERR_NOT_INITIALIZED             = -4,  /* the library not  initialized */
    CTT_ERR_NOT_FOUND                   = -5,  /* the specified object is not found */
    CTT_ERR_ALREADY_INITIALIZED	        = -6,  /* the library already initialized */
    CTT_ERR_NO_DATA                     = -7,  /* no or wrong metrics data from driver */
    CTT_ERR_OUT_OF_RANGE                = -8,  /* parameter out of range */
    CTT_ERR_DRIVER_NOT_FOUND            = -9,  /* i915 driver not installed */
    CTT_ERR_DRIVER_NO_INSTRUMENTATION   = -10, /* i915 driver has no instrumentation */
    CTT_ERR_NO_ROOT_PRIVILEDGES         = -11  /* not enough priviledges to get metrics data */

} cttStatus;


/*
    Initializes media metrics library.
*/
cttStatus CTTMetrics_Init(const char *device);

/*
    Returns the number of available metrics.
    Must be called after CTTMetrics_Init().

    out_count - Pointer to the number of metrics available on the system.
*/
cttStatus CTTMetrics_GetMetricCount(unsigned int* out_count);

/*
    Returns ids of available metrics.
    Must be called after CTTMetrics_Init().

    count - Number of elements in *out_metric_ids* array.
    out_metric_ids - Output array of metric ids available on the system. See cttMetric enum.
                     Must be allocated and de-allocated by app.
*/
cttStatus CTTMetrics_GetMetricInfo(unsigned int count, cttMetric* out_metric_ids);

/*
    Specifies metric IDs to collect in *metric* array. Must be called after CTTMetrics_Init().

    count - Number of metric ids in the *in_metric_ids* array.
    in_metric_ids - Input array of metric ids. Array size is *count* elements. Must be allocated and de-allocated by app.
                    Ids must be in increasing order.
*/
cttStatus CTTMetrics_Subscribe(unsigned int count, cttMetric* in_metric_ids);

/*
    Sets the number of metric samples to collect during sampling period. Default = 100. Valid range 1..1000.
    Must be called after CTTMetrics_Init().

    in_num - Number of metric samples to collect during sampling period.
*/
cttStatus CTTMetrics_SetSampleCount(unsigned int in_num);

/*
    Sets the sampling period to collect metric samples. Default = 500. Valid range 10..1000.
    Must be called after CTTMetrics_Init().

    in_period - Sampling period in milliseconds.
*/
cttStatus CTTMetrics_SetSamplePeriod(unsigned int in_period);

/*
    Closes media metrics library and stops metrics collection.
*/
void CTTMetrics_Close();


/*
    Returns metric values.
    Number of values equals to *count* - numbers of metric ids in CTTMetrics_Init().

    count - Number of metrics to collect.
    out_metric_values - Output array of metric values (floats). Must be allocated and de-allocated by app.
                        out_metric_values[i] corresponds to in_metric_ids[i] in CTTMetrics_Subscribe().
*/
cttStatus CTTMetrics_GetValue(unsigned int count, float* out_metric_values);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CTT_METRICS__ */
