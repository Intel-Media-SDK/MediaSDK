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
//      Purpose : IPPI Color Space Conversion
// 
// 
*/



#include <emmintrin.h> // __m128i etc
#include "owncc.h"

#if ( (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7) )

#if(!( (_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9)))
/* Lib = W7 M7 */
/* Caller = mfxiYCbCr420ToYCbCr422_8u_P2C2R */
extern void  mfxmyNV12ToYUY2_8u_P2C2R(const Ipp8u* pSrcY, int srcYStep,const Ipp8u* pSrcUV, int srcUVStep, Ipp8u* pDst, int dstStep, IppiSize roiSize)
{
  int h,w,width8;
  int width  = roiSize.width ;
  int height = roiSize.height;
  width8  =  width & ~0x1f;

  for( h = 0;h < height; h += 2 )
  {
    const Ipp8u* srcy;
    const Ipp8u* srcu;
    Ipp8u*  dst;
    srcu = pSrcUV;
    srcy = pSrcY;dst  = pDst;
    for(w = 0; w < width8; w += 32 )
    {
      __m128i mSrcy;
      __m128i mSrcy1;
      mSrcy = _mm_loadu_si128((__m128i*)(srcy));
      mSrcy1= _mm_loadu_si128((__m128i*)(srcy+16));
      _mm_storeu_si128((__m128i*) dst    ,_mm_unpacklo_epi8(mSrcy,_mm_loadu_si128((__m128i*)(srcu))));
      _mm_storeu_si128((__m128i*)(dst+16),_mm_unpackhi_epi8(mSrcy,_mm_loadu_si128((__m128i*)(srcu))));
      _mm_storeu_si128((__m128i*)(dst+32),_mm_unpacklo_epi8(mSrcy1,_mm_loadu_si128((__m128i*)(srcu+16))));
      _mm_storeu_si128((__m128i*)(dst+48),_mm_unpackhi_epi8(mSrcy1,_mm_loadu_si128((__m128i*)(srcu+16))));

      mSrcy = _mm_loadu_si128((__m128i*)(srcy+srcYStep));
      mSrcy1= _mm_loadu_si128((__m128i*)(srcy+16+srcYStep));
      _mm_storeu_si128((__m128i*)(dst+   dstStep),_mm_unpacklo_epi8(mSrcy,_mm_loadu_si128((__m128i*)(srcu))));
      _mm_storeu_si128((__m128i*)(dst+16+dstStep),_mm_unpackhi_epi8(mSrcy,_mm_loadu_si128((__m128i*)(srcu))));
      _mm_storeu_si128((__m128i*)(dst+32+dstStep),_mm_unpacklo_epi8(mSrcy1,_mm_loadu_si128((__m128i*)(srcu+16))));
      _mm_storeu_si128((__m128i*)(dst+48+dstStep),_mm_unpackhi_epi8(mSrcy1,_mm_loadu_si128((__m128i*)(srcu+16))));
      srcy += 32;
      srcu += 32;
      dst += 64;
    }
    if(( width & 0xf )>= 0xf)
    {
      __m128i mSrcy;
      mSrcy = _mm_loadu_si128((__m128i*)(srcy));
      _mm_storeu_si128((__m128i*) dst    ,_mm_unpacklo_epi8(mSrcy,_mm_loadu_si128((__m128i*)(srcu))));
      _mm_storeu_si128((__m128i*)(dst+16),_mm_unpackhi_epi8(mSrcy,_mm_loadu_si128((__m128i*)(srcu))));
      mSrcy = _mm_loadu_si128((__m128i*)(srcy+srcYStep));
      _mm_storeu_si128((__m128i*)(dst+   dstStep),_mm_unpacklo_epi8(mSrcy,_mm_loadu_si128((__m128i*)(srcu))));
      _mm_storeu_si128((__m128i*)(dst+16+dstStep),_mm_unpackhi_epi8(mSrcy,_mm_loadu_si128((__m128i*)(srcu))));
      srcy += 16;
      srcu += 16;
      dst  += 32;
      w += 16;
    }
    for( ; w < width; w += 2 )
    {
      dst[0] = srcy[0]; dst[2] =  srcy[1];
      dst[1] = srcu[0]; dst[3] =  srcu[1];

      dst[0+dstStep] = srcy[0+srcYStep]; dst[2+dstStep]  =  srcy[1+srcYStep];
      dst[1+dstStep] = srcu[0]; dst[3+dstStep] =  srcu[1];
      dst+=4; srcy+= 2; srcu+= 2;
    }
    pDst   = pDst  + 2*dstStep;
    pSrcY  = pSrcY + 2*srcYStep;
    pSrcUV = pSrcUV + srcUVStep;
  }
}
#endif//(!( (_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9)))


