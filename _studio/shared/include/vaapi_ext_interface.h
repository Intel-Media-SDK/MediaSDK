// Copyright (c) 2017 Intel Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __VAAPI_EXT_INTERFACE_H__
#define __VAAPI_EXT_INTERFACE_H__

#include "mfx_config.h"



#define VA_CODED_BUF_STATUS_HW_TEAR_DOWN 0x4000




/*======================= FEI ==================================*/

#define VA_ENC_FUNCTION_DEFAULT_INTEL 0x00000000
#define VA_ENC_FUNCTION_ENC_INTEL     0x00000001
#define VA_ENC_FUNCTION_PAK_INTEL     0x00000002
#define VA_ENC_FUNCTION_ENC_PAK_INTEL 0x00000004

#define VAEntrypointEncFEIIntel     1001
#define VAEntrypointStatisticsIntel 1002

#define VAEncMiscParameterTypeFEIFrameControlIntel 1001

#define VAEncFEIMVBufferTypeIntel                   1001
#define VAEncFEIModeBufferTypeIntel                 1002
#define VAEncFEIDistortionBufferTypeIntel           1003
#define VAEncFEIMBControlBufferTypeIntel            1004
#define VAEncFEIMVPredictorBufferTypeIntel          1005
#define VAStatsStatisticsParameterBufferTypeIntel   1006
#define VAStatsStatisticsBufferTypeIntel            1007
#define VAStatsMotionVectorBufferTypeIntel          1008
#define VAStatsMVPredictorBufferTypeIntel           1009

typedef struct _VAMotionVectorIntel {
    short  mv0[2];
    short  mv1[2];
} VAMotionVectorIntel;


typedef VAGenericID VAMFEContextID;

#define VPG_EXT_VA_CREATE_MFECONTEXT  "DdiMedia_CreateMfeContext"
typedef VAStatus (*vaExtCreateMfeContext)(
    VADisplay           dpy,
    VAMFEContextID      *mfe_context
);

#define VPG_EXT_VA_ADD_CONTEXT  "DdiMedia_AddContext"
typedef VAStatus (*vaExtAddContext)(
    VADisplay           dpy,
    VAContextID         context,
    VAMFEContextID      mfe_context
);

#define VPG_EXT_VA_RELEASE_CONTEXT  "DdiMedia_ReleaseContext"
typedef VAStatus (*vaExtReleaseContext)(
    VADisplay           dpy,
    VAContextID         context,
    VAMFEContextID      mfe_context
);

#define VPG_EXT_VA_MFE_SUBMIT  "DdiMedia_MfeSubmit"
typedef VAStatus (*vaExtMfeSubmit)(
    VADisplay           dpy,
    VAMFEContextID      mfe_context,
    VAContextID         *contexts,
    int                 num_contexts
);

#endif // __VAAPI_EXT_INTERFACE_H__
