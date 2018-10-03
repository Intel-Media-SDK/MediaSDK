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
//    DCT+quantization+level shift+range convert functions (Inverse transform)
//
//  Contents:
//    mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R
//
*/
#include <emmintrin.h> // __m128i etc

#include "precomp.h"
#include "ownj.h"

//#ifndef __PS_ANARITH_H__
//#include "ps_anarith.h"
//#endif
//#ifndef __PJDECLS_H__
//#include "pjdecls.h"
//#endif
//#ifndef __PJQUANT_H__
//#include "pjquant.h"
//#endif

#if !((_IPP>=_IPP_H9)||(_IPP32E>=_IPP32E_L9))
#if ((_IPP>=_IPP_V8)||(_IPP32E>=_IPP32E_U8))

#define XMMCONST           static const _Alignas(16)

#define SH_2020            _MM_SHUFFLE(2,0,2,0)
#define SH_3131            _MM_SHUFFLE(3,1,3,1)
#define SH_1032            _MM_SHUFFLE(1,0,3,2)
#define SH_0123            _MM_SHUFFLE(0,1,2,3)
#define SH_0000            _MM_SHUFFLE(0,0,0,0)

#define PADDSW(a, b)       _mm_adds_epi16( a, b )
#define PSUBSW(a, b)       _mm_subs_epi16( a, b )
#define PADDW(a, b)        _mm_add_epi16( a, b )
#define PADDD(a, b)        _mm_add_epi32( a, b )
#define PSUBD(a, b)        _mm_sub_epi32( a, b )
#define PMULHW(a, b)       _mm_mulhi_epi16( a, b )
#define PMADDWD(a, b)      _mm_madd_epi16( a, b )
#define POR(a, b)          _mm_or_si128( a, b )
#define PMAXUB(a, b)       _mm_max_epu8( a, b )
#define PMINUB(a, b)       _mm_min_epu8( a, b )
#define PSLLW(a, c)        _mm_slli_epi16( a, c )
#define PSRAW(a, c)        _mm_srai_epi16( a, c )
#define PSRAD(a, c)        _mm_srai_epi32( a, c )
#define PACKUSWB(a, b)     _mm_packus_epi16( a, b )
#define PACKSSDW(a, b)     _mm_packs_epi32( a, b )
#define PSHUFLW(a, m)      _mm_shufflelo_epi16( a, m )
#define PSHUFHW(a, m)      _mm_shufflehi_epi16( a, m )
#define PSHUFD(a, m)       _mm_shuffle_epi32( a, m )
#define PUNPACKLBW(a, b)   _mm_unpacklo_epi8( a, b )
#define PUNPACKLDQ(a, b)   _mm_unpacklo_epi32( a, b )
#define PUNPACKHDQ(a, b)   _mm_unpackhi_epi32( a, b )
#define PUNPACKLQDQ(a, b)  _mm_unpacklo_epi64( a, b )
#define PUNPACKHQDQ(a, b)  _mm_unpackhi_epi64( a, b )
#define SET1(w)            _mm_set1_epi16( w )
#define ZERO               _mm_setzero_si128()

#define LDX(p)             _mm_loadu_si128( (__m128i*)(p) )
#define STX(p, v)          _mm_storeu_si128( (__m128i*)(p), v )
#define LDK(p)             ( *(__m128i*)(&(p)) )

#define LDL(p)             _mm_loadl_epi64( (__m128i*)(p) )
#define CAST128(a)         _mm_castsi128_ps( a )
#define STL(p, v)          _mm_storel_pi( (__m64*)(p), CAST128(v) )
#define STH(p, v)          _mm_storeh_pi( (__m64*)(p), CAST128(v) )
#define STLH(p1, p2, v)    { STL( p1, v ); STH( p2, v ); }


#define BITS_FWD_ACC       3
#define SHIFT_FWD_COL      BITS_FWD_ACC
#define SHIFT_FWD_ROW      BITS_FWD_ACC + 17
#define RND_FWD_ROW        (1 << (SHIFT_FWD_ROW-1))

#define BITS_INV_ACC       5
#define SHIFT_INV_ROW     16 - BITS_INV_ACC
#define SHIFT_INV_COL      1 + BITS_INV_ACC
#define RND_INV_ROW        (1 << (SHIFT_INV_ROW-1))

