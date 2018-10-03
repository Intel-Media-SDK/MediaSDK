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

#include "precomp.h"
#include <emmintrin.h> // __m128 etc
#include "owncc.h"

#if ( (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7) )

#if ( (_IPP >= _IPP_W7) )
extern void mfxownccCopy_8u_C1_W7( const Ipp8u* pSrc, Ipp8u* pDst, int len, int nonTemporal );
#elif ( (_IPP32E >= _IPP32E_M7) )
extern void mfxownccCopy_8u_C1_M7( const Ipp8u* pSrc, Ipp8u* pDst, int len, int nonTemporal );
#endif


#define CLIP(x) ((x < 0) ? 0 : ((x > 255) ? 255 : x))

/*
    Lib = W7 M7
    Caller = mfxiYCbCr422ToYCbCr420_8u_P3R
*/
extern  void   mfxownYCbCr422ToYCbCr420_8u_P3R(const Ipp8u* pSrc[3], int srcStep[3], Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize )
{
  int h;
  int width  = roiSize.width ;
  int height = roiSize.height;
  int nonTemporal = 0;
  /* Y plane */
  if( width*height > 348602 /* 131072 */ ) nonTemporal = 1;
  for( h = 0; h < height; h ++ )
  {
#if ( (_IPP >= _IPP_W7) )
    mfxownccCopy_8u_C1_W7( pSrc[0] + h * srcStep[0], pDst[0] + h * dstStep[0], width, nonTemporal );
#elif ( (_IPP32E >= _IPP32E_M7) )
    mfxownccCopy_8u_C1_M7( pSrc[0] + h * srcStep[0], pDst[0] + h * dstStep[0], width, nonTemporal );
#endif
  }
  height /= 2;
  width  /= 2;

 // if( width*height > 348602 /* 131072 */ ) nonTemporal = 1;

  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* srcu;
    const Ipp8u* srcv;
    Ipp8u* dstu;
    Ipp8u* dstv;
    srcu = pSrc[1] + h * 2 * srcStep[1];
    dstu = pDst[1] + h * dstStep[1];
    srcv = pSrc[2] + h * 2 * srcStep[2];
    dstv = pDst[2] + h * dstStep[2];
#if ( (_IPP >= _IPP_W7) )
    mfxownccCopy_8u_C1_W7( srcu, dstu, width, 0 );
    mfxownccCopy_8u_C1_W7( srcv, dstv, width, 0 );
#elif ( (_IPP32E >= _IPP32E_M7) )
    mfxownccCopy_8u_C1_M7( srcu, dstu, width, 0 );
    mfxownccCopy_8u_C1_M7( srcv, dstv, width, 0 );
#endif
  }
}


