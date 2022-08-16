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

#if defined(__SSE4_1__) || defined(_WIN32)

#include "asc_sse4_impl.h"

#define _mm_loadh_epi64(a, ptr) _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(a), (__m64 *)(ptr)))
#define _mm_movehl_epi64(a, b) _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b)))

// Load 0..3 floats to XMM register from memory
// NOTE: elements of XMM are permuted [ 2 - 1 ]
__m128 LoadPartialXmm(
    float  * pSrc,
    mfxI32   len
)
{
    __m128 xmm = _mm_setzero_ps();
    if (len & 2)
    {
        xmm = _mm_loadh_pi(xmm, (__m64 *)pSrc);
        pSrc += 2;
    }
    if (len & 1)
    {
        xmm = _mm_move_ss(xmm, _mm_load_ss(pSrc));
    }
    return xmm;
}

// Store 0..3 floats from XMM register to memory
// NOTE: elements of XMM are permuted [ 2 - 1 ]
void StorePartialXmm(
    float  * pDst,
    __m128   xmm,
    mfxI32   len
)
{
    if (len & 2)
    {
        _mm_storeh_pi((__m64 *)pDst, xmm);
        pDst += 2;
    }
    if (len & 1)
    {
        _mm_store_ss(pDst, xmm);
    }
}

// Load 0..15 bytes to XMM register from memory
// NOTE: elements of XMM are permuted [ 8 4 2 - 1 ]
template <char init>
__m128i LoadPartialXmm(
    unsigned char * pSrc,
    mfxI32          len
)
{
    __m128i xmm = _mm_set1_epi8(init);
    if (len & 8) {
        xmm = _mm_loadh_epi64(xmm, (__m64 *)pSrc);
        pSrc += 8;
    }
    if (len & 4) {
        xmm = _mm_insert_epi32(xmm, *((int *)pSrc), 1);
        pSrc += 4;
    }
    if (len & 2) {
        xmm = _mm_insert_epi16(xmm, *((short *)pSrc), 1);
        pSrc += 2;
    }
    if (len & 1) {
        xmm = _mm_insert_epi8(xmm, *pSrc, 0);
    }
    return xmm;
}

mfxU16 ME_SAD_8x8_Block_SSE4(mfxU8 *pSrc, mfxU8 *pRef, mfxU32 srcPitch, mfxU32 refPitch)
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

void ME_VAR_8x8_Block_SSE4(mfxU8 *pSrc, mfxU8 *pRef, mfxU8 *pMCref, mfxI16 srcAvgVal, mfxI16 refAvgVal, mfxU32 srcPitch, mfxU32 refPitch, mfxI32 &var, mfxI32 &jtvar, mfxI32 &jtMCvar)
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


