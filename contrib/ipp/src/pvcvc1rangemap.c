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
//  Description:
//      VC1 Range Map transformation
//
//  Contents:
//    mfxiRangeMap_VC1_8u_C1R
//
*/

#include "precomp.h"
#include "ownvc.h"
#include "pvcown.h"

#include "pvcvc1functions.h"

#define VC1_CLIP(x) (!(x&~255)?x:(x<0?0:255))

#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)
#ifndef imp_mfxiRangeMapping_VC1_8u_C1R
#define imp_mfxiRangeMapping_VC1_8u_C1R
IPPFUN(IppStatus, mfxiRangeMapping_VC1_8u_C1R ,(Ipp8u* pSrc, Ipp32s srcStep,
                             Ipp8u* pDst, Ipp32s dstStep,
                             IppiSize roiSize, Ipp32s rangeMapParam))
{
    Ipp32s i=0;
    Ipp32s j=0;
    Ipp32s temp;

    IPP_BAD_PTR2_RET(pSrc,pDst);
    IPP_BAD_RANGE_RET(rangeMapParam, 0, 7);

    if( (roiSize.height<=0) || (roiSize.width<=0))
        return ippStsNoErr;

    if((roiSize.width>>3<<3 != roiSize.width) || (roiSize.width>srcStep) || ((roiSize.width>dstStep))){
        for (i = 0; i < roiSize.height; i++)
        {
            for (j = 0; j < roiSize.width; j++)
            {
                temp = pSrc[i*srcStep+j];

                temp = (temp - 128)*(rangeMapParam+9)+4;
                temp = temp>>3;
                temp = temp+128;
                pDst[i*dstStep+j] = (Ipp8u)VC1_CLIP(temp);
             }
        }
    } else {
        mfxrangemapping_vc1_sse2(pSrc,srcStep,pDst,dstStep,(roiSize.height),(roiSize.width>>3),rangeMapParam);
    }

    return ippStsNoErr;
}
#endif //#ifndef imp_mfxiRangeMapping_VC1_8u_C1R
#endif //#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)

#ifndef imp_mfxiRangeMapping_VC1_8u_C1R
#define imp_mfxiRangeMapping_VC1_8u_C1R

IPPFUN(IppStatus, mfxiRangeMapping_VC1_8u_C1R ,(Ipp8u* pSrc, Ipp32s srcStep,
                             Ipp8u* pDst, Ipp32s dstStep,
                             IppiSize roiSize, Ipp32s rangeMapParam))
{
    Ipp32s i=0;
    Ipp32s j=0;
    Ipp32s temp;

    IPP_BAD_PTR2_RET(pSrc,pDst);
    IPP_BAD_RANGE_RET(rangeMapParam, 0, 7);

    for (i = 0; i < roiSize.height; i++)
    {
        for (j = 0; j < roiSize.width; j++)
        {
            temp = pSrc[i*srcStep+j];

            temp = (temp - 128)*(rangeMapParam+9)+4;
            temp = temp>>3;
            temp = temp+128;
            pDst[i*dstStep+j] = (Ipp8u)VC1_CLIP(temp);
         }
    }
    return ippStsNoErr;
}
#endif //#ifndef imp_mfxiRangeMapping_VC1_8u_C1R