#define c_inv_corr_0     -1024 * (6 - BITS_INV_ACC) + 65536 /* -0.5 + 32.0  */
#define c_inv_corr_1      1877 * (6 - BITS_INV_ACC)         /*  0.9167      */
#define c_inv_corr_2      1236 * (6 - BITS_INV_ACC)         /*  0.6035      */
#define c_inv_corr_3       680 * (6 - BITS_INV_ACC)         /*  0.3322      */
#define c_inv_corr_4         0 * (6 - BITS_INV_ACC)         /*  0.0         */
#define c_inv_corr_5      -569 * (6 - BITS_INV_ACC)         /* -0.278       */
#define c_inv_corr_6      -512 * (6 - BITS_INV_ACC)         /* -0.25        */
#define c_inv_corr_7      -651 * (6 - BITS_INV_ACC)         /* -0.3176      */

#define RND_INV_ROW_0      RND_INV_ROW + c_inv_corr_0
#define RND_INV_ROW_1      RND_INV_ROW + c_inv_corr_1
#define RND_INV_ROW_2      RND_INV_ROW + c_inv_corr_2
#define RND_INV_ROW_3      RND_INV_ROW + c_inv_corr_3
#define RND_INV_ROW_4      RND_INV_ROW + c_inv_corr_4
#define RND_INV_ROW_5      RND_INV_ROW + c_inv_corr_5
#define RND_INV_ROW_6      RND_INV_ROW + c_inv_corr_6
#define RND_INV_ROW_7      RND_INV_ROW + c_inv_corr_7

XMMCONST int round_i_0[4] =
   { RND_INV_ROW_0, RND_INV_ROW_0, RND_INV_ROW_0, RND_INV_ROW_0 };
XMMCONST int round_i_1[4] =
   { RND_INV_ROW_1, RND_INV_ROW_1, RND_INV_ROW_1, RND_INV_ROW_1 };
XMMCONST int round_i_2[4] =
   { RND_INV_ROW_2, RND_INV_ROW_2, RND_INV_ROW_2, RND_INV_ROW_2 };
XMMCONST int round_i_3[4] =
   { RND_INV_ROW_3, RND_INV_ROW_3, RND_INV_ROW_3, RND_INV_ROW_3 };
XMMCONST int round_i_4[4] =
   { RND_INV_ROW_4, RND_INV_ROW_4, RND_INV_ROW_4, RND_INV_ROW_4 };
XMMCONST int round_i_5[4] =
   { RND_INV_ROW_5, RND_INV_ROW_5, RND_INV_ROW_5, RND_INV_ROW_5 };
XMMCONST int round_i_6[4] =
   { RND_INV_ROW_6, RND_INV_ROW_6, RND_INV_ROW_6, RND_INV_ROW_6 };
XMMCONST int round_i_7[4] =
   { RND_INV_ROW_7, RND_INV_ROW_7, RND_INV_ROW_7, RND_INV_ROW_7 };


XMMCONST short int tg_1_16[8] =
   { 13036,  13036,  13036,  13036,  13036,  13036,  13036,  13036 };
XMMCONST short int tg_2_16[8] =
   { 27146,  27146,  27146,  27146,  27146,  27146,  27146,  27146 };
XMMCONST short int tg_3_16[8] =
   {-21746, -21746, -21746, -21746, -21746, -21746, -21746, -21746 };
XMMCONST short int cos_4_16[8] =
   {-19195, -19195, -19195, -19195, -19195, -19195, -19195, -19195 };


XMMCONST short int tab_i_04[32] =
   { 16384,  21407,  16384,   8867, -16384,  21407,  16384,  -8867,
     16384,  -8867,  16384, -21407,  16384,   8867, -16384, -21407,
     22725,  19266,  19266,  -4520,   4520,  19266,  19266, -22725,
     12873, -22725,   4520, -12873,  12873,   4520, -22725, -12873 };
XMMCONST short int tab_i_17[32] =
   { 22725,  29692,  22725,  12299, -22725,  29692,  22725, -12299,
     22725, -12299,  22725, -29692,  22725,  12299, -22725, -29692,
     31521,  26722,  26722,  -6270,   6270,  26722,  26722, -31521,
     17855, -31521,   6270, -17855,  17855,   6270, -31521, -17855 };
