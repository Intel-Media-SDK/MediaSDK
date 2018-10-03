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
//  Contents:
//            mfxowniCopy_16s_C4P4R_cn
//            mfxowniCopy_16s_C3P3R_cn
//            mfxowniCopy_8u_C4P4R_cn
//            mfxowniCopy_8u_C3P3R_cn
//
*/
#include <emmintrin.h> // __m128i etc
#include <tmmintrin.h> // _mm_shuffle_epi8
#include "precomp.h"
#include "owni.h"
#include "ippcore.h"

#if ( _IPP >= _IPP_P8 ) || ( _IPP32E >= _IPP32E_Y8 )

#define BOUND_NT_8u_C3P3R   1048576 
#define BOUND_NT_8u_C4P4R   786432 
#define BOUND_NT_16s_C3P3R  524288 
#define BOUND_NT_16s_C4P4R  393216 

/*************************************  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 */
static __ALIGN16 Ipp8u msk_00[16] = {   0,  3,  6,  9, 12, 15,128,128,128,128,128,128,128,128,128,128 };
static __ALIGN16 Ipp8u msk_01[16] = { 128,128,128,128,128,128,  2,  5,  8, 11, 14,128,128,128,128,128 };
static __ALIGN16 Ipp8u msk_02[16] = { 128,128,128,128,128,128,128,128,128,128,128,  1,  4,  7, 10, 13 };
static __ALIGN16 Ipp8u msk_10[16] = {   1,  4,  7, 10, 13,128,128,128,128,128,128,128,128,128,128,128 };
static __ALIGN16 Ipp8u msk_11[16] = { 128,128,128,128,128,  0,  3,  6,  9, 12, 15,128,128,128,128,128 };
static __ALIGN16 Ipp8u msk_12[16] = { 128,128,128,128,128,128,128,128,128,128,128,  2,  5,  8, 11, 14 };
static __ALIGN16 Ipp8u msk_20[16] = {   2,  5,  8, 11, 14,128,128,128,128,128,128,128,128,128,128,128 };
static __ALIGN16 Ipp8u msk_21[16] = { 128,128,128,128,128,  1,  4,  7, 10, 13,128,128,128,128,128,128 };
static __ALIGN16 Ipp8u msk_22[16] = { 128,128,128,128,128,128,128,128,128,128,  0,  3,  6,  9, 12, 15 };
/*************************************  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 */
static __ALIGN16 Ipp8u m16_00[16] = {   0,  1,  6,  7, 12, 13,128,128,128,128,128,128,128,128,128,128 };
static __ALIGN16 Ipp8u m16_01[16] = { 128,128,128,128,128,128,  2,  3,  8,  9, 14, 15,128,128,128,128 };
static __ALIGN16 Ipp8u m16_02[16] = { 128,128,128,128,128,128,128,128,128,128,128,128,  4,  5, 10, 11 };
static __ALIGN16 Ipp8u m16_10[16] = {   2,  3,  8,  9, 14, 15,128,128,128,128,128,128,128,128,128,128 };
static __ALIGN16 Ipp8u m16_11[16] = { 128,128,128,128,128,128,  4,  5, 10, 11,128,128,128,128,128,128 };
static __ALIGN16 Ipp8u m16_12[16] = { 128,128,128,128,128,128,128,128,128,128,  0,  1,  6,  7, 12, 13 };
static __ALIGN16 Ipp8u m16_20[16] = {   4,  5, 10, 11,128,128,128,128,128,128,128,128,128,128,128,128 };
static __ALIGN16 Ipp8u m16_21[16] = { 128,128,128,128,  0,  1,  6,  7, 12, 13,128,128,128,128,128,128 };
static __ALIGN16 Ipp8u m16_22[16] = { 128,128,128,128,128,128,128,128,128,128,  2,  3,  8,  9, 14, 15 };