/*
    Lib = W7 M7
    Caller = mfxiYCbCr420ToYCbCr422_8u_P3R
*/
extern  void  mfxownYCbCr420ToYCbCr422_8u_P3R(const Ipp8u* pSrc[3], int srcStep[3], Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize,int orderUV )
{
  int h,w,width64,width32,width16,width8;
  const Ipp8u* pSrcU = pSrc[1];
  const Ipp8u* pSrcV = pSrc[2];
  int width  = roiSize.width ;
  int height = roiSize.height;
  int srcStepU = srcStep[1], srcStepV = srcStep[2];
//  int nonTemporal = 0;
  int sStep = srcStep[0],dStep = dstStep[0];
  const Ipp8u* sRc = pSrc[0];
  Ipp8u* dSt = pDst[0];

  if( orderUV ){
    pSrcU = pSrc[2];
    pSrcV = pSrc[1];
    srcStepU = srcStep[2]; srcStepV = srcStep[1];
  }
  /* Y plane */
 // if( width*height > 348602 /* 131072 */ ) nonTemporal = 1;
  for( h = 0; h < height; h++ )
  {
#if ( (_IPP >= _IPP_W7) )
      mfxownccCopy_8u_C1_W7( sRc, dSt, width, 0 );
#elif ( (_IPP32E >= _IPP32E_M7) )
      mfxownccCopy_8u_C1_M7( sRc, dSt, width, 0 );
#endif
      sRc += sStep, dSt += dStep;
  }
  height /= 2;width  /= 2;
  width64 = width & ~0x3f; width32 = width &  0x3f; width16 = width &  0x1f; width8  = width &  0x0f;
  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* srcu;
    const Ipp8u* srcv;
    Ipp8u* dstu;
    Ipp8u* dstv;
    int dsu = dstStep[1];
    int dsv = dstStep[2];
    srcu = pSrcU   + h *     srcStepU;
    dstu = pDst[1] + h * 2 * dstStep[1];
    srcv = pSrcV   + h *     srcStepV;
    dstv = pDst[2] + h * 2 * dstStep[2];
    for( w = 0; w < width64; w += 64 )
    {
      __m128i t0,t1,t2,t3;
      __m128i t4,t5,t6,t7;
      t0 = _mm_loadu_si128((__m128i*)srcu);
      t1 = _mm_loadu_si128((__m128i*)(srcu+16));
      t2 = _mm_loadu_si128((__m128i*)(srcu+32));
      t3 = _mm_loadu_si128((__m128i*)(srcu+48));
      t4 = _mm_loadu_si128((__m128i*)srcv);
      t5 = _mm_loadu_si128((__m128i*)(srcv+16));
      t6 = _mm_loadu_si128((__m128i*)(srcv+32));
      t7 = _mm_loadu_si128((__m128i*)(srcv+48));

      _mm_storeu_si128((__m128i*) dstu    , t0);
      _mm_storeu_si128((__m128i*)(dstu+16), t1);
      _mm_storeu_si128((__m128i*)(dstu+32), t2);
      _mm_storeu_si128((__m128i*)(dstu+48), t3);
      _mm_storeu_si128((__m128i*)(dstu+dsu) , t0);
      _mm_storeu_si128((__m128i*)(dstu+16+dsu), t1);
      _mm_storeu_si128((__m128i*)(dstu+32+dsu), t2);
      _mm_storeu_si128((__m128i*)(dstu+48+dsu), t3);

      _mm_storeu_si128((__m128i*) dstv    , t4);
      _mm_storeu_si128((__m128i*)(dstv+16), t5);
      _mm_storeu_si128((__m128i*)(dstv+32), t6);
      _mm_storeu_si128((__m128i*)(dstv+48), t7);
      _mm_storeu_si128((__m128i*)(dstv+dsv), t4);
      _mm_storeu_si128((__m128i*)(dstv+16+dsv), t5);
      _mm_storeu_si128((__m128i*)(dstv+32+dsv), t6);
      _mm_storeu_si128((__m128i*)(dstv+48+dsv), t7);
      dstu += 64; srcu += 64;dstv += 64; srcv += 64;
    }
    if( width32 >= 32 )
    {
      __m128i t0,t1;
      __m128i t4,t5;
      w += 32;
      t0 = _mm_loadu_si128((__m128i*)srcu);
      t1 = _mm_loadu_si128((__m128i*)(srcu+16));
      t4 = _mm_loadu_si128((__m128i*)srcv);
      t5 = _mm_loadu_si128((__m128i*)(srcv+16));
      _mm_storeu_si128((__m128i*) dstu    , t0);
      _mm_storeu_si128((__m128i*)(dstu+16), t1);
      _mm_storeu_si128((__m128i*)(dstu+dsu) , t0);
      _mm_storeu_si128((__m128i*)(dstu+16+dsu), t1);
      _mm_storeu_si128((__m128i*) dstv    , t4);
      _mm_storeu_si128((__m128i*)(dstv+16), t5);
      _mm_storeu_si128((__m128i*)(dstv+dsu), t4);
      _mm_storeu_si128((__m128i*)(dstv+16+dsu), t5);
      dstu += 32; srcu += 32; dstv += 32; srcv += 32;
    }
    if( width16 >= 16 )
    {
      __m128i t0;
      __m128i t4;
      w += 16;
      t0 = _mm_loadu_si128((__m128i*)srcu);
      t4 = _mm_loadu_si128((__m128i*)srcv);
      _mm_storeu_si128((__m128i*) dstu    , t0);
      _mm_storeu_si128((__m128i*)(dstu+dsu) , t0);
      _mm_storeu_si128((__m128i*) dstv    , t4);
      _mm_storeu_si128((__m128i*)(dstv+dsu), t4);
      dstu += 16; srcu += 16; dstv += 16; srcv += 16;
    }
    if( width8 >= 8 )
    {
      __m128i t0;
      __m128i t4;
      w += 8;
      t0 = _mm_loadl_epi64((__m128i*)srcu);
      t4 = _mm_loadl_epi64((__m128i*)srcv);
      _mm_storel_epi64((__m128i*) dstu , t0);
      _mm_storel_epi64((__m128i*)(dstu+dsu) ,t0);
      _mm_storel_epi64((__m128i*) dstv , t4);
      _mm_storel_epi64((__m128i*)(dstv+dsu), t4);
      dstu += 8; srcu += 8; dstv += 8; srcv += 8;
    }
    for( ; w < width; w ++ )
    {
      *(dstu+dstStep[1]) = *srcu;
      *dstu++            = *srcu++;
      *(dstv+dstStep[2]) = *srcv;
      *dstv++            = *srcv++;
    }
  }
}


