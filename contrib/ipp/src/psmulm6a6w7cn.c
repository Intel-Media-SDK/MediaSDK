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

/*M*
//
//  Purpose: Functions of operation of multiplication
//
//  Contents:
//      mfxownsMulC_16s_I
//      mfxownsMul_16s_I
//
*M*/
#include <emmintrin.h> // __m128i etc

#include "precomp.h"

#if !defined(__OWNS_H__)

  #include "owns.h"

#endif

#if ( (_IPP >= _IPP_M6) || (_IPP32E >= _IPP32E_M7) )

#if (_IPP == _IPP_M6)

  #include <mmintrin.h>

#endif

#include "ps_anarith.h"

#if ( (_IPP >= _IPP_M6) && (_IPP <= _IPP_A6) )

/* Lib = M6, A6 */
/* Caller = ippsMul_16s_I, ippsMul_16s_ISfs, mfxiMul_16s_C1IRSfs */
void mfxownsMul_16s_I(const Ipp16s* pSrc, Ipp16s* pSrcDst, int len)
{
  _Alignas(16)
  __m64 t0, t1, t2, t3, t4, t5, t6, t7, mmxZero;
  Ipp32s  tmpValue;
  int     tmp;

  if( len >= (8 + 3) ) {
    mmxZero = _mm_setzero_si64();
    if( (IPP_INT_PTR(pSrcDst) & 1) == 0 ) {
      if( (tmp = (int)(IPP_INT_PTR(pSrcDst) & 7)) != 0 ) {
        tmp = ( -(tmp >> 1) & 3);
        len -= tmp;
        do {
          tmpValue = IPP_MIN((Ipp32s)(*pSrcDst * *pSrc), IPP_MAX_16S);
          *pSrcDst = (Ipp16s)IPP_MAX(tmpValue, IPP_MIN_16S);
          pSrc++;
          pSrcDst++;
        } while( --tmp );
      } /* if */
    } /* if */
    tmp = len & ~7;
    len = len &  7;
    do {
      t3 = *((__m64*)pSrc);
      t7 = *((__m64*)pSrc + 1);
      t1 = *((__m64*)pSrcDst);
      t5 = *((__m64*)pSrcDst + 1);
      t0 = _m_punpcklwd(t1, t1);
      t4 = _m_punpcklwd(t5, t5);
      t2 = _m_punpcklwd(t3, mmxZero);
      t6 = _m_punpcklwd(t7, mmxZero);
      t1 = _m_punpckhwd(t1, t1);
      t5 = _m_punpckhwd(t5, t5);
      t3 = _m_punpckhwd(t3, mmxZero);
      t7 = _m_punpckhwd(t7, mmxZero);
      t0 = _m_pmaddwd(t0, t2);
      t4 = _m_pmaddwd(t4, t6);
      t1 = _m_pmaddwd(t1, t3);
      t5 = _m_pmaddwd(t5, t7);
      t0 = _m_packssdw(t0, t1);
      t4 = _m_packssdw(t4, t5);
      *((__m64*)pSrcDst) = t0;
      *((__m64*)pSrcDst + 1) = t4;
      pSrc    += 8;
      pSrcDst += 8;
    } while( tmp -= 8 );
    _m_empty();
  } /* if */
  for( ; len--; pSrc++, pSrcDst++ ) {
    tmpValue = IPP_MIN((Ipp32s)(*pSrcDst * *pSrc), IPP_MAX_16S);
    *pSrcDst = (Ipp16s)IPP_MAX(tmpValue, IPP_MIN_16S);
  } /* for */
} /* mfxownsMul_16s_I */

