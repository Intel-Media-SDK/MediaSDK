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
//               Intel(R) Integrated Performance Primitives
//              Color Conversion Library (ippCC)
// 
// 
*/


#if !defined( __IPPCC_H__ ) || defined( _OWN_BLDPCS )
#define __IPPCC_H__

#if defined (_WIN32_WCE) && defined (_M_IX86) && defined (__stdcall)
  #define _IPP_STDCALL_CDECL
  #undef __stdcall
#endif

#ifndef __IPPDEFS_H__
  #include "ippdefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ////////////////////////////////////////////////////////////////////////////
//  Name:       mfxiYCbCr422_8u_P3C2R
//  Purpose:    Converts 422 planar image to YUY2
//  Returns:
//    ippStsNoErr              OK
//    ippStsNullPtrErr         One or more pointers are NULL
//    ippStsSizeErr            Width of first plain 422-image less than 2(4)
//                             or height equal zero
//
//  Parameters:
//    pSrc[3]                  Array of pointers to the source image planes
//    srcStep[3]               Array of steps through the source image planes
//    pDst[3]                  Array of pointers to the destination image planes
//    dstStep[3]               Array of steps through the destination image planes
//    pSrc                     Pointer to the source pixel-order image
//    srcStep                  Step through the source pixel-order image
//    pDst                     Pointer to the destination pixel-order image
//    dstStep                  Step through the destination pixel-order image
//    roiSize                  Size of the ROI
*/
IPPAPI (IppStatus, mfxiYCbCr422_8u_P3C2R,          ( const Ipp8u* pSrc[3], int srcStep[3], Ipp8u* pDst, int dstStep, IppiSize  roiSize))

IPPAPI(IppStatus, mfxiYCbCr420ToYCbCr422_8u_P2C2R,(const Ipp8u* pSrcY, int srcYStep,const Ipp8u* pSrcCbCr,
                  int srcCbCrStep, Ipp8u* pDst, int dstStep, IppiSize roiSize))

IPPAPI(IppStatus, mfxiYCrCb420ToYCbCr422_8u_P3C2R,( const Ipp8u* pSrc[3],int srcStep[3], Ipp8u* pDst, int dstStep, IppiSize roiSize ))

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Name:       mfxiYCbCr422_8u_C2P3R
//  Purpose:    Converts a YUY2 image to the P422 image
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         One or more pointers are NULL
//    ippStsSizeErr            if roiSize.width < 2
//
//  Arguments:
//    pSrc                     pointer to the source image
//    srcStep                  step for the source image
//    pDst                     array of pointers to the components of the destination image
//    dstStep                  array of steps values for every component
//     roiSize                 region of interest to be processed, in pixels
//  Notes:
//    roiSize.width  should be multiple 2.
*/
IPPAPI(IppStatus, mfxiYCbCr422_8u_C2P3R,( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize ))

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Name:       mfxiYCbCr422ToYCbCr420_8u_C2P3R
//  Purpose:    Converts a 2-channel YUY2 image to the I420(IYUV) image
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         One or more pointers are NULL
//    ippStsSizeErr            if roiSize.width < 2 || roiSize.height < 2
//
//  Arguments:
//    pSrc                     pointer to the source image
//    srcStep                  step for the source image
//    pDst                     array of pointers to the components of the destination image
//    dstStep                  array of steps values for every component
//     roiSize                 region of interest to be processed, in pixels
//  Notes:
//    roiSize.width and roiSize.height  should be multiple 2.
*/
IPPAPI(IppStatus, mfxiYCbCr422ToYCbCr420_8u_C2P3R,( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize ))

/* ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Name:       mfxiYCbCr420_8u_P2P3R
//  Purpose:    Converts a NV12 image to the I420(IYUV) image.
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         One or more pointers are NULL
//    ippStsSizeErr            if roiSize.width < 2 || roiSize.height < 2
//
//  Arguments:
//    pSrcY                    pointer to the source Y plane
//    srcYStep                 step  for the source Y plane
//    pSrcCbCr                 pointer to the source CbCr plane
//    srcCbCrStep              step  for the source CbCr plane
//    pDst                     array of pointers to the components of the destination image
//    dstStep                  array of steps values for every component
//     roiSize                 region of interest to be processed, in pixels
//  Notes:
//    roiSize.width  should be multiple 2.
//    roiSize.height should be multiple 2.
*/
IPPAPI(IppStatus, mfxiYCbCr420_8u_P2P3R,(const Ipp8u* pSrcY,int srcYStep,const Ipp8u* pSrcCbCr, int srcCbCrStep,
Ipp8u* pDst[3], int dstStep[3], IppiSize roiSize ))

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Name:       mfxiCbYCr422ToYCbCr422_8u_C2P3R
//  Purpose:    Converts a UYVY image to the P422 image
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr         One or more pointers are NULL
//    ippStsSizeErr            if roiSize.width < 2
//
//  Arguments:
//    pSrc                     pointer to the source image
//    srcStep                  step for the source image
//    pDst                     array of pointers to the components of the destination image
//    dstStep                  array of steps values for every component
//     roiSize                 region of interest to be processed, in pixels
//  Notes:
//    roiSize.width  should be multiple 2.
*/
IPPAPI(IppStatus, mfxiCbYCr422ToYCbCr422_8u_C2P3R,( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize ))


/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Name:       mfxiYCbCr422ToYCbCr420_8u_P3R
//  Purpose:    Converts a P422 image to the I420(IYUV)image
//  Name:       mfxiYCbCr420ToYCbCr422_8u_P3R
//  Purpose:    Converts a IYUV image to the P422 image
//  Return:
//    ippStsNoErr              Ok
//    ippStsNullPtrErr        One or more pointers are NULL
//    ippStsSizeErr            if roiSize.width < 2 || roiSize.height < 2
//
//  Arguments:
//    pSrc                     array of pointers to the components of the source image
//    srcStep                  array of step values for every component
//    pDst                     array of pointers to the components of the destination image
//    dstStep                  array of steps values for every component
//     roiSize                 region of interest to be processed, in pixels
//  Notes:
//    roiSize.width and roiSize.height  should be multiple 2.
*/
IPPAPI(IppStatus, mfxiYCbCr422ToYCbCr420_8u_P3R,( const Ipp8u* pSrc[3], int srcStep[3], Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize ))
IPPAPI(IppStatus, mfxiYCbCr420ToYCbCr422_8u_P3R,( const Ipp8u* pSrc[3], int srcStep[3], Ipp8u* pDst[3],
        int dstStep[3], IppiSize roiSize ))


#ifdef __cplusplus
}
#endif

#if defined (_IPP_STDCALL_CDECL)
  #undef  _IPP_STDCALL_CDECL
  #define __stdcall __cdecl
#endif

#endif /* __IPPCC_H__ */