/*
    Lib = W7 M7
    Caller = mfxiYCbCr422_8u_C2P3R
*/
extern  void  mfxalYCbCr422ToYCbCr422_8u_C2P3R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize )
{
  int h,w,width32,width8,width16;
  int width  = roiSize.width ;
  int height = roiSize.height;
 __m128i mask = _mm_set1_epi32(0x00ff00ff);

  width32 = width & ~0x1f; width16 = width &  0x1f;
  width8  = width &  0x0f;
  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* src;
    Ipp8u* dsty,*dstu,*dstv;
    src = pSrc + h * srcStep;
    dsty = pDst[0] + h * dstStep[0];
    dstu = pDst[1] + h * dstStep[1];
    dstv = pDst[2] + h * dstStep[2];
    for( w = 0; w < width32; w += 32 )
    {
      __m128i t0,t1,t2,t3;
      __m128i t4,t5,t6,t7;
      t0 = _mm_load_si128((__m128i*) src);
      t1 = _mm_load_si128((__m128i*)(src+16));
      t2 = _mm_load_si128((__m128i*)(src+32));
      t3 = _mm_load_si128((__m128i*)(src+48));
      t4 = _mm_and_si128( t0, mask );
      t5 = _mm_and_si128( t1, mask );
      t6 = _mm_and_si128( t2, mask );
      t7 = _mm_and_si128( t3, mask );
      t4 = _mm_packus_epi16( t4, t5);
      t6 = _mm_packus_epi16( t6, t7);
      t0 = _mm_srli_epi16( t0 , 8 );
      t1 = _mm_srli_epi16( t1 , 8 );
      t2 = _mm_srli_epi16( t2 , 8 );
      t3 = _mm_srli_epi16( t3 , 8 );
      t0 = _mm_packus_epi16( t0, t1);
      t2 = _mm_packus_epi16( t2, t3);
      t1 = _mm_and_si128( t0, mask );
      t3 = _mm_and_si128( t2, mask );
      t1 = _mm_packus_epi16( t1, t3); //U _mm_stream_si128
      t0 = _mm_srli_epi16( t0 , 8 );
      t2 = _mm_srli_epi16( t2 , 8 );
      t0 = _mm_packus_epi16( t0, t2); //V
      _mm_store_si128((__m128i*) (dsty)   , t4);
      _mm_store_si128((__m128i*) (dsty+16), t6);
      _mm_storeu_si128((__m128i*) (dstu)   , t1);
      _mm_storeu_si128((__m128i*) (dstv)   , t0);
      src += 64;dsty += 32;dstu += 16;dstv += 16;
    }
    if( width16 >= 16 )
    {
      __m128i t0,t1;
      __m128i t4,t5;

      t0 = _mm_load_si128((__m128i*) src);
      t1 = _mm_load_si128((__m128i*)(src+16));

      t4 = _mm_and_si128( t0, mask );
      t5 = _mm_and_si128( t1, mask );
      t4 = _mm_packus_epi16( t4, t5);

      t0 = _mm_srli_epi16( t0 , 8 );
      t1 = _mm_srli_epi16( t1 , 8 );
      t0 = _mm_packus_epi16( t0, t1);
      t1 = _mm_and_si128( t0, mask );
      t1 = _mm_packus_epi16( t1, t1); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t1); //V
      _mm_store_si128((__m128i*) (dsty) , t4);
      _mm_storel_epi64( (__m128i*) dstu , t1);
      _mm_storel_epi64( (__m128i*) dstv , t0);
      src += 32;dsty += 16;dstu += 8;dstv += 8;
     }
    if( width8 >= 8 )
    {
      __m128i t0, t1;
      __m128i t4;

      t0 = _mm_load_si128((__m128i*) src);
      t4 = _mm_and_si128( t0, mask );
      t4 = _mm_packus_epi16( t4, t4);
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t0);
      t1 = _mm_and_si128( t0, mask );
      t1 = _mm_packus_epi16( t1, t1); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t1); //V
      _mm_storel_epi64( (__m128i*) dsty , t4);
      *(int*)dstu = _mm_cvtsi128_si32( t1 );
      *(int*)dstv = _mm_cvtsi128_si32( t0 );
     }
  }
  _mm_sfence();
}