/* Lib = M6, A6 */
/* Caller = ippsMulC_16s_I, ippsMulC_16s_ISfs, mfxiMulC_16s_C1IRSfs */
void mfxownsMulC_16s_I(Ipp16s val, Ipp16s* pSrcDst, int len)
{
  _Alignas(16)
  __m64  t0, t1, t2, t3, mmxVal;
  Ipp32s tmpVal;
  int    tmp;

  if( len >= (16 + 3) ) {
    mmxVal = _mm_set1_pi32((int)((short)val & 0xFFFF));
    if( (IPP_INT_PTR(pSrcDst) & 1) == 0 ) {
      if( (tmp = (int)(IPP_INT_PTR(pSrcDst) & 7)) != 0 ) {
        tmp = ((0 - (tmp >> 1)) & 3);
        len -= tmp;
        do {
          tmpVal   = (Ipp32s)(*pSrcDst * val);
          tmpVal   = IPP_MIN(tmpVal, IPP_MAX_16S);
          *pSrcDst = (Ipp16s)IPP_MAX(tmpVal, IPP_MIN_16S);
          pSrcDst++;
        } while( --tmp );
      } /* if */
    } /* if */
    tmp = len & ~15;
    len = len &  15;
    do {
      t1 = *((__m64*)pSrcDst);
      t3 = *((__m64*)pSrcDst + 1);
      t0 = _m_punpcklwd(t1, t1);
      t1 = _m_punpckhwd(t1, t1);
      t2 = _m_punpcklwd(t3, t3);
      t3 = _m_punpckhwd(t3, t3);
      t0 = _m_pmaddwd(t0, mmxVal);
      t1 = _m_pmaddwd(t1, mmxVal);
      t2 = _m_pmaddwd(t2, mmxVal);
      t3 = _m_pmaddwd(t3, mmxVal);
      t0 = _m_packssdw(t0, t1);
      t2 = _m_packssdw(t2, t3);
      *((__m64*)pSrcDst)     = t0;
      *((__m64*)pSrcDst + 1) = t2;

      t1 = *((__m64*)pSrcDst + 2);
      t3 = *((__m64*)pSrcDst + 3);
      t0 = _m_punpcklwd(t1, t1);
      t1 = _m_punpckhwd(t1, t1);
      t2 = _m_punpcklwd(t3, t3);
      t3 = _m_punpckhwd(t3, t3);
      t0 = _m_pmaddwd(t0, mmxVal);
      t1 = _m_pmaddwd(t1, mmxVal);
      t2 = _m_pmaddwd(t2, mmxVal);
      t3 = _m_pmaddwd(t3, mmxVal);
      t0 = _m_packssdw(t0, t1);
      t2 = _m_packssdw(t2, t3);
      *((__m64*)pSrcDst + 2) = t0;
      *((__m64*)pSrcDst + 3) = t2;
      pSrcDst += 16;
    } while( tmp -= 16 );
    _m_empty();
  } /* if */
  for( ; len--; pSrcDst++ ) {
    tmpVal   = (Ipp32s)(*pSrcDst * val);
    tmpVal   = IPP_MIN(tmpVal, IPP_MAX_16S);
    *pSrcDst = (Ipp16s)IPP_MAX(tmpVal, IPP_MIN_16S);
  } /* for */
} /* mfxownsMulC_16s_I */


#endif


#if ( (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7) )

