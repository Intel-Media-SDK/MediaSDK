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
//              Intel(R) Integrated Performance Primitives
//              Video Coding (ippVC)
// 
// 
*/


#if !defined( __IPPVC_H__ ) || defined( _OWN_BLDPCS )
#define __IPPVC_H__

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

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//      mfxiDeinterlaceFilterTriangle_8u_C1R
//
//  Purpose:
//      This function deinterlaces video plane.
//      The function performs triangle filtering of the image to remove interlacing
//      flicker effect that arises when analogue interlaced TV data is
//      viewed on a computer monitor.
//
//  Parameters:
//      pSrc            Pointer to the source video plane.
//      srcStep         Step through the source video plane.
//      pDst            Pointer to the destination video plane.
//      dstStep         Step through the destination video plane.
//      roiSize         Size of ROI. Height should be greater than 3.
//      centerWeight    Weight of filtered pixel, must lie within the range from 0 to 256.
//      layout          Plane layout, required when the plane is only a part of the frame.
//                      Takes the following values:
//                      IPP_UPPER for the first slice
//                      IPP_CENTER for the middle slices
//                      IPP_LOWER for the last slice
//                      IPP_LOWER && IPP_UPPER && IPP_CENTER for the image that is not  sliced.
//
//  Returns:
//      ippStsNoErr     Indicates no error.
//      ippStsNullPtrErr Indicates an error when at least one input pointer is NULL.
//      ippStsSizeErr   Indicates an error when roiSize  has a field with zero or negative value.
//      ippStsBadArgErr Indicates invalid argument.
*/

IPPAPI(IppStatus, mfxiDeinterlaceFilterTriangle_8u_C1R, (
  const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        IppiSize roiSize,
        Ipp32u   centerWeight,
        Ipp32u   layout))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiRangeMapping_VC1_8u_C1R
//
//  Purpose:
//    Performs range map transformation according to VC-1 standard (6.2.15.1)
//
//  Parameters:
//    pSrc - the pointer to the source block. Block coefficient could be in the range [0, 255]
//    srcStep - the step of the source block
//    pDst    - the pointer to the destination block.
//    dstStep - the step of the destination block
//    rangeMapParam - parameter for range map. It should be in the range [0, 7].
//    roiSize - size of source block.
//
//  Returns:
//    ippStsNoErr        No error.
//    ippStsNullPtrErr   At least one input pointer is NULL
//    ippStsOutOfRangeErr Indicates an error if rangeMapParam is out of the range [0,7].
*/
IPPAPI(IppStatus, mfxiRangeMapping_VC1_8u_C1R ,(Ipp8u* pSrc, Ipp32s srcStep,
                             Ipp8u* pDst, Ipp32s dstStep,
                             IppiSize roiSize, Ipp32s rangeMapParam))


#if defined __cplusplus
}
#endif

#if defined (_IPP_STDCALL_CDECL)
  #undef  _IPP_STDCALL_CDECL
  #define __stdcall __cdecl
#endif

#endif /* __IPPVC_H__ */
