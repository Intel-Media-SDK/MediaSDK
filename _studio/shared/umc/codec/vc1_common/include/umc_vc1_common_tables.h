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

#ifndef __UMC_VC1_COMMON_TABLES_H__
#define __UMC_VC1_COMMON_TABLES_H__

#include "umc_defs.h"

//VC-1 Table 58: Escape mode 3 level codeword size conservative code-table (used for 1 <= PQUANT <= 7 or if
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
const extern int32_t VC1_EscapeMode3Conservative_tbl[];

//VC-1 Table 59: Escape mode 3 level codeword size efficient code-table (used for 8 <= PQUANT <= 31 and if
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
const extern int32_t VC1_EscapeMode3Efficient_tbl[];


//VC-1 Table 68: IMODE Codetable
//CODING MODE    CODEWORD
//Raw            0000
//Norm-2        10
//Diff-2        001
//Norm-6        11
//Diff-6        0001
//Rowskip        010
//Colskip        011

const extern int32_t VC1_Bitplane_IMODE_tbl[];

//VC-1 Table 81: Code table for 3x2 and 2x3 tiles
const extern int32_t VC1_BitplaneTaledbitsTbl[];

//VC-1 7.1.1.14 Table 40
const extern int32_t VC1_BFraction_tbl[];
const extern int32_t VC1_BFraction_indexes[8][9];

#endif //__umc_vc1_common_tables_H__
#endif //MFX_ENABLE_VC1_VIDEO_DECODE