/***********************************************************************/
void mfxowniCopy_16s_C4P4R_cn( const Ipp16s* pSrc, int srcStep,
                            Ipp16s* const pDst[4], int dstStep,
                                IppiSize roiSize )
{
  __ALIGN16 __m128i v0, v1, v2, v3, v4, v5;
  Ipp16s *dst0 = pDst[0], *dst1 = pDst[1], *dst2 = pDst[2], *dst3 = pDst[3];
  int h, s, d, cache, nonTemporal = 0;
  Ipp64s width, width_;
  IppStatus ownStatus = ippStsNoErr;

    if( roiSize.width < 8 ) {
      for( h = 0; h < roiSize.height; h++ ) {
        for( s = 0, d = 0; d < roiSize.width; s += 4, d++ ) {
          dst0[d] = pSrc[s]; 
          dst1[d] = pSrc[s+1]; 
          dst2[d] = pSrc[s+2]; 
          dst3[d] = pSrc[s+3]; 
        }
        pSrc = (Ipp16s*)((Ipp8u*)pSrc + srcStep);
        dst0 = (Ipp16s*)((Ipp8u*)dst0 + dstStep);
        dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
        dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
        dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
      }
      return;
    }

    if( (dstStep*4 == srcStep) && (dstStep == roiSize.width*2) ) {
      width = roiSize.width*roiSize.height;
      roiSize.height = 1;  
      if( width > BOUND_NT_16s_C4P4R ) {
        ownStatus = mfxGetMaxCacheSizeB( &cache );
        if( ownStatus == ippStsNoErr  ) {
          if( width*16 > cache ) nonTemporal = 1;
        }
      }
    } else {
      width = roiSize.width;
    }

    width_ = width - (width & 0x7);

    if( (IPP_UINT_PTR(pSrc)&0xf) || (srcStep&0xf) || (dstStep&0xf) ||
        (IPP_UINT_PTR(dst0)&0xf) || (IPP_UINT_PTR(dst1)&0xf) || (IPP_UINT_PTR(dst2)&0xf) || (IPP_UINT_PTR(dst3)&0xf) )
      {
      for( h = 0; h < roiSize.height; h++ ) {
        for( s = 0, d = 0; d < width_; s += 32, d += 8 ) {
          v0 = _mm_loadu_si128( (__m128i*)(pSrc + s) );
          v1 = _mm_loadu_si128( (__m128i*)(pSrc + s + 8) );
          v2 = _mm_loadu_si128( (__m128i*)(pSrc + s + 16) );
          v3 = _mm_loadu_si128( (__m128i*)(pSrc + s + 24) );
          v4 = _mm_unpacklo_epi16( v0, v1 );
          v0 = _mm_unpackhi_epi16( v0, v1 );
          v5 = _mm_unpacklo_epi16( v2, v3 );
          v2 = _mm_unpackhi_epi16( v2, v3 );

          v1 = _mm_unpacklo_epi16( v4, v0 );
          v4 = _mm_unpackhi_epi16( v4, v0 );
          v3 = _mm_unpacklo_epi16( v5, v2 );
          v5 = _mm_unpackhi_epi16( v5, v2 );

          v0 = _mm_unpacklo_epi64( v1, v3 );
          v1 = _mm_unpackhi_epi64( v1, v3 );
          v2 = _mm_unpacklo_epi64( v4, v5 );
          v3 = _mm_unpackhi_epi64( v4, v5 );
          _mm_storeu_si128( (__m128i*)(dst0 + d), v0 );
          _mm_storeu_si128( (__m128i*)(dst1 + d), v1 );
          _mm_storeu_si128( (__m128i*)(dst2 + d), v2 );
          _mm_storeu_si128( (__m128i*)(dst3 + d), v3 );
        }
        for( ; d < width; s += 4, d++ ) {
          dst0[d] = pSrc[s]; 
          dst1[d] = pSrc[s+1]; 
          dst2[d] = pSrc[s+2]; 
          dst3[d] = pSrc[s+3]; 
        }
        pSrc = (Ipp16s*)((Ipp8u*)pSrc + srcStep);
        dst0 = (Ipp16s*)((Ipp8u*)dst0 + dstStep);
        dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
        dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
        dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
      }
    } else {
      if( nonTemporal ) {
        for( h = 0; h < roiSize.height; h++ ) {
          for( s = 0, d = 0; d < width_; s += 32, d += 8 ) {
            v0 = _mm_load_si128( (__m128i*)(pSrc + s) );
            v1 = _mm_load_si128( (__m128i*)(pSrc + s + 8) );
            v2 = _mm_load_si128( (__m128i*)(pSrc + s + 16) );
            v3 = _mm_load_si128( (__m128i*)(pSrc + s + 24) );
            v4 = _mm_unpacklo_epi16( v0, v1 );
            v0 = _mm_unpackhi_epi16( v0, v1 );
            v5 = _mm_unpacklo_epi16( v2, v3 );
            v2 = _mm_unpackhi_epi16( v2, v3 );

            v1 = _mm_unpacklo_epi16( v4, v0 );
            v4 = _mm_unpackhi_epi16( v4, v0 );
            v3 = _mm_unpacklo_epi16( v5, v2 );
            v5 = _mm_unpackhi_epi16( v5, v2 );

            v0 = _mm_unpacklo_epi64( v1, v3 );
            v1 = _mm_unpackhi_epi64( v1, v3 );
            v2 = _mm_unpacklo_epi64( v4, v5 );
            v3 = _mm_unpackhi_epi64( v4, v5 );
            _mm_stream_si128( (__m128i*)(dst0 + d), v0 );
            _mm_stream_si128( (__m128i*)(dst1 + d), v1 );
            _mm_stream_si128( (__m128i*)(dst2 + d), v2 );
            _mm_stream_si128( (__m128i*)(dst3 + d), v3 );
          }
          for( ; d < width; s += 4, d++ ) {
            dst0[d] = pSrc[s]; 
            dst1[d] = pSrc[s+1]; 
            dst2[d] = pSrc[s+2]; 
            dst3[d] = pSrc[s+3]; 
          }
          pSrc = (Ipp16s*)((Ipp8u*)pSrc + srcStep);
          dst0 = (Ipp16s*)((Ipp8u*)dst0 + dstStep);
          dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
          dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
          dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
        }
        _mm_sfence();
      } else {
        for( h = 0; h < roiSize.height; h++ ) {
          for( s = 0, d = 0; d < width_; s += 32, d += 8 ) {
            v0 = _mm_load_si128( (__m128i*)(pSrc + s) );
            v1 = _mm_load_si128( (__m128i*)(pSrc + s + 8) );
            v2 = _mm_load_si128( (__m128i*)(pSrc + s + 16) );
            v3 = _mm_load_si128( (__m128i*)(pSrc + s + 24) );
            v4 = _mm_unpacklo_epi16( v0, v1 );
            v0 = _mm_unpackhi_epi16( v0, v1 );
            v5 = _mm_unpacklo_epi16( v2, v3 );
            v2 = _mm_unpackhi_epi16( v2, v3 );

            v1 = _mm_unpacklo_epi16( v4, v0 );
            v4 = _mm_unpackhi_epi16( v4, v0 );
            v3 = _mm_unpacklo_epi16( v5, v2 );
            v5 = _mm_unpackhi_epi16( v5, v2 );

            v0 = _mm_unpacklo_epi64( v1, v3 );
            v1 = _mm_unpackhi_epi64( v1, v3 );
            v2 = _mm_unpacklo_epi64( v4, v5 );
            v3 = _mm_unpackhi_epi64( v4, v5 );
            _mm_store_si128( (__m128i*)(dst0 + d), v0 );
            _mm_store_si128( (__m128i*)(dst1 + d), v1 );
            _mm_store_si128( (__m128i*)(dst2 + d), v2 );
            _mm_store_si128( (__m128i*)(dst3 + d), v3 );
          }
          for( ; d < width; s += 4, d++ ) {
            dst0[d] = pSrc[s]; 
            dst1[d] = pSrc[s+1]; 
            dst2[d] = pSrc[s+2]; 
            dst3[d] = pSrc[s+3]; 
          }
          pSrc = (Ipp16s*)((Ipp8u*)pSrc + srcStep);
          dst0 = (Ipp16s*)((Ipp8u*)dst0 + dstStep);
          dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
          dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
          dst3 = (Ipp16s*)((Ipp8u*)dst3 + dstStep);
        }
      }
    }

  return;
}

