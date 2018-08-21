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

#include "umc_defs.h"
#if defined(MFX_ENABLE_H264_VIDEO_ENCODE)

#ifndef __UMC_H264_COMMON_H__
#define __UMC_H264_COMMON_H__

#include "umc_defs.h"

#define H264_LIMIT_TABLE_MAX_MBPS    0
#define H264_LIMIT_TABLE_MAX_FS      1
#define H264_LIMIT_TABLE_MAX_DPB     2
#define H264_LIMIT_TABLE_MAX_BR      3
#define H264_LIMIT_TABLE_MAX_CPB     4
#define H264_LIMIT_TABLE_MAX_MVV     5

namespace UMC
{
    typedef enum {
        H264_BASE_PROFILE     = 66,
        H264_MAIN_PROFILE     = 77,
        H264_SCALABLE_BASELINE_PROFILE  = 83,
        H264_SCALABLE_HIGH_PROFILE      = 86,
        H264_EXTENDED_PROFILE = 88,
        H264_HIGH_PROFILE     = 100,
        H264_HIGH10_PROFILE   = 110,
        H264_MULTIVIEWHIGH_PROFILE  = 118,
        H264_STEREOHIGH_PROFILE  = 128,
        H264_HIGH422_PROFILE  = 122,
        H264_HIGH444_PROFILE  = 144
    } H264_PROFILE_IDC;
}

namespace UMC_H264_ENCODER
{
typedef enum {
    H264_LIMIT_TABLE_BASE_PROFILE     = 0,
    H264_LIMIT_TABLE_MAIN_PROFILE     = 0,
    H264_LIMIT_TABLE_EXTENDED_PROFILE = 0,
    H264_LIMIT_TABLE_HIGH_PROFILE     = 1,
    H264_LIMIT_TABLE_STEREOHIGH_PROFILE = 1,
    H264_LIMIT_TABLE_MULTIVIEWHIGH_PROFILE = 1,
    H264_LIMIT_TABLE_HIGH10_PROFILE   = 2,
    H264_LIMIT_TABLE_HIGH422_PROFILE  = 3,
    H264_LIMIT_TABLE_HIGH444_PROFILE  = 3,
    H264_LIMIT_TABLE_INVALID_PROFILE  = 4
} H264_LIMIT_TABLE_PROFILE_IDC;

typedef enum {
    H264_LIMIT_TABLE_LEVEL_1,
    H264_LIMIT_TABLE_LEVEL_1B,
    H264_LIMIT_TABLE_LEVEL_11,
    H264_LIMIT_TABLE_LEVEL_12,
    H264_LIMIT_TABLE_LEVEL_13,
    H264_LIMIT_TABLE_LEVEL_2,
    H264_LIMIT_TABLE_LEVEL_21,
    H264_LIMIT_TABLE_LEVEL_22,
    H264_LIMIT_TABLE_LEVEL_3,
    H264_LIMIT_TABLE_LEVEL_31,
    H264_LIMIT_TABLE_LEVEL_32,
    H264_LIMIT_TABLE_LEVEL_4,
    H264_LIMIT_TABLE_LEVEL_41,
    H264_LIMIT_TABLE_LEVEL_42,
    H264_LIMIT_TABLE_LEVEL_5,
    H264_LIMIT_TABLE_LEVEL_51,
    H264_LIMIT_TABLE_LEVEL_52,
    H264_LIMIT_TABLE_LEVEL_MAX = H264_LIMIT_TABLE_LEVEL_52
} H264_LIMIT_TABLE_LEVEL_IDC;


#define ConvertProfileToTable(profile) (profile == H264_BASE_PROFILE) ? H264_LIMIT_TABLE_BASE_PROFILE : \
    (profile == H264_MAIN_PROFILE) ? H264_LIMIT_TABLE_MAIN_PROFILE : \
    (profile == H264_EXTENDED_PROFILE) ? H264_LIMIT_TABLE_EXTENDED_PROFILE : \
    (profile == H264_HIGH_PROFILE) ? H264_LIMIT_TABLE_HIGH_PROFILE : \
    (profile == H264_MULTIVIEWHIGH_PROFILE) ? H264_LIMIT_TABLE_MULTIVIEWHIGH_PROFILE : \
    (profile == H264_STEREOHIGH_PROFILE) ? H264_LIMIT_TABLE_STEREOHIGH_PROFILE : \
    (profile == H264_SCALABLE_BASELINE_PROFILE) ? H264_LIMIT_TABLE_BASE_PROFILE : \
    (profile == H264_SCALABLE_HIGH_PROFILE) ? H264_LIMIT_TABLE_HIGH_PROFILE : \
    (profile == H264_HIGH10_PROFILE) ? H264_LIMIT_TABLE_HIGH10_PROFILE : \
    (profile == H264_HIGH422_PROFILE) ? H264_LIMIT_TABLE_HIGH422_PROFILE : \
    (profile == H264_HIGH444_PROFILE) ?  H264_LIMIT_TABLE_HIGH444_PROFILE : -1;

#define ConvertProfileFromTable(profile) ((profile == H264_LIMIT_TABLE_BASE_PROFILE) ? H264_BASE_PROFILE : \
    (profile == H264_LIMIT_TABLE_MAIN_PROFILE) ? H264_MAIN_PROFILE : \
    (profile == H264_LIMIT_TABLE_EXTENDED_PROFILE) ? H264_EXTENDED_PROFILE : \
    (profile == H264_LIMIT_TABLE_HIGH_PROFILE) ? H264_HIGH_PROFILE : \
    (profile == H264_LIMIT_TABLE_STEREOHIGH_PROFILE) ? H264_STEREOHIGH_PROFILE : \
    (profile == H264_LIMIT_TABLE_MULTIVIEWHIGH_PROFILE) ? H264_MULTIVIEWHIGH_PROFILE : \
    (profile == H264_LIMIT_TABLE_HIGH10_PROFILE) ? H264_HIGH10_PROFILE : \
    (profile == H264_LIMIT_TABLE_HIGH422_PROFILE) ? H264_HIGH422_PROFILE : H264_HIGH444_PROFILE);

