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

#if defined (MFX_ENABLE_VC1_VIDEO_DECODE)

#include "umc_vc1_spl_tbl.h"

AspectRatio AspectRatioTable[16] =
{
    {0, 0},     {1, 1},    {12, 11},    {10, 11},
    {16, 11},   {40, 33},  {24, 11},    {20, 11},
    {32, 11},   {80, 33},  {18, 11},    {15, 11},
    {64, 33},   {169, 99}, {0, 0}
};

double FrameRateNumerator[256] =
{
    0,       24000.0,    25000.0,    30000.0,
    50000.0, 60000.0,    48000.0,    72000.0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

double FrameRateDenomerator[16] =
{
    0, 1000.0, 1001.0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};


uint32_t bMax_LevelLimits[4][5] =
{
     {   /* Simple Profile */
        20,     /* Low    Level */
        77,     /* Medium Level */
        0,      /* Hight Level */
        0,
        0
    },
    {   /* Main Profile */
        306,        /* Low    Level */
        611,        /* Medium Level */
        2442,       /* Hight Level */
        0,
        0
    },
    {   /* Reserved Profile */
        0,0,0,0,0
    },
    {   /* Advanced Profile */
        250,        /*L0 level*/
        1250,       /*L1 level*/
        2500,       /*L2 level*/
        5500,       /*L3 level*/
        16500
    }
};

#endif //UMC_ENABLE_VC1_SPLITTER || MFX_ENABLE_VC1_VIDEO_DECODE
