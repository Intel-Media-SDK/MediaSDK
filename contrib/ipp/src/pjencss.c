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
//    mfxiSampleDownH2V1_JPEG_8u_C1R
//    mfxiSampleDownH2V2_JPEG_8u_C1R
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




/* ---------------------- library functions definitions -------------------- */

/* ////////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleDownH2V1_JPEG_8u_C1R
//
//  Purpose:
//    Sample down by horizontal 2:1
//
//  Parameter:
//    pSrc      pointer to input data
//    srcStep   line offset in input data
//    srcSize   ROI size
//    pDst      pointer to output array
//    dstStep   line offset in output data
//    dstSize   ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiSampleDownH2V1_JPEG_8u_C1R, (
  const Ipp8u*   pSrc,
        int      srcStep,
        IppiSize srcSize,
        Ipp8u*   pDst,
        int      dstStep,
        IppiSize dstSize))
{
  int i;

  IPP_BAD_ENC_SS_C1C1_RET();

#ifndef _W7
#ifdef _OPENMP
#pragma omp parallel for IPP_OMP_NUM_THREADS() \
  shared(pSrc,srcStep,pDst,dstStep,dstSize) \
  private(i) default(none) \
  if((dstSize.height*dstSize.width) > (OMP_BOUNDARY))
#endif
#endif

  for(i = 0; i < dstSize.height; i++)
  {
    const Ipp8u* src = pSrc + i * srcStep;
          Ipp8u* dst = pDst + i * dstStep;

#if (_IPP >= _IPP_M6) || (_IPP32E >= _IPP32E_M7) || (_IPP64 >= _IPP64_I7) || (_IPPLRB>=_IPPLRB_B1)
    mfxownpj_SampleDownH2V1_JPEG_8u_C1(src,dst,dstSize.width);
#else
    {
      int j, k;

      for(k = 0, j = 0; j < dstSize.width; j++, k += 2)
      {
        dst[j] = (Ipp8u)((src[k + 0] + src[k + 1] + 1) >> 1);
      }
    }
#endif
  }

  return ippStsNoErr;
} /* mfxiSampleDownH2V1_JPEG_8u_C1R() */


/* ////////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleDownH2V2_JPEG_8u_C1R
//
//  Purpose:
//    Sample down by horizontal 2:1 and vertical 2:1
//
//  Parameter:
//    pSrc      pointer to input data
//    srcStep   line offset in input data
//    srcSize   ROI size
//    pDst      pointer to output array
//    dstStep   line offset in output data
//    dstSize   ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiSampleDownH2V2_JPEG_8u_C1R, (
  const Ipp8u*   pSrc,
        int      srcStep,
        IppiSize srcSize,
        Ipp8u*   pDst,
        int      dstStep,
        IppiSize dstSize))
{
  int i;

  IPP_BAD_ENC_SS_C1C1_RET();

#ifdef _OPENMP
#pragma omp parallel for IPP_OMP_NUM_THREADS() \
  shared(pSrc,srcStep,pDst,dstStep,dstSize) \
  private(i) default(none) \
  if((dstSize.height*dstSize.width) > (OMP_BOUNDARY))
#endif

  for(i = 0; i < dstSize.height; i++)
  {
    const Ipp8u* src = pSrc + i * (srcStep << 1);
          Ipp8u* dst = pDst + i * dstStep;

#if (_IPP >= _IPP_M6) || (_IPP32E >= _IPP32E_M7) || (_IPP64 >= _IPP64_I7) || (_IPPLRB>=_IPPLRB_B1)
    mfxownpj_SampleDownH2V2_JPEG_8u_C1(src,srcStep,dst,dstSize.width);
#else
    {
      int j, k;

      for(k = 0, j = 0; j < dstSize.width; j++, k += 2)
      {
        dst[j] = (Ipp8u)
          ((src[k + 0 + 0*srcStep] + src[k + 1 + 0*srcStep] +
            src[k + 0 + 1*srcStep] + src[k + 1 + 1*srcStep] + 3) >> 2);
      }
    }
#endif
  }

  return ippStsNoErr;
} /* mfxiSampleDownH2V2_JPEG_8u_C1R() */
