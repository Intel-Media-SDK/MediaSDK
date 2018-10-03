// Copyright (c) 2018 Intel Corporation
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

#ifndef __PJHUFFTBL_H__
#define __PJHUFFTBL_H__

#ifndef __OWNJ_H__
#include "ownj.h"
#endif

/*
// Figure F.12: extend sign bit.
// sometime, a shift and add faster than a table lookup.
*/

#undef AVOID_TABLES
#ifdef AVOID_TABLES

#define mfxownpj_huff_extend(x,s) \
  ((x) < (1 << ((s) - 1)) ? (x) + (((-1) << (s)) + 1) : (x))

#else

/* entry i is 2**(i-1) */
extern const int mfxown_pj_extend_test[16];

/* entry i is (-1 << i) + 1 */
extern const int mfxown_pj_extend_offset[16];


#define mfxownpj_huff_extend(x,s) \
  ((x) < mfxown_pj_extend_test[s] ? (x) + mfxown_pj_extend_offset[s] : (x))

#endif /* AVOID_TABLES */

extern const int mfxown_pj_eobsize[];
extern const int mfxown_pj_csize[];
extern const int mfxown_pj_lowest_coef[];

#endif /* __PJHUFFTBL_H__ */

