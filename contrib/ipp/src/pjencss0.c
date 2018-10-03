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
//    Downsampling functions
//
//  Contents:
//    mfxiSampleDownRowH2V1_Box_JPEG_8u_C1
//    mfxiSampleDownRowH2V2_Box_JPEG_8u_C1
//
*/

#include "precomp.h"

#ifdef _OPENMP
#include <omp.h>
#endif
#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#ifndef __PJENCSS_H__
#include "pjencss.h"
#endif



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleDownRowH2V1_Box_JPEG_8u_C1
//
//  Purpose:
//    Sample down by horizontal 2:1
//
//  Parameters:
//    pSrc      pointer to source row
//    srcWidth  width of source row
//    pDst      pointer to sampled down output row
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Downsampling is performed as simple "Box" filter
*/

IPPFUN(IppStatus, mfxiSampleDownRowH2V1_Box_JPEG_8u_C1, (
  const Ipp8u*   pSrc,
        int      srcWidth,
        Ipp8u*   pDst))
{
  IppStatus retStat = ippStsNoErr;
  /* Check if srcWidth is uneven */
  if(srcWidth & 0x01) {
      srcWidth &= ~1;  /* Make source width one byte less */
      retStat = ippStsSizeWrn; /* If srcWidth is 1, it will fail as "bad size" */
  }
  IPP_BAD_PTR2_RET(pSrc,pDst)
  IPP_BAD_SIZE_RET(srcWidth)
#if IPPJ_ENCSS_OPT || (_IPPXSC >= _IPPXSC_S2)
#if (_IPP == _IPP_W7)
  if(srcWidth < 512)
    mfxownpj_SampleDownRowH2V1_Box_JPEG_8u_C1(pSrc,srcWidth,pDst);
  else
    mfxownpj_SampleDownRowH2V1_Box_JPEG_8u_C1_w7(pSrc,srcWidth,pDst);
#else
  mfxownpj_SampleDownRowH2V1_Box_JPEG_8u_C1(pSrc,srcWidth,pDst);
#endif
#else
  {
    int i;
    int bias = 0;

    for(i = 0; i < srcWidth; i += 2)
    {
      *pDst++ = (Ipp8u)((pSrc[i+0] + pSrc[i+1] + bias) >> 1);
      bias ^= 1;  /* bias = 0,1,0,1,... for successive samples */
    }
  }
#endif

  return retStat;
} /* mfxiSampleDownRowH2V1_Box_JPEG_8u_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleDownRowH2V2_Box_JPEG_8u_C1
//
//  Purpose:
//    Sample down by horizontal 2:1 and vertical 2:1
//
//  Parameters:
//    pSrc1     pointer to the source row
//    pSrc2     pointer to the next row
//    srcWidth  width of source rows
//    pDst      pointer to sampled down output row
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Downsampling is performed as simple "Box" filter
*/

IPPFUN(IppStatus, mfxiSampleDownRowH2V2_Box_JPEG_8u_C1, (
  const Ipp8u*   pSrc1,
  const Ipp8u*   pSrc2,
        int      srcWidth,
        Ipp8u*   pDst))
{
  IPP_BAD_PTR3_RET(pSrc1,pSrc2,pDst)
  IPP_BAD_SIZE_RET(srcWidth)

#if IPPJ_ENCSS_OPT || (_IPPXSC >= _IPPXSC_S2)
  if(srcWidth > 31)
  {
    mfxownpj_SampleDownRowH2V2_Box_JPEG_8u_C1(pSrc1,pSrc2,srcWidth,pDst);
  }
  else
#endif
  {
    int i;
    int bias = 1;

    for(i = 0; i < srcWidth; i += 2)
    {
      *pDst++ = (Ipp8u)((pSrc1[i+0] + pSrc1[i+1] + pSrc2[i+0] + pSrc2[i+1] + bias) >> 2);
      bias ^= 3;  /* bias = 1,2,1,2,... for successive samples */
    }
  }

  return ippStsNoErr;
} /* mfxiSampleDownRowH2V2_Box_JPEG_8u_C1() */