XMMCONST short int tab_i_26[32] =
   { 21407,  27969,  21407,  11585, -21407,  27969,  21407, -11585,
     21407, -11585,  21407, -27969,  21407,  11585, -21407, -27969,
     29692,  25172,  25172,  -5906,   5906,  25172,  25172, -29692,
     16819, -29692,   5906, -16819,  16819,   5906, -29692, -16819 };
XMMCONST short int tab_i_35[32] =
   { 19266,  25172,  19266,  10426, -19266,  25172,  19266, -10426,
     19266, -10426,  19266, -25172,  19266,  10426, -19266, -25172,
     26722,  22654,  22654,  -5315,   5315,  22654,  22654, -26722,
     15137, -26722,   5315, -15137,  15137,   5315, -26722, -15137 };


__INLINE void dct_8x8_inv_16s_algnd( const Ipp16s* pSrc, Ipp8u* pDst, int dstStep, const Ipp16s* pQuantInvTable );
__INLINE void dct_8x8_inv_16s      ( const Ipp16s* pSrc, Ipp8u* pDst, int dstStep, const Ipp16s* pQuantInvTable );



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R
//  Purpose:
//    Inverse DCT transform, de-quantization and level shift
//  Parameter:
//    pSrc           - pointer to source
//    pDst           - pointer to output array
//    dstStep        - line offset for output data
//    pQuantInvTable - pointer to Quantization table
//  Returns:
//    IppStatus
*/

IPPFUN(IppStatus, mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R, (
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable))
{
   //__ALIGN32 Ipp16s buff[DCTSIZE2];

   IPP_BAD_PTR2_RET(pSrc,pDst)
   IPP_BAD_STEP_RET(dstStep)
   IPP_BAD_PTR1_RET(pQuantInvTable)

   if ( !((IPP_INT_PTR(pSrc)|IPP_INT_PTR(pQuantInvTable)) & 15) ) {
      dct_8x8_inv_16s_algnd ( pSrc, pDst, dstStep, (const Ipp16s*)pQuantInvTable);
   } else {
      dct_8x8_inv_16s ( pSrc, pDst, dstStep, (const Ipp16s*)pQuantInvTable);
   }

   return ippStsNoErr;
} /* mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R() */