/* Lib = W7 */
/* Caller = ippsMul_16s_I, ippsMul_16s_ISfs, mfxiMul_16s_C1IRSfs */
void mfxownsMul_16s_I(const Ipp16s* pSrc, Ipp16s* pSrcDst, int len)
{
  _Alignas(16)
  __m128i t0, t1, t2, t3, t4, t5, t6, t7, emmZero;
  Ipp32s  tmpValue;
  int     tmp;

  if( len >= (16 + 7) ) {
    emmZero = _mm_setzero_si128();
    if( (IPP_INT_PTR(pSrcDst) & 1) == 0 ) {
      if( (tmp = (int)(IPP_INT_PTR(pSrcDst) & 15)) != 0 ) {
        tmp = ( -(tmp >> 1) & 7);
        len -= tmp;
        do {
          tmpValue = IPP_MIN((Ipp32s)(*pSrcDst * *pSrc), IPP_MAX_16S);
          *pSrcDst = (Ipp16s)IPP_MAX(tmpValue, IPP_MIN_16S);
          pSrc++;
          pSrcDst++;
        } while( --tmp );
      } /* if */
      tmp = len & ~15;
      len = len &  15;
      if( (IPP_INT_PTR(pSrc) & 15) != 0 ) {
        do {
          t1 = _mm_loadu_si128((__m128i*)pSrc);
          t3 = _mm_loadu_si128((__m128i*)pSrc + 1);
          t5 = _mm_load_si128((__m128i*)pSrcDst);
          t7 = _mm_load_si128((__m128i*)pSrcDst + 1);
          t0 = _mm_unpacklo_epi16(t1, emmZero);
          t1 = _mm_unpackhi_epi16(t1, emmZero);
          t2 = _mm_unpacklo_epi16(t3, emmZero);
          t3 = _mm_unpackhi_epi16(t3, emmZero);
          t4 = _mm_unpacklo_epi16(t5, emmZero);
          t5 = _mm_unpackhi_epi16(t5, emmZero);
          t6 = _mm_unpacklo_epi16(t7, emmZero);
          t7 = _mm_unpackhi_epi16(t7, emmZero);
          t0 = _mm_madd_epi16(t0, t4);
          t1 = _mm_madd_epi16(t1, t5);
          t2 = _mm_madd_epi16(t2, t6);
          t3 = _mm_madd_epi16(t3, t7);
          t0 = _mm_packs_epi32(t0, t1);
          t2 = _mm_packs_epi32(t2, t3);
          _mm_store_si128((__m128i*)pSrcDst,     t0);
          _mm_store_si128((__m128i*)pSrcDst + 1, t2);
          pSrc    += 16;
          pSrcDst += 16;
        } while( tmp -= 16 );
      } /* if */
      else {
        do {
          t1 = _mm_load_si128((__m128i*)pSrc);
          t3 = _mm_load_si128((__m128i*)pSrc + 1);
          t5 = _mm_load_si128((__m128i*)pSrcDst);
          t7 = _mm_load_si128((__m128i*)pSrcDst + 1);
          t0 = _mm_unpacklo_epi16(t1, emmZero);
          t1 = _mm_unpackhi_epi16(t1, emmZero);
          t2 = _mm_unpacklo_epi16(t3, emmZero);
          t3 = _mm_unpackhi_epi16(t3, emmZero);
          t4 = _mm_unpacklo_epi16(t5, emmZero);
          t5 = _mm_unpackhi_epi16(t5, emmZero);
          t6 = _mm_unpacklo_epi16(t7, emmZero);
          t7 = _mm_unpackhi_epi16(t7, emmZero);
          t0 = _mm_madd_epi16(t0, t4);
          t1 = _mm_madd_epi16(t1, t5);
          t2 = _mm_madd_epi16(t2, t6);
          t3 = _mm_madd_epi16(t3, t7);
          t0 = _mm_packs_epi32(t0, t1);
          t2 = _mm_packs_epi32(t2, t3);
          _mm_store_si128((__m128i*)pSrcDst,     t0);
          _mm_store_si128((__m128i*)pSrcDst + 1, t2);
          pSrc    += 16;
          pSrcDst += 16;
        } while( tmp -= 16 );
      } /* else */
    } /* if */
    else {
      tmp = len & ~15;
      len = len &  15;
      if( (IPP_INT_PTR(pSrc) & 15) != 0 ) {
        do {
          t1 = _mm_loadu_si128((__m128i*)pSrc);
          t3 = _mm_loadu_si128((__m128i*)pSrc + 1);
          t5 = _mm_loadu_si128((__m128i*)pSrcDst);
          t7 = _mm_loadu_si128((__m128i*)pSrcDst + 1);
          t0 = _mm_unpacklo_epi16(t1, emmZero);
          t1 = _mm_unpackhi_epi16(t1, emmZero);
          t2 = _mm_unpacklo_epi16(t3, emmZero);
          t3 = _mm_unpackhi_epi16(t3, emmZero);
          t4 = _mm_unpacklo_epi16(t5, emmZero);
          t5 = _mm_unpackhi_epi16(t5, emmZero);
          t6 = _mm_unpacklo_epi16(t7, emmZero);
          t7 = _mm_unpackhi_epi16(t7, emmZero);
          t0 = _mm_madd_epi16(t0, t4);
          t1 = _mm_madd_epi16(t1, t5);
          t2 = _mm_madd_epi16(t2, t6);
          t3 = _mm_madd_epi16(t3, t7);
          t0 = _mm_packs_epi32(t0, t1);
          t2 = _mm_packs_epi32(t2, t3);
          _mm_storeu_si128((__m128i*)pSrcDst,     t0);
          _mm_storeu_si128((__m128i*)pSrcDst + 1, t2);
          pSrc    += 16;
          pSrcDst += 16;
        } while( tmp -= 16 );
      } /* if */
      else {
        do {
          t1 = _mm_load_si128((__m128i*)pSrc);
          t3 = _mm_load_si128((__m128i*)pSrc + 1);
          t5 = _mm_loadu_si128((__m128i*)pSrcDst);
          t7 = _mm_loadu_si128((__m128i*)pSrcDst + 1);
          t0 = _mm_unpacklo_epi16(t1, emmZero);
          t1 = _mm_unpackhi_epi16(t1, emmZero);
          t2 = _mm_unpacklo_epi16(t3, emmZero);
          t3 = _mm_unpackhi_epi16(t3, emmZero);
          t4 = _mm_unpacklo_epi16(t5, emmZero);
          t5 = _mm_unpackhi_epi16(t5, emmZero);
          t6 = _mm_unpacklo_epi16(t7, emmZero);
          t7 = _mm_unpackhi_epi16(t7, emmZero);
          t0 = _mm_madd_epi16(t0, t4);
          t1 = _mm_madd_epi16(t1, t5);
          t2 = _mm_madd_epi16(t2, t6);
          t3 = _mm_madd_epi16(t3, t7);
          t0 = _mm_packs_epi32(t0, t1);
          t2 = _mm_packs_epi32(t2, t3);
          _mm_storeu_si128((__m128i*)pSrcDst,     t0);
          _mm_storeu_si128((__m128i*)pSrcDst + 1, t2);
          pSrc    += 16;
          pSrcDst += 16;
        } while( tmp -= 16 );
      } /* else */
    } /* else */
  } /* if */
  for( ; len--; pSrc++, pSrcDst++ ) {
    tmpValue = IPP_MIN((Ipp32s)(*pSrcDst * *pSrc), IPP_MAX_16S);
    *pSrcDst = (Ipp16s)IPP_MAX(tmpValue, IPP_MIN_16S);
  } /* for */
} /* mfxownsMul_16s_I */


