// Copyright (c) 2017-2018 Intel Corporation
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

#include "umc_vc1_common_tables.h"
#include "umc_vc1_common_defs.h"

//VC-1 Table 58: Escape mode 3 level codeword size conservative code-table
//(used for 1 <= PQUANT <= 7 or if
//VOPDQUANT is present)
//1 <= PQUANT <= 7
//ESCLVLSZ VLC    Level codeword size
//  001                 1
//  010                 2
//  011                 3
//  100                 4
//  101                 5
//  110                 6
//  111                 7
//  00000               8
//  00001               9
//  00010               10
//  00011               11

//it is not used, because esier get FLC 3 or 5 bits length
const extern int32_t VC1_EscapeMode3Conservative_tbl[] =
{
 5, /* max bits */
 2,  /* total subtables */
 3 ,2,/* subtable sizes */

 0, /* 1-bit codes */
 0, /* 2-bit codes */
 7, /* 3-bit codes */
    1,1,    2,2,    3,3,    4,4,
    5,5,    6,6,    7,7,
 0, /* 4-bit codes */
 4, /* 5-bit codes */
    0,8,    1,9,    2,10,    3,11,

  -1
};

//VC-1 Table 59: Escape mode 3 level codeword size efficient code-table
//(used for 8 <= PQUANT <= 31 and if
//VOPDQUANT is absent)
//8 <= PQUANT <= 31
//ESCLVLSZ VLC    Level codeword size
//  1                   2
//  01                  3
//  001                 4
//  0001                5
//  00001               6
//  000001              7
//  000000              8

//it is not used for while,
const extern int32_t VC1_EscapeMode3Efficient_tbl[] =
{
 6, /* max bits */
 2,  /* total subtables */
 5 ,1,/* subtable sizes */

 1, /* 1-bit codes */
     1, 2,
 1, /* 2-bit codes */
     1, 3,
 1, /* 3-bit codes */
     1, 4,
 1, /* 4-bit codes */
     1, 5,
 1, /* 5-bit codes */
     1, 6,
 2, /* 6-bit codes */
     0, 8,    1, 7,
 -1
};


//VC-1 Table 68: IMODE Codetable
//CODING MODE    CODEWORD
//Raw            0000
//Norm-2         10
//Diff-2         001
//Norm-6         11
//Diff-6         0001
//Rowskip        010
//Colskip        011

const extern int32_t VC1_Bitplane_IMODE_tbl[] =
{
 4, /* max bits */
 1, /* total subtables */
 4, /* subtable sizes */

 0, /* 1-bit codes */
 2, /* 2-bit codes */
    2, VC1_BITPLANE_NORM2_MODE,
    3, VC1_BITPLANE_NORM6_MODE,
 3, /* 3-bit codes */
    1, VC1_BITPLANE_DIFF2_MODE,
    2, VC1_BITPLANE_ROWSKIP_MODE,
    3, VC1_BITPLANE_COLSKIP_MODE,
 2, /* 4-bit codes */
    0, VC1_BITPLANE_RAW_MODE,
    1, VC1_BITPLANE_DIFF6_MODE,
-1
};


//VC-1 Table 81: Code table for 3x2 and 2x3 tiles
//VLC / Escape symbol Followed by
//k   Codeword    Codelength  Codeword    Codelength
//0       1           1

//1       2           4
//2       3           4
//4       4           4
//8       5           4
//16      6           4
//32      7           4

//3       0           8
//5       1           8
//6       2           8
//9       3           8
//10      4           8
//12      5           8
//17      6           8
//18      7           8
//20      8           8
//24      9           8
//33      10          8
//34      11          8
//36      12          8
//40      13          8
//48      14          8

//11      2           5         11      5 = (1 << 6)| 11    10
//7       2           5         7       5 = (1 << 6)|  7    10
//13      2           5         13      5 = (1 << 6)| 13    10
//14      2           5         14      5 = (1 << 6)| 14    10
//19      2           5         19      5 = (1 << 6)| 19    10
//21      2           5         21      5 = (1 << 6)| 21    10
//22      2           5         22      5 = (1 << 6)| 22    10
//25      2           5         25      5 = (1 << 6)| 25    10
//26      2           5         26      5 = (1 << 6)| 26    10
//28      2           5         28      5 = (1 << 6)| 28    10
//35      2           5         3       5 = (1 << 6)|  3    10
//37      2           5         5       5 = (1 << 6)|  5    10
//38      2           5         6       5 = (1 << 6)|  6    10
//41      2           5         9       5 = (1 << 6)|  9    10
//42      2           5         10      5 = (1 << 6)| 10    10
//44      2           5         12      5 = (1 << 6)| 12    10
//49      2           5         17      5 = (1 << 6)| 17    10
//50      2           5         18      5 = (1 << 6)| 18    10
//52      2           5         20      5 = (1 << 6)| 20    10
//56      2           5         24      5 = (1 << 6)| 24    10

//63      3           5         1       1 = (3 << 1)| 1     6

//31      3           5         7       4 = (3 << 4)| 7     9
//47      3           5         6       4 = (3 << 4)| 6     9
//55      3           5         5       4 = (3 << 4)| 5     9
//59      3           5         4       4 = (3 << 4)| 4     9
//61      3           5         3       4 = (3 << 4)| 3     9
//62      3           5         2       4 = (3 << 4)| 2     9