__INLINE void dct_8x8_inv_16s_algnd ( const Ipp16s* pSrc, Ipp8u* pDst, int dstStep, const Ipp16s* pQuantInvTable )
{
   __m128i x0, x1, x2, x3, x4, x5, x6, x7,
         y0, y1, y2, y3, y4, y5, y6, y7,
         t0, t1, t2, t3, t4, t5, t6, t7,
         xe, xo, t1e, t2e, t1o, t2o, a0, b0, s0, s1,
         tp03, tm03, tp12, tm12, tp65, tm65,
         tp465, tm465, tp765, tm765;
   __m128i xmmTmp;

/* ------------------------------------------------------------------------ */
/* rows                                                                     */
/* ------------------------------------------------------------------------ */

   x0 = _mm_load_si128( (__m128i*)(pSrc + 0*8) );
   x0 = _mm_mullo_epi16(x0, *((__m128i*)(pQuantInvTable + 0*8)) );
   xe    = PSHUFLW( x0, SH_2020 );
   xo    = PSHUFLW( x0, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_04[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_04[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_04[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_04[24]) );
   t1e   = PADDD( t1e, LDK(round_i_0) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x0    = PACKSSDW( s0, s1 );
   x0    = PSHUFHW( x0, SH_0123 );

   x4 = _mm_load_si128( (__m128i*)(pSrc + 4*8) );
   x4 = _mm_mullo_epi16(x4, *((__m128i*)(pQuantInvTable + 4*8)) );
   xe    = PSHUFLW( x4, SH_2020 );
   xo    = PSHUFLW( x4, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_04[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_04[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_04[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_04[24]) );
   t1e   = PADDD( t1e, LDK(round_i_4) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x4    = PACKSSDW( s0, s1 );
   x4    = PSHUFHW( x4, SH_0123 );

   x1 = _mm_load_si128( (__m128i*)(pSrc + 1*8) );
   x1 = _mm_mullo_epi16(x1, *((__m128i*)(pQuantInvTable + 1*8)) );
   xe    = PSHUFLW( x1, SH_2020 );
   xo    = PSHUFLW( x1, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_17[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_17[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_17[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_17[24]) );
   t1e   = PADDD( t1e, LDK(round_i_1) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x1    = PACKSSDW( s0, s1 );
   x1    = PSHUFHW( x1, SH_0123 );

   x7 = _mm_load_si128( (__m128i*)( pSrc + 7*8 ) );
   x7 = _mm_mullo_epi16(x7, *((__m128i*)(pQuantInvTable + 7*8)) );
   xe    = PSHUFLW( x7, SH_2020 );
   xo    = PSHUFLW( x7, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_17[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_17[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_17[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_17[24]) );
   t1e   = PADDD( t1e, LDK(round_i_7) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x7    = PACKSSDW( s0, s1 );
   x7    = PSHUFHW( x7, SH_0123 );

   x3 = _mm_load_si128( (__m128i*)(pSrc + 3*8) );
   x3 = _mm_mullo_epi16(x3, *((__m128i*)(pQuantInvTable + 3*8)) );
   xe    = PSHUFLW( x3, SH_2020 );
   xo    = PSHUFLW( x3, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_35[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_35[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_35[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_35[24]) );
   t1e   = PADDD( t1e, LDK(round_i_3) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x3    = PACKSSDW( s0, s1 );
   x3    = PSHUFHW( x3, SH_0123 );

   x5 = _mm_load_si128( (__m128i*)(pSrc + 5*8) );
   x5 = _mm_mullo_epi16(x5, *((__m128i*)(pQuantInvTable + 5*8)) );
   xe    = PSHUFLW( x5, SH_2020 );
   xo    = PSHUFLW( x5, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_35[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_35[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_35[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_35[24]) );
   t1e   = PADDD( t1e, LDK(round_i_5) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x5    = PACKSSDW( s0, s1 );
   x5    = PSHUFHW( x5, SH_0123 );

   x2 = _mm_load_si128( (__m128i*)(pSrc + 2*8) );
   x2 = _mm_mullo_epi16(x2, *((__m128i*)(pQuantInvTable + 2*8)) );
   xe    = PSHUFLW( x2, SH_2020 );
   xo    = PSHUFLW( x2, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_26[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_26[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_26[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_26[24]) );
   t1e   = PADDD( t1e, LDK(round_i_2) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x2    = PACKSSDW( s0, s1 );
   x2    = PSHUFHW( x2, SH_0123 );

   x6 = _mm_load_si128( (__m128i*)(pSrc + 6*8) );
   x6 = _mm_mullo_epi16(x6, *((__m128i*)(pQuantInvTable + 6*8)) );
   xe    = PSHUFLW( x6, SH_2020 );
   xo    = PSHUFLW( x6, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_26[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_26[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_26[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_26[24]) );
   t1e   = PADDD( t1e, LDK(round_i_6) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x6    = PACKSSDW( s0, s1 );
   x6    = PSHUFHW( x6, SH_0123 );

/* ------------------------------------------------------------------------ */
/* columns                                                                  */
/* ------------------------------------------------------------------------ */

   t3    = PADDSW( PMULHW( x3, LDK(tg_3_16) ), x3 );
   t5    = PADDSW( PMULHW( x5, LDK(tg_3_16) ), x5 );
   tm765 = PADDSW( t5, x3 );
   tm465 = PSUBSW( x5, t3 );

   t1    = PMULHW( x1, LDK(tg_1_16) );
   t7    = PMULHW( x7, LDK(tg_1_16) );
   tp765 = PADDSW( x1, t7 );
   tp465 = PSUBSW( t1, x7 );

   t7    = PADDSW( tp765, tm765 );
   tp65  = PSUBSW( tp765, tm765 );
   t4    = PADDSW( tp465, tm465 );
   tm65  = PSUBSW( tp465, tm465 );

   t2    = PMULHW( x2, LDK(tg_2_16) );
   t6    = PMULHW( x6, LDK(tg_2_16) );
   tm03  = PADDSW( x2, t6 );
   tm12  = PSUBSW( t2, x6 );

   t5    = PSUBSW( tp65, tm65 );
   t6    = PADDSW( tp65, tm65 );
   t5    = PADDSW( PMULHW( t5, LDK(cos_4_16) ), t5 );
   t6    = PADDSW( PMULHW( t6, LDK(cos_4_16) ), t6 );

   tp03  = PADDSW( x0, x4 );
   tp12  = PSUBSW( x0, x4 );

   t0    = PADDSW( tp03, tm03 );
   t3    = PSUBSW( tp03, tm03 );
   t1    = PADDSW( tp12, tm12 );
   t2    = PSUBSW( tp12, tm12 );

   y0    = PADDSW( t0, t7 );
   y7    = PSUBSW( t0, t7 );
   y1    = PADDSW( t1, t6 );
   y6    = PSUBSW( t1, t6 );
   y2    = PADDSW( t2, t5 );
   y5    = PSUBSW( t2, t5 );
   y3    = PADDSW( t3, t4 );
   y4    = PSUBSW( t3, t4 );

   y0    = PSRAW( y0, SHIFT_INV_COL );
   y1    = PSRAW( y1, SHIFT_INV_COL );
   y2    = PSRAW( y2, SHIFT_INV_COL );
   y3    = PSRAW( y3, SHIFT_INV_COL );
   y4    = PSRAW( y4, SHIFT_INV_COL );
   y5    = PSRAW( y5, SHIFT_INV_COL );
   y6    = PSRAW( y6, SHIFT_INV_COL );
   y7    = PSRAW( y7, SHIFT_INV_COL );

   xmmTmp = _mm_set1_epi16( 128 );
   //STX( pDst + 0*8, y0 );
   //STX( pDst + 1*8, y1 );
   y0 = PADDW( y0, xmmTmp );
   y1 = PADDW( y1, xmmTmp );
   y0 = PACKUSWB( y0, y1 );
   STLH( (char*)pDst + 0*dstStep, (char*)pDst + 1*dstStep, y0 );
   //STX( pDst + 2*8, y2 );
   //STX( pDst + 3*8, y3 );
   y2 = PADDW( y2, xmmTmp );
   y3 = PADDW( y3, xmmTmp );
   y2 = PACKUSWB( y2, y3 );
   STLH( (char*)pDst + 2*dstStep, (char*)pDst + 3*dstStep, y2 );
   //STX( pDst + 4*8, y4 );
   //STX( pDst + 5*8, y5 );
   y4 = PADDW( y4, xmmTmp );
   y5 = PADDW( y5, xmmTmp );
   y4 = PACKUSWB( y4, y5 );
   STLH( (char*)pDst + 4*dstStep, (char*)pDst + 5*dstStep, y4 );
   //STX( pDst + 6*8, y6 );
   //STX( pDst + 7*8, y7 );
   y6 = PADDW( y6, xmmTmp );
   y7 = PADDW( y7, xmmTmp );
   y6 = PACKUSWB( y6, y7 );
   STLH( (char*)pDst + 6*dstStep, (char*)pDst + 7*dstStep, y6 );
}


__INLINE void dct_8x8_inv_16s ( const Ipp16s* pSrc, Ipp8u* pDst, int dstStep, const Ipp16s* pQuantInvTable )
{
   __m128i x0, x1, x2, x3, x4, x5, x6, x7,
         y0, y1, y2, y3, y4, y5, y6, y7,
         t0, t1, t2, t3, t4, t5, t6, t7,
         xe, xo, t1e, t2e, t1o, t2o, a0, b0, s0, s1,
         tp03, tm03, tp12, tm12, tp65, tm65,
         tp465, tm465, tp765, tm765;
   __m128i xmmTmp;

/* ------------------------------------------------------------------------ */
/* rows                                                                     */
/* ------------------------------------------------------------------------ */

   xmmTmp = LDX( pQuantInvTable + 0*8 );
   x0    = LDX( pSrc + 0*8 );
   x0 = _mm_mullo_epi16(x0, xmmTmp);
   xe    = PSHUFLW( x0, SH_2020 );
   xo    = PSHUFLW( x0, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_04[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_04[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_04[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_04[24]) );
   t1e   = PADDD( t1e, LDK(round_i_0) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x0    = PACKSSDW( s0, s1 );
   x0    = PSHUFHW( x0, SH_0123 );

   xmmTmp = LDX( pQuantInvTable + 4*8 );
   x4    = LDX( pSrc + 4*8 );
   x4 = _mm_mullo_epi16(x4, xmmTmp);
   xe    = PSHUFLW( x4, SH_2020 );
   xo    = PSHUFLW( x4, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_04[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_04[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_04[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_04[24]) );
   t1e   = PADDD( t1e, LDK(round_i_4) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x4    = PACKSSDW( s0, s1 );
   x4    = PSHUFHW( x4, SH_0123 );

   xmmTmp = LDX( pQuantInvTable + 1*8 );
   x1    = LDX( pSrc + 1*8 );
   x1 = _mm_mullo_epi16(x1, xmmTmp);
   xe    = PSHUFLW( x1, SH_2020 );
   xo    = PSHUFLW( x1, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_17[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_17[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_17[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_17[24]) );
   t1e   = PADDD( t1e, LDK(round_i_1) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x1    = PACKSSDW( s0, s1 );
   x1    = PSHUFHW( x1, SH_0123 );

   xmmTmp = LDX( pQuantInvTable + 7*8 );
   x7    = LDX( pSrc + 7*8 );
   x7 = _mm_mullo_epi16(x7, xmmTmp);
   xe    = PSHUFLW( x7, SH_2020 );
   xo    = PSHUFLW( x7, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_17[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_17[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_17[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_17[24]) );
   t1e   = PADDD( t1e, LDK(round_i_7) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x7    = PACKSSDW( s0, s1 );
   x7    = PSHUFHW( x7, SH_0123 );

   xmmTmp = LDX( pQuantInvTable + 3*8 );
   x3    = LDX( pSrc + 3*8 );
   x3 = _mm_mullo_epi16(x3, xmmTmp);
   xe    = PSHUFLW( x3, SH_2020 );
   xo    = PSHUFLW( x3, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_35[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_35[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_35[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_35[24]) );
   t1e   = PADDD( t1e, LDK(round_i_3) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x3    = PACKSSDW( s0, s1 );
   x3    = PSHUFHW( x3, SH_0123 );

   xmmTmp = LDX( pQuantInvTable + 5*8 );
   x5    = LDX( pSrc + 5*8 );
   x5 = _mm_mullo_epi16(x5, xmmTmp);
   xe    = PSHUFLW( x5, SH_2020 );
   xo    = PSHUFLW( x5, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_35[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_35[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_35[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_35[24]) );
   t1e   = PADDD( t1e, LDK(round_i_5) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x5    = PACKSSDW( s0, s1 );
   x5    = PSHUFHW( x5, SH_0123 );

   xmmTmp = LDX( pQuantInvTable + 2*8 );
   x2    = LDX( pSrc + 2*8 );
   x2 = _mm_mullo_epi16(x2, xmmTmp);
   xe    = PSHUFLW( x2, SH_2020 );
   xo    = PSHUFLW( x2, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_26[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_26[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_26[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_26[24]) );
   t1e   = PADDD( t1e, LDK(round_i_2) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x2    = PACKSSDW( s0, s1 );
   x2    = PSHUFHW( x2, SH_0123 );

   xmmTmp = LDX( pQuantInvTable + 6*8 );
   x6    = LDX( pSrc + 6*8 );
   x6 = _mm_mullo_epi16(x6, xmmTmp);
   xe    = PSHUFLW( x6, SH_2020 );
   xo    = PSHUFLW( x6, SH_3131 );
   xe    = PSHUFHW( xe, SH_2020 );
   xo    = PSHUFHW( xo, SH_3131 );
   t1e   = PMADDWD( xe, LDK(tab_i_26[0]) );
   t2e   = PMADDWD( xe, LDK(tab_i_26[8]) );
   t1o   = PMADDWD( xo, LDK(tab_i_26[16]) );
   t2o   = PMADDWD( xo, LDK(tab_i_26[24]) );
   t1e   = PADDD( t1e, LDK(round_i_6) );
   t2e   = PSHUFD( t2e, SH_1032 );
   t2o   = PSHUFD( t2o, SH_1032 );
   a0    = PADDD( t1e, t2e );
   b0    = PADDD( t1o, t2o );
   s0    = PADDD( a0, b0 );
   s1    = PSUBD( a0, b0 );
   s0    = PSRAD( s0, SHIFT_INV_ROW );
   s1    = PSRAD( s1, SHIFT_INV_ROW );
   x6    = PACKSSDW( s0, s1 );
   x6    = PSHUFHW( x6, SH_0123 );

/* ------------------------------------------------------------------------ */
/* columns                                                                  */
/* ------------------------------------------------------------------------ */

   t3    = PADDSW( PMULHW( x3, LDK(tg_3_16) ), x3 );
   t5    = PADDSW( PMULHW( x5, LDK(tg_3_16) ), x5 );
   tm765 = PADDSW( t5, x3 );
   tm465 = PSUBSW( x5, t3 );

   t1    = PMULHW( x1, LDK(tg_1_16) );
   t7    = PMULHW( x7, LDK(tg_1_16) );
   tp765 = PADDSW( x1, t7 );
   tp465 = PSUBSW( t1, x7 );

   t7    = PADDSW( tp765, tm765 );
   tp65  = PSUBSW( tp765, tm765 );
   t4    = PADDSW( tp465, tm465 );
   tm65  = PSUBSW( tp465, tm465 );

   t2    = PMULHW( x2, LDK(tg_2_16) );
   t6    = PMULHW( x6, LDK(tg_2_16) );
   tm03  = PADDSW( x2, t6 );
   tm12  = PSUBSW( t2, x6 );

   t5    = PSUBSW( tp65, tm65 );
   t6    = PADDSW( tp65, tm65 );
   t5    = PADDSW( PMULHW( t5, LDK(cos_4_16) ), t5 );
   t6    = PADDSW( PMULHW( t6, LDK(cos_4_16) ), t6 );

   tp03  = PADDSW( x0, x4 );
   tp12  = PSUBSW( x0, x4 );

   t0    = PADDSW( tp03, tm03 );
   t3    = PSUBSW( tp03, tm03 );
   t1    = PADDSW( tp12, tm12 );
   t2    = PSUBSW( tp12, tm12 );

   y0    = PADDSW( t0, t7 );
   y7    = PSUBSW( t0, t7 );
   y1    = PADDSW( t1, t6 );
   y6    = PSUBSW( t1, t6 );
   y2    = PADDSW( t2, t5 );
   y5    = PSUBSW( t2, t5 );
   y3    = PADDSW( t3, t4 );
   y4    = PSUBSW( t3, t4 );

   y0    = PSRAW( y0, SHIFT_INV_COL );
   y1    = PSRAW( y1, SHIFT_INV_COL );
   y2    = PSRAW( y2, SHIFT_INV_COL );
   y3    = PSRAW( y3, SHIFT_INV_COL );
   y4    = PSRAW( y4, SHIFT_INV_COL );
   y5    = PSRAW( y5, SHIFT_INV_COL );
   y6    = PSRAW( y6, SHIFT_INV_COL );
   y7    = PSRAW( y7, SHIFT_INV_COL );

   xmmTmp = _mm_set1_epi16( 128 );
   //STX( pDst + 0*8, y0 );
   //STX( pDst + 1*8, y1 );
   y0 = PADDW( y0, xmmTmp );
   y1 = PADDW( y1, xmmTmp );
   y0 = PACKUSWB( y0, y1 );
   STLH( (char*)pDst + 0*dstStep, (char*)pDst + 1*dstStep, y0 );
   //STX( pDst + 2*8, y2 );
   //STX( pDst + 3*8, y3 );
   y2 = PADDW( y2, xmmTmp );
   y3 = PADDW( y3, xmmTmp );
   y2 = PACKUSWB( y2, y3 );
   STLH( (char*)pDst + 2*dstStep, (char*)pDst + 3*dstStep, y2 );
   //STX( pDst + 4*8, y4 );
   //STX( pDst + 5*8, y5 );
   y4 = PADDW( y4, xmmTmp );
   y5 = PADDW( y5, xmmTmp );
   y4 = PACKUSWB( y4, y5 );
   STLH( (char*)pDst + 4*dstStep, (char*)pDst + 5*dstStep, y4 );
   //STX( pDst + 6*8, y6 );
   //STX( pDst + 7*8, y7 );
   y6 = PADDW( y6, xmmTmp );
   y7 = PADDW( y7, xmmTmp );
   y6 = PACKUSWB( y6, y7 );
   STLH( (char*)pDst + 6*dstStep, (char*)pDst + 7*dstStep, y6 );
}



#endif //((_IPP>=_IPP_V8)||(_IPP32E>=_IPP32E_U8))
#endif //!((_IPP>=_IPP_H9)||(_IPP32E>=_IPP32E_L9))
