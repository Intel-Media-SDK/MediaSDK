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

/*
//
//  Purpose:
//    DCT + quantization+level shift + range convert functions (Forward transform)
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif


#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)

#include <emmintrin.h>

extern void mfxdct_8x8_fwd_32f(Ipp32f* x, Ipp32f* y);

static const __ALIGN16 Ipp32f fLS[4] = { 2048., 2048., 2048., 2048. };

extern void mfxdct_qnt_fwd_8x8_ls(
  const Ipp16u* pSrc,
        int     srcStep,
        Ipp16s* pDst,
  const Ipp32f* pQntFwdTbl)
{
  __ALIGN16 __m128  _fS0, _fS1, _fS2;
  __ALIGN16 __m128i _iS0, _iS1, _iS2;
  __ALIGN16 Ipp32f wb[64];

  _fS2 = _mm_load_ps(fLS);

  _iS0 = _mm_loadu_si128((__m128i*) ((Ipp8u*)pSrc + 0*srcStep));
  _iS1 = _mm_srai_epi16(_iS0, 15);
  _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
  _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
  _fS1 = _mm_cvtepi32_ps(_iS2);
  _fS0 = _mm_cvtepi32_ps(_iS0);

  _fS1 = _mm_sub_ps(_fS1, _fS2);
  _fS0 = _mm_sub_ps(_fS0, _fS2);
  _mm_store_ps(wb +  0, _fS1);
  _mm_store_ps(wb +  4, _fS0);

  _iS0 = _mm_loadu_si128((__m128i*) ((Ipp8u*)pSrc + 1*srcStep));
  _iS1 = _mm_srai_epi16(_iS0, 15);
  _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
  _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
  _fS1 = _mm_cvtepi32_ps(_iS2);
  _fS0 = _mm_cvtepi32_ps(_iS0);

  _fS1 = _mm_sub_ps(_fS1, _fS2);
  _fS0 = _mm_sub_ps(_fS0, _fS2);
  _mm_store_ps(wb +  8, _fS1);
  _mm_store_ps(wb + 12, _fS0);

  _iS0 = _mm_loadu_si128((__m128i*) ((Ipp8u*)pSrc + 2*srcStep));
  _iS1 = _mm_srai_epi16(_iS0, 15);
  _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
  _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
  _fS1 = _mm_cvtepi32_ps(_iS2);
  _fS0 = _mm_cvtepi32_ps(_iS0);

  _fS1 = _mm_sub_ps(_fS1, _fS2);
  _fS0 = _mm_sub_ps(_fS0, _fS2);
  _mm_store_ps(wb + 16, _fS1);
  _mm_store_ps(wb + 20, _fS0);

  _iS0 = _mm_loadu_si128((__m128i*) ((Ipp8u*)pSrc + 3*srcStep));
  _iS1 = _mm_srai_epi16(_iS0, 15);
  _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
  _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
  _fS1 = _mm_cvtepi32_ps(_iS2);
  _fS0 = _mm_cvtepi32_ps(_iS0);

  _fS1 = _mm_sub_ps(_fS1, _fS2);
  _fS0 = _mm_sub_ps(_fS0, _fS2);
  _mm_store_ps(wb + 24, _fS1);
  _mm_store_ps(wb + 28, _fS0);

  _iS0 = _mm_loadu_si128((__m128i*) ((Ipp8u*)pSrc + 4*srcStep));
  _iS1 = _mm_srai_epi16(_iS0, 15);
  _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
  _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
  _fS1 = _mm_cvtepi32_ps(_iS2);
  _fS0 = _mm_cvtepi32_ps(_iS0);

  _fS1 = _mm_sub_ps(_fS1, _fS2);
  _fS0 = _mm_sub_ps(_fS0, _fS2);
  _mm_store_ps(wb + 32, _fS1);
  _mm_store_ps(wb + 36, _fS0);

  _iS0 = _mm_loadu_si128((__m128i*) ((Ipp8u*)pSrc + 5*srcStep));
  _iS1 = _mm_srai_epi16(_iS0, 15);
  _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
  _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
  _fS1 = _mm_cvtepi32_ps(_iS2);
  _fS0 = _mm_cvtepi32_ps(_iS0);

  _fS1 = _mm_sub_ps(_fS1, _fS2);
  _fS0 = _mm_sub_ps(_fS0, _fS2);
  _mm_store_ps(wb + 40, _fS1);
  _mm_store_ps(wb + 44, _fS0);

  _iS0 = _mm_loadu_si128((__m128i*) ((Ipp8u*)pSrc + 6*srcStep));
  _iS1 = _mm_srai_epi16(_iS0, 15);
  _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
  _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
  _fS1 = _mm_cvtepi32_ps(_iS2);
  _fS0 = _mm_cvtepi32_ps(_iS0);

  _fS1 = _mm_sub_ps(_fS1, _fS2);
  _fS0 = _mm_sub_ps(_fS0, _fS2);
  _mm_store_ps(wb + 48, _fS1);
  _mm_store_ps(wb + 52, _fS0);

  _iS0 = _mm_loadu_si128((__m128i*) ((Ipp8u*)pSrc + 7*srcStep));
  _iS1 = _mm_srai_epi16(_iS0, 15);
  _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
  _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
  _fS1 = _mm_cvtepi32_ps(_iS2);
  _fS0 = _mm_cvtepi32_ps(_iS0);

  _fS1 = _mm_sub_ps(_fS1, _fS2);
  _fS0 = _mm_sub_ps(_fS0, _fS2);
  _mm_store_ps(wb + 56, _fS1);
  _mm_store_ps(wb + 60, _fS0);

  mfxdct_8x8_fwd_32f(wb, wb);

  if(!((intptr_t)pQntFwdTbl & 15) && !((intptr_t)pDst & 15))
  {
    _fS0 = _mm_load_ps(wb +  0);
    _fS2 = _mm_load_ps(pQntFwdTbl +  0);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb +  4);
    _fS2 = _mm_load_ps(pQntFwdTbl +  4);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_store_si128((__m128i*)(pDst +  0), _iS0);

    _fS0 = _mm_load_ps(wb +  8);
    _fS2 = _mm_load_ps(pQntFwdTbl +  8);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 12);
    _fS2 = _mm_load_ps(pQntFwdTbl + 12);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_store_si128((__m128i*) (pDst + 8), _iS0);

    _fS0 = _mm_load_ps(wb + 16);
    _fS2 = _mm_load_ps(pQntFwdTbl + 16);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 20);
    _fS2 = _mm_load_ps(pQntFwdTbl + 20);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_store_si128((__m128i*) (pDst + 16), _iS0);

    _fS0 = _mm_load_ps(wb + 24);
    _fS2 = _mm_load_ps(pQntFwdTbl + 24);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 28);
    _fS2 = _mm_load_ps(pQntFwdTbl + 28);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_store_si128((__m128i*) (pDst + 24), _iS0);

    _fS0 = _mm_load_ps(wb + 32);
    _fS2 = _mm_load_ps(pQntFwdTbl + 32);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 36);
    _fS2 = _mm_load_ps(pQntFwdTbl + 36);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_store_si128((__m128i*) (pDst + 32), _iS0);

    _fS0 = _mm_load_ps(wb + 40);
    _fS2 = _mm_load_ps(pQntFwdTbl + 40);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 44);
    _fS2 = _mm_load_ps(pQntFwdTbl + 44);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_store_si128((__m128i*) (pDst + 40), _iS0);

    _fS0 = _mm_load_ps(wb + 48);
    _fS2 = _mm_load_ps(pQntFwdTbl + 48);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 52);
    _fS2 = _mm_load_ps(pQntFwdTbl + 52);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_store_si128((__m128i*) (pDst + 48), _iS0);

    _fS0 = _mm_load_ps(wb + 56);
    _fS2 = _mm_load_ps(pQntFwdTbl + 56);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 60);
    _fS2 = _mm_load_ps(pQntFwdTbl + 60);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_store_si128((__m128i*) (pDst + 56), _iS0);
  }
  else
  {
    _fS0 = _mm_load_ps(wb +  0);
    _fS2 = _mm_loadu_ps(pQntFwdTbl +  0);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb +  4);
    _fS2 = _mm_loadu_ps(pQntFwdTbl +  4);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_storeu_si128((__m128i*)(pDst +  0), _iS0);

    _fS0 = _mm_load_ps(wb +  8);
    _fS2 = _mm_loadu_ps(pQntFwdTbl +  8);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 12);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 12);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_storeu_si128((__m128i*) (pDst + 8), _iS0);

    _fS0 = _mm_load_ps(wb + 16);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 16);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 20);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 20);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_storeu_si128((__m128i*) (pDst + 16), _iS0);

    _fS0 = _mm_load_ps(wb + 24);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 24);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 28);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 28);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_storeu_si128((__m128i*) (pDst + 24), _iS0);

    _fS0 = _mm_load_ps(wb + 32);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 32);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 36);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 36);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_storeu_si128((__m128i*) (pDst + 32), _iS0);

    _fS0 = _mm_load_ps(wb + 40);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 40);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 44);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 44);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_storeu_si128((__m128i*) (pDst + 40), _iS0);

    _fS0 = _mm_load_ps(wb + 48);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 48);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 52);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 52);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_storeu_si128((__m128i*) (pDst + 48), _iS0);

    _fS0 = _mm_load_ps(wb + 56);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 56);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _fS1 = _mm_load_ps(wb + 60);
    _fS2 = _mm_loadu_ps(pQntFwdTbl + 60);
    _fS1 = _mm_mul_ps(_fS1, _fS2);

    _iS0 = _mm_cvtps_epi32(_fS0);
    _iS1 = _mm_cvtps_epi32(_fS1);
    _iS0 = _mm_packs_epi32(_iS0, _iS1);
    _mm_storeu_si128((__m128i*) (pDst + 56), _iS0);
  }

  return;
} /* mfxdct_qnt_fwd_8x8_ls() */

#endif