/***********************************************************************/
void mfxowniCopy_16s_C3P3R_cn( const Ipp16s* pSrc, int srcStep,
                            Ipp16s* const pDst[3], int dstStep,
                                IppiSize roiSize )
{
  __ALIGN16 __m128i s0, s1, s2;
  __ALIGN16 __m128i d0, d1, d2, t0, t1, t2;
  Ipp16s *dst0 = pDst[0], *dst1 = pDst[1], *dst2 = pDst[2];
  int h, s, d, cache, nonTemporal = 0;
  Ipp64s width, width_;
  IppStatus ownStatus = ippStsNoErr;

    if( roiSize.width < 8 ) {
      for( h = 0; h < roiSize.height; h++ ) {
        for( s = 0, d = 0; d < roiSize.width; s += 3, d++ ) {
          dst0[d] = pSrc[s]; 
          dst1[d] = pSrc[s+1]; 
          dst2[d] = pSrc[s+2]; 
        }
        pSrc = (Ipp16s*)((Ipp8u*)pSrc + srcStep);
        dst0 = (Ipp16s*)((Ipp8u*)dst0 + dstStep);
        dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
        dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
      }
      return;
    }

    if( (dstStep*3 == srcStep) && (dstStep == roiSize.width*2) ) {
      width = roiSize.width*roiSize.height;
      roiSize.height = 1;  
      if( width > BOUND_NT_16s_C3P3R ) {
        ownStatus = mfxGetMaxCacheSizeB( &cache );
        if( ownStatus == ippStsNoErr  ) {
          if( width*12 > cache ) nonTemporal = 1;
        }
      }
    } else {
      width = roiSize.width;
    }

    width_ = width - (width & 0x7);

    if( (IPP_UINT_PTR(pSrc)&0xf) || (srcStep&0xf) || 
        (IPP_UINT_PTR(dst0)&0xf) || (IPP_UINT_PTR(dst1)&0xf) || (IPP_UINT_PTR(dst2)&0xf) || (dstStep&0xf) )
      {
      for( h = 0; h < roiSize.height; h++ ) {
        for( s = 0, d = 0; d < width_; s += 24, d += 8 ) {
          s0 = _mm_loadu_si128( (__m128i*)(pSrc + s) );
          s1 = _mm_loadu_si128( (__m128i*)(pSrc + s + 8) );
          s2 = _mm_loadu_si128( (__m128i*)(pSrc + s + 16) );
          d0 = _mm_shuffle_epi8( s0, *(__m128i*)m16_00 );
          d1 = _mm_shuffle_epi8( s0, *(__m128i*)m16_10 );
          d2 = _mm_shuffle_epi8( s0, *(__m128i*)m16_20 );
          t0 = _mm_shuffle_epi8( s1, *(__m128i*)m16_01 );
          t1 = _mm_shuffle_epi8( s1, *(__m128i*)m16_11 );
          t2 = _mm_shuffle_epi8( s1, *(__m128i*)m16_21 );
          d0 = _mm_or_si128( d0, t0);
          d1 = _mm_or_si128( d1, t1);
          d2 = _mm_or_si128( d2, t2);
          t0 = _mm_shuffle_epi8( s2, *(__m128i*)m16_02 );
          t1 = _mm_shuffle_epi8( s2, *(__m128i*)m16_12 );
          t2 = _mm_shuffle_epi8( s2, *(__m128i*)m16_22 );
          d0 = _mm_or_si128( d0, t0);
          d1 = _mm_or_si128( d1, t1);
          d2 = _mm_or_si128( d2, t2);
          _mm_storeu_si128( (__m128i*)(dst0 + d), d0 );
          _mm_storeu_si128( (__m128i*)(dst1 + d), d1 );
          _mm_storeu_si128( (__m128i*)(dst2 + d), d2 );
        }
        for( ; d < width; s += 3, d++ ) {
          dst0[d] = pSrc[s]; 
          dst1[d] = pSrc[s+1]; 
          dst2[d] = pSrc[s+2]; 
        }
        pSrc = (Ipp16s*)((Ipp8u*)pSrc + srcStep);
        dst0 = (Ipp16s*)((Ipp8u*)dst0 + dstStep);
        dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
        dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
      }
    } else {
      if( nonTemporal ) {
        for( h = 0; h < roiSize.height; h++ ) {
          for( s = 0, d = 0; d < width_; s += 24, d += 8 ) {
            s0 = _mm_load_si128( (__m128i*)(pSrc + s) );
            s1 = _mm_load_si128( (__m128i*)(pSrc + s + 8) );
            s2 = _mm_load_si128( (__m128i*)(pSrc + s + 16) );
            d0 = _mm_shuffle_epi8( s0, *(__m128i*)m16_00 );
            d1 = _mm_shuffle_epi8( s0, *(__m128i*)m16_10 );
            d2 = _mm_shuffle_epi8( s0, *(__m128i*)m16_20 );
            t0 = _mm_shuffle_epi8( s1, *(__m128i*)m16_01 );
            t1 = _mm_shuffle_epi8( s1, *(__m128i*)m16_11 );
            t2 = _mm_shuffle_epi8( s1, *(__m128i*)m16_21 );
            d0 = _mm_or_si128( d0, t0);
            d1 = _mm_or_si128( d1, t1);
            d2 = _mm_or_si128( d2, t2);
            t0 = _mm_shuffle_epi8( s2, *(__m128i*)m16_02 );
            t1 = _mm_shuffle_epi8( s2, *(__m128i*)m16_12 );
            t2 = _mm_shuffle_epi8( s2, *(__m128i*)m16_22 );
            d0 = _mm_or_si128( d0, t0);
            d1 = _mm_or_si128( d1, t1);
            d2 = _mm_or_si128( d2, t2);
            _mm_stream_si128( (__m128i*)(dst0 + d), d0 );
            _mm_stream_si128( (__m128i*)(dst1 + d), d1 );
            _mm_stream_si128( (__m128i*)(dst2 + d), d2 );
          }
          for( ; d < width; s += 3, d++ ) {
            dst0[d] = pSrc[s]; 
            dst1[d] = pSrc[s+1]; 
            dst2[d] = pSrc[s+2]; 
          }
          pSrc = (Ipp16s*)((Ipp8u*)pSrc + srcStep);
          dst0 = (Ipp16s*)((Ipp8u*)dst0 + dstStep);
          dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
          dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
        }
        _mm_sfence();
      } else {
        for( h = 0; h < roiSize.height; h++ ) {
          for( s = 0, d = 0; d < width_; s += 24, d += 8 ) {
            s0 = _mm_load_si128( (__m128i*)(pSrc + s) );
            s1 = _mm_load_si128( (__m128i*)(pSrc + s + 8) );
            s2 = _mm_load_si128( (__m128i*)(pSrc + s + 16) );
            d0 = _mm_shuffle_epi8( s0, *(__m128i*)m16_00 );
            d1 = _mm_shuffle_epi8( s0, *(__m128i*)m16_10 );
            d2 = _mm_shuffle_epi8( s0, *(__m128i*)m16_20 );
            t0 = _mm_shuffle_epi8( s1, *(__m128i*)m16_01 );
            t1 = _mm_shuffle_epi8( s1, *(__m128i*)m16_11 );
            t2 = _mm_shuffle_epi8( s1, *(__m128i*)m16_21 );
            d0 = _mm_or_si128( d0, t0);
            d1 = _mm_or_si128( d1, t1);
            d2 = _mm_or_si128( d2, t2);
            t0 = _mm_shuffle_epi8( s2, *(__m128i*)m16_02 );
            t1 = _mm_shuffle_epi8( s2, *(__m128i*)m16_12 );
            t2 = _mm_shuffle_epi8( s2, *(__m128i*)m16_22 );
            d0 = _mm_or_si128( d0, t0);
            d1 = _mm_or_si128( d1, t1);
            d2 = _mm_or_si128( d2, t2);
            _mm_store_si128( (__m128i*)(dst0 + d), d0 );
            _mm_store_si128( (__m128i*)(dst1 + d), d1 );
            _mm_store_si128( (__m128i*)(dst2 + d), d2 );
          }
          for( ; d < width; s += 3, d++ ) {
            dst0[d] = pSrc[s]; 
            dst1[d] = pSrc[s+1]; 
            dst2[d] = pSrc[s+2]; 
          }
          pSrc = (Ipp16s*)((Ipp8u*)pSrc + srcStep);
          dst0 = (Ipp16s*)((Ipp8u*)dst0 + dstStep);
          dst1 = (Ipp16s*)((Ipp8u*)dst1 + dstStep);
          dst2 = (Ipp16s*)((Ipp8u*)dst2 + dstStep);
        }
      }
    }

  return;
}

