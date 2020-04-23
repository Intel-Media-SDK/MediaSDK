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
#include "asc_sse3_impl.h"


mfxU16 ME_SAD_8x8_Block_SSE3(mfxU8 *pSrc, mfxU8 *pRef, mfxU32 srcPitch, mfxU32 refPitch)
{
    __m128i s0 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[0 * srcPitch]), (__m128i *)&pSrc[1 * srcPitch]);
    __m128i r0 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pRef[0 * refPitch]), (__m128i *)&pRef[1 * refPitch]);
    __m128i s1 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[2 * srcPitch]), (__m128i *)&pSrc[3 * srcPitch]);
    __m128i r1 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pRef[2 * refPitch]), (__m128i *)&pRef[3 * refPitch]);
    __m128i s2 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[4 * srcPitch]), (__m128i *)&pSrc[5 * srcPitch]);
    __m128i r2 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pRef[4 * refPitch]), (__m128i *)&pRef[5 * refPitch]);
    __m128i s3 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[6 * srcPitch]), (__m128i *)&pSrc[7 * srcPitch]);
    __m128i r3 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pRef[6 * refPitch]), (__m128i *)&pRef[7 * refPitch]);

    s0 = _mm_sad_epu8(s0, r0);
    s1 = _mm_sad_epu8(s1, r1);
    s2 = _mm_sad_epu8(s2, r2);
    s3 = _mm_sad_epu8(s3, r3);

    s0 = _mm_add_epi32(s0, s1);
    s2 = _mm_add_epi32(s2, s3);

    s0 = _mm_add_epi32(s0, s2);
    s2 = _mm_movehl_epi64(s2, s0);
    s0 = _mm_add_epi32(s0, s2);

    return (mfxU16)_mm_extract_epi16(s0, 0);
}

void ME_VAR_8x8_Block_SSE3(mfxU8 *pSrc, mfxU8 *pRef, mfxU8 *pMCref, mfxI16 srcAvgVal, mfxI16 refAvgVal, mfxU32 srcPitch, mfxU32 refPitch, mfxI32 &var, mfxI32 &jtvar, mfxI32 &jtMCvar)
{
    __m128i srcAvg = _mm_set1_epi16(srcAvgVal);
    __m128i refAvg = _mm_set1_epi16(refAvgVal);

    __m128i src[8],
        ref[8],
        rmc[8],
        accuVar = { 0 },
        accuJtvar = { 0 },
        accuMcJtvar = { 0 };

    for (mfxU8 i = 0; i < 8; i++)
    {
        src[i] = _mm_cvtepu8_epi16(_mm_loadu_si128((__m128i *)&pSrc[i * srcPitch]));
        ref[i] = _mm_cvtepu8_epi16(_mm_loadu_si128((__m128i *)&pRef[i * refPitch]));
        rmc[i] = _mm_cvtepu8_epi16(_mm_loadu_si128((__m128i *)&pMCref[i * refPitch]));
        src[i] = _mm_sub_epi16(src[i], srcAvg);
        ref[i] = _mm_sub_epi16(ref[i], refAvg);
        rmc[i] = _mm_sub_epi16(rmc[i], refAvg);
        accuVar = _mm_add_epi32(_mm_madd_epi16(src[i], src[i]), accuVar);
        accuJtvar = _mm_add_epi32(_mm_madd_epi16(src[i], ref[i]), accuJtvar);
        accuMcJtvar = _mm_add_epi32(_mm_madd_epi16(src[i], rmc[i]), accuMcJtvar);
    }

    accuVar = _mm_hadd_epi32(accuVar, accuVar);
    accuVar = _mm_hadd_epi32(accuVar, accuVar);

    accuJtvar = _mm_hadd_epi32(accuJtvar, accuJtvar);
    accuJtvar = _mm_hadd_epi32(accuJtvar, accuJtvar);

    accuMcJtvar = _mm_hadd_epi32(accuMcJtvar, accuMcJtvar);
    accuMcJtvar = _mm_hadd_epi32(accuMcJtvar, accuMcJtvar);

    var += _mm_extract_epi32(accuVar, 0);
    jtvar += _mm_extract_epi32(accuJtvar, 0);
    jtMCvar += _mm_extract_epi32(accuMcJtvar, 0);
}
