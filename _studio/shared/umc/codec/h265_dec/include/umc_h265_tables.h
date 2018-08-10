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
#ifdef MFX_ENABLE_H265_VIDEO_DECODE

#ifndef __UMC_H265_DEC_TABLES_H__
#define __UMC_H265_DEC_TABLES_H__

#include "umc_h265_dec_defs.h"

namespace UMC_HEVC_DECODER
{
// Scaling list initialization scan lookup table
extern const uint16_t g_sigLastScanCG32x32[64];
// Scaling list initialization scan lookup table
extern const uint16_t ScanTableDiag4x4[16];
// Default scaling list 8x8 for intra prediction
extern const int32_t g_quantIntraDefault8x8[64];
// Default scaling list 8x8 for inter prediction
extern const int32_t g_quantInterDefault8x8[64];
// Default scaling list 4x4
extern const int32_t g_quantTSDefault4x4[16];

// Scaling list table sizes
static const uint32_t g_scalingListSize [4] = {16, 64, 256, 1024};
// Number of possible scaling lists of different sizes
static const uint32_t g_scalingListNum[SCALING_LIST_SIZE_NUM]={6, 6, 6, 2};

// Sample aspect ratios by aspect_ratio_idc index. HEVC spec E.3.1
const uint16_t SAspectRatio[17][2] =
{
    { 0,  0}, { 1,  1}, {12, 11}, {10, 11}, {16, 11}, {40, 33}, { 24, 11},
    {20, 11}, {32, 11}, {80, 33}, {18, 11}, {15, 11}, {64, 33}, {160, 99},
    {4,   3}, {3,   2}, {2,   1}
};

// Inverse QP scale lookup table
extern const uint16_t g_invQuantScales[6];            // IQ(QP%6)


} // namespace UMC_HEVC_DECODER

#endif //__UMC_H265_DEC_TABLES_H__
#endif // MFX_ENABLE_H265_VIDEO_DECODE
