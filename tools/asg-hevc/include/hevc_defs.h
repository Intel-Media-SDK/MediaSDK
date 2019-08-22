// Copyright (c) 2018-2019 Intel Corporation
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

#ifndef __HEVC_DEFS_H__
#define __HEVC_DEFS_H__

#include "mfxvideo.h"

#if MFX_VERSION >= MFX_VERSION_NEXT

//HEVC standard-defined limits on block sizes in terms of luma samples

//CTUs
#define HEVC_MIN_CTU_SIZE 16
#define HEVC_MAX_CTU_SIZE 64

#define HEVC_MIN_LOG2_CTU_SIZE 4
#define HEVC_MAX_LOG2_CTU_SIZE 6

//CUs
//Min size is 8x8, max size is equal to current CTU size
#define HEVC_MIN_CU_SIZE 8
#define HEVC_MIN_LOG2_CU_SIZE 3

//PUs
//Min size is either 4x8 or 8x4, max size is equal to current CU size
#define HEVC_MIN_PU_SIZE_LO 4
#define HEVC_MIN_PU_SIZE_HI 8

//TUs
//Min size is 4x4, max size is 32x32 (cannot be larger than CU min size)
#define HEVC_MIN_TU_SIZE 4
#define HEVC_MAX_TU_SIZE 32

#define HEVC_MIN_LOG2_TU_SIZE 2
#define HEVC_MAX_LOG2_TU_SIZE 5

//Maximum QP value
#define HEVC_MAX_QP 51

// Luma sub-sample interpolation filter
#define LUMA_SUBSAMPLE_INTERPOLATION_FILTER_POSITIONS 4
#define LUMA_TAPS_NUMBER 8

const mfxI32 LUMA_SUBSAMPLE_FILTER_COEFF[LUMA_SUBSAMPLE_INTERPOLATION_FILTER_POSITIONS][LUMA_TAPS_NUMBER] =
{
    {  0,  0,   0,  1,  0,   0,  0,  0 },
    { -1,  4, -10, 58, 17,  -5,  1,  0 },
    { -1,  4, -11, 40, 40, -11,  4, -1 },
    {  0,  1,  -5, 17, 58, -10,  4, -1 },
};

#endif // MFX_VERSION

#endif //__HEVC_DEFS_H__
