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
//    Upsampling functions
//
//  Contents:
//    mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1
//    mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#ifndef __PJDECSS_H__
#include "pjdecss.h"
#endif



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1
//
//  Purpose:
//    sample up horizontally 1:2 and vertically 1:1
//
//  Parameters:
//    pSrc      pointer to the source row
//    srcWidth  width of source row
//    pDst      pointer to sampled up output row
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Triangle filter used there (3/4 * nearer pixel + 1/4 * further pixel)
*/

IPPFUN(IppStatus, mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1, (
  const Ipp8u* pSrc,
        int    srcWidth,
        Ipp8u* pDst))
{
  IPP_BAD_PTR2_RET(pSrc,pDst)
  IPP_BAD_SIZE_RET(srcWidth)

#if IPPJ_DECSS_OPT || (_IPPXSC >= _IPPXSC_S2)
  ownpj_SampleUpRowH2V1_Triangle_JPEG_8u_C1(pSrc, srcWidth, pDst);
#else
  {
    int i;
    int invalue;

    /* Special case for first column */
    invalue = pSrc[0];
    *pDst++ = (Ipp8u)invalue;
    *pDst++ = (Ipp8u)((invalue * 3 + pSrc[1] + 2) >> 2);

    for(i = 2; i < srcWidth; i++)
    {
      /* General case: 3/4 * nearer pixel + 1/4 * further pixel */
      invalue = pSrc[i - 1] * 3;
      *pDst++ = (Ipp8u)((invalue + pSrc[i - 2] + 1) >> 2);
      *pDst++ = (Ipp8u)((invalue + pSrc[i    ] + 2) >> 2);
    }

    /* Special case for last column */
    invalue = pSrc[srcWidth - 1];
    *pDst++ = (Ipp8u)((invalue * 3 + pSrc[srcWidth - 2] + 1) >> 2);
    *pDst = (Ipp8u)invalue;
  }
#endif

  return ippStsNoErr;
} /* mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1() */



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1
//
//  Purpose:
//    sample up horizontally 1:2 and vertically 1:2
//
//  Parameters:
//    pSrc1     pointer to the source row
//    pSrc2     pointer to the next source row
//    srcWidth  width of source rows
//    pDst      pointer to sampled up output row
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Triangle filter used there (3/4 * nearer pixel + 1/4 * further pixel),
*/

IPPFUN(IppStatus, mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1, (
  const Ipp8u* pSrc1,
  const Ipp8u* pSrc2,
        int    srcWidth,
        Ipp8u* pDst))
{
  IPP_BAD_PTR3_RET(pSrc1,pSrc2,pDst)
  IPP_BAD_SIZE_RET(srcWidth)

#if IPPJ_DECSS_OPT// || (_IPPXSC >= _IPPXSC_S2)
  ownpj_SampleUpRowH2V2_Triangle_JPEG_8u_C1(pSrc1, pSrc2, srcWidth, pDst);
#else
  {
    int i;
    int thiscolsum, lastcolsum, nextcolsum;

    /* Special case for first column */
    thiscolsum = pSrc1[0] * 3 + pSrc2[0];
    nextcolsum = pSrc1[1] * 3 + pSrc2[1];

    *pDst++ = (Ipp8u)((thiscolsum * 4 + 8) >> 4);
    *pDst++ = (Ipp8u)((thiscolsum * 3 + nextcolsum + 7) >> 4);

    lastcolsum = thiscolsum;
    thiscolsum = nextcolsum;

    for(i = 2; i < srcWidth; i++)
    {
      /* General case: 3/4 * nearer pixel + 1/4 * further pixel */
      /* in each dimension, thus 9/16, 3/16, 3/16, 1/16 overall */
      nextcolsum = pSrc1[i] * 3 + pSrc2[i];
      *pDst++ = (Ipp8u)((thiscolsum * 3 + lastcolsum + 8) >> 4);
      *pDst++ = (Ipp8u)((thiscolsum * 3 + nextcolsum + 7) >> 4);

      lastcolsum = thiscolsum;
      thiscolsum = nextcolsum;
    } /* for width */

    /* Special case for last column */
    *pDst++ = (Ipp8u)((thiscolsum * 3 + lastcolsum + 8) >> 4);
    *pDst = (Ipp8u)((thiscolsum * 4 + 7) >> 4);
  }
#endif

  return ippStsNoErr;
} /* mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1() */
