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
//    DCT + quantization+level shift and
//    range convert functions (Inverse transform)
//
//  Contents:
//    mfxiDCTQuantInv8x8LS_JPEG_16s16u_C1R
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#include "ps_anarith.h"


#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)

#include <emmintrin.h>

extern void mfxdct_8x8_inv_32f(Ipp32f* x, Ipp32f* y);

static const __ALIGN16 Ipp32f fLS[4] = { 2048., 2048., 2048., 2048. };
static const __ALIGN16 Ipp16u iST[8] = { 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095 };

extern void mfxdct_qnt_inv_8x8_ls(
  const Ipp16s* pSrc,
        Ipp16u* pDst,
        int     dstStep,
  const Ipp32f* pQntInvTbl)
{
//  int i;
  __ALIGN16 __m128  _fS0, _fS1, _fS2;
  __ALIGN16 __m128i _iS0, _iS1, _iS2, _iST, _iZR;
  __ALIGN16 Ipp32f wb[64];
  __ALIGN16 Ipp32f wb2[64]; /* 665 clocks with second buf (690 with only one) */

  if(!((intptr_t)pQntInvTbl & 15) && !((intptr_t)pSrc & 15))
  {
    _iS0 = _mm_load_si128((__m128i*)(pSrc +  0));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_load_ps(pQntInvTbl);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_load_ps(pQntInvTbl +  4);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb +  0, _fS1);
    _mm_store_ps(wb +  4, _fS0);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  8));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_load_ps(pQntInvTbl +  8);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_load_ps(pQntInvTbl + 12);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb +  8, _fS1);
    _mm_store_ps(wb + 12, _fS0);

    _iS0 = _mm_load_si128((__m128i*)(pSrc + 16));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_load_ps(pQntInvTbl + 16);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_load_ps(pQntInvTbl + 20);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 16, _fS1);
    _mm_store_ps(wb + 20, _fS0);

    _iS0 = _mm_load_si128((__m128i*)(pSrc + 24));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_load_ps(pQntInvTbl + 24);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_load_ps(pQntInvTbl + 28);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 24, _fS1);
    _mm_store_ps(wb + 28, _fS0);

    _iS0 = _mm_load_si128((__m128i*)(pSrc + 32));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_load_ps(pQntInvTbl + 32);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_load_ps(pQntInvTbl + 36);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 32, _fS1);
    _mm_store_ps(wb + 36, _fS0);

    _iS0 = _mm_load_si128((__m128i*)(pSrc + 40));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_load_ps(pQntInvTbl + 40);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_load_ps(pQntInvTbl + 44);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 40, _fS1);
    _mm_store_ps(wb + 44, _fS0);

    _iS0 = _mm_load_si128((__m128i*)(pSrc + 48));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_load_ps(pQntInvTbl + 48);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_load_ps(pQntInvTbl + 52);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 48, _fS1);
    _mm_store_ps(wb + 52, _fS0);

    _iS0 = _mm_load_si128((__m128i*)(pSrc + 56));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_load_ps(pQntInvTbl + 56);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_load_ps(pQntInvTbl + 60);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 56, _fS1);
    _mm_store_ps(wb + 60, _fS0);
  }
  else
  {
    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  0));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_loadu_ps(pQntInvTbl);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_loadu_ps(pQntInvTbl +  4);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb +  0, _fS1);
    _mm_store_ps(wb +  4, _fS0);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  8));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_loadu_ps(pQntInvTbl +  8);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_loadu_ps(pQntInvTbl + 12);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb +  8, _fS1);
    _mm_store_ps(wb + 12, _fS0);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc + 16));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_loadu_ps(pQntInvTbl + 16);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_loadu_ps(pQntInvTbl + 20);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 16, _fS1);
    _mm_store_ps(wb + 20, _fS0);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc + 24));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_loadu_ps(pQntInvTbl + 24);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_loadu_ps(pQntInvTbl + 28);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 24, _fS1);
    _mm_store_ps(wb + 28, _fS0);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc + 32));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_loadu_ps(pQntInvTbl + 32);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_loadu_ps(pQntInvTbl + 36);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 32, _fS1);
    _mm_store_ps(wb + 36, _fS0);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc + 40));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_loadu_ps(pQntInvTbl + 40);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_loadu_ps(pQntInvTbl + 44);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 40, _fS1);
    _mm_store_ps(wb + 44, _fS0);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc + 48));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_loadu_ps(pQntInvTbl + 48);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_loadu_ps(pQntInvTbl + 52);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 48, _fS1);
    _mm_store_ps(wb + 52, _fS0);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc + 56));
    _iS1 = _mm_srai_epi16(_iS0, 15);
    _iS2 = _mm_unpacklo_epi16(_iS0, _iS1);
    _iS0 = _mm_unpackhi_epi16(_iS0, _iS1);
    _fS1 = _mm_cvtepi32_ps(_iS2);
    _fS0 = _mm_cvtepi32_ps(_iS0);

    _fS2 = _mm_loadu_ps(pQntInvTbl + 56);
    _fS1 = _mm_mul_ps(_fS1, _fS2);
    _fS2 = _mm_loadu_ps(pQntInvTbl + 60);
    _fS0 = _mm_mul_ps(_fS0, _fS2);
    _mm_store_ps(wb + 56, _fS1);
    _mm_store_ps(wb + 60, _fS0);
  }

  mfxdct_8x8_inv_32f(wb, wb2);

  _iZR = _mm_setzero_si128();
  _iST = _mm_load_si128((__m128i*)iST);
  _fS2 = _mm_load_ps(fLS);

  _fS0 = _mm_load_ps(wb2 +  0);
  _fS0 = _mm_add_ps(_fS0, _fS2);
  _fS1 = _mm_load_ps(wb2 +  4);
  _fS1 = _mm_add_ps(_fS1, _fS2);

  _iS0 = _mm_cvttps_epi32(_fS0);
  _iS1 = _mm_cvttps_epi32(_fS1);
  _iS0 = _mm_packs_epi32(_iS0,_iS1);
  // saturate to 0...4095
  _iS0 = _mm_max_epi16(_iS0,_iZR);
  _iS0 = _mm_min_epi16(_iS0,_iST);
  _mm_storeu_si128((__m128i*)((Ipp8u*)pDst + 0*dstStep), _iS0);

  _fS0 = _mm_load_ps(wb2 +  8);
  _fS0 = _mm_add_ps(_fS0, _fS2);
  _fS1 = _mm_load_ps(wb2 + 12);
  _fS1 = _mm_add_ps(_fS1, _fS2);

  _iS0 = _mm_cvttps_epi32(_fS0);
  _iS1 = _mm_cvttps_epi32(_fS1);
  _iS0 = _mm_packs_epi32(_iS0, _iS1);
  // saturate to 0...4095
  _iS0 = _mm_max_epi16(_iS0,_iZR);
  _iS0 = _mm_min_epi16(_iS0,_iST);
  _mm_storeu_si128((__m128i*)((Ipp8u*)pDst + 1*dstStep), _iS0);

  _fS0 = _mm_load_ps(wb2 + 16);
  _fS0 = _mm_add_ps(_fS0, _fS2);
  _fS1 = _mm_load_ps(wb2 + 20);
  _fS1 = _mm_add_ps(_fS1, _fS2);

  _iS0 = _mm_cvttps_epi32(_fS0);
  _iS1 = _mm_cvttps_epi32(_fS1);
  _iS0 = _mm_packs_epi32(_iS0, _iS1);
  // saturate to 0...4095
  _iS0 = _mm_max_epi16(_iS0,_iZR);
  _iS0 = _mm_min_epi16(_iS0,_iST);
  _mm_storeu_si128((__m128i*)((Ipp8u*)pDst + 2*dstStep), _iS0);

  _fS0 = _mm_load_ps(wb2 + 24);
  _fS0 = _mm_add_ps(_fS0, _fS2);
  _fS1 = _mm_load_ps(wb2 + 28);
  _fS1 = _mm_add_ps(_fS1, _fS2);

  _iS0 = _mm_cvttps_epi32(_fS0);
  _iS1 = _mm_cvttps_epi32(_fS1);
  _iS0 = _mm_packs_epi32(_iS0, _iS1);
  // saturate to 0...4095
  _iS0 = _mm_max_epi16(_iS0,_iZR);
  _iS0 = _mm_min_epi16(_iS0,_iST);
  _mm_storeu_si128((__m128i*)((Ipp8u*)pDst + 3*dstStep), _iS0);

  _fS0 = _mm_load_ps(wb2 + 32);
  _fS0 = _mm_add_ps(_fS0, _fS2);
  _fS1 = _mm_load_ps(wb2 + 36);
  _fS1 = _mm_add_ps(_fS1, _fS2);

  _iS0 = _mm_cvttps_epi32(_fS0);
  _iS1 = _mm_cvttps_epi32(_fS1);
  _iS0 = _mm_packs_epi32(_iS0, _iS1);
  // saturate to 0...4095
  _iS0 = _mm_max_epi16(_iS0,_iZR);
  _iS0 = _mm_min_epi16(_iS0,_iST);
  _mm_storeu_si128((__m128i*)((Ipp8u*)pDst + 4*dstStep), _iS0);

  _fS0 = _mm_load_ps(wb2 + 40);
  _fS0 = _mm_add_ps(_fS0, _fS2);
  _fS1 = _mm_load_ps(wb2 + 44);
  _fS1 = _mm_add_ps(_fS1, _fS2);

  _iS0 = _mm_cvttps_epi32(_fS0);
  _iS1 = _mm_cvttps_epi32(_fS1);
  _iS0 = _mm_packs_epi32(_iS0, _iS1);
  // saturate to 0...4095
  _iS0 = _mm_max_epi16(_iS0,_iZR);
  _iS0 = _mm_min_epi16(_iS0,_iST);
  _mm_storeu_si128((__m128i*)((Ipp8u*)pDst + 5*dstStep), _iS0);

  _fS0 = _mm_load_ps(wb2 + 48);
  _fS0 = _mm_add_ps(_fS0, _fS2);
  _fS1 = _mm_load_ps(wb2 + 52);
  _fS1 = _mm_add_ps(_fS1, _fS2);

  _iS0 = _mm_cvttps_epi32(_fS0);
  _iS1 = _mm_cvttps_epi32(_fS1);
  _iS0 = _mm_packs_epi32(_iS0, _iS1);
  // saturate to 0...4095
  _iS0 = _mm_max_epi16(_iS0,_iZR);
  _iS0 = _mm_min_epi16(_iS0,_iST);
  _mm_storeu_si128((__m128i*)((Ipp8u*)pDst + 6*dstStep), _iS0);

  _fS0 = _mm_load_ps(wb2 + 56);
  _fS0 = _mm_add_ps(_fS0, _fS2);
  _fS1 = _mm_load_ps(wb2 + 60);
  _fS1 = _mm_add_ps(_fS1, _fS2);

  _iS0 = _mm_cvttps_epi32(_fS0);
  _iS1 = _mm_cvttps_epi32(_fS1);
  _iS0 = _mm_packs_epi32(_iS0, _iS1);
  // saturate to 0...4095
  _iS0 = _mm_max_epi16(_iS0,_iZR);
  _iS0 = _mm_min_epi16(_iS0,_iST);
  _mm_storeu_si128((__m128i*)((Ipp8u*)pDst + 7*dstStep), _iS0);

  return;
} /* mfxdct_qnt_fwd_8x8_ls() */

#endif
