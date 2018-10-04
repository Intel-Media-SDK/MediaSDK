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
#pragma once
#ifndef _ASC_COMMON_IMPL_H_
#define _ASC_COMMON_IMPL_H_
#include "asc_defs.h"
#include "asc_structures.h"


static const int HIST_THRESH_LO = 1;
static const int HIST_THRESH_HI = 12;
#define SAD_SEARCH_VSTEP 2  // 1=FS 2=FHS

ASC_ALIGN_DECL(16) static const mfxU16 tab_twostep[8] = {
    0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff, 0x0000, 0xffff,
};

ASC_ALIGN_DECL(16) static const mfxU16 tab_killmask[8][8] = {
    { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff },
    { 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff },
    { 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff },
    { 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff },
};

#define _mm_loadh_epi64(a, ptr) _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(a), (__m64 *)(ptr)))
#define _mm_movehl_epi64(a, b) _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b)))

// Load 0..3 floats to XMM register from memory
// NOTE: elements of XMM are permuted [ 2 - 1 ]
static inline __m128 LoadPartialXmm(float *pSrc, mfxI32 len)
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
static inline void StorePartialXmm(float *pDst, __m128 xmm, mfxI32 len)
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

#if defined(__AVX2__)
// Load 0..7 floats to YMM register from memory
// NOTE: elements of YMM are permuted [ 4 2 - 1 ]
static inline __m256 LoadPartialYmm(float *pSrc, mfxI32 len)
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
static inline void StorePartialYmm(float *pDst, __m256 ymm, mfxI32 len)
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
#endif //defined(__AVX2__)

// Load 0..15 bytes to XMM register from memory
// NOTE: elements of XMM are permuted [ 8 4 2 - 1 ]
template <char init>
static inline __m128i LoadPartialXmm(unsigned char *pSrc, mfxI32 len)
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

#if defined(__AVX2__)
// Load 0..31 bytes to YMM register from memory
// NOTE: elements of YMM are permuted [ 16 8 4 2 - 1 ]
template <char init>
static inline __m256i LoadPartialYmm(unsigned char *pSrc, mfxI32 len)
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
#endif //defined(__AVX2__)

static inline void calc_RACA_4x4_C(mfxU8 *pSrc, mfxI32 pitch, mfxI32 *RS, mfxI32 *CS) {
    mfxI32 i, j;
    mfxU8 *pS = pSrc;
    mfxU8 *pS2 = pSrc + pitch;
    mfxI32 Rs, Cs;

    Cs = 0;
    Rs = 0;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            Cs += (pS[j] > pS[j + 1]) ? (pS[j] - pS[j + 1]) : (pS[j + 1] - pS[j]);
            Rs += (pS[j] > pS2[j]) ? (pS[j] - pS2[j]) : (pS2[j] - pS[j]);
        }
        pS += pitch;
        pS2 += pitch;
    }

    *CS += Cs >> 4;
    *RS += Rs >> 4;
}

#endif //_ASC_COMMON_IMPL_H_
