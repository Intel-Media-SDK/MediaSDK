/*//////////////////////////////////////////////////////////////////////////////
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
*/
#include "asc_avx2_impl.h"

#if defined(__AVX2__)

#define _mm_loadh_epi64(a, ptr) _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(a), (__m64 *)(ptr)))
#define _mm_movehl_epi64(a, b) _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b)))

// Load 0..7 floats to YMM register from memory
// NOTE: elements of YMM are permuted [ 4 2 - 1 ]
__m256 LoadPartialYmm(
    float  * pSrc,
    mfxI32   len
)
{
    __m128 xlo = _mm_setzero_ps();
    __m128 xhi = _mm_setzero_ps();
    if (len & 4) {
        xhi = _mm_loadu_ps(pSrc);
        pSrc += 4;
    }
    if (len & 2) {
        xlo = _mm_loadh_pi(xlo, (__m64 *)pSrc);
        pSrc += 2;
    }
    if (len & 1) {
        xlo = _mm_move_ss(xlo, _mm_load_ss(pSrc));
    }
    return _mm256_insertf128_ps(_mm256_castps128_ps256(xlo), xhi, 1);
}

// Store 0..7 floats from YMM register to memory
// NOTE: elements of YMM are permuted [ 4 2 - 1 ]
void StorePartialYmm(
    float  * pDst,
    __m256   ymm,
    mfxI32   len
)
{
    __m128 xlo = _mm256_castps256_ps128(ymm);
    __m128 xhi = _mm256_extractf128_ps(ymm, 1);
    if (len & 4) {
        _mm_storeu_ps(pDst, xhi);
        pDst += 4;
    }
    if (len & 2) {
        _mm_storeh_pi((__m64 *)pDst, xlo);
        pDst += 2;
    }
    if (len & 1) {
        _mm_store_ss(pDst, xlo);
    }
}

// Load 0..31 bytes to YMM register from memory
// NOTE: elements of YMM are permuted [ 16 8 4 2 - 1 ]
template <char init>
__m256i LoadPartialYmm(
    unsigned char * pSrc,
    mfxI32          len
)
{
    __m128i xlo = _mm_set1_epi8(init);
    __m128i xhi = _mm_set1_epi8(init);
    if (len & 16) {
        xhi = _mm_loadu_si128((__m128i *)pSrc);
        pSrc += 16;
    }
    if (len & 8) {
        xlo = _mm_loadh_epi64(xlo, (__m64 *)pSrc);
        pSrc += 8;
    }
    if (len & 4) {
        xlo = _mm_insert_epi32(xlo, *((int *)pSrc), 1);
        pSrc += 4;
    }
    if (len & 2) {
        xlo = _mm_insert_epi16(xlo, *((short *)pSrc), 1);
        pSrc += 2;
    }
    if (len & 1) {
        xlo = _mm_insert_epi8(xlo, *pSrc, 0);
    }
    return _mm256_inserti128_si256(_mm256_castsi128_si256(xlo), xhi, 1);
}

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
