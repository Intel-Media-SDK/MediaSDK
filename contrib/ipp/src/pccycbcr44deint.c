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
//      Purpose : IPPI color space convert
// 
//   Contents:   NV12(YCrCb)To YUV2(CrYCbY) YV12(YCrCb)
// 
*/



#include "owncc.h"

#if !( (_IPP > _IPP_A6) || (_IPP32E>=_IPP32E_M7) )

static void  mfxmyNV12ToYUY2_8u_P2C2R(const Ipp8u* pSrcY, int srcYStep,const Ipp8u* pSrcUV, int srcUVStep, Ipp8u* pDst, int dstStep, IppiSize roiSize)
{
  int h,i,w;
  int width  = roiSize.width ;
  int height = roiSize.height;
  for( h = 0;h < height; h += 2 )
  {
    const Ipp8u* srcy;
    const Ipp8u* srcu;
    Ipp8u*  dst;
    for( i = 0; i < 2; i++ )
    {
      srcy = pSrcY;dst  = pDst;srcu = pSrcUV;
      for( w = 0; w < width; w += 2 )
      {
        dst[0] = srcy[0]; dst[2] =  srcy[1];
        dst[1] = srcu[0]; dst[3] =  srcu[1];
        dst+=4; srcy+= 2; srcu+= 2;
      }
      pDst  = pDst  + dstStep;
      pSrcY = pSrcY + srcYStep;
    }
    pSrcUV = pSrcUV + srcUVStep;
  }
}

/* YV12( YCrCb420_P3 ) > YUY2( YCbCr422_C2 ) */
static void  mfxmyYV12ToYUY2420_8u_P3C2R(const Ipp8u* pSrc[3],int srcStep[3], Ipp8u* pDst, int dstStep, IppiSize roiSize )
{
  int h,w;
  int width  = roiSize.width ;
  int height = roiSize.height;
  const Ipp8u* pSrcY = pSrc[0];
  const Ipp8u* pSrcU = pSrc[2];//???
  const Ipp8u* pSrcV = pSrc[1];

  for( h = 0;h < height; h += 2 )
  {
    const Ipp8u* srcy;
    const Ipp8u* srcu;
    const Ipp8u* srcv;
    Ipp8u* dst;
    srcy = pSrcY; srcu = pSrcU; srcv = pSrcV;
    dst  = pDst;
    for( w = 0; w < width; w += 2 )
    {
      dst[0] = *srcy++;  dst[1] = *srcu++;  dst[2] = *srcy++;  dst[3] = *srcv++;
      dst += 4;
    }
    pSrcY += srcStep[0];
    pDst  += dstStep;
    srcy = pSrcY; srcu = pSrcU; srcv = pSrcV;
    dst  = pDst;
    for( w = 0; w < width; w += 2 )
    {
      dst[0] = *srcy++;  dst[1] = *srcu++;  dst[2] = *srcy++;  dst[3] = *srcv++;
      dst += 4;
    }
    pSrcY += srcStep[0];
    pSrcU += srcStep[2];
    pSrcV += srcStep[1];
    pDst  += dstStep;
  }
}


#else

#if(!( (_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9)))
extern void  mfxmyYV12ToYUY2420_8u_P3C2R(const Ipp8u* pSrc[3],int srcStep[3], Ipp8u* pDst, int dstStep, IppiSize roiSize );
extern void  mfxmyNV12ToYUY2_8u_P2C2R(const Ipp8u* pSrcY, int srcYStep,const  Ipp8u* pSrcUV, int srcUVStep, Ipp8u* pDst, int dstStep, IppiSize roiSize);
#endif//(!( (_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9)))

#endif

#if(!( (_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9)))
/* YV12( YCrCb420_P3 ) > YUY2( YCbCr422_C2 )
   formerly mfxiYVToYU420_8u_P3C2R
*/
IPPFUN(IppStatus, mfxiYCrCb420ToYCbCr422_8u_P3C2R,( const Ipp8u* pSrc[3],int srcStep[3], Ipp8u* pDst, int dstStep, IppiSize roiSize ))
{
  IPP_BAD_PTR2_RET( pSrc, pDst );
  IPP_BAD_PTR3_RET( pSrc[0], pSrc[1], pSrc[2]);
  IPP_BADARG_RET((roiSize.width  < 2), ippStsSizeErr);
  IPP_BADARG_RET((roiSize.height < 2), ippStsSizeErr);
  roiSize.width  &= ~1;
  roiSize.height &= ~1;
  mfxmyYV12ToYUY2420_8u_P3C2R( pSrc, srcStep, pDst, dstStep, roiSize);
  return ippStsNoErr;
}
/*  NV12( YCbCr420_P2 ) > YUY2( YCbCr422_C2 )
  formerly mfxiJoin420_8u_P2C2R
*/
IPPFUN(IppStatus, mfxiYCbCr420ToYCbCr422_8u_P2C2R,(const Ipp8u* pSrcY, int srcYStep,const Ipp8u* pSrcUV,
                  int srcUVStep, Ipp8u* pDst, int dstStep, IppiSize roiSize))
{
  ownpi_CnvrtEpilog3(pSrcY, pSrcUV, pDst, roiSize);
  IPP_BADARG_RET((roiSize.width  < 2), ippStsSizeErr);
  IPP_BADARG_RET((roiSize.height < 2), ippStsSizeErr);
  roiSize.width &= ~1;
  roiSize.height &= ~1;
  mfxmyNV12ToYUY2_8u_P2C2R( pSrcY, srcYStep, pSrcUV, srcUVStep, pDst, dstStep, roiSize);
  return ippStsNoErr;
}
#endif//(!( (_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9)))

