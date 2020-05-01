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

#ifndef _ASC_AVX2_IMPL_H_
#define _ASC_AVX2_IMPL_H_

#include "asc_common_impl.h"
// Load 0..7 floats to YMM register from memory
// NOTE: elements of YMM are permuted [ 4 2 - 1 ]
__m256 LoadPartialYmm(
    float  * pSrc,
    mfxI32   len
);

// Store 0..7 floats from YMM register to memory
// NOTE: elements of YMM are permuted [ 4 2 - 1 ]
void StorePartialYmm(
    float  * pDst,
    __m256   ymm,
    mfxI32   len
);

// Load 0..31 bytes to YMM register from memory
// NOTE: elements of YMM are permuted [ 16 8 4 2 - 1 ]
template <char init>
__m256i LoadPartialYmm(
    unsigned char * pSrc,
    mfxI32          len
);

void ME_SAD_8x8_Block_Search_AVX2(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
    mfxU16 *bestSAD, int *bestX, int *bestY);
void ME_SAD_8x8_Block_FSearch_AVX2(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
    mfxU32 *bestSAD, int *bestX, int *bestY);
void RsCsCalc_4x4_AVX2(pmfxU8 pSrc, int srcPitch, int wblocks, int hblocks, pmfxU16 pRs,
    pmfxU16 pCs);
void RsCsCalc_bound_AVX2(pmfxU16 pRs, pmfxU16 pCs, pmfxU16 pRsCs, pmfxU32 pRsFrame,
    pmfxU32 pCsFrame, int wblocks, int hblocks);
void RsCsCalc_diff_AVX2(pmfxU16 pRs0, pmfxU16 pCs0, pmfxU16 pRs1, pmfxU16 pCs1, int wblocks,
    int hblocks, pmfxU32 pRsDiff, pmfxU32 pCsDiff);
void ImageDiffHistogram_AVX2(pmfxU8 pSrc, pmfxU8 pRef, mfxU32 pitch, mfxU32 width, mfxU32 height,
    mfxI32 histogram[5], mfxI64 *pSrcDC, mfxI64 *pRefDC);
void GainOffset_AVX2(pmfxU8 *pSrc, pmfxU8 *pDst, mfxU16 width, mfxU16 height, mfxU16 pitch,
    mfxI16 gainDiff);
mfxStatus Calc_RaCa_pic_AVX2(mfxU8 *pSrc, mfxI32 width, mfxI32 height, mfxI32 pitch, mfxF64 &RsCs);
mfxI16 AvgLumaCalc_AVX2(pmfxU32 pAvgLineVal, int len);

#endif //_ASC_AVX2_IMPL_H_