#if !((_IPP32E >= _IPP32E_L9) || (_IPP >= _IPP_H9))
/* Lib = W7 */
/* Caller = ippsMulC_16s_I, ippsMulC_16s_ISfs, mfxiMulC_16s_C1IRSfs */
void mfxownsMulC_16s_I(Ipp16s val, Ipp16s* pSrcDst, int len)
{
  _Alignas(16)
  __m128i t0, t1, t2, t3, emmValue;
  Ipp32s  tmpVal;
  int     tmp;

  if( len >= (16 + 7) ) {
    emmValue = _mm_set1_epi32((int)((short)val & 0xFFFF));
    if( (IPP_INT_PTR(pSrcDst) & 1) == 0 ) {
      if( (tmp = (int)(IPP_INT_PTR(pSrcDst) & 15)) != 0 ) {
        tmp = ((0 - (tmp >> 1)) & 7);
        len -= tmp;
        do {
          tmpVal   = IPP_MIN((Ipp32s)(*pSrcDst * val), IPP_MAX_16S);
          *pSrcDst = (Ipp16s)IPP_MAX(tmpVal, IPP_MIN_16S);
          pSrcDst++;
        } while( --tmp );
      } /* if */
      tmp = len & ~15;
      len = len &  15;
      do {
        t1 = _mm_load_si128((__m128i*)pSrcDst);
        t3 = _mm_load_si128((__m128i*)pSrcDst + 1);
        t0 = _mm_unpacklo_epi16(t1, t1);
        t1 = _mm_unpackhi_epi16(t1, t1);
        t2 = _mm_unpacklo_epi16(t3, t3);
        t3 = _mm_unpackhi_epi16(t3, t3);
        t0 = _mm_madd_epi16(t0, emmValue);
        t1 = _mm_madd_epi16(t1, emmValue);
        t2 = _mm_madd_epi16(t2, emmValue);
        t3 = _mm_madd_epi16(t3, emmValue);
        t0 = _mm_packs_epi32(t0, t1);
        t2 = _mm_packs_epi32(t2, t3);
        _mm_store_si128((__m128i*)pSrcDst,     t0);
        _mm_store_si128((__m128i*)pSrcDst + 1, t2);
        pSrcDst += 16;
      } while( tmp -= 16 );
    } /* if */
    else {
      tmp = len & ~15;
      len = len &  15;
      do {
        t1 = _mm_loadu_si128((__m128i*)pSrcDst);
        t3 = _mm_loadu_si128((__m128i*)pSrcDst + 1);
        t0 = _mm_unpacklo_epi16(t1, t1);
        t1 = _mm_unpackhi_epi16(t1, t1);
        t2 = _mm_unpacklo_epi16(t3, t3);
        t3 = _mm_unpackhi_epi16(t3, t3);
        t0 = _mm_madd_epi16(t0, emmValue);
        t1 = _mm_madd_epi16(t1, emmValue);
        t2 = _mm_madd_epi16(t2, emmValue);
        t3 = _mm_madd_epi16(t3, emmValue);
        t0 = _mm_packs_epi32(t0, t1);
        t2 = _mm_packs_epi32(t2, t3);
        _mm_storeu_si128((__m128i*)pSrcDst,     t0);
        _mm_storeu_si128((__m128i*)pSrcDst + 1, t2);
        pSrcDst += 16;
      } while( tmp -= 16 );
    } /* else */
  } /* if */
  for( ; len--; pSrcDst++ ) {
    tmpVal   = IPP_MIN((Ipp32s)(*pSrcDst * val), IPP_MAX_16S);
    *pSrcDst = (Ipp16s)IPP_MAX(tmpVal, IPP_MIN_16S);
  } /* for */
} /* mfxownsMulC_16s_I */

