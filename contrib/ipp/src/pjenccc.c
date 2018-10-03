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
//    Color conversions functions, forward transform
//
//  Contents:
//    mfxiRGBToY_JPEG_8u_C3C1R
//
*/

#include "precomp.h"

#ifdef _OPENMP
#include <omp.h>
#endif
#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#ifndef __PJENCCCTBL_H__
#include "pjenccctbl.h"
#endif
#ifndef __PJENCCC_H__
#include "pjenccc.h"
#endif



#if (_IPPXSC < _IPPXSC_S2)
#if !((IPPJ_ENCCC_OPT) || (_IPPLRB>=_IPPLRB_B1))

LOCFUN(void, mfxownpj_RGBToY_JPEG_8u_C3C1R, (
  const Ipp8u* rgb,
        Ipp8u* y,
        int    width))
{
  int i;

  for(i = 0; i < width; i++)
  {
    y[0]  = (Ipp8u)((mfxcc_table[rgb[0] + R_Y_OFFS] +
                     mfxcc_table[rgb[1] + G_Y_OFFS] +
                     mfxcc_table[rgb[2] + B_Y_OFFS] + 3) >> 16);
    rgb += 3;
    y   += 1;
  }

  return;
} /* mfxownpj_RGBToY_JPEG_8u_C3C1R() */

#endif
#endif



/* ---------------------- library functions definitions -------------------- */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiRGBToY_JPEG_8u_C3C1R
//
//  Purpose:
//    RGB to Y color convert
//
//  Parameter:
//    pSrc      pointer to input data
//    SrcStep   line offset in input data
//    pDst      pointer to output array
//    DstStep   line offset in output data
//    roiSize   ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//    the color conversion equations:
//      Y  =  0.29900*R + 0.58700*G + 0.11400*B
*/

IPPFUN(IppStatus, mfxiRGBToY_JPEG_8u_C3C1R, (
  const Ipp8u*   pSrc,
        int      SrcStep,
        Ipp8u*   pDst,
        int      DstStep,
        IppiSize roiSize))
{
  int   i;

  IPP_BAD_ENC_CC_C3C1_RET()

#ifdef _OPENMP
#pragma omp parallel for IPP_OMP_NUM_THREADS() \
  shared(pSrc,pDst,SrcStep,DstStep,roiSize) \
  private(i) default(none) \
  if((roiSize.height*roiSize.width) > (OMP_BOUNDARY))
#endif

  for(i = 0; i < roiSize.height; i++)
  {
    const Ipp8u* rgb;
    Ipp8u* y;

    rgb = pSrc + i * SrcStep;
    y   = pDst + i * DstStep;

    mfxownpj_RGBToY_JPEG_8u_C3C1R(rgb, y, roiSize.width);
  }

  return ippStsNoErr;
} /* mfxiRGBToY_JPEG_8u_C3C1R() */