#if(!( (_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9)))
/* Lib = W7 M7 */
/* Caller = mfxiYCrCb420ToYCbCr422_8u_P3C2R */
/* YV12( YCrCb420_P3 ) > YUY2( YCbCr422_C2 ) */
extern void  mfxmyYV12ToYUY2420_8u_P3C2R(const Ipp8u* pSrc[3],int srcStep[3], Ipp8u* pDst, int dstStep, IppiSize roiSize )
{
  int h,w,width8;
  int width  = roiSize.width ;
  int height = roiSize.height;
  const Ipp8u* pSrcY = pSrc[0];
  const Ipp8u* pSrcU = pSrc[2];
  const Ipp8u* pSrcV = pSrc[1];
  width8  =  width & ~0x1f;
  for( h = 0;h < height; h += 2 )
  {
    const Ipp8u* srcy;
    const Ipp8u* srcy1;
    const Ipp8u* srcu;
    const Ipp8u* srcv;
    Ipp8u* dst;
    Ipp8u* dst1;
    srcy = pSrcY; srcy1 = pSrcY + srcStep[0];
    srcu = pSrcU;
    srcv = pSrcV;
    dst  = pDst;  dst1  = pDst + dstStep;
    for(w = 0; w < width8; w += 32 )
    {
      __m128i eSrcy,eSrcy1,eSrcu,eSrcv;
      __m128i eT0,eT1;
      eSrcy = _mm_loadu_si128((__m128i*)( srcy ));
      eSrcy1= _mm_loadu_si128((__m128i*)( srcy + 16 ));

      eSrcu = _mm_loadu_si128((__m128i*)(srcu));
      eSrcv = _mm_loadu_si128((__m128i*)(srcv));
      eT0   = _mm_unpacklo_epi8( eSrcu, eSrcv);
      eT1   = _mm_unpacklo_epi8( eSrcy, eT0);                         /* Y7v3Y6u3   ... Y3v1Y2u1 Y1v0Y0u0 */
      _mm_storeu_si128((__m128i*)dst  , eT1);
      eT0   = _mm_unpackhi_epi64( eT0, eT0);
      eT0   = _mm_unpacklo_epi8( _mm_unpackhi_epi64(eSrcy,eSrcy), eT0); /* Y15v7Y15u7 ... Y3v9Y2u9 Y8v4Y8u4 */
      _mm_storeu_si128((__m128i*)(dst+16),eT0);

      eSrcy = eSrcy1;
      eT0   = _mm_unpackhi_epi8( eSrcu, eSrcv);
      eT1   = _mm_unpacklo_epi8( eSrcy, eT0 );                         /* Y7v3Y6u3   ... Y3v9Y2u9 Y1v8Y0u8 */
      _mm_storeu_si128((__m128i*)(dst+32), eT1);
      eT0   = _mm_unpackhi_epi64( eT0, eT0);
      eT0   = _mm_unpacklo_epi8(  _mm_unpackhi_epi64(eSrcy,eSrcy), eT0); /* Y15v7Y15u7 ... Y3v9Y2u9 Y8v4Y8u4 */
      _mm_storeu_si128((__m128i*)(dst+48),eT0);

      eSrcy = _mm_loadu_si128((__m128i*)( srcy1 ));
      eSrcy1= _mm_loadu_si128((__m128i*)( srcy1 + 16 ));
      eT0   = _mm_unpacklo_epi8( eSrcu, eSrcv);
      eT1   = _mm_unpacklo_epi8( eSrcy, eT0   );                         /* Y7v3Y6u3   ... Y3v1Y2u1 Y1v0Y0u0 */
      _mm_storeu_si128((__m128i*)dst1  , eT1);
      eT0   = _mm_unpackhi_epi64( eT0, eT0);
      eT0   = _mm_unpacklo_epi8(  _mm_unpackhi_epi64(eSrcy,eSrcy), eT0); /* Y15v7Y15u7 ... Y3v9Y2u9 Y8v4Y8u4 */
      _mm_storeu_si128((__m128i*)(dst1+16),eT0);

      eSrcy = eSrcy1;
      eT0   = _mm_unpackhi_epi8( eSrcu, eSrcv);
      eT1   = _mm_unpacklo_epi8( eSrcy, eT0   );                         /* Y7v3Y6u3   ... Y3v9Y2u9 Y1v8Y0u8 */
      _mm_storeu_si128((__m128i*)(dst1+32), eT1);
      eT0   = _mm_unpackhi_epi64( eT0, eT0);
      eT0   = _mm_unpacklo_epi8(  _mm_unpackhi_epi64(eSrcy,eSrcy), eT0); /* Y15v7Y15u7 ... Y3v9Y2u9 Y8v4Y8u4 */
      _mm_storeu_si128((__m128i*)(dst1+48),eT0);
      srcy += 32;
      srcy1+= 32;
      srcu += 16;
      srcv += 16;
      dst += 64; dst1 += 64;
    }
    for( ; w < width; w += 2 )
    {
      dst[0]  = *srcy++;   dst[1]  = *srcu; dst[2]  = *srcy++;   dst[3]  = *srcv;
      dst1[0] = *srcy1++;  dst1[1] = *srcu++; dst1[2] = *srcy1++;  dst1[3] = *srcv++;
      dst += 4;dst1 += 4;
    }
    pSrcY += srcStep[0];
    pSrcY += srcStep[0];
    pDst  += dstStep;
    pDst  += dstStep;
    pSrcU += srcStep[1];
    pSrcV += srcStep[2];
  }
}
#endif//(!( (_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9)))




#endif
