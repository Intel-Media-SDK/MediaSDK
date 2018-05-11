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
#include "asc_avx2_impl.h"

#if defined(__AVX2__)

void ME_SAD_8x8_Block_Search_AVX2(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
    mfxU16 *bestSAD, int *bestX, int *bestY) {
    __m256i
        s0 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[0 * pitch]), (__m128i *)&pSrc[1 * pitch])),
        s1 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[2 * pitch]), (__m128i *)&pSrc[3 * pitch])),
        s2 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[4 * pitch]), (__m128i *)&pSrc[5 * pitch])),
        s3 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[6 * pitch]), (__m128i *)&pSrc[7 * pitch]));

    for (int y = 0; y < yrange; y += SAD_SEARCH_VSTEP) {
        for (int x = 0; x < xrange; x += 8) {
            pmfxU8 pr = pRef + (y * pitch) + x;
            __m256i
                r0 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[0 * pitch])),
                r1 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[1 * pitch])),
                r2 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[2 * pitch])),
                r3 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[3 * pitch])),
                r4 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[4 * pitch])),
                r5 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[5 * pitch])),
                r6 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[6 * pitch])),
                r7 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[7 * pitch]));
            r0 = _mm256_mpsadbw_epu8(r0, s0, 0x28);
            r1 = _mm256_mpsadbw_epu8(r1, s0, 0x3a);
            r2 = _mm256_mpsadbw_epu8(r2, s1, 0x28);
            r3 = _mm256_mpsadbw_epu8(r3, s1, 0x3a);
            r4 = _mm256_mpsadbw_epu8(r4, s2, 0x28);
            r5 = _mm256_mpsadbw_epu8(r5, s2, 0x3a);
            r6 = _mm256_mpsadbw_epu8(r6, s3, 0x28);
            r7 = _mm256_mpsadbw_epu8(r7, s3, 0x3a);
            r0 = _mm256_add_epi16(r0, r1);
            r2 = _mm256_add_epi16(r2, r3);
            r4 = _mm256_add_epi16(r4, r5);
            r6 = _mm256_add_epi16(r6, r7);
            r0 = _mm256_add_epi16(r0, r2);
            r4 = _mm256_add_epi16(r4, r6);
            r0 = _mm256_add_epi16(r0, r4);
            // horizontal sum
            __m128i
                t = _mm_add_epi16(_mm256_castsi256_si128(r0), _mm256_extractf128_si256(r0, 1));
            // kill every other SAD results, simulating search every two in X dimension
            t = _mm_or_si128(t, _mm_load_si128((__m128i *)tab_twostep));
            // kill out-of-bound values
            if (xrange - x < 8)
                t = _mm_or_si128(t, _mm_load_si128((__m128i *)tab_killmask[xrange - x]));
            t = _mm_minpos_epu16(t);
            mfxU16
                SAD = (mfxU16)_mm_extract_epi16(t, 0);
            if (SAD < *bestSAD) {
                *bestSAD = SAD;
                *bestX = x + _mm_extract_epi16(t, 1);
                *bestY = y;
            }
        }
    }
}

void ME_SAD_8x8_Block_FSearch_AVX2(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
    mfxU32 *bestSAD, int *bestX, int *bestY) {
    __m256i
        s0 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[0 * pitch]), (__m128i *)&pSrc[1 * pitch])),
        s1 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[2 * pitch]), (__m128i *)&pSrc[3 * pitch])),
        s2 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[4 * pitch]), (__m128i *)&pSrc[5 * pitch])),
        s3 = _mm256_broadcastsi128_si256(_mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[6 * pitch]), (__m128i *)&pSrc[7 * pitch]));

    for (int y = 0; y < yrange; y++) {
        for (int x = 0; x < xrange; x += 8) {
            pmfxU8
                pr = pRef + (y * pitch) + x;
            __m256i r0 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[0 * pitch])),
                r1 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[1 * pitch])),
                r2 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[2 * pitch])),
                r3 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[3 * pitch])),
                r4 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[4 * pitch])),
                r5 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[5 * pitch])),
                r6 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[6 * pitch])),
                r7 = _mm256_broadcastsi128_si256(_mm_loadu_si128((__m128i *)&pr[7 * pitch]));
            r0 = _mm256_mpsadbw_epu8(r0, s0, 0x28);
            r1 = _mm256_mpsadbw_epu8(r1, s0, 0x3a);
            r2 = _mm256_mpsadbw_epu8(r2, s1, 0x28);
            r3 = _mm256_mpsadbw_epu8(r3, s1, 0x3a);
            r4 = _mm256_mpsadbw_epu8(r4, s2, 0x28);
            r5 = _mm256_mpsadbw_epu8(r5, s2, 0x3a);
            r6 = _mm256_mpsadbw_epu8(r6, s3, 0x28);
            r7 = _mm256_mpsadbw_epu8(r7, s3, 0x3a);
            r0 = _mm256_add_epi16(r0, r1);
            r2 = _mm256_add_epi16(r2, r3);
            r4 = _mm256_add_epi16(r4, r5);
            r6 = _mm256_add_epi16(r6, r7);
            r0 = _mm256_add_epi16(r0, r2);
            r4 = _mm256_add_epi16(r4, r6);
            r0 = _mm256_add_epi16(r0, r4);
            // horizontal sum
            __m128i
                t = _mm_add_epi16(_mm256_castsi256_si128(r0), _mm256_extractf128_si256(r0, 1));
            // kill out-of-bound values
            if (xrange - x < 8)
                t = _mm_or_si128(t, _mm_load_si128((__m128i *)tab_killmask[xrange - x]));
            t = _mm_minpos_epu16(t);
            mfxU32
                SAD = _mm_extract_epi16(t, 0);
            if (SAD < *bestSAD) {
                *bestSAD = SAD;
                *bestX = x + _mm_extract_epi16(t, 1);
                *bestY = y;
            }
        }
    }
}


mfxI16 AvgLumaCalc_AVX2(pmfxU32 pAvgLineVal, int len) {
    __m256i
        acc = _mm256_setzero_si256();
    __m128i
        shiftVal = _mm_setr_epi32(9, 0, 0, 0);
    mfxI16
        avgVal = 0;

    for (int i = 0; i < len - 7; i += 8) {
        acc = _mm256_add_epi32(_mm256_loadu_si256((__m256i*)&pAvgLineVal[i]), acc);
    }
    acc = _mm256_hadd_epi32(acc, acc);
    acc = _mm256_hadd_epi32(acc, acc);
    __m128i
        tmp = _mm_add_epi32(_mm256_extracti128_si256(acc, 0), _mm256_extracti128_si256(acc, 1));
    tmp = _mm_sra_epi32(tmp, shiftVal);

    avgVal = (mfxI16)_mm_extract_epi32(tmp, 0);
    return avgVal;
}
#endif //defined(__AVX2__)