/*
    Lib = W7 M7
    Caller = mfxiYCbCr422_8u_C2P3R
*/
extern  void  mfxownYCbCr422ToYCbCr422_8u_C2P3R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize, int orderUV  )
{
  int h,w,width32,width8,width16;
  Ipp8u* pDstU = pDst[1];
  Ipp8u* pDstV = pDst[2];
  int dstStepU = dstStep[1], dstStepV = dstStep[2];
  int width  = roiSize.width ;
  int height = roiSize.height;
  __m128i mask = _mm_set1_epi32(0x00ff00ff);
  if( orderUV ){
    pDstU = pDst[2];
    pDstV = pDst[1];
    dstStepU = dstStep[2]; dstStepV = dstStep[1];
  }
  width32 = width & ~0x1f; width16 = width &  0x1f; width8  = width &  0x0f;
  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* src;
    Ipp8u* dsty,*dstu,*dstv;
    src = pSrc + h * srcStep;
    dsty = pDst[0] + h * dstStep[0];
    dstu = pDstU + h * dstStepU;
    dstv = pDstV + h * dstStepV;
    for( w = 0; w < width32; w += 32 )
    {
      __m128i t0,t1,t2,t3;
      __m128i t4,t5,t6,t7;
      t0 = _mm_loadu_si128((__m128i*) src);
      t1 = _mm_loadu_si128((__m128i*)(src+16));
      t2 = _mm_loadu_si128((__m128i*)(src+32));
      t3 = _mm_loadu_si128((__m128i*)(src+48));

      t4 = _mm_and_si128( t0, mask );
      t5 = _mm_and_si128( t1, mask );
      t6 = _mm_and_si128( t2, mask );
      t7 = _mm_and_si128( t3, mask );
      t4 = _mm_packus_epi16( t4, t5);
      t6 = _mm_packus_epi16( t6, t7);
      t0 = _mm_srli_epi16( t0 , 8 );
      t1 = _mm_srli_epi16( t1 , 8 );
      t2 = _mm_srli_epi16( t2 , 8 );
      t3 = _mm_srli_epi16( t3 , 8 );
      t0 = _mm_packus_epi16( t0, t1);
      t2 = _mm_packus_epi16( t2, t3);
      t1 = _mm_and_si128( t0, mask );
      t3 = _mm_and_si128( t2, mask );
      t1 = _mm_packus_epi16( t1, t3); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t2 = _mm_srli_epi16( t2 , 8 );
      t0 = _mm_packus_epi16( t0, t2); //V
      _mm_storeu_si128((__m128i*) (dsty) , t4);
      _mm_storeu_si128((__m128i*) (dsty+16), t6);
      _mm_storeu_si128((__m128i*) (dstu) , t1);
      _mm_storeu_si128((__m128i*) (dstv) , t0);
      src += 64;dsty += 32;dstu += 16;dstv += 16;
    }
    if( width16 >= 16 )
    {
      __m128i t0,t1;
      __m128i t4,t5;
      w += 16;
      t0 = _mm_loadu_si128((__m128i*) src);
      t1 = _mm_loadu_si128((__m128i*)(src+16));

      t4 = _mm_and_si128( t0, mask );
      t5 = _mm_and_si128( t1, mask );
      t4 = _mm_packus_epi16( t4, t5);

      t0 = _mm_srli_epi16( t0 , 8 );
      t1 = _mm_srli_epi16( t1 , 8 );
      t0 = _mm_packus_epi16( t0, t1);
      t1 = _mm_and_si128( t0, mask );
      t1 = _mm_packus_epi16( t1, t1); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t1); //V
      _mm_storeu_si128((__m128i*) (dsty) , t4);
      _mm_storel_epi64( (__m128i*) dstu , t1);
      _mm_storel_epi64( (__m128i*) dstv , t0);
      src += 32;dsty += 16;dstu += 8;dstv += 8;
     }
    if( width8 >= 8 )
    {
      __m128i t0, t1;
      __m128i t4;
      w += 8;
      t0 = _mm_loadu_si128((__m128i*) src);
      t4 = _mm_and_si128( t0, mask );
      t4 = _mm_packus_epi16( t4, t4);
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t0);
      t1 = _mm_and_si128( t0, mask );
      t1 = _mm_packus_epi16( t1, t1); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t1); //V
      _mm_storel_epi64( (__m128i*) dsty , t4);
      *(int*)dstu = _mm_cvtsi128_si32( t1 );
      *(int*)dstv = _mm_cvtsi128_si32( t0 );
      src += 16;dsty += 8;dstu += 4;dstv += 4;
    }
    for( ; w < width; w += 2 )
    {
      *dsty++ = *src++;
      *dstu++ = *src++;
      *dsty++ = *src++;
      *dstv++ = *src++;
    }
  }
}