/***********************************************************************/
void mfxowniCopy_8u_C4P4R_cn( const Ipp8u* pSrc, int srcStep,
                           Ipp8u* const pDst[4], int dstStep,
                               IppiSize roiSize )
{
  __ALIGN16 __m128i v0, v1, v2, v3, v4, v5;
  Ipp8u *dst0 = pDst[0], *dst1 = pDst[1], *dst2 = pDst[2], *dst3 = pDst[3];
  int h, s, d, cache, nonTemporal = 0;
  Ipp64s width, width_;
  IppStatus ownStatus = ippStsNoErr;

    if( roiSize.width < 16 ) {
      for( h = 0; h < roiSize.height; h++ ) {
        for( s = 0, d = 0; d < roiSize.width; s += 4, d++ ) {
          dst0[d] = pSrc[s]; 
          dst1[d] = pSrc[s+1]; 
          dst2[d] = pSrc[s+2]; 
          dst3[d] = pSrc[s+3]; 
        }
        pSrc += srcStep;
        dst0 += dstStep;
        dst1 += dstStep;
        dst2 += dstStep;
        dst3 += dstStep;
      }
      return;
    }

    if( (dstStep*4 == srcStep) && (dstStep == roiSize.width) ) {
      width = roiSize.width*roiSize.height;
      roiSize.height = 1;  
      if( width > BOUND_NT_8u_C4P4R ) {
        ownStatus = mfxGetMaxCacheSizeB( &cache );
        if( ownStatus == ippStsNoErr  ) {
          if( width*8 > cache ) nonTemporal = 1;
        }
      }
    } else {
      width = roiSize.width;
    }

    width_ = width - (width & 0xf);

    if( (IPP_UINT_PTR(pSrc)&0xf) || (srcStep&0xf) || (dstStep&0xf) ||
        (IPP_UINT_PTR(dst0)&0xf) || (IPP_UINT_PTR(dst1)&0xf) || (IPP_UINT_PTR(dst2)&0xf) || (IPP_UINT_PTR(dst3)&0xf) )
      {
      for( h = 0; h < roiSize.height; h++ ) {
        for( s = 0, d = 0; d < width_; s += 64, d += 16 ) {
          v0 = _mm_loadu_si128( (__m128i*)(pSrc + s) );
          v1 = _mm_loadu_si128( (__m128i*)(pSrc + s + 16) );
          v2 = _mm_loadu_si128( (__m128i*)(pSrc + s + 32) );
          v3 = _mm_loadu_si128( (__m128i*)(pSrc + s + 48) );
          v4 = _mm_unpacklo_epi8( v0, v1 );
          v0 = _mm_unpackhi_epi8( v0, v1 );
          v5 = _mm_unpacklo_epi8( v2, v3 );
          v2 = _mm_unpackhi_epi8( v2, v3 );

          v1 = _mm_unpacklo_epi8( v4, v0 );
          v4 = _mm_unpackhi_epi8( v4, v0 );
          v3 = _mm_unpacklo_epi8( v5, v2 );
          v5 = _mm_unpackhi_epi8( v5, v2 );

          v0 = _mm_unpacklo_epi8( v1, v4 );
          v1 = _mm_unpackhi_epi8( v1, v4 );
          v2 = _mm_unpacklo_epi8( v3, v5 );
          v3 = _mm_unpackhi_epi8( v3, v5 );

          v4 = _mm_unpacklo_epi64( v0, v2 );
          v0 = _mm_unpackhi_epi64( v0, v2 );
          v5 = _mm_unpacklo_epi64( v1, v3 );
          v1 = _mm_unpackhi_epi64( v1, v3 );
          _mm_storeu_si128( (__m128i*)(dst0 + d), v4 );
          _mm_storeu_si128( (__m128i*)(dst1 + d), v0 );
          _mm_storeu_si128( (__m128i*)(dst2 + d), v5 );
          _mm_storeu_si128( (__m128i*)(dst3 + d), v1 );
        }
        for( ; d < width; s += 4, d++ ) {
          dst0[d] = pSrc[s]; 
          dst1[d] = pSrc[s+1]; 
          dst2[d] = pSrc[s+2]; 
          dst3[d] = pSrc[s+3]; 
        }
        pSrc += srcStep;
        dst0 += dstStep;
        dst1 += dstStep;
        dst2 += dstStep;
        dst3 += dstStep;
      }
    } else {
      if( nonTemporal ) {
        for( h = 0; h < roiSize.height; h++ ) {
          for( s = 0, d = 0; d < width_; s += 64, d += 16 ) {
            v0 = _mm_load_si128( (__m128i*)(pSrc + s) );
            v1 = _mm_load_si128( (__m128i*)(pSrc + s + 16) );
            v2 = _mm_load_si128( (__m128i*)(pSrc + s + 32) );
            v3 = _mm_load_si128( (__m128i*)(pSrc + s + 48) );

            v4 = _mm_unpacklo_epi8( v0, v1 );
            v0 = _mm_unpackhi_epi8( v0, v1 );
            v5 = _mm_unpacklo_epi8( v2, v3 );
            v2 = _mm_unpackhi_epi8( v2, v3 );

            v1 = _mm_unpacklo_epi8( v4, v0 );
            v4 = _mm_unpackhi_epi8( v4, v0 );
            v3 = _mm_unpacklo_epi8( v5, v2 );
            v5 = _mm_unpackhi_epi8( v5, v2 );

            v0 = _mm_unpacklo_epi8( v1, v4 );
            v1 = _mm_unpackhi_epi8( v1, v4 );
            v2 = _mm_unpacklo_epi8( v3, v5 );
            v3 = _mm_unpackhi_epi8( v3, v5 );

            v4 = _mm_unpacklo_epi64( v0, v2 );
            v0 = _mm_unpackhi_epi64( v0, v2 );
            v5 = _mm_unpacklo_epi64( v1, v3 );
            v1 = _mm_unpackhi_epi64( v1, v3 );
            _mm_stream_si128( (__m128i*)(dst0 + d), v4 );
            _mm_stream_si128( (__m128i*)(dst1 + d), v0 );
            _mm_stream_si128( (__m128i*)(dst2 + d), v5 );
            _mm_stream_si128( (__m128i*)(dst3 + d), v1 );
          }
          for( ; d < width; s += 4, d++ ) {
            dst0[d] = pSrc[s]; 
            dst1[d] = pSrc[s+1]; 
            dst2[d] = pSrc[s+2]; 
            dst3[d] = pSrc[s+3]; 
          }
          pSrc += srcStep;
          dst0 += dstStep;
          dst1 += dstStep;
          dst2 += dstStep;
          dst3 += dstStep;
        }
        _mm_sfence();
      } else {
        for( h = 0; h < roiSize.height; h++ ) {
          for( s = 0, d = 0; d < width_; s += 64, d += 16 ) {
            v0 = _mm_load_si128( (__m128i*)(pSrc + s) );
            v1 = _mm_load_si128( (__m128i*)(pSrc + s + 16) );
            v2 = _mm_load_si128( (__m128i*)(pSrc + s + 32) );
            v3 = _mm_load_si128( (__m128i*)(pSrc + s + 48) );

            v4 = _mm_unpacklo_epi8( v0, v1 );
            v0 = _mm_unpackhi_epi8( v0, v1 );
            v5 = _mm_unpacklo_epi8( v2, v3 );
            v2 = _mm_unpackhi_epi8( v2, v3 );

            v1 = _mm_unpacklo_epi8( v4, v0 );
            v4 = _mm_unpackhi_epi8( v4, v0 );
            v3 = _mm_unpacklo_epi8( v5, v2 );
            v5 = _mm_unpackhi_epi8( v5, v2 );

            v0 = _mm_unpacklo_epi8( v1, v4 );
            v1 = _mm_unpackhi_epi8( v1, v4 );
            v2 = _mm_unpacklo_epi8( v3, v5 );
            v3 = _mm_unpackhi_epi8( v3, v5 );

            v4 = _mm_unpacklo_epi64( v0, v2 );
            v0 = _mm_unpackhi_epi64( v0, v2 );
            v5 = _mm_unpacklo_epi64( v1, v3 );
            v1 = _mm_unpackhi_epi64( v1, v3 );
            _mm_store_si128( (__m128i*)(dst0 + d), v4 );
            _mm_store_si128( (__m128i*)(dst1 + d), v0 );
            _mm_store_si128( (__m128i*)(dst2 + d), v5 );
            _mm_store_si128( (__m128i*)(dst3 + d), v1 );
          }
          for( ; d < width; s += 4, d++ ) {
            dst0[d] = pSrc[s]; 
            dst1[d] = pSrc[s+1]; 
            dst2[d] = pSrc[s+2]; 
            dst3[d] = pSrc[s+3]; 
          }
          pSrc += srcStep;
          dst0 += dstStep;
          dst1 += dstStep;
          dst2 += dstStep;
          dst3 += dstStep;
        }
      }
    }

  return;
}

