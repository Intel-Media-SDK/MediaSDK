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

#include "umc_vc1_common_tables_adv.h"
#include "umc_vc1_common_defs.h"

//VC-1 Table 107: Refdist table
//    REFDIST     VLC         VLC
//                Size
//    0            2           0
//    1            2           1
//    2            2           2
//    3            3           6
//    4            4           14
//    5            5           30
//    6            6           62
//    7            7           126
//    8            8           254
//    9            9           510
//    10           10          1022
//    11           11          2046
//    12           12          4094
//    13           13          8190
//    14           14          16382
//    15           15          32766
//    16           16          65534

const extern int32_t VC1_FieldRefdistTable[] =
{

    16, /* max bits */
    3,  /* total subtables */
    5, 6, 5,/* subtable sizes */

    0, /* 1-bit codes */
    3, /* 2-bit codes */
        0, 0,      1, 1,      2, 2,
    1, /* 3-bit codes */
        6,3,
    1, /* 4-bit codes */
        14, 4,
    1, /* 5-bit codes */
        30, 5,
    1, /* 6-bit codes */
        62, 6,
    1, /* 7-bit codes */
        126, 7,
    1, /* 8-bit codes */
        254, 8,
    1, /* 9-bit codes */
        510, 9,
    1, /* 10-bit codes */
        1022, 10,
    1, /* 11-bit codes */
        2046, 11,
    1, /* 12-bit codes */
        4094, 12,
    1, /* 13-bit codes */
        8190, 13,
    1, /* 14-bit codes */
        16382, 14,
    1, /* 15-bit codes */
        32766, 15,
    1, /* 16-bit codes */
        65534, 16,

-1 /* end of table */
};
#endif //MFX_ENABLE_VC1_VIDEO_DECODE