/*
    Lib = W7 M7
    Caller = mfxiYCbCr422ToYCbCr420_8u_C2P3R
*/
extern void  mfxownYCbCr422ToYCbCr420_8u_C2P3R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize, int orderUV)
{
  int h,w,width32,width8,width16;
  Ipp8u* pDstU = pDst[1];
  Ipp8u* pDstV = pDst[2];
  int dstS = dstStep[0];
  int dstStepU = dstStep[1], dstStepV = dstStep[2];
  int width  = roiSize.width ;
  int height = roiSize.height;
  __m128i mask = _mm_set1_epi32(0x00ff00ff);
  if( orderUV ){
    pDstU = pDst[2];
    pDstV = pDst[1];
    dstStepU = dstStep[2]; dstStepV = dstStep[1];
  }
  width32 = width & ~0x3f; width16 = width &  0x1f; width8  = width &  0x0f;
  for( h = 0; h < height; h += 2 )
  {
    const Ipp8u* src;
    Ipp8u* dsty,*dstu,*dstv;
    src = pSrc + h * srcStep;
    dsty = pDst[0] + h * dstStep[0];
    dstu = pDstU + (h>>1) * dstStepU;
    dstv = pDstV + (h>>1) * dstStepV;
    for( w = 0; w < width32; w += 32 )
    {
      __m128i t0,t1,t2,t3;
      __m128i t4,t5,t6,t7;
      t0 = _mm_loadu_si128((__m128i*) src);
      t1 = _mm_loadu_si128((__m128i*)(src+16));
      t2 = _mm_loadu_si128((__m128i*)(src+32));
      t3 = _mm_loadu_si128((__m128i*)(src+48));

      t4 = _mm_and_si128( t0, mask );
      t5 = _mm_and_si128( t1, mask );
      t6 = _mm_and_si128( t2, mask );
      t7 = _mm_and_si128( t3, mask );
      t4 = _mm_packus_epi16( t4, t5);
      t6 = _mm_packus_epi16( t6, t7);
      t0 = _mm_srli_epi16( t0 , 8 );
      t1 = _mm_srli_epi16( t1 , 8 );
      t2 = _mm_srli_epi16( t2 , 8 );
      t3 = _mm_srli_epi16( t3 , 8 );
      t0 = _mm_packus_epi16( t0, t1);
      t2 = _mm_packus_epi16( t2, t3);
      t1 = _mm_and_si128( t0, mask );
      t3 = _mm_and_si128( t2, mask );
      t1 = _mm_packus_epi16( t1, t3); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t2 = _mm_srli_epi16( t2 , 8 );
      t0 = _mm_packus_epi16( t0, t2); //V
      _mm_storeu_si128((__m128i*) (dsty) , t4);
      _mm_storeu_si128((__m128i*) (dsty+16), t6);
      _mm_storeu_si128((__m128i*) (dstu) , t1);
      _mm_storeu_si128((__m128i*) (dstv) , t0);
      t0 = _mm_loadu_si128((__m128i*)(src+srcStep));
      t1 = _mm_loadu_si128((__m128i*)(src+16+srcStep));
      t2 = _mm_loadu_si128((__m128i*)(src+32+srcStep));
      t3 = _mm_loadu_si128((__m128i*)(src+48+srcStep));
      t4 = _mm_and_si128( t0, mask );
      t5 = _mm_and_si128( t1, mask );
      t6 = _mm_and_si128( t2, mask );
      t7 = _mm_and_si128( t3, mask );
      t4 = _mm_packus_epi16( t4, t5);
      t6 = _mm_packus_epi16( t6, t7);
      _mm_storeu_si128((__m128i*) (dsty+dstS)   , t4);
      _mm_storeu_si128((__m128i*) (dsty+16+dstS), t6);
      src += 64;dsty += 32;dstu += 16;dstv += 16;
    }
    if( width16 >= 16 )
    {
      __m128i t0,t1;
      __m128i t4,t5;
      w += 16;
      t0 = _mm_loadu_si128((__m128i*) src);
      t1 = _mm_loadu_si128((__m128i*)(src+16));

      t4 = _mm_and_si128( t0, mask );
      t5 = _mm_and_si128( t1, mask );
      t4 = _mm_packus_epi16( t4, t5);

      t0 = _mm_srli_epi16( t0 , 8 );
      t1 = _mm_srli_epi16( t1 , 8 );
      t0 = _mm_packus_epi16( t0, t1);
      t1 = _mm_and_si128( t0, mask );
      t1 = _mm_packus_epi16( t1, t1); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t1); //V
      _mm_storeu_si128((__m128i*) (dsty) , t4);
      _mm_storel_epi64( (__m128i*) dstu , t1);
      _mm_storel_epi64( (__m128i*) dstv , t0);
      t0 = _mm_loadu_si128((__m128i*)(src+srcStep));
      t1 = _mm_loadu_si128((__m128i*)(src+16+srcStep));
      t4 = _mm_and_si128( t0, mask );
      t5 = _mm_and_si128( t1, mask );
      t4 = _mm_packus_epi16( t4, t5);
      _mm_storeu_si128((__m128i*) (dsty+dstS)   , t4);
      src += 32;dsty += 16;dstu += 8;dstv += 8;
     }
    if( width8 >= 8 )
    {
      __m128i t0, t1;
      __m128i t4;
      w += 8;
      t0 = _mm_loadu_si128((__m128i*) src);
      t4 = _mm_and_si128( t0, mask );
      t4 = _mm_packus_epi16( t4, t4);
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t0);
      t1 = _mm_and_si128( t0, mask );
      t1 = _mm_packus_epi16( t1, t1); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t1); //V
      _mm_storel_epi64( (__m128i*) dsty , t4);
      *(int*)dstu = _mm_cvtsi128_si32( t1 );
      *(int*)dstv = _mm_cvtsi128_si32( t0 );
      t0 = _mm_loadu_si128((__m128i*)(src+srcStep));
      t4 = _mm_and_si128( t0, mask );
      t4 = _mm_packus_epi16( t4, t4);
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t0);
      t1 = _mm_and_si128( t0, mask );
      t1 = _mm_packus_epi16( t1, t1); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t1); //V
      _mm_storel_epi64( (__m128i*) (dsty+dstS), t4);
      src += 16;dsty += 8;dstu += 4;dstv += 4;
    }
    for( ; w < width; w += 2 )
    {
      dsty[0] = src[0];
      dsty[0 + dstStep[0]] = src[0 + srcStep];
      dsty[1] = src[2];
      dsty[1 + dstStep[0]] = src[2 + srcStep];
      *dstu++  = src[1];
      *dstv++  = src[3];
      dsty += 2; src += 4;
    }
  }
}