#define ConvertLevelToTable(level) (level == 10) ? H264_LIMIT_TABLE_LEVEL_1 : \
    (level == 11) ? H264_LIMIT_TABLE_LEVEL_11 : \
    (level == 12) ? H264_LIMIT_TABLE_LEVEL_12 : \
    (level == 13) ? H264_LIMIT_TABLE_LEVEL_13 : \
    (level == 20) ? H264_LIMIT_TABLE_LEVEL_2 : \
    (level == 21) ? H264_LIMIT_TABLE_LEVEL_21 : \
    (level == 22) ? H264_LIMIT_TABLE_LEVEL_22 : \
    (level == 30) ? H264_LIMIT_TABLE_LEVEL_3 : \
    (level == 31) ? H264_LIMIT_TABLE_LEVEL_31 : \
    (level == 32) ? H264_LIMIT_TABLE_LEVEL_32 : \
    (level == 40) ? H264_LIMIT_TABLE_LEVEL_4 : \
    (level == 41) ? H264_LIMIT_TABLE_LEVEL_41 : \
    (level == 42) ? H264_LIMIT_TABLE_LEVEL_42 : \
    (level == 50) ? H264_LIMIT_TABLE_LEVEL_5 : \
    (level == 51) ? H264_LIMIT_TABLE_LEVEL_51 : \
    (level == 52) ? H264_LIMIT_TABLE_LEVEL_52 : -1;

#define ConvertLevelFromTable(level) ((level == H264_LIMIT_TABLE_LEVEL_1) ? 10 : \
    ((level == H264_LIMIT_TABLE_LEVEL_11) || (level == H264_LIMIT_TABLE_LEVEL_1B)) ? 11 : \
    (level == H264_LIMIT_TABLE_LEVEL_12) ? 12 : \
    (level == H264_LIMIT_TABLE_LEVEL_13) ? 13 : \
    (level == H264_LIMIT_TABLE_LEVEL_2) ? 20 : \
    (level == H264_LIMIT_TABLE_LEVEL_21) ? 21 : \
    (level == H264_LIMIT_TABLE_LEVEL_22) ? 22 : \
    (level == H264_LIMIT_TABLE_LEVEL_3) ? 30 : \
    (level == H264_LIMIT_TABLE_LEVEL_31) ? 31 : \
    (level == H264_LIMIT_TABLE_LEVEL_32) ? 32 : \
    (level == H264_LIMIT_TABLE_LEVEL_4) ? 40 : \
    (level == H264_LIMIT_TABLE_LEVEL_41) ? 41 : \
    (level == H264_LIMIT_TABLE_LEVEL_42) ? 42 : \
    (level == H264_LIMIT_TABLE_LEVEL_5) ? 50 :  \
    (level == H264_LIMIT_TABLE_LEVEL_51) ? 51 : 52);

//////////////////////////////////////////////////////////
// scan matrices
const int32_t dec_single_scan[2][16] = {
    {0,1,4,8,5,2,3,6,9,12,13,10,7,11,14,15},
    {0,4,1,8,12,5,9,13,2,6,10,14,3,7,11,15}
};

const int32_t dec_single_scan_8x8[2][64] = {
    {0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63},
    {0, 8, 16, 1, 9, 24, 32, 17, 2, 25, 40, 48, 56, 33, 10, 3, 18, 41, 49, 57, 26, 11, 4, 19, 34, 42, 50, 58, 27, 12, 5, 20, 35, 43, 51, 59, 28, 13, 6, 21, 36, 44, 52, 60, 29, 14, 22, 37, 45, 53, 61, 30, 7, 15, 38, 46, 54, 62, 23, 31, 39, 47, 55, 63}
};


} // end namespace UMC_H264_ENCODER

#endif // __UMC_H264_COMMON_H__
#endif // #if defined(MFX_ENABLE_H264_VIDEO_ENCODE)
