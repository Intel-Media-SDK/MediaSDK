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
//   Inverse DCT transform with nonzero elements only in
//   top left 1x1, 2x2 or 4x4  quadrant + de-quantization and level shift
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif

#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)

#include <emmintrin.h>

extern void mfxdct_8x8_inv_2x2_16s(Ipp16s* pSrc, Ipp16s* pDst);
extern void mfxdct_8x8_inv_4x4_16s(Ipp16s* pSrc, Ipp16s* pDst);

static const __ALIGN16 Ipp16u iSA[8] = { 128, 128, 128, 128, 128, 128, 128, 128 };


extern void mfxdct_quant_inv8x8_1x1_ls(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable)
{
  __ALIGN16 __m128i _iS0;

  Ipp16s val = ((pSrc[0] * pQuantInvTable[0]) >> 3) + 128;
  pDst[0] = (Ipp8u)(val > 255 ? 255 : (val < 0 ? 0 : val));

  _iS0 = _mm_set1_epi8((char)pDst[0]);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 0*dstStep), _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 1*dstStep), _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 2*dstStep), _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 3*dstStep), _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 4*dstStep), _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 5*dstStep), _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 6*dstStep), _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 7*dstStep), _iS0);

  return;
} /* mfxdct_quant_inv8x8_1x1_ls() */


extern void mfxdct_quant_inv8x8_2x2_ls(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable)
{
  Ipp16s*   buf;
  Ipp16s    ptr[64*sizeof(Ipp16s) + CPU_CACHE_LINE-1];

  __ALIGN16 __m128i _iS0, _iS1, _iS2, _iSA;

  buf = (Ipp16s*)IPP_ALIGNED_PTR(&ptr[0],CPU_CACHE_LINE);

  if((IPP_UINT_PTR(pSrc) || IPP_UINT_PTR(pQuantInvTable)) & 0x0F) {  /* Source or quant table is not 16-byte-aligned */
    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  0));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  0));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 0), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  8));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  8));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 8), _iS2);
  } else {
    _iS0 = _mm_load_si128((__m128i*)(pSrc +  0));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  0));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 0), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  8));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  8));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 8), _iS2);
  }

  mfxdct_8x8_inv_2x2_16s(buf, buf);

  _mm_setzero_si128();
  _iSA = _mm_load_si128((__m128i*)(iSA + 0));

  _iS0 = _mm_load_si128((__m128i*)(buf + 0));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS2 = _mm_packus_epi16(_iS0, _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 0*dstStep), _iS2);

  _iS1 = _mm_load_si128((__m128i*)(buf + 8));
  _iS1 = _mm_adds_epi16(_iS1,_iSA);
  _iS2 = _mm_packus_epi16(_iS1, _iS1);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 1*dstStep), _iS2);

  _iS0 = _mm_load_si128((__m128i*)(buf + 16));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS2 = _mm_packus_epi16(_iS0, _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 2*dstStep), _iS2);

  _iS1 = _mm_load_si128((__m128i*)(buf + 24));
  _iS1 = _mm_adds_epi16(_iS1,_iSA);
  _iS2 = _mm_packus_epi16(_iS1, _iS1);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 3*dstStep), _iS2);

  _iS0 = _mm_load_si128((__m128i*)(buf + 32));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS2 = _mm_packus_epi16(_iS0, _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 4*dstStep), _iS2);

  _iS1 = _mm_load_si128((__m128i*)(buf + 40));
  _iS1 = _mm_adds_epi16(_iS1,_iSA);
  _iS2 = _mm_packus_epi16(_iS1, _iS1);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 5*dstStep), _iS2);

  _iS0 = _mm_load_si128((__m128i*)(buf + 48));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS2 = _mm_packus_epi16(_iS0, _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 6*dstStep), _iS2);

  _iS1 = _mm_load_si128((__m128i*)(buf + 56));
  _iS1 = _mm_adds_epi16(_iS1,_iSA);
  _iS2 = _mm_packus_epi16(_iS1, _iS1);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 7*dstStep), _iS2);

  return;
} /* mfxdct_quant_inv8x8_2x2_ls() */