void ME_SAD_8x8_Block_Search_SSE4(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
    mfxU16 *bestSAD, int *bestX, int *bestY)
{
    __m128i
        s0 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[0 * pitch]), (__m128i *)&pSrc[1 * pitch]),
        s1 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[2 * pitch]), (__m128i *)&pSrc[3 * pitch]),
        s2 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[4 * pitch]), (__m128i *)&pSrc[5 * pitch]),
        s3 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[6 * pitch]), (__m128i *)&pSrc[7 * pitch]);
    for (int y = 0; y < yrange; y += SAD_SEARCH_VSTEP) {
        for (int x = 0; x < xrange; x += 8) {
            pmfxU8
                pr = pRef + (y * pitch) + x;
            __m128i
                r0 = _mm_loadu_si128((__m128i *)&pr[0 * pitch]),
                r1 = _mm_loadu_si128((__m128i *)&pr[1 * pitch]),
                r2 = _mm_loadu_si128((__m128i *)&pr[2 * pitch]),
                r3 = _mm_loadu_si128((__m128i *)&pr[3 * pitch]),
                r4 = _mm_loadu_si128((__m128i *)&pr[4 * pitch]),
                r5 = _mm_loadu_si128((__m128i *)&pr[5 * pitch]),
                r6 = _mm_loadu_si128((__m128i *)&pr[6 * pitch]),
                r7 = _mm_loadu_si128((__m128i *)&pr[7 * pitch]);
            r0 = _mm_add_epi16(_mm_mpsadbw_epu8(r0, s0, 0), _mm_mpsadbw_epu8(r0, s0, 5));
            r1 = _mm_add_epi16(_mm_mpsadbw_epu8(r1, s0, 2), _mm_mpsadbw_epu8(r1, s0, 7));
            r2 = _mm_add_epi16(_mm_mpsadbw_epu8(r2, s1, 0), _mm_mpsadbw_epu8(r2, s1, 5));
            r3 = _mm_add_epi16(_mm_mpsadbw_epu8(r3, s1, 2), _mm_mpsadbw_epu8(r3, s1, 7));
            r4 = _mm_add_epi16(_mm_mpsadbw_epu8(r4, s2, 0), _mm_mpsadbw_epu8(r4, s2, 5));
            r5 = _mm_add_epi16(_mm_mpsadbw_epu8(r5, s2, 2), _mm_mpsadbw_epu8(r5, s2, 7));
            r6 = _mm_add_epi16(_mm_mpsadbw_epu8(r6, s3, 0), _mm_mpsadbw_epu8(r6, s3, 5));
            r7 = _mm_add_epi16(_mm_mpsadbw_epu8(r7, s3, 2), _mm_mpsadbw_epu8(r7, s3, 7));
            r0 = _mm_add_epi16(r0, r1);
            r2 = _mm_add_epi16(r2, r3);
            r4 = _mm_add_epi16(r4, r5);
            r6 = _mm_add_epi16(r6, r7);
            r0 = _mm_add_epi16(r0, r2);
            r4 = _mm_add_epi16(r4, r6);
            r0 = _mm_add_epi16(r0, r4);
            // kill every other SAD results, simulating search every two in X dimension
            r0 = _mm_or_si128(r0, _mm_load_si128((__m128i *)tab_twostep));
            // kill out-of-bound values
            if (xrange - x < 8)
                r0 = _mm_or_si128(r0, _mm_load_si128((__m128i *)tab_killmask[xrange - x]));
            r0 = _mm_minpos_epu16(r0);
            mfxU16
                SAD = (mfxU16)_mm_extract_epi16(r0, 0);
            if (SAD < *bestSAD) {
                *bestSAD = SAD;
                *bestX = x + _mm_extract_epi16(r0, 1);
                *bestY = y;
            }
        }
    }
}

void ME_SAD_8x8_Block_FSearch_SSE4(mfxU8 *pSrc, mfxU8 *pRef, int pitch, int xrange, int yrange,
    mfxU32 *bestSAD, int *bestX, int *bestY) {
    __m128i
        s0 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[0 * pitch]), (__m128i *)&pSrc[1 * pitch]),
        s1 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[2 * pitch]), (__m128i *)&pSrc[3 * pitch]),
        s2 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[4 * pitch]), (__m128i *)&pSrc[5 * pitch]),
        s3 = _mm_loadh_epi64(_mm_loadl_epi64((__m128i *)&pSrc[6 * pitch]), (__m128i *)&pSrc[7 * pitch]);
    for (int y = 0; y < yrange; y++) {
        for (int x = 0; x < xrange; x += 8) {
            pmfxU8
                pr = pRef + (y * pitch) + x;
            __m128i
                r0 = _mm_loadu_si128((__m128i *)&pr[0 * pitch]),
                r1 = _mm_loadu_si128((__m128i *)&pr[1 * pitch]),
                r2 = _mm_loadu_si128((__m128i *)&pr[2 * pitch]),
                r3 = _mm_loadu_si128((__m128i *)&pr[3 * pitch]),
                r4 = _mm_loadu_si128((__m128i *)&pr[4 * pitch]),
                r5 = _mm_loadu_si128((__m128i *)&pr[5 * pitch]),
                r6 = _mm_loadu_si128((__m128i *)&pr[6 * pitch]),
                r7 = _mm_loadu_si128((__m128i *)&pr[7 * pitch]);
            r0 = _mm_add_epi16(_mm_mpsadbw_epu8(r0, s0, 0), _mm_mpsadbw_epu8(r0, s0, 5));
            r1 = _mm_add_epi16(_mm_mpsadbw_epu8(r1, s0, 2), _mm_mpsadbw_epu8(r1, s0, 7));
            r2 = _mm_add_epi16(_mm_mpsadbw_epu8(r2, s1, 0), _mm_mpsadbw_epu8(r2, s1, 5));
            r3 = _mm_add_epi16(_mm_mpsadbw_epu8(r3, s1, 2), _mm_mpsadbw_epu8(r3, s1, 7));
            r4 = _mm_add_epi16(_mm_mpsadbw_epu8(r4, s2, 0), _mm_mpsadbw_epu8(r4, s2, 5));
            r5 = _mm_add_epi16(_mm_mpsadbw_epu8(r5, s2, 2), _mm_mpsadbw_epu8(r5, s2, 7));
            r6 = _mm_add_epi16(_mm_mpsadbw_epu8(r6, s3, 0), _mm_mpsadbw_epu8(r6, s3, 5));
            r7 = _mm_add_epi16(_mm_mpsadbw_epu8(r7, s3, 2), _mm_mpsadbw_epu8(r7, s3, 7));
            r0 = _mm_add_epi16(r0, r1);
            r2 = _mm_add_epi16(r2, r3);
            r4 = _mm_add_epi16(r4, r5);
            r6 = _mm_add_epi16(r6, r7);
            r0 = _mm_add_epi16(r0, r2);
            r4 = _mm_add_epi16(r4, r6);
            r0 = _mm_add_epi16(r0, r4);
            // kill out-of-bound values
            if (xrange - x < 8)
                r0 = _mm_or_si128(r0, _mm_load_si128((__m128i *)tab_killmask[xrange - x]));
            r0 = _mm_minpos_epu16(r0);
            mfxU32
                SAD = _mm_extract_epi16(r0, 0);
            if (SAD < *bestSAD) {
                *bestSAD = SAD;
                *bestX = x + _mm_extract_epi16(r0, 1);
                *bestY = y;
            }
        }
    }
}

