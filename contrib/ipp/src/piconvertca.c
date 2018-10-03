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

// XXX cleanup

/*M*
//  Purpose:    Convert:    8u<=>16u, 8u<=>16s,
//                          8u<=>32f, 16u<=>32f,
//                          8s<=>32f, 16s<=>32f
//                          8s<=>32s, 8u<=>32s
//
//  Contents:
//         (M7) mfxiConvert_16u8u_C1R   (A6,W7,T7)      NT
//              mfxiConvert_16u8u_AC4R
//         (M7) mfxiConvert_16u8u_C4R   (A6,W7,T7)      NT
//
*M*/

#include "precomp.h"
#include "owni.h"
//#if defined (_PX) || defined (_M6) || defined (_A6) || ( _IPP >= _IPP_W7 ) || (_IPP32E >= _IPP32E_M7) || defined (_I7)
// dlaptev:: enabled for all platforms
#include "cpudef.h"
#include "ippcore.h"
//#endif

#if !( (_IPP>=_IPP_M6) || (_IPP32E>=_IPP32E_M7) || (_IPP64>=_IPP64_I7) )
#define _CCODE
#endif

#if ( _IPPLP32 == _IPPLP32_S8 ) || ( _IPPLP64 == _IPPLP64_N8 ) || ( _IPP <= _IPP_W7 )
#define BOUND_NT  524288
#else
#define BOUND_NT  1048576
#endif

#if (_IPP32E >= _IPP32E_M7)
#if (_IPP32E < _IPP32E_L9)
extern void mfxowniConvert_16u8u_M7( const Ipp16u *pSrc, Ipp8u *pDst, int len );
#endif
#endif

#if ( _IPP >= _IPP_T7 ) && ( _IPP < _IPP_H9 )
extern void mfxowniConvert_16u8u_T7( const Ipp16u *pSrc, Ipp8u *pDst, int len , int nt);
#endif

#if ( _IPP >= _IPP_W7 )
#if   defined (_W7)
extern void mfxowniConvert_16u8u_W7( const Ipp16u *pSrc, Ipp8u *pDst, int len );
#endif
#endif
#if   defined (_A6)
extern void mfxowniConvert_16u8u_A6( const Ipp16u *pSrc, Ipp8u *pDst, int len );
#endif
#if (_IPPLRB>=_IPPLRB_B1)
IppStatus mfxowniConvert_16u8u_B1( const Ipp16u* pSrc, int srcStep,
                           Ipp8u* pDst, int dstStep, IppiSize roiSize,int nCh);
IppStatus mfxowniConvert_16u8u_A_B1( const Ipp16u* pSrc, int srcStep,
                                   Ipp8u* pDst, int dstStep, IppiSize roiSize);
#endif

#if ((_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9))
extern void mfxowniConvert_16u8u( const Ipp16u *pSrc, Ipp8u *pDst, int len );
#endif


/*******************************************************************/
IPPFUN ( IppStatus, mfxiConvert_16u8u_C1R,
        ( const Ipp16u* pSrc, int srcStep, Ipp8u* pDst, int dstStep,
          IppiSize roiSize ))
{
    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_SIZE_RET( roiSize.width )
    IPP_BAD_SIZE_RET( roiSize.height )
    IPP_BAD_STEP_RET( srcStep )
    IPP_BAD_STEP_RET( dstStep )
#if !(_IPPLRB>=_IPPLRB_B1)
   {
    const Ipp16u *src = pSrc;
    Ipp8u *dst = pDst;
    int width = roiSize.width, height = roiSize.height;
    int h;
#if defined (_CCODE) || defined (_M6) || defined (_I7)
    int n;
#endif
#if ( _IPP >= _IPP_T7 ) && ( _IPP < _IPP_H9 )
    IppStatus ownStatus = ippStsNoErr;
    int nonTemporal = 0, size = 0, cache = 0;
#endif

#if ( _IPP >= _IPP_T7 )  && ( _IPP < _IPP_H9 )
    size = width*height*3;
    if( size > BOUND_NT ) {
      ownStatus = mfxGetMaxCacheSizeB( &cache );
      if( ownStatus == ippStsNoErr  ) {
        if( size >= cache ) nonTemporal = 1;
      }
    }
#endif
    if( (srcStep == dstStep*2) && (dstStep == width) ) {
        width *= height;
        height = 1;
    }

    for( h = 0; h < height; h++ ) {
        #if ((_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9))
        mfxowniConvert_16u8u( src, dst, width  );
        #elif (_IPP32E >= _IPP32E_M7)
        mfxowniConvert_16u8u_M7( src, dst, width  );
        #elif ( _IPP >= _IPP_T7 )
        mfxowniConvert_16u8u_T7( src, dst, width, nonTemporal );
        #elif ( _IPP == _IPP_W7 )
        mfxowniConvert_16u8u_W7( src, dst, width  );
        #elif defined (_A6)
        mfxowniConvert_16u8u_A6( src, dst, width  );
        #else
        for( n = 0; n < width; n++ ) {
            if( src[n] >= IPP_MAX_8U ) {
                dst[n] = IPP_MAX_8U;
            } else {
                dst[n] = (Ipp8u)src[n];
            }
        }
        #endif
        src = (Ipp16u*)((Ipp8u*)src + srcStep);
        dst += dstStep;
    }
    return ippStsNoErr;
}
#else
    return mfxowniConvert_16u8u_B1( pSrc,srcStep,pDst,dstStep,roiSize,1);
#endif
}

