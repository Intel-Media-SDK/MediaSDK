// Copyright (c) 2018-2020 Intel Corporation
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
#pragma once
#ifndef _ASC_SSE4_IMPL_H_
#define _ASC_SSE4_IMPL_H_
#include "asc_common_impl.h"

// Load 0..3 floats to XMM register from memory
// NOTE: elements of XMM are permuted [ 2 - 1 ]
__m128 LoadPartialXmm(
    float  * pSrc,
    mfxI32   len
);

// Store 0..3 floats from XMM register to memory
// NOTE: elements of XMM are permuted [ 2 - 1 ]
void StorePartialXmm(
    float  * pDst,
    __m128   xmm,
    mfxI32   len
);

// Load 0..15 bytes to XMM register from memory
// NOTE: elements of XMM are permuted [ 8 4 2 - 1 ]
template <char init>
__m128i LoadPartialXmm(
    unsigned char * pSrc,
    mfxI32          len
);

/*ME_SAD_8x8_Block_SSE4 and ME_VAR_8x8_Block_SSE4 also use SSE3 instructions but because
they include SSE4 instructions too then they have been integrated here*/
mfxU16 ME_SAD_8x8_Block_SSE4(mfxU8 *pSrc, mfxU8 *pRef, mfxU32 srcPitch, mfxU32 refPitch);
void ME_VAR_8x8_Block_SSE4(mfxU8 *pSrc, mfxU8 *pRef, mfxU8 *pMCref, mfxI16 srcAvgVal,
    mfxI16 refAvgVal, mfxU32 srcPitch, mfxU32 refPitch, mfxI32 &var, mfxI32 &jtvar, mfxI32 &jtMCvar);

void ME_SAD_8x8_Block_Search_SSE4(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
    mfxU16 *bestSAD, int *bestX, int *bestY);
void ME_SAD_8x8_Block_FSearch_SSE4(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
    mfxU32 *bestSAD, int *bestX, int *bestY);
void RsCsCalc_4x4_SSE4(pmfxU8 pSrc, int srcPitch, int wblocks, int hblocks, pmfxU16 pRs,
    pmfxU16 pCs);
void RsCsCalc_bound_SSE4(pmfxU16 pRs, pmfxU16 pCs, pmfxU16 pRsCs, pmfxU32 pRsFrame,
    pmfxU32 pCsFrame, int wblocks, int hblocks);
void RsCsCalc_diff_SSE4(pmfxU16 pRs0, pmfxU16 pCs0, pmfxU16 pRs1, pmfxU16 pCs1, int wblocks,
    int hblocks, pmfxU32 pRsDiff, pmfxU32 pCsDiff);
void ImageDiffHistogram_SSE4(pmfxU8 pSrc, pmfxU8 pRef, mfxU32 pitch, mfxU32 width, mfxU32 height, mfxI32 histogram[5], mfxI64 *pSrcDC, mfxI64 *pRefDC);
void GainOffset_SSE4(pmfxU8 *pSrc, pmfxU8 *pDst, mfxU16 width, mfxU16 height, mfxU16 pitch,
    mfxI16 gainDiff);
mfxStatus Calc_RaCa_pic_SSE4(mfxU8 *pSrc, mfxI32 width, mfxI32 height, mfxI32 pitch, mfxF64 &RsCs);
mfxI16 AvgLumaCalc_SSE4(pmfxU32 pAvgLineVal, int len);


#endif //_ASC_SSE4_IMPL_H_