void RsCsCalc_4x4_SSE4(pmfxU8 pSrc, int srcPitch, int wblocks, int hblocks, pmfxU16 pRs, pmfxU16 pCs)
{
    pSrc += (4 * srcPitch) + 4;
    for (mfxI16 i = 0; i < hblocks - 2; i++)
    {
        // 4 horizontal blocks at a time
        mfxI16 j;
        for (j = 0; j < wblocks - 5; j += 4)
        {
            __m128i rs0 = _mm_setzero_si128();
            __m128i cs0 = _mm_setzero_si128();
            __m128i rs1 = _mm_setzero_si128();
            __m128i cs1 = _mm_setzero_si128();
            __m128i a0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[-srcPitch + 0]));
            __m128i a1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[-srcPitch + 8]));

            for (mfxI32 k = 0; k < 4; k++)
            {
                __m128i b0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[-1]));
                __m128i b1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[7]));
                __m128i c0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[0]));
                __m128i c1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[8]));
                pSrc += srcPitch;

                // accRs += dRs * dRs
                a0 = _mm_srai_epi16(_mm_abs_epi16(_mm_sub_epi16(c0, a0)), 2);
                a1 = _mm_srai_epi16(_mm_abs_epi16(_mm_sub_epi16(c1, a1)), 2);
                a0 = _mm_madd_epi16(a0, a0);
                a1 = _mm_madd_epi16(a1, a1);
                rs0 = _mm_add_epi32(rs0, a0);
                rs1 = _mm_add_epi32(rs1, a1);

                // accCs += dCs * dCs
                b0 = _mm_srai_epi16(_mm_abs_epi16(_mm_sub_epi16(c0, b0)), 2);
                b1 = _mm_srai_epi16(_mm_abs_epi16(_mm_sub_epi16(c1, b1)), 2);
                b0 = _mm_madd_epi16(b0, b0);
                b1 = _mm_madd_epi16(b1, b1);
                cs0 = _mm_add_epi32(cs0, b0);
                cs1 = _mm_add_epi32(cs1, b1);

                // reuse next iteration
                a0 = c0;
                a1 = c1;
            }
            rs0 = _mm_hadd_epi32(rs0, rs1);
            cs0 = _mm_hadd_epi32(cs0, cs1);

            // store
            rs0 = _mm_packus_epi32(rs0, cs0);
#ifdef ARCH64
            pmfxU64 t = (pmfxU64)&(pRs[i * wblocks + j]);
            *t = _mm_extract_epi64(rs0, 0);
            t = (pmfxU64)&(pCs[i * wblocks + j]);
            *t = _mm_extract_epi64(rs0, 1);
#else
            pmfxU32 t = (pmfxU32)&(pRs[i * wblocks + j]);
            *t = _mm_extract_epi32(rs0, 0);
             t = (pmfxU32)&(pRs[i * wblocks + j + 2]);
            *t = _mm_extract_epi32(rs0, 1);
            t = (pmfxU32)&(pCs[i * wblocks + j]);
            *t = _mm_extract_epi32(rs0, 2);
            t = (pmfxU32)&(pCs[i * wblocks + j + 2]);
            *t = _mm_extract_epi32(rs0, 3);
