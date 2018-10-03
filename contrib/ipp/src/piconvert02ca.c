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
#include "owni.h"
#include "ippcore.h"

#if !((_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9))

#if !( (_IPP>=_IPP_M6) || (_IPP32E>=_IPP32E_M7) || (_IPP64>=_IPP64_I7) )
#define _CCODE
#endif

#if ( _IPPLP32 == _IPPLP32_S8 ) || ( _IPPLP64 == _IPPLP64_N8 ) || ( _IPP <= _IPP_W7 )
#define BOUND_NT  524288
#else
#define BOUND_NT  1048576
#endif

#if ( _IPP32E >= _IPP32E_M7 )
extern void mfxowniConvert_8u16u_M7
            ( const Ipp8u *pSrc, Ipp16u *pDst, int len, int nonTemporal );
#endif
#if ( _IPP >= _IPP_W7 )
extern void mfxowniConvert_8u16u_W7
            ( const Ipp8u *pSrc, Ipp16u *pDst, int len, int nonTemporal );
#endif
#if   defined (_A6)
extern void mfxowniConvert_8u16u_A6( const Ipp8u *pSrc, Ipp16u *pDst, int len );
#endif
#if (_IPPLRB>=_IPPLRB_B1)
extern void mfxowniConvert_8u16u( const Ipp8u *pSrc, int srcStep, Ipp16u *pDst, int dstStep,
                              int width, int height, int nCh );
#endif

#endif

/*******************************************************************/
IPPFUN ( IppStatus, mfxiConvert_8u16u_C1R,
        ( const Ipp8u* __RESTRICT pSrc, int srcStep, Ipp16u* __RESTRICT pDst, int dstStep,
          IppiSize roiSize )) {

#if !((_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9))

#if !(_IPPLRB>=_IPPLRB_B1)
    const Ipp8u* __RESTRICT src = pSrc;
    Ipp16u* __RESTRICT dst = pDst;
    int width = roiSize.width, height = roiSize.height;
    int h;
#if defined (_CCODE) || defined (_M6) || defined (_I7)
    int n;
#endif
#if ( _IPP >= _IPP_W7 ) || ( _IPP32E >= _IPP32E_M7 )
    IppStatus ownStatus = ippStsNoErr;
    int nonTemporal = 0, size = 0, cache = 0;
#endif
#endif

    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_SIZE_RET( roiSize.width )
    IPP_BAD_SIZE_RET( roiSize.height )
    IPP_BAD_STEP_RET( srcStep )
    IPP_BAD_STEP_RET( dstStep )

#if !(_IPPLRB>=_IPPLRB_B1)
#if ( _IPP >= _IPP_W7 ) || ( _IPP32E >= _IPP32E_M7 )
    size = width*height*3;
    if( size > BOUND_NT ) {
      ownStatus = mfxGetMaxCacheSizeB( &cache );
      if( ownStatus == ippStsNoErr  ) {
        if( size >= cache ) nonTemporal = 1;
      }
    }
#endif
    if( (srcStep*2 == dstStep) && (srcStep == width) ) {
        width *= height;
        height = 1;
    }

    for( h = 0; h < height; h++ ) {
        #if ( _IPP32E >= _IPP32E_M7 )
        mfxowniConvert_8u16u_M7( src, dst, width, nonTemporal );
        #elif ( _IPP >= _IPP_W7 )
        mfxowniConvert_8u16u_W7( src, dst, width, nonTemporal );
        #elif defined (_A6)
        mfxowniConvert_8u16u_A6( src, dst, width );
        #else
        for( n = 0; n < width; n++ ) {
            dst[n] = (Ipp16u)src[n];
        }
        #endif
        src += srcStep;
        dst = (Ipp16u*)((Ipp8u*)dst + dstStep);
    }
#endif

#if (_IPPLRB>=_IPPLRB_B1)
    mfxowniConvert_8u16u( pSrc, srcStep, pDst, dstStep, roiSize.width, roiSize.height, 1 );
#endif

    return ippStsNoErr;

#else
/* ((_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9)) */
    return mfxiConvert_8u16s_C1R(pSrc, srcStep, (Ipp16s *)pDst, dstStep, roiSize);

#endif

}