//15      3           5         14      8 = (3 << 8)| 14    13
//23      3           5         13      8 = (3 << 8)| 13    13
//27      3           5         12      8 = (3 << 8)| 12    13
//29      3           5         11      8 = (3 << 8)| 11    13
//30      3           5         10      8 = (3 << 8)| 10    13
//39      3           5         9       8 = (3 << 8)|  9    13
//43      3           5         8       8 = (3 << 8)|  8    13
//45      3           5         7       8 = (3 << 8)|  7    13
//46      3           5         6       8 = (3 << 8)|  6    13
//51      3           5         5       8 = (3 << 8)|  5    13
//53      3           5         4       8 = (3 << 8)|  4    13
//54      3           5         3       8 = (3 << 8)|  3    13
//57      3           5         2       8 = (3 << 8)|  2    13
//58      3           5         1       8 = (3 << 8)|  1    13
//60      3           5         0       8 = (3 << 8)|  0    13

const extern int32_t VC1_BitplaneTaledbitsTbl[] =
{
 13, /* max bits */
 2,  /* total subtables */
 6,7,/* subtable sizes */

 1, /* 1-bit codes */
    1,       0 ,
 0, /* 2-bit codes */
 0, /* 3-bit codes */
 6, /* 4-bit codes */
    2, 1,    3, 2,     4, 4,    5, 8,
    6, 16,   7, 32,
 0, /* 5-bit codes */
 1, /* 6-bit codes */
    (3 << 1)| 1,     63,
 0, /* 7-bit codes */
 15, /* 8-bit codes */
    0, 3,    1, 5,    2, 6,    3, 9,
    4, 10,   5, 12,   6, 17,   7, 18,
    8, 20,   9, 24,   10, 33,  11, 34,
    12, 36,  13, 40,  14, 48,
 6, /* 9-bit codes */
    (3 << 4)| 7,    31,
    (3 << 4)| 6,    47,
    (3 << 4)| 5,    55,
    (3 << 4)| 4,    59,

    (3 << 4)| 3,    61,
    (3 << 4)| 2,    62,
 20, /* 10-bit codes */
    (1 << 6)| 11,  11,
    (1 << 6)|  7,  7 ,
    (1 << 6)| 13,  13,
    (1 << 6)| 14,  14,

    (1 << 6)| 19,  19,
    (1 << 6)| 21,  21,
    (1 << 6)| 22,  22,
    (1 << 6)| 25,  25,

    (1 << 6)| 26,  26,
    (1 << 6)| 28,  28,
    (1 << 6)|  3,  35,
    (1 << 6)|  5,  37,

    (1 << 6)|  6,  38,
    (1 << 6)|  9,  41,
    (1 << 6)| 10,  42,
    (1 << 6)| 12,  44,

    (1 << 6)| 17,  49,
    (1 << 6)| 18,  50,
    (1 << 6)| 20,  52,
    (1 << 6)| 24,  56,
 0,  /* 11-bit codes */
 0,  /* 12-bit codes */
 15, /* 13-bit codes */
    (3 << 8)| 14,  15,
    (3 << 8)| 13,  23,
    (3 << 8)| 12,  27,
    (3 << 8)| 11,  29,

    (3 << 8)| 10,  30,
    (3 << 8)|  9,  39,
    (3 << 8)|  8,  43,
    (3 << 8)|  7,  45,

    (3 << 8)|  6,  46,
    (3 << 8)|  5,  51,
    (3 << 8)|  4,  53,
    (3 << 8)|  3,  54,

    (3 << 8)|  2,  57,
    (3 << 8)|  1,  58,
    (3 << 8)|  0,  60,
    -1
};
const extern int32_t VC1_BFraction_tbl[]=
{
 7,        /* max bits */
 2,        /* total subtables */
 3,4,    /* subtable sizes */
 0,        /* 1-bit codes */
 0,        /* 2-bit codes */
 7,        /* 3-bit codes */
    0x00,1,2,    0x01,1,3,    0x02,2,3,    0x03,1,4,
    0x04,3,4,    0x05,1,5,    0x06,2,5,
 0,        /* 4-bit codes */
 0,        /* 5-bit codes */
 0,        /* 6-bit codes */
 16,    /* 7-bit codes */
    0x70, 3,5,    0x71, 4,5,    0x72, 1,6,    0x73, 5,6,
    0x74, 1,7,    0x75, 2,7,    0x76, 3,7,    0x77, 4,7,
    0x78, 5,7,    0x79, 6,7,    0x7A, 1,8,    0x7B, 3,8,
    0x7C, 5,8,    0x7D, 7,8,
    0x7E, VC1_BRACTION_INVALID,VC1_BRACTION_INVALID,
    0x7F, VC1_BRACTION_BI, VC1_BRACTION_BI,

    -1
};
const extern int32_t VC1_BFraction_indexes[8][9] =
{
/* 21 is an invalid index */
/*          0   1   2   3   4   5   6   7   8*/
/* 0 */    {21, 21, 21, 21, 21, 21, 21, 21, 21},
/* 1 */    {21, 21,  0,  1,  3,  5,  9, 11, 17},
/* 2 */    {21, 21, 21,  2, 21,  6, 21, 12, 21},
/* 3 */    {21, 21, 21, 21,  4,  7, 21, 13, 18},
/* 4 */    {21, 21, 21, 21, 21,  8, 21, 14, 21},
/* 5 */    {21, 21, 21, 21, 21, 21, 10, 15, 19},
/* 6 */    {21, 21, 21, 21, 21, 21, 21, 16, 21},
/* 7 */    {21, 21, 21, 21, 21, 21, 21, 21, 20}
};
//////////////////////////////////////////
///////////////////////////////////////////
#endif //MFX_ENABLE_VC1_VIDEO_DECODE