#endif

            pSrc -= 4 * srcPitch;
            pSrc += 16;
        }
        //2 horizontal blocks
        for (; j < wblocks - 3; j += 2)
        {
            __m128i rs0 = _mm_setzero_si128();
            __m128i cs0 = _mm_setzero_si128();
            __m128i rs1 = _mm_setzero_si128();
            __m128i cs1 = _mm_setzero_si128();
            __m128i a0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[-srcPitch + 0]));
            __m128i a1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[-srcPitch + 8]));

            for (mfxI32 k = 0; k < 4; k++)
            {
                __m128i b0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[-1]));
                __m128i b1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[7]));
                __m128i c0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[0]));
                __m128i c1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pSrc[8]));
                pSrc += srcPitch;

                // accRs += dRs * dRs
                a0 = _mm_srai_epi16(_mm_abs_epi16(_mm_sub_epi16(c0, a0)), 2);
                a1 = _mm_srai_epi16(_mm_abs_epi16(_mm_sub_epi16(c1, a1)), 2);
                a0 = _mm_madd_epi16(a0, a0);
                a1 = _mm_madd_epi16(a1, a1);
                rs0 = _mm_add_epi32(rs0, a0);
                rs1 = _mm_add_epi32(rs1, a1);

                // accCs += dCs * dCs
                b0 = _mm_srai_epi16(_mm_abs_epi16(_mm_sub_epi16(c0, b0)), 2);
                b1 = _mm_srai_epi16(_mm_abs_epi16(_mm_sub_epi16(c1, b1)), 2);
                b0 = _mm_madd_epi16(b0, b0);
                b1 = _mm_madd_epi16(b1, b1);
                cs0 = _mm_add_epi32(cs0, b0);
                cs1 = _mm_add_epi32(cs1, b1);

                // reuse next iteration
                a0 = c0;
                a1 = c1;
            }
            rs0 = _mm_hadd_epi32(rs0, rs1);
            cs0 = _mm_hadd_epi32(cs0, cs1);

            // store
            rs0 = _mm_packus_epi32(rs0, cs0);
            pmfxU32 t = (pmfxU32)&(pRs[i * wblocks + j]);
            *t = _mm_extract_epi32(rs0, 0);
            t = (pmfxU32)&(pCs[i * wblocks + j]);
            *t = _mm_extract_epi32(rs0, 2);

            pSrc -= 4 * srcPitch;
            pSrc += 8;
        }

        // remaining blocks
        for (; j < wblocks - 2; j++)
        {
            mfxU16 accRs = 0;
            mfxU16 accCs = 0;

            for (mfxU16 k = 0; k < 4; k++)
            {
                for (mfxI16 l = 0; l < 4; l++)
                {
                    mfxU16 dRs = (mfxU16)abs(pSrc[l] - pSrc[l - srcPitch]) >> 2;
                    mfxU16 dCs = (mfxU16)abs(pSrc[l] - pSrc[l - 1]) >> 2;
                    accRs += dRs * dRs;
                    accCs += dCs * dCs;
                }
                pSrc += srcPitch;
            }
            pRs[i * wblocks + j] = accRs;
            pCs[i * wblocks + j] = accCs;

            pSrc -= 4 * srcPitch;
            pSrc += 4;
        }
        pSrc -= 4 * (wblocks - 2);
        pSrc += 4 * srcPitch;
    }
}