/*
    Lib = W7 M7
    Caller = mfxiYCbCr420_8u_P2P3R
*/
extern void mfxownYCbCr420ToYCbCr420_8u_P2P3R(const Ipp8u* pSrcY,int srcYStep,const Ipp8u* pSrcUV,
                                  int srcUVStep, Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize )
{
  int h,w,width32,width16,width8;
  int width  = roiSize.width ;
  int height = roiSize.height;
//  int nonTemporal = 0;
  int dStep = dstStep[0];
  const Ipp8u* sRc = pSrcY;
  __m128i mask = _mm_set1_epi32(0x00ff00ff);
  Ipp8u* dSt = pDst[0];
  /* Y plane */
 // if( width*height > 348602 /* 131072 */ ) nonTemporal = 1;
  for( h = 0; h < height; h++ )
  {
#if ( (_IPP >= _IPP_W7) )
      mfxownccCopy_8u_C1_W7( sRc, dSt, width, 0 );
#elif ( (_IPP32E >= _IPP32E_M7) )
      mfxownccCopy_8u_C1_M7( sRc, dSt, width, 0 );
#endif
      sRc += srcYStep, dSt += dStep;
  }
  height /= 2;width  /= 2;
  width32 = width & ~0x1f;width16 = width &  0x1f; width8  = width &  0x0f;
  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* srcuv;
    Ipp8u* dstu;
    Ipp8u* dstv;
    srcuv = pSrcUV + h *  srcUVStep ;
    dstu = pDst[1] + h *  dstStep[1];
    dstv = pDst[2] + h *  dstStep[2];
    for( w = 0; w < width32; w += 32 )
    {
      __m128i t0,t1,t2,t3;
      __m128i t4,t5,t6,t7;
      t0 = _mm_loadu_si128((__m128i*) srcuv );
      t1 = _mm_loadu_si128((__m128i*)(srcuv+16 ));
      t2 = _mm_loadu_si128((__m128i*)(srcuv+32 ));
      t3 = _mm_loadu_si128((__m128i*)(srcuv+48 ));
      t4 = _mm_and_si128( t0, mask );
      t5 = _mm_and_si128( t1, mask );
      t6 = _mm_and_si128( t2, mask );
      t7 = _mm_and_si128( t3, mask );
      t4 = _mm_packus_epi16( t4, t5);//u
      t6 = _mm_packus_epi16( t6, t7);//u
      t0 = _mm_srli_epi16( t0 , 8 );
      t1 = _mm_srli_epi16( t1 , 8 );
      t2 = _mm_srli_epi16( t2 , 8 );
      t3 = _mm_srli_epi16( t3 , 8 );
      t0 = _mm_packus_epi16( t0, t1);
      t2 = _mm_packus_epi16( t2, t3);//v
      _mm_storeu_si128((__m128i*) dstu    , t4);
      _mm_storeu_si128((__m128i*)(dstu+16), t6);
      _mm_storeu_si128((__m128i*) dstv    , t0);
      _mm_storeu_si128((__m128i*)(dstv+16), t2);
      dstu += 32; srcuv += 64;dstv += 32;
    }
    if( width16 >= 16 )
    {
      __m128i t0,t1;
      __m128i t4,t5;
      w += 16;
      t0 = _mm_loadu_si128((__m128i*) srcuv );
      t1 = _mm_loadu_si128((__m128i*)(srcuv+16 ));
      t4 = _mm_and_si128( t0, mask );
      t5 = _mm_and_si128( t1, mask );
      t4 = _mm_packus_epi16( t4, t5);//u
      t0 = _mm_srli_epi16( t0 , 8 );
      t1 = _mm_srli_epi16( t1 , 8 );
      t0 = _mm_packus_epi16( t0, t1);
      _mm_storeu_si128((__m128i*) dstu    , t4);
       _mm_storeu_si128((__m128i*) dstv    , t0);
       dstu += 16; srcuv += 32;dstv += 16;
    }
    if( width8 >= 8 )
    {
      __m128i t0;
      __m128i t4;
      w += 8;
      t0 = _mm_loadu_si128((__m128i*) srcuv );
      t4 = _mm_and_si128( t0, mask );
      t4 = _mm_packus_epi16( t4, t4);//u
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t0);
      _mm_storel_epi64((__m128i*) dstu , t4);
      _mm_storel_epi64((__m128i*) dstv , t0);
       dstu += 8; srcuv += 16;dstv += 8;
    }
    for( ; w < width; w ++ )
    {
      *dstu++    = srcuv[0];
      *dstv++    = srcuv[1];
      srcuv += 2;
    }
  }
}