extern void mfxdct_quant_inv8x8_4x4_ls(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable)
{
  Ipp16s*   buf;
  Ipp16s    ptr[64*sizeof(Ipp16s) + CPU_CACHE_LINE-1];

  __ALIGN16 __m128i _iS0, _iS1, _iS2, _iSA;

  buf = (Ipp16s*)IPP_ALIGNED_PTR(&ptr[0],CPU_CACHE_LINE);
  if((IPP_UINT_PTR(pSrc) || IPP_UINT_PTR(pQuantInvTable)) & 0x0F) {  /* If pSrc or pQuantInvTable is not aligned on 16 byte */
    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  0));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  0));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 0), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  8));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  8));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 8), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  16));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  16));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 16), _iS2);

    _iS0 = _mm_loadu_si128((__m128i*)(pSrc +  24));
    _iS1 = _mm_loadu_si128((__m128i*)(pQuantInvTable +  24));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 24), _iS2);
  } else { /* Everything is aligned */
    _iS0 = _mm_load_si128((__m128i*)(pSrc +  0));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  0));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 0), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  8));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  8));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 8), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  16));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  16));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 16), _iS2);

    _iS0 = _mm_load_si128((__m128i*)(pSrc +  24));
    _iS1 = _mm_load_si128((__m128i*)(pQuantInvTable +  24));
    _iS2 = _mm_mullo_epi16(_iS0, _iS1);
    _mm_store_si128((__m128i*)((Ipp16s*)buf + 24), _iS2);
  }

  mfxdct_8x8_inv_4x4_16s(buf, buf);

  _mm_setzero_si128();
  _iSA = _mm_load_si128((__m128i*)(iSA + 0));

  _iS0 = _mm_load_si128((__m128i*)(buf + 0));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS2 = _mm_packus_epi16(_iS0, _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 0*dstStep), _iS2);

  _iS1 = _mm_load_si128((__m128i*)(buf + 8));
  _iS1 = _mm_adds_epi16(_iS1,_iSA);
  _iS2 = _mm_packus_epi16(_iS1, _iS1);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 1*dstStep), _iS2);

  _iS0 = _mm_load_si128((__m128i*)(buf + 16));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS2 = _mm_packus_epi16(_iS0, _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 2*dstStep), _iS2);

  _iS1 = _mm_load_si128((__m128i*)(buf + 24));
  _iS1 = _mm_adds_epi16(_iS1,_iSA);
  _iS2 = _mm_packus_epi16(_iS1, _iS1);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 3*dstStep), _iS2);

  _iS0 = _mm_load_si128((__m128i*)(buf + 32));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS2 = _mm_packus_epi16(_iS0, _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 4*dstStep), _iS2);

  _iS1 = _mm_load_si128((__m128i*)(buf + 40));
  _iS1 = _mm_adds_epi16(_iS1,_iSA);
  _iS2 = _mm_packus_epi16(_iS1, _iS1);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 5*dstStep), _iS2);

  _iS0 = _mm_load_si128((__m128i*)(buf + 48));
  _iS0 = _mm_adds_epi16(_iS0,_iSA);
  _iS2 = _mm_packus_epi16(_iS0, _iS0);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 6*dstStep), _iS2);

  _iS1 = _mm_load_si128((__m128i*)(buf + 56));
  _iS1 = _mm_adds_epi16(_iS1,_iSA);
  _iS2 = _mm_packus_epi16(_iS1, _iS1);
  _mm_storel_epi64((__m128i*)((Ipp8u*)pDst + 7*dstStep), _iS2);
  return;
} /* mfxdct_quant_inv8x8_4x4_ls() */

#endif