/* Lib = W7 */
/* Caller = mfxs_Mul_16u16s_Sfs */
void mfxownsMul_16u16s_PosSfs(const Ipp16u* pSrcU, const Ipp16s* pSrcS, Ipp16s* pDst, int len,
                           int scaleFactor)
{
  _Alignas(16)
  __m128i t0, t1, t2, t3, t4, t5, t6, const0, const01, const11, constW, sFactor;
  const   Ipp32s const1 = (1 << (scaleFactor - 1)) - 1;
  Ipp32s  tmpVal;
  int     tmp;

  scaleFactor--;
  if( len >= (8 + 7) ) {
    if( (IPP_INT_PTR(pDst) & 1) == 0 ) {
      if( (tmp = (int)(IPP_INT_PTR(pDst) & 15)) != 0 ) {
        tmp = ((-(tmp >> 1)) & 7);
        len -= tmp;
        do {
          tmpVal = (Ipp32s)(((Ipp32s)*pSrcU) * *pSrcS);
          tmpVal = ((tmpVal >> 1) + (((tmpVal & 1) + const1 +
                    (((tmpVal >> 1) >> scaleFactor) & 1)) >> 1)) >> scaleFactor;
          tmpVal = IPP_MIN(tmpVal, IPP_MAX_16S);
          *pDst  = (Ipp16s)IPP_MAX(tmpVal, IPP_MIN_16S);
          pSrcU++;
          pSrcS++;
          pDst++;
        } while( --tmp );
      } /* if */
      sFactor = _mm_cvtsi32_si128(scaleFactor);
      constW  = _mm_set1_epi32((int)(const1 >> 1));
      const0  = _mm_setzero_si128();
      const01 = _mm_set1_epi32(0x00000001);
      const11 = _mm_set1_epi32(0x00010001);
      tmp     = len >> 3;
      len     = len &  7;
      if( (IPP_INT_PTR(pSrcU) & 15) != 0 ) {
        if( (IPP_INT_PTR(pSrcS) & 15) != 0 ) {
          do {
            t1 = _mm_loadu_si128((__m128i*)pSrcU);
            t2 = _mm_loadu_si128((__m128i*)pSrcS);
            t0 = _mm_srli_epi16(t1, 1);
            t1 = _mm_and_si128(t1, const11);
            t4 = _mm_unpackhi_epi16(t0, t1);
            t0 = _mm_unpacklo_epi16(t0, t1);
            t1 = _mm_and_si128(t1, t2);
            t6 = _mm_unpackhi_epi16(t1, const0);
            t1 = _mm_unpacklo_epi16(t1, const0);
            t3 = _mm_srai_epi16(t2, 1);
            t5 = _mm_unpackhi_epi16(t2, t3);
            t2 = _mm_unpacklo_epi16(t2, t3);
            t4 = _mm_madd_epi16(t4, t5);
            t0 = _mm_madd_epi16(t0, t2);
            t2 = _mm_srl_epi32(t4, sFactor);
            t3 = _mm_srl_epi32(t0, sFactor);
            t4 = _mm_add_epi32(t4, constW);
            t0 = _mm_add_epi32(t0, constW);
            t2 = _mm_and_si128(t2, const01);
            t3 = _mm_and_si128(t3, const01);
            t6 = _mm_or_si128(t6, t2);
            t1 = _mm_or_si128(t1, t3);
            t4 = _mm_add_epi32(t4, t6);
            t0 = _mm_add_epi32(t0, t1);
            t4 = _mm_sra_epi32(t4, sFactor);
            t0 = _mm_sra_epi32(t0, sFactor);
            t0 = _mm_packs_epi32(t0, t4);
            _mm_store_si128((__m128i*)pDst, t0);
            pSrcU += 8;
            pSrcS += 8;
            pDst  += 8;
          } while( --tmp );
        } /* if */
        else {
          do {
            t1 = _mm_loadu_si128((__m128i*)pSrcU);
            t2 = _mm_load_si128((__m128i*)pSrcS);
            t0 = _mm_srli_epi16(t1, 1);
            t1 = _mm_and_si128(t1, const11);
            t4 = _mm_unpackhi_epi16(t0, t1);
            t0 = _mm_unpacklo_epi16(t0, t1);
            t1 = _mm_and_si128(t1, t2);
            t6 = _mm_unpackhi_epi16(t1, const0);
            t1 = _mm_unpacklo_epi16(t1, const0);
            t3 = _mm_srai_epi16(t2, 1);
            t5 = _mm_unpackhi_epi16(t2, t3);
            t2 = _mm_unpacklo_epi16(t2, t3);
            t4 = _mm_madd_epi16(t4, t5);
            t0 = _mm_madd_epi16(t0, t2);
            t2 = _mm_srl_epi32(t4, sFactor);
            t3 = _mm_srl_epi32(t0, sFactor);
            t4 = _mm_add_epi32(t4, constW);
            t0 = _mm_add_epi32(t0, constW);
            t2 = _mm_and_si128(t2, const01);
            t3 = _mm_and_si128(t3, const01);
            t6 = _mm_or_si128(t6, t2);
            t1 = _mm_or_si128(t1, t3);
            t4 = _mm_add_epi32(t4, t6);
            t0 = _mm_add_epi32(t0, t1);
            t4 = _mm_sra_epi32(t4, sFactor);
            t0 = _mm_sra_epi32(t0, sFactor);
            t0 = _mm_packs_epi32(t0, t4);
            _mm_store_si128((__m128i*)pDst, t0);
            pSrcU += 8;
            pSrcS += 8;
            pDst  += 8;
          } while( --tmp );
        } /* else */
      } /* if */
      else {
        if( (IPP_INT_PTR(pSrcS) & 15) != 0 ) {
          do {
            t1 = _mm_load_si128((__m128i*)pSrcU);
            t2 = _mm_loadu_si128((__m128i*)pSrcS);
            t0 = _mm_srli_epi16(t1, 1);
            t1 = _mm_and_si128(t1, const11);
            t4 = _mm_unpackhi_epi16(t0, t1);
            t0 = _mm_unpacklo_epi16(t0, t1);
            t1 = _mm_and_si128(t1, t2);
            t6 = _mm_unpackhi_epi16(t1, const0);
            t1 = _mm_unpacklo_epi16(t1, const0);
            t3 = _mm_srai_epi16(t2, 1);
            t5 = _mm_unpackhi_epi16(t2, t3);
            t2 = _mm_unpacklo_epi16(t2, t3);
            t4 = _mm_madd_epi16(t4, t5);
            t0 = _mm_madd_epi16(t0, t2);
            t2 = _mm_srl_epi32(t4, sFactor);
            t3 = _mm_srl_epi32(t0, sFactor);
            t4 = _mm_add_epi32(t4, constW);
            t0 = _mm_add_epi32(t0, constW);
            t2 = _mm_and_si128(t2, const01);
            t3 = _mm_and_si128(t3, const01);
            t6 = _mm_or_si128(t6, t2);
            t1 = _mm_or_si128(t1, t3);
            t4 = _mm_add_epi32(t4, t6);
            t0 = _mm_add_epi32(t0, t1);
            t4 = _mm_sra_epi32(t4, sFactor);
            t0 = _mm_sra_epi32(t0, sFactor);
            t0 = _mm_packs_epi32(t0, t4);
            _mm_store_si128((__m128i*)pDst, t0);
            pSrcU += 8;
            pSrcS += 8;
            pDst  += 8;
          } while( --tmp );
        } /* if */
        else {
          do {
            t1 = _mm_load_si128((__m128i*)pSrcU);
            t2 = _mm_load_si128((__m128i*)pSrcS);
            t0 = _mm_srli_epi16(t1, 1);
            t1 = _mm_and_si128(t1, const11);
            t4 = _mm_unpackhi_epi16(t0, t1);
            t0 = _mm_unpacklo_epi16(t0, t1);
            t1 = _mm_and_si128(t1, t2);
            t6 = _mm_unpackhi_epi16(t1, const0);
            t1 = _mm_unpacklo_epi16(t1, const0);
            t3 = _mm_srai_epi16(t2, 1);
            t5 = _mm_unpackhi_epi16(t2, t3);
            t2 = _mm_unpacklo_epi16(t2, t3);
            t4 = _mm_madd_epi16(t4, t5);
            t0 = _mm_madd_epi16(t0, t2);
            t2 = _mm_srl_epi32(t4, sFactor);
            t3 = _mm_srl_epi32(t0, sFactor);
            t4 = _mm_add_epi32(t4, constW);
            t0 = _mm_add_epi32(t0, constW);
            t2 = _mm_and_si128(t2, const01);
            t3 = _mm_and_si128(t3, const01);
            t6 = _mm_or_si128(t6, t2);
            t1 = _mm_or_si128(t1, t3);
            t4 = _mm_add_epi32(t4, t6);
            t0 = _mm_add_epi32(t0, t1);
            t4 = _mm_sra_epi32(t4, sFactor);
            t0 = _mm_sra_epi32(t0, sFactor);
            t0 = _mm_packs_epi32(t0, t4);
            _mm_store_si128((__m128i*)pDst, t0);
            pSrcU += 8;
            pSrcS += 8;
            pDst  += 8;
          } while( --tmp );
        } /* else */
      } /* else */
    } /* if */
    else {
      sFactor = _mm_cvtsi32_si128(scaleFactor);
      constW  = _mm_set1_epi32((int)(const1 >> 1));
      const0  = _mm_setzero_si128();
      const01 = _mm_set1_epi32(0x00000001);
      const11 = _mm_set1_epi32(0x00010001);
      tmp     = len >> 3;
      len     = len &  7;
      if( (IPP_INT_PTR(pSrcU) & 15) != 0 ) {
        if( (IPP_INT_PTR(pSrcS) & 15) != 0 ) {
          do {
            t1 = _mm_loadu_si128((__m128i*)pSrcU);
            t2 = _mm_loadu_si128((__m128i*)pSrcS);
            t0 = _mm_srli_epi16(t1, 1);
            t1 = _mm_and_si128(t1, const11);
            t4 = _mm_unpackhi_epi16(t0, t1);
            t0 = _mm_unpacklo_epi16(t0, t1);
            t1 = _mm_and_si128(t1, t2);
            t6 = _mm_unpackhi_epi16(t1, const0);
            t1 = _mm_unpacklo_epi16(t1, const0);
            t3 = _mm_srai_epi16(t2, 1);
            t5 = _mm_unpackhi_epi16(t2, t3);
            t2 = _mm_unpacklo_epi16(t2, t3);
            t4 = _mm_madd_epi16(t4, t5);
            t0 = _mm_madd_epi16(t0, t2);
            t2 = _mm_srl_epi32(t4, sFactor);
            t3 = _mm_srl_epi32(t0, sFactor);
            t4 = _mm_add_epi32(t4, constW);
            t0 = _mm_add_epi32(t0, constW);
            t2 = _mm_and_si128(t2, const01);
            t3 = _mm_and_si128(t3, const01);
            t6 = _mm_or_si128(t6, t2);
            t1 = _mm_or_si128(t1, t3);
            t4 = _mm_add_epi32(t4, t6);
            t0 = _mm_add_epi32(t0, t1);
            t4 = _mm_sra_epi32(t4, sFactor);
            t0 = _mm_sra_epi32(t0, sFactor);
            t0 = _mm_packs_epi32(t0, t4);
            _mm_storeu_si128((__m128i*)pDst, t0);
            pSrcU += 8;
            pSrcS += 8;
            pDst  += 8;
          } while( --tmp );
        } /* if */
        else {
          do {
            t1 = _mm_loadu_si128((__m128i*)pSrcU);
            t2 = _mm_load_si128((__m128i*)pSrcS);
            t0 = _mm_srli_epi16(t1, 1);
            t1 = _mm_and_si128(t1, const11);
            t4 = _mm_unpackhi_epi16(t0, t1);
            t0 = _mm_unpacklo_epi16(t0, t1);
            t1 = _mm_and_si128(t1, t2);
            t6 = _mm_unpackhi_epi16(t1, const0);
            t1 = _mm_unpacklo_epi16(t1, const0);
            t3 = _mm_srai_epi16(t2, 1);
            t5 = _mm_unpackhi_epi16(t2, t3);
            t2 = _mm_unpacklo_epi16(t2, t3);
            t4 = _mm_madd_epi16(t4, t5);
            t0 = _mm_madd_epi16(t0, t2);
            t2 = _mm_srl_epi32(t4, sFactor);
            t3 = _mm_srl_epi32(t0, sFactor);
            t4 = _mm_add_epi32(t4, constW);
            t0 = _mm_add_epi32(t0, constW);
            t2 = _mm_and_si128(t2, const01);
            t3 = _mm_and_si128(t3, const01);
            t6 = _mm_or_si128(t6, t2);
            t1 = _mm_or_si128(t1, t3);
            t4 = _mm_add_epi32(t4, t6);
            t0 = _mm_add_epi32(t0, t1);
            t4 = _mm_sra_epi32(t4, sFactor);
            t0 = _mm_sra_epi32(t0, sFactor);
            t0 = _mm_packs_epi32(t0, t4);
            _mm_storeu_si128((__m128i*)pDst, t0);
            pSrcU += 8;
            pSrcS += 8;
            pDst  += 8;
          } while( --tmp );
        } /* else */
      } /* if */
      else {
        if( (IPP_INT_PTR(pSrcS) & 15) != 0 ) {
          do {
            t1 = _mm_load_si128((__m128i*)pSrcU);
            t2 = _mm_loadu_si128((__m128i*)pSrcS);
            t0 = _mm_srli_epi16(t1, 1);
            t1 = _mm_and_si128(t1, const11);
            t4 = _mm_unpackhi_epi16(t0, t1);
            t0 = _mm_unpacklo_epi16(t0, t1);
            t1 = _mm_and_si128(t1, t2);
            t6 = _mm_unpackhi_epi16(t1, const0);
            t1 = _mm_unpacklo_epi16(t1, const0);
            t3 = _mm_srai_epi16(t2, 1);
            t5 = _mm_unpackhi_epi16(t2, t3);
            t2 = _mm_unpacklo_epi16(t2, t3);
            t4 = _mm_madd_epi16(t4, t5);
            t0 = _mm_madd_epi16(t0, t2);
            t2 = _mm_srl_epi32(t4, sFactor);
            t3 = _mm_srl_epi32(t0, sFactor);
            t4 = _mm_add_epi32(t4, constW);
            t0 = _mm_add_epi32(t0, constW);
            t2 = _mm_and_si128(t2, const01);
            t3 = _mm_and_si128(t3, const01);
            t6 = _mm_or_si128(t6, t2);
            t1 = _mm_or_si128(t1, t3);
            t4 = _mm_add_epi32(t4, t6);
            t0 = _mm_add_epi32(t0, t1);
            t4 = _mm_sra_epi32(t4, sFactor);
            t0 = _mm_sra_epi32(t0, sFactor);
            t0 = _mm_packs_epi32(t0, t4);
            _mm_storeu_si128((__m128i*)pDst, t0);
            pSrcU += 8;
            pSrcS += 8;
            pDst  += 8;
          } while( --tmp );
        } /* if */
        else {
          do {
            t1 = _mm_load_si128((__m128i*)pSrcU);
            t2 = _mm_load_si128((__m128i*)pSrcS);
            t0 = _mm_srli_epi16(t1, 1);
            t1 = _mm_and_si128(t1, const11);
            t4 = _mm_unpackhi_epi16(t0, t1);
            t0 = _mm_unpacklo_epi16(t0, t1);
            t1 = _mm_and_si128(t1, t2);
            t6 = _mm_unpackhi_epi16(t1, const0);
            t1 = _mm_unpacklo_epi16(t1, const0);
            t3 = _mm_srai_epi16(t2, 1);
            t5 = _mm_unpackhi_epi16(t2, t3);
            t2 = _mm_unpacklo_epi16(t2, t3);
            t4 = _mm_madd_epi16(t4, t5);
            t0 = _mm_madd_epi16(t0, t2);
            t2 = _mm_srl_epi32(t4, sFactor);
            t3 = _mm_srl_epi32(t0, sFactor);
            t4 = _mm_add_epi32(t4, constW);
            t0 = _mm_add_epi32(t0, constW);
            t2 = _mm_and_si128(t2, const01);
            t3 = _mm_and_si128(t3, const01);
            t6 = _mm_or_si128(t6, t2);
            t1 = _mm_or_si128(t1, t3);
            t4 = _mm_add_epi32(t4, t6);
            t0 = _mm_add_epi32(t0, t1);
            t4 = _mm_sra_epi32(t4, sFactor);
            t0 = _mm_sra_epi32(t0, sFactor);
            t0 = _mm_packs_epi32(t0, t4);
            _mm_storeu_si128((__m128i*)pDst, t0);
            pSrcU += 8;
            pSrcS += 8;
            pDst  += 8;
          } while( --tmp );
        } /* else */
      } /* else */
    } /* else */
  } /* if */
  for( ; len--; pSrcU++, pSrcS++, pDst++ ) {
    tmpVal = (Ipp32s)(((Ipp32s)*pSrcU) * *pSrcS);
    tmpVal = ((tmpVal >> 1) + (((tmpVal & 1) + const1 +
              (((tmpVal >> 1) >> scaleFactor) & 1)) >> 1)) >> scaleFactor;
    tmpVal = IPP_MIN(tmpVal, IPP_MAX_16S);
    *pDst  = (Ipp16s)IPP_MAX(tmpVal, IPP_MIN_16S);
  } /* for */
} /* mfxownsMul_16u16s_PosSfs */




#endif

#endif

#endif