/*******************************************************************/
IPPFUN ( IppStatus, mfxiConvert_16u8u_AC4R,
        ( const Ipp16u* pSrc, int srcStep, Ipp8u* pDst, int dstStep,
          IppiSize roiSize ))
{
    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_SIZE_RET( roiSize.width )
    IPP_BAD_SIZE_RET( roiSize.height )
    IPP_BAD_STEP_RET( srcStep )
    IPP_BAD_STEP_RET( dstStep )
#if !(_IPPLRB>=_IPPLRB_B1)
   {
    const Ipp16u *src = pSrc;
    Ipp8u *dst = pDst;
    int width = roiSize.width*4, height = roiSize.height;
    int h;
    int n;
    if( (srcStep == dstStep*2) && (dstStep == width) ) {
        width *= height;
        height = 1;
    }
    for( h = 0; h < height; h++ ) {
        for( n = 0; n < width; n += 4 ) {
            if( src[n] >= IPP_MAX_8U ) {
                dst[n] = IPP_MAX_8U;
            } else {
                dst[n] = (Ipp8u)src[n];
            }
            if( src[n + 1] >= IPP_MAX_8U ) {
                dst[n + 1] = IPP_MAX_8U;
            } else {
                dst[n + 1] = (Ipp8u)src[n + 1];
            }
            if( src[n + 2] >= IPP_MAX_8U ) {
                dst[n + 2] = IPP_MAX_8U;
            } else {
                dst[n + 2] = (Ipp8u)src[n + 2];
            }
        }
        src = (Ipp16u*)((Ipp8u*)src + srcStep);
        dst += dstStep;
    }
    return ippStsNoErr;
}
#else
    return mfxowniConvert_16u8u_A_B1( pSrc,srcStep,pDst,dstStep,roiSize);
#endif
}

/*******************************************************************/
IPPFUN ( IppStatus, mfxiConvert_16u8u_C4R,
        ( const Ipp16u* pSrc, int srcStep, Ipp8u* pDst, int dstStep,
          IppiSize roiSize ))
{

#if !((_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9))

    IPP_BAD_PTR2_RET( pSrc, pDst )
    IPP_BAD_SIZE_RET( roiSize.width )
    IPP_BAD_SIZE_RET( roiSize.height )
    IPP_BAD_STEP_RET( srcStep )
    IPP_BAD_STEP_RET( dstStep )
#if !(_IPPLRB>=_IPPLRB_B1)
   {
    const Ipp16u *src = pSrc;
    Ipp8u *dst = pDst;
    int width = roiSize.width*4, height = roiSize.height;
    int h;
#if defined (_CCODE) || defined (_M6) || defined (_I7)
    int n;
#endif
#if ( _IPP >= _IPP_T7 )
    IppStatus ownStatus = ippStsNoErr;
    int nonTemporal = 0, size = 0, cache = 0;
#endif
#if ( _IPP >= _IPP_T7 )
    size = width*height*3;
    if( size > BOUND_NT ) {
      ownStatus = mfxGetMaxCacheSizeB( &cache );
      if( ownStatus == ippStsNoErr  ) {
        if( size >= cache ) nonTemporal = 1;
      }
    }
#endif
    if( (srcStep == dstStep*2) && (dstStep == width) ) {
        width *= height;
        height = 1;
    }

    for( h = 0; h < height; h++ ) {
        #if (_IPP32E >= _IPP32E_M7)
        mfxowniConvert_16u8u_M7( src, dst, width  );
        #elif ( _IPP >= _IPP_T7 )
        mfxowniConvert_16u8u_T7( src, dst, width, nonTemporal );
        #elif ( _IPP == _IPP_W7 )
        mfxowniConvert_16u8u_W7( src, dst, width  );
        #elif defined (_A6)
        mfxowniConvert_16u8u_A6( src, dst, width  );
        #else
        for( n = 0; n < width; n++ ) {
            if( src[n] >= IPP_MAX_8U ) {
                dst[n] = IPP_MAX_8U;
            } else {
                dst[n] = (Ipp8u)src[n];
            }
        }
        #endif
        src = (Ipp16u*)((Ipp8u*)src + srcStep);
        dst += dstStep;
    }
    return ippStsNoErr;
}
#else
    return mfxowniConvert_16u8u_B1( pSrc,srcStep,pDst,dstStep,roiSize,4);
#endif

#else
    IPP_BAD_SIZE_RET( roiSize.width )
    roiSize.width *= 4;
    return mfxiConvert_16u8u_C1R(pSrc, srcStep, pDst, dstStep, roiSize);

#endif

}