void ImageDiffHistogram_SSE4(pmfxU8 pSrc, pmfxU8 pRef, mfxU32 pitch, mfxU32 width, mfxU32 height, mfxI32 histogram[5], mfxI64 *pSrcDC, mfxI64 *pRefDC) {
    __m128i sDC = _mm_setzero_si128();
    __m128i rDC = _mm_setzero_si128();

    __m128i h0 = _mm_setzero_si128();
    __m128i h1 = _mm_setzero_si128();
    __m128i h2 = _mm_setzero_si128();
    __m128i h3 = _mm_setzero_si128();

    __m128i zero = _mm_setzero_si128();

    for (mfxU32 i = 0; i < height; i++)
    {
        // process 16 pixels per iteration
        mfxU32 j;
        for (j = 0; j < width - 15; j += 16)
        {
            __m128i s = _mm_loadu_si128((__m128i *)(&pSrc[j]));
            __m128i r = _mm_loadu_si128((__m128i *)(&pRef[j]));

            sDC = _mm_add_epi64(sDC, _mm_sad_epu8(s, zero));    //accumulate horizontal sums
            rDC = _mm_add_epi64(rDC, _mm_sad_epu8(r, zero));

            r = _mm_sub_epi8(r, _mm_set1_epi8(-128));   // convert to signed
            s = _mm_sub_epi8(s, _mm_set1_epi8(-128));

            __m128i dn = _mm_subs_epi8(r, s);   // -d saturated to [-128,127]
            __m128i dp = _mm_subs_epi8(s, r);   // +d saturated to [-128,127]

            __m128i m0 = _mm_cmpgt_epi8(dn, _mm_set1_epi8(HIST_THRESH_HI)); // d < -12
            __m128i m1 = _mm_cmpgt_epi8(dn, _mm_set1_epi8(HIST_THRESH_LO)); // d < -4
            __m128i m2 = _mm_cmpgt_epi8(_mm_set1_epi8(HIST_THRESH_LO), dp); // d < +4
            __m128i m3 = _mm_cmpgt_epi8(_mm_set1_epi8(HIST_THRESH_HI), dp); // d < +12

            m0 = _mm_sub_epi8(zero, m0);    // negate masks from 0xff to 1
            m1 = _mm_sub_epi8(zero, m1);
            m2 = _mm_sub_epi8(zero, m2);
            m3 = _mm_sub_epi8(zero, m3);

            h0 = _mm_add_epi32(h0, _mm_sad_epu8(m0, zero)); // accumulate horizontal sums
            h1 = _mm_add_epi32(h1, _mm_sad_epu8(m1, zero));
            h2 = _mm_add_epi32(h2, _mm_sad_epu8(m2, zero));
            h3 = _mm_add_epi32(h3, _mm_sad_epu8(m3, zero));
        }

        // process remaining 1..15 pixels
        if (j < width)
        {
            __m128i s = LoadPartialXmm<0>(&pSrc[j], width & 0xf);
            __m128i r = LoadPartialXmm<0>(&pRef[j], width & 0xf);

            sDC = _mm_add_epi64(sDC, _mm_sad_epu8(s, zero));    //accumulate horizontal sums
            rDC = _mm_add_epi64(rDC, _mm_sad_epu8(r, zero));

            s = LoadPartialXmm<-1>(&pSrc[j], width & 0xf);      // ensure unused elements not counted

            r = _mm_sub_epi8(r, _mm_set1_epi8(-128));   // convert to signed
            s = _mm_sub_epi8(s, _mm_set1_epi8(-128));

            __m128i dn = _mm_subs_epi8(r, s);   // -d saturated to [-128,127]
            __m128i dp = _mm_subs_epi8(s, r);   // +d saturated to [-128,127]

            __m128i m0 = _mm_cmpgt_epi8(dn, _mm_set1_epi8(HIST_THRESH_HI)); // d < -12
            __m128i m1 = _mm_cmpgt_epi8(dn, _mm_set1_epi8(HIST_THRESH_LO)); // d < -4
            __m128i m2 = _mm_cmpgt_epi8(_mm_set1_epi8(HIST_THRESH_LO), dp); // d < +4
            __m128i m3 = _mm_cmpgt_epi8(_mm_set1_epi8(HIST_THRESH_HI), dp); // d < +12

            m0 = _mm_sub_epi8(zero, m0);    // negate masks from 0xff to 1
            m1 = _mm_sub_epi8(zero, m1);
            m2 = _mm_sub_epi8(zero, m2);
            m3 = _mm_sub_epi8(zero, m3);

            h0 = _mm_add_epi32(h0, _mm_sad_epu8(m0, zero)); // accumulate horizontal sums
            h1 = _mm_add_epi32(h1, _mm_sad_epu8(m1, zero));
            h2 = _mm_add_epi32(h2, _mm_sad_epu8(m2, zero));
            h3 = _mm_add_epi32(h3, _mm_sad_epu8(m3, zero));
        }
        pSrc += pitch;
        pRef += pitch;
    }

    // finish horizontal sums
    sDC = _mm_add_epi64(sDC, _mm_movehl_epi64(sDC, sDC));
    rDC = _mm_add_epi64(rDC, _mm_movehl_epi64(rDC, rDC));

    h0 = _mm_add_epi32(h0, _mm_movehl_epi64(h0, h0));
    h1 = _mm_add_epi32(h1, _mm_movehl_epi64(h1, h1));
    h2 = _mm_add_epi32(h2, _mm_movehl_epi64(h2, h2));
    h3 = _mm_add_epi32(h3, _mm_movehl_epi64(h3, h3));

    _mm_storel_epi64((__m128i *)pSrcDC, sDC);
    _mm_storel_epi64((__m128i *)pRefDC, rDC);

    histogram[0] = _mm_cvtsi128_si32(h0);
    histogram[1] = _mm_cvtsi128_si32(h1);
    histogram[2] = _mm_cvtsi128_si32(h2);
    histogram[3] = _mm_cvtsi128_si32(h3);
    histogram[4] = width * height;

    // undo cumulative counts, by differencing
    histogram[4] -= histogram[3];
    histogram[3] -= histogram[2];
    histogram[2] -= histogram[1];
    histogram[1] -= histogram[0];
}

