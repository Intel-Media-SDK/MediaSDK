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
//    Color conversions functions, back/forward transform
//
//  Contents:
//    mfxiYCbCrToBGR_JPEG_8u_P3C4R
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#define CLIP(x) ((x < 0) ? 0 : ((x > 255) ? 255 : x))

#if ( _IPP >= _IPP_V8 )||( _IPP32E >= _IPP32E_U8 )
extern void mfxownRGBToYCbCr_JPEG_8u_C4P3R(
const Ipp8u* pBGR, int bgrStep, Ipp8u* pYCC[3], int yccStep, IppiSize roiSize);
extern void mfxownYCbCrToBGR_JPEG_8u_P3C4R(
const Ipp8u* pYCC[3],int yccStep,Ipp8u* pBGR,int bgrStep,IppiSize roiSize, Ipp8u aval,int orger);
#endif
#define kRCr 0x000166e8
#define kGCr 0x0000b6d1
#define kGCb 0x00005819
#define kBCb 0x0001c5a0
#define kR   0x00b37408
#define kG   0x00877530
#define kB   0x00e2d002


/* ---------------------- library functions definitions -------------------- */

IPPFUN(IppStatus, mfxiRGBToYCbCr_JPEG_8u_C4P3R,(
const Ipp8u* pBGR, int bgrStep, Ipp8u* pYCC[3], int yccStep, IppiSize roiSize))
{
  IPP_BAD_PTR2_RET( pBGR, pYCC );
  IPP_BAD_PTR3_RET( pYCC[0], pYCC[1], pYCC[2]);
  IPP_BADARG_RET((roiSize.width < 2 || roiSize.height < 1), ippStsSizeErr);
  IPP_BADARG_RET(( bgrStep == 0 || yccStep == 0), ippStsStepErr);
#if ( _IPP >= _IPP_V8 )||( _IPP32E >= _IPP32E_U8 )
  mfxownRGBToYCbCr_JPEG_8u_C4P3R( pBGR, bgrStep, pYCC, yccStep, roiSize);
#else
  {
     int h,w;
     int width = roiSize.width;
     Ipp8u* pDstU = pYCC[1];
     Ipp8u* pDstV = pYCC[2];
     int height = roiSize.height;

     for( h = 0; h < height; h ++ )
     {
        const Ipp8u* src;
        Ipp8u* dsty,*dstu,*dstv;
        src  = pBGR   + h * bgrStep;
        dsty = pYCC[0]+ h * yccStep;
        dstu = pDstU  + h * yccStep;
        dstv = pDstV  + h * yccStep;
        for( w = 0; w < width; w ++ )
        {
           int g,r,b;
           b = src[2];g = src[1];r = src[0];
           src += 4;
           *dsty++ = ( Ipp8u )(( iRY * r + iGY * g + iBY * b + 0x0008000) >> 16 );
           *dstu++ = ( Ipp8u )((-iRu * r - iGu * g + iBu * b + 0x0804000) >> 16 ); /* Cb */
           *dstv++ = ( Ipp8u )(( iBu * r - iGv * g - iBv * b + 0x0804000) >> 16 ); /* Cr */
        }
     }
  }
#endif
  return ippStsNoErr;
}

IPPFUN(IppStatus,mfxiYCbCrToBGR_JPEG_8u_P3C4R,(
const Ipp8u* pYCC[3],int yccStep,Ipp8u* pBGR,int bgrStep,IppiSize roiSize, Ipp8u aval))
{
  IPP_BAD_PTR2_RET( pBGR, pYCC );
  IPP_BAD_PTR3_RET( pYCC[0], pYCC[1], pYCC[2]);
  IPP_BADARG_RET((roiSize.width < 2 || roiSize.height < 1), ippStsSizeErr);
  IPP_BADARG_RET(( yccStep == 0 || bgrStep == 0 ), ippStsStepErr);
#if ( _IPP >= _IPP_V8 )||( _IPP32E >= _IPP32E_U8 )
  mfxownYCbCrToBGR_JPEG_8u_P3C4R( pYCC, yccStep, pBGR, bgrStep, roiSize, aval, 1 );
#else
  {
     int h, w;
     int width = roiSize.width;
     for(h = 0; h < roiSize.height; h++ )
     {
        const Ipp8u*  srcy = pYCC[0] + h * yccStep;
        const Ipp8u*  srcu = pYCC[1] + h * yccStep;
        const Ipp8u*  srcv = pYCC[2] + h * yccStep;
              Ipp8u*  dst  = pBGR    + h * bgrStep;
        for(w = 0; w < width; w++)
        {
           int Y0,Cb, Cr;
           Y0 = (int)(srcy[0])<<16;
           Cb  = srcu[0];Cr = srcv[0];
           dst[2] = ( Ipp8u )CLIP(((Y0 + kRCr * Cr - kR + 0x00008000)>>16));
           dst[1] = ( Ipp8u )CLIP(((Y0 - kGCb * Cb - kGCr * Cr + kG + 0x00008000 ) >> 16 ));
           dst[0] = ( Ipp8u )CLIP(((Y0 + kBCb * Cb - kB + 0x00008000 )>>16));
           dst[3] = aval;
           dst+=4;srcy++;srcu++;srcv++;
        }
     }
  }
#endif
  return ippStsNoErr;
}