/*
    Lib = W7 M7
    Caller = mfxiCbYCr422ToYCbCr422_8u_C2P3R
*/
extern  void  mfxownCbYCr422ToYCbCr422_8u_C2P3R( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize  )
{
  int h,w,width32,width8,width16;
  Ipp8u* pDstU = pDst[1];
  Ipp8u* pDstV = pDst[2];
  int dstStepU = dstStep[1], dstStepV = dstStep[2];
  int width  = roiSize.width ;
  int height = roiSize.height;
  __m128i mask = _mm_set1_epi32(0x00ff00ff);
  width32 = width & ~0x1f; width16 = width &  0x1f; width8  = width &  0x0f;
  for( h = 0; h < height; h ++ )
  {
    const Ipp8u* src;
    Ipp8u* dsty,*dstu,*dstv;
    src = pSrc + h * srcStep;
    dsty = pDst[0] + h * dstStep[0];
    dstu = pDstU + h * dstStepU;
    dstv = pDstV + h * dstStepV;
    for( w = 0; w < width32; w += 32 )
    {
      __m128i t0,t1,t2,t3;
      __m128i t4,t5,t6,t7;
      t0 = _mm_loadu_si128((__m128i*) src);
      t1 = _mm_loadu_si128((__m128i*)(src+16));
      t2 = _mm_loadu_si128((__m128i*)(src+32));
      t3 = _mm_loadu_si128((__m128i*)(src+48));
      t4 = _mm_srli_epi16( t0 , 8 ); //Y
      t5 = _mm_srli_epi16( t1 , 8 );
      t6 = _mm_srli_epi16( t2 , 8 );
      t7 = _mm_srli_epi16( t3 , 8 );
      t4 = _mm_packus_epi16( t4, t5);
      t6 = _mm_packus_epi16( t6, t7);
      t0 = _mm_and_si128( t0, mask ); // V U
      t1 = _mm_and_si128( t1, mask );
      t2 = _mm_and_si128( t2, mask );
      t3 = _mm_and_si128( t3, mask );
      t0 = _mm_packus_epi16( t0, t1);
      t2 = _mm_packus_epi16( t2, t3);
      t1 = _mm_and_si128( t0, mask );
      t3 = _mm_and_si128( t2, mask );
      t1 = _mm_packus_epi16( t1, t3); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t2 = _mm_srli_epi16( t2 , 8 );
      t0 = _mm_packus_epi16( t0, t2); //V
      _mm_storeu_si128((__m128i*) (dsty) , t4);
      _mm_storeu_si128((__m128i*) (dsty+16), t6);
      _mm_storeu_si128((__m128i*) (dstu) , t1);
      _mm_storeu_si128((__m128i*) (dstv) , t0);
      src += 64;dsty += 32;dstu += 16;dstv += 16;
    }
    if( width16 >= 16 )
    {
      __m128i t0,t1;
      __m128i t4,t5;
      w += 16;
      t0 = _mm_loadu_si128((__m128i*) src);
      t1 = _mm_loadu_si128((__m128i*)(src+16));
      t4 = _mm_srli_epi16( t0 , 8 ); //Y
      t5 = _mm_srli_epi16( t1 , 8 );
      t4 = _mm_packus_epi16( t4, t5);
      t0 = _mm_and_si128( t0, mask ); // V U
      t1 = _mm_and_si128( t1, mask );
      t0 = _mm_packus_epi16( t0, t1);
      t1 = _mm_and_si128( t0, mask );
      t1 = _mm_packus_epi16( t1, t1); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t1); //V
      _mm_storeu_si128((__m128i*) (dsty) , t4);
      _mm_storel_epi64( (__m128i*) dstu , t1);
      _mm_storel_epi64( (__m128i*) dstv , t0);
      src += 32;dsty += 16;dstu += 8;dstv += 8;
     }
    if( width8 >= 8 )
    {
      __m128i t0, t1;
      __m128i t4;
      w += 8;
      t0 = _mm_loadu_si128((__m128i*) src);
      t4 = _mm_srli_epi16( t0 , 8 ); //Y
      t4 = _mm_packus_epi16( t4, t4);
      t0 = _mm_and_si128( t0, mask ); // V U
      t0 = _mm_packus_epi16( t0, t0);
      t1 = _mm_and_si128( t0, mask );
      t1 = _mm_packus_epi16( t1, t1); //U
      t0 = _mm_srli_epi16( t0 , 8 );
      t0 = _mm_packus_epi16( t0, t1); //V
      _mm_storel_epi64( (__m128i*) dsty , t4);
      *(int*)dstu = _mm_cvtsi128_si32( t1 );
      *(int*)dstv = _mm_cvtsi128_si32( t0 );
      src += 16;dsty += 8;dstu += 4;dstv += 4;
    }
    for( ; w < width; w += 2 )
    {
      *dstu++ = *src++;
      *dsty++ = *src++;
      *dstv++ = *src++;
      *dsty++ = *src++;
    }
  }
}


#endif