mfxStatus Calc_RaCa_pic_SSE4(mfxU8 *pSrc, mfxI32 width, mfxI32 height, mfxI32 pitch, mfxF64 &RsCs) {
    mfxU8*
        pY = pSrc + (4 * pitch) + 4;
    mfxI32
        RS = 0,
        CS = 0,
        i;
    for (i = 0; i < height - 8; i += 4)
    {
        // 4 horizontal blocks at a time
        mfxI32 j;
        for (j = 0; j < width - 20; j += 16)
        {
            __m128i rs = _mm_setzero_si128();
            __m128i cs = _mm_setzero_si128();
            __m128i c0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pY[0]));
            __m128i c1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pY[8]));
            for (mfxI32 k = 0; k < 4; k++)
            {
                __m128i b0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pY[1]));
                __m128i b1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pY[9]));
                __m128i a0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pY[pitch + 0]));
                __m128i a1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i *)&pY[pitch + 8]));
                pY += pitch;

                // Cs += (pS[j] > pS[j + 1]) ? (pS[j] - pS[j + 1]) : (pS[j + 1] - pS[j]);
                b0 = _mm_abs_epi16(_mm_sub_epi16(c0, b0));
                b1 = _mm_abs_epi16(_mm_sub_epi16(c1, b1));
                b0 = _mm_hadd_epi16(b0, b1);
                cs = _mm_add_epi16(cs, b0);

                // Rs += (pS[j] > pS2[j]) ? (pS[j] - pS2[j]) : (pS2[j] - pS[j]);
                c0 = _mm_abs_epi16(_mm_sub_epi16(c0, a0));
                c1 = _mm_abs_epi16(_mm_sub_epi16(c1, a1));
                c0 = _mm_hadd_epi16(c0, c1);
                rs = _mm_add_epi16(rs, c0);

                // reuse next iteration
                c0 = a0;
                c1 = a1;
            }

            // horizontal sum
            rs = _mm_hadd_epi16(rs, cs);
            //Cs >> 4; Rs >> 4;
            rs = _mm_srai_epi16(rs, 4);
            //*CS += Cs; *RS += Rs;
            rs = _mm_hadd_epi16(rs, rs);
            rs = _mm_hadd_epi16(rs, rs);
            RS += _mm_extract_epi16(rs, 0);
            CS += _mm_extract_epi16(rs, 1);

            pY -= 4 * pitch;
            pY += 16;
        }

        // remaining blocks
        for (; j < width - 8; j += 4)
        {
            calc_RACA_4x4_C(pY, pitch, &RS, &CS);
            pY += 4;
        }

        pY -= width - 8;
        pY += 4 * pitch;
    }
    mfxI32 w4 = (width - 8) >> 2;
    mfxI32 h4 = (height - 8) >> 2;
    mfxF64 d1 = 1.0 / (mfxF64)(w4*h4);
    mfxF64 drs = (mfxF64)RS * d1;
    mfxF64 dcs = (mfxF64)CS * d1;

    RsCs = sqrt(drs * drs + dcs * dcs);
    return MFX_ERR_NONE;
}

mfxI16 AvgLumaCalc_SSE4(pmfxU32 pAvgLineVal, int len) {
    __m128i
        acc = _mm_setzero_si128(),
        shiftVal = _mm_setr_epi32(9, 0, 0, 0);
    mfxI16
        avgVal = 0;

    for (int i = 0; i < len - 3; i += 4) {
        acc = _mm_add_epi32(_mm_loadu_si128((__m128i*)&pAvgLineVal[i]), acc);
    }
    acc = _mm_hadd_epi32(acc, acc);
    acc = _mm_hadd_epi32(acc, acc);
    acc = _mm_sra_epi32(acc, shiftVal);

    avgVal = (mfxI16)_mm_extract_epi32(acc, 0);
    return avgVal;
}

#endif // __SSE4_1__ || _WIN32
