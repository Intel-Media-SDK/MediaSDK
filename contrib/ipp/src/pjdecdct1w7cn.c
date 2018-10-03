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
//   Inverse DCT transform with scale image size and
//   de-quantization and level shift
//
//  Contents:
//   mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R
//   mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif

#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)

#include <emmintrin.h>

extern void mfxdct_8x8To2x2_inv_16s(Ipp16s* pSrc, Ipp16s* pDst);
extern void mfxdct_8x8To4x4_inv_16s(Ipp16s* pSrc, Ipp16s* pDst);

static const __ALIGN16 Ipp16u iSA[8] = { 128, 128, 128, 128, 128, 128, 128, 128 };


extern void mfxdct_quant_inv8x8To4x4_ls(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable)
{
  Ipp16s* workbuf;
  Ipp8u   buf[DCTSIZE2 * sizeof(Ipp16s) + CPU_CACHE_LINE-1];
  Ipp32s* p1;
  Ipp32s* p2;
  Ipp32s* p3;
  Ipp32s* p4;

  __ALIGN16 __m128i _iS0, _iS1, _iS2, _iSA;

   p1 = (Ipp32s*)(pDst + 0*dstStep);
   p2 = (Ipp32s*)(pDst + 1*dstStep);
   p3 = (Ipp32s*)(pDst + 2*dstStep);
   p4 = (Ipp32s*)(pDst + 3*dstStep);

  workbuf = (Ipp16s*)IPP_ALIGNED_PTR(&buf[0],CPU_CACHE_LINE);

  if(!((intptr_t)pQuantInvTable & 15) && !((intptr_t)pSrc & 15))
  {
    _iS0 = _mm_load_si128((__m128i*)(pSrc +  0));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  0));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 0), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  8));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  8));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 8), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  16));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  16));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 16), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  24));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  24));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 24), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  32));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  32));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 32), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  40));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  40));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 40), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  48));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  48));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 48), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  56));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  56));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 56), _iS2);
  }
  else
  {
    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  0));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  0));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 0), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  8));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  8));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 8), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  16));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  16));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 16), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  24));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  24));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 24), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  32));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  32));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 32), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  40));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  40));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 40), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  48));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  48));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 48), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  56));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  56));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 56), _iS2);
  }

  mfxdct_8x8To4x4_inv_16s(workbuf, workbuf);

  _iSA = _mm_load_si128((__m128i*)(iSA + 0));

  _iS0 = _mm_load_si128((__m128i*)(workbuf + 0));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS1 = _mm_packus_epi16(_iS0, _iS0);

  *p1 = (Ipp32s)_mm_cvtsi128_si32(_iS1);
  _iS2 = _mm_srli_si128(_iS1, 4);
  *p2 = (Ipp32s)_mm_cvtsi128_si32(_iS2);

  _iS0 = _mm_load_si128((__m128i*)(workbuf + 8));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS1 = _mm_packus_epi16(_iS0, _iS0);

  *p3 = (Ipp32s)_mm_cvtsi128_si32(_iS1);
  _iS2 = _mm_srli_si128(_iS1, 4);
  *p4 = (Ipp32s)_mm_cvtsi128_si32(_iS2);

  return;
} /* mfxdct_quant_inv8x8To4x4_ls() */


extern void mfxdct_quant_inv8x8To2x2_ls(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable)
{
  int      t1;
  Ipp16s*  workbuf;
  Ipp16s*  p1;
  Ipp16s*  p2;

  Ipp8u   buf[DCTSIZE2 * sizeof(Ipp16s) + CPU_CACHE_LINE-1];

  __ALIGN16 __m128i _iS0, _iS1, _iS2, _iSA;

  p1 = (Ipp16s*)(pDst + 0*dstStep);
  p2 = (Ipp16s*)(pDst + 1*dstStep);

  workbuf = (Ipp16s*)IPP_ALIGNED_PTR(&buf[0],CPU_CACHE_LINE);

  if(!((intptr_t)pQuantInvTable & 15) && !((intptr_t)pSrc & 15))
  {
    _iS0 = _mm_load_si128((__m128i*)(pSrc +  0));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  0));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 0), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  8));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  8));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 8), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  16));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  16));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 16), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  24));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  24));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 24), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  32));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  32));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 32), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  40));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  40));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 40), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  48));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  48));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 48), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  56));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  56));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 56), _iS2);
  }
  else
  {
    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  0));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  0));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 0), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  8));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  8));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 8), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  16));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  16));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 16), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  24));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  24));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 24), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  32));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  32));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 32), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  40));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  40));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 40), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  48));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  48));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 48), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  56));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  56));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)(workbuf + 56), _iS2);
  }

  mfxdct_8x8To2x2_inv_16s(workbuf, workbuf);

  _iSA = _mm_load_si128((__m128i*)(iSA + 0));

  _iS0 = _mm_load_si128((__m128i*)(workbuf + 0));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS1 = _mm_packus_epi16(_iS0, _iS0);

  t1 = _mm_cvtsi128_si32(_iS1);

  *p1 = (Ipp16s)t1;
  *p2 = (Ipp16s) (t1 >> 16) ;

  return;
} /* mfxdct_quant_inv8x8To4x4_ls() */

#endif