/***********************************************************************/
void mfxowniCopy_8u_C3P3R_cn( const Ipp8u* pSrc, int srcStep,
                           Ipp8u* const pDst[3], int dstStep,
                               IppiSize roiSize )
{
  __ALIGN16 __m128i s0, s1, s2;
  __ALIGN16 __m128i d0, d1, d2, t0, t1, t2;
  Ipp8u *dst0 = pDst[0], *dst1 = pDst[1], *dst2 = pDst[2];
  int h, s, d, cache, nonTemporal = 0;
  Ipp64s width, width_;
  IppStatus ownStatus = ippStsNoErr;

    if( roiSize.width < 16 ) {
      for( h = 0; h < roiSize.height; h++ ) {
        for( s = 0, d = 0; d < roiSize.width; s += 3, d++ ) {
          dst0[d] = pSrc[s]; 
          dst1[d] = pSrc[s+1]; 
          dst2[d] = pSrc[s+2]; 
        }
        pSrc += srcStep;
        dst0 += dstStep;
        dst1 += dstStep;
        dst2 += dstStep;
      }
      return;
    }

    if( (dstStep*3 == srcStep) && (dstStep == roiSize.width) ) {
      width = roiSize.width*roiSize.height;
      roiSize.height = 1;  
      if( width > BOUND_NT_8u_C3P3R ) {
        ownStatus = mfxGetMaxCacheSizeB( &cache );
        if( ownStatus == ippStsNoErr  ) {
          if( width*6 > cache ) nonTemporal = 1;
        }
      }
    } else {
      width = roiSize.width;
    }

    width_ = width - (width & 0xf);

    if( (IPP_UINT_PTR(pSrc)&0xf) || (srcStep&0xf) || 
        (IPP_UINT_PTR(dst0)&0xf) || (IPP_UINT_PTR(dst1)&0xf) || (IPP_UINT_PTR(dst2)&0xf) || (dstStep&0xf) )
      {
      for( h = 0; h < roiSize.height; h++ ) {
        for( s = 0, d = 0; d < width_; s += 48, d += 16 ) {
          s0 = _mm_loadu_si128( (__m128i*)(pSrc + s) );
          s1 = _mm_loadu_si128( (__m128i*)(pSrc + s + 16) );
          s2 = _mm_loadu_si128( (__m128i*)(pSrc + s + 32) );
          d0 = _mm_shuffle_epi8( s0, *(__m128i*)msk_00 );
          d1 = _mm_shuffle_epi8( s0, *(__m128i*)msk_10 );
          d2 = _mm_shuffle_epi8( s0, *(__m128i*)msk_20 );
          t0 = _mm_shuffle_epi8( s1, *(__m128i*)msk_01 );
          t1 = _mm_shuffle_epi8( s1, *(__m128i*)msk_11 );
          t2 = _mm_shuffle_epi8( s1, *(__m128i*)msk_21 );
          d0 = _mm_or_si128( d0, t0);
          d1 = _mm_or_si128( d1, t1);
          d2 = _mm_or_si128( d2, t2);
          t0 = _mm_shuffle_epi8( s2, *(__m128i*)msk_02 );
          t1 = _mm_shuffle_epi8( s2, *(__m128i*)msk_12 );
          t2 = _mm_shuffle_epi8( s2, *(__m128i*)msk_22 );
          d0 = _mm_or_si128( d0, t0);
          d1 = _mm_or_si128( d1, t1);
          d2 = _mm_or_si128( d2, t2);
          _mm_storeu_si128( (__m128i*)(dst0 + d), d0 );
          _mm_storeu_si128( (__m128i*)(dst1 + d), d1 );
          _mm_storeu_si128( (__m128i*)(dst2 + d), d2 );
        }
        for( ; d < width; s += 3, d++ ) {
          dst0[d] = pSrc[s]; 
          dst1[d] = pSrc[s+1]; 
          dst2[d] = pSrc[s+2]; 
        }
        pSrc += srcStep;
        dst0 += dstStep;
        dst1 += dstStep;
        dst2 += dstStep;
      }
    } else {
      if( nonTemporal ) {
        for( h = 0; h < roiSize.height; h++ ) {
          for( s = 0, d = 0; d < width_; s += 48, d += 16 ) {
            s0 = _mm_load_si128( (__m128i*)(pSrc + s) );
            s1 = _mm_load_si128( (__m128i*)(pSrc + s + 16) );
            s2 = _mm_load_si128( (__m128i*)(pSrc + s + 32) );
            d0 = _mm_shuffle_epi8( s0, *(__m128i*)msk_00 );
            d1 = _mm_shuffle_epi8( s0, *(__m128i*)msk_10 );
            d2 = _mm_shuffle_epi8( s0, *(__m128i*)msk_20 );
            t0 = _mm_shuffle_epi8( s1, *(__m128i*)msk_01 );
            t1 = _mm_shuffle_epi8( s1, *(__m128i*)msk_11 );
            t2 = _mm_shuffle_epi8( s1, *(__m128i*)msk_21 );
            d0 = _mm_or_si128( d0, t0);
            d1 = _mm_or_si128( d1, t1);
            d2 = _mm_or_si128( d2, t2);
            t0 = _mm_shuffle_epi8( s2, *(__m128i*)msk_02 );
            t1 = _mm_shuffle_epi8( s2, *(__m128i*)msk_12 );
            t2 = _mm_shuffle_epi8( s2, *(__m128i*)msk_22 );
            d0 = _mm_or_si128( d0, t0);
            d1 = _mm_or_si128( d1, t1);
            d2 = _mm_or_si128( d2, t2);
            _mm_stream_si128( (__m128i*)(dst0 + d), d0 );
            _mm_stream_si128( (__m128i*)(dst1 + d), d1 );
            _mm_stream_si128( (__m128i*)(dst2 + d), d2 );
          }
          for( ; d < width; s += 3, d++ ) {
            dst0[d] = pSrc[s]; 
            dst1[d] = pSrc[s+1]; 
            dst2[d] = pSrc[s+2]; 
          }
          pSrc += srcStep;
          dst0 += dstStep;
          dst1 += dstStep;
          dst2 += dstStep;
        }
        _mm_sfence();
      } else {
        for( h = 0; h < roiSize.height; h++ ) {
          for( s = 0, d = 0; d < width_; s += 48, d += 16 ) {
            s0 = _mm_load_si128( (__m128i*)(pSrc + s) );
            s1 = _mm_load_si128( (__m128i*)(pSrc + s + 16) );
            s2 = _mm_load_si128( (__m128i*)(pSrc + s + 32) );
            d0 = _mm_shuffle_epi8( s0, *(__m128i*)msk_00 );
            d1 = _mm_shuffle_epi8( s0, *(__m128i*)msk_10 );
            d2 = _mm_shuffle_epi8( s0, *(__m128i*)msk_20 );
            t0 = _mm_shuffle_epi8( s1, *(__m128i*)msk_01 );
            t1 = _mm_shuffle_epi8( s1, *(__m128i*)msk_11 );
            t2 = _mm_shuffle_epi8( s1, *(__m128i*)msk_21 );
            d0 = _mm_or_si128( d0, t0);
            d1 = _mm_or_si128( d1, t1);
            d2 = _mm_or_si128( d2, t2);
            t0 = _mm_shuffle_epi8( s2, *(__m128i*)msk_02 );
            t1 = _mm_shuffle_epi8( s2, *(__m128i*)msk_12 );
            t2 = _mm_shuffle_epi8( s2, *(__m128i*)msk_22 );
            d0 = _mm_or_si128( d0, t0);
            d1 = _mm_or_si128( d1, t1);
            d2 = _mm_or_si128( d2, t2);
            _mm_store_si128( (__m128i*)(dst0 + d), d0 );
            _mm_store_si128( (__m128i*)(dst1 + d), d1 );
            _mm_store_si128( (__m128i*)(dst2 + d), d2 );
          }
          for( ; d < width; s += 3, d++ ) {
            dst0[d] = pSrc[s]; 
            dst1[d] = pSrc[s+1]; 
            dst2[d] = pSrc[s+2]; 
          }
          pSrc += srcStep;
          dst0 += dstStep;
          dst1 += dstStep;
          dst2 += dstStep;
        }
      }
    }

  return;
}

#endif
