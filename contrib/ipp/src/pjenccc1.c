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
//    mfxiCMYKToYCCK_JPEG_8u_C4P4R
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


#if _IPPLRB >= _IPPLRB_B1
extern void mfxownpj_CMYKToYCCK_JPEG_8u_C4P4R(const Ipp8u* rgba, Ipp8u* y, Ipp8u* cb, Ipp8u* cr, Ipp8u* a, int width);
#else
LOCFUN(void, mfxownpj_CMYKToYCCK_JPEG_8u_C4P4R, (
  const Ipp8u* rgba,
        Ipp8u* y,
        Ipp8u* cb,
        Ipp8u* cr,
        Ipp8u* a,
        int    width))
{
  int i;

  int rs, gs, bs;

  for(i = 0; i < width; i++)
  {
    rs  = 255 - rgba[0];
    gs  = 255 - rgba[1];
    bs  = 255 - rgba[2];

    y[i]  = (Ipp8u)((mfxcc_table[rs + R_Y_OFFS] +
                     mfxcc_table[gs + G_Y_OFFS] +
                     mfxcc_table[bs + B_Y_OFFS] + 3) >> 16);

    cb[i] = (Ipp8u)((mfxcc_table[rs + R_CB_OFF] +
                     mfxcc_table[gs + G_CB_OFF] +
                     mfxcc_table[bs + B_CB_OFF] + 3) >> 16);

    cr[i] = (Ipp8u)((mfxcc_table[rs + R_CR_OFF] +
                     mfxcc_table[gs + G_CR_OFF] +
                     mfxcc_table[bs + B_CR_OFF] + 3) >> 16);

    a[i]  = (Ipp8u)rgba[3];

    rgba += 4;
  }

  return;
} /* mfxownpj_CMYKToYCCK_JPEG_8u_C4P4R() */
#endif


/* ---------------------- library functions definitions -------------------- */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiCMYKToYCCK_JPEG_8u_C4P4R
//
//  Purpose:
//    CMYK to YCCK color conversation
//
//  Parameter:
//    pSrc      pointer to input data
//    SrcStep   line offset in input data
//    pDst      pointer to pointers to output data
//    DstStep   line offset in output data
//    roiSize   ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//    the color conversion equations:
//      Y  = 255 - ( 0.29900*R + 0.58700*G + 0.11400*B)
//      Cb = 255 - (-0.16874*R - 0.33126*G + 0.50000*B + 128.0)
//      Cr = 255 - ( 0.50000*R - 0.41869*G - 0.08131*B + 128.0)
//    A  = K
*/

IPPFUN(IppStatus, mfxiCMYKToYCCK_JPEG_8u_C4P4R, (
  const Ipp8u*   pSrc,
        int      SrcStep,
        Ipp8u*   pDst[4],
        int      DstStep,
        IppiSize roiSize))
{
  int   i;

  IPP_BAD_ENC_CC_C4P4_RET()

#ifdef _OPENMP
#pragma omp parallel for IPP_OMP_NUM_THREADS() \
  shared(pSrc,pDst,SrcStep,DstStep,roiSize) \
  private(i) default(none) \
  if((roiSize.height*roiSize.width) > (OMP_BOUNDARY))
#endif

  for(i = 0; i < roiSize.height; i++)
  {
    const Ipp8u* rgba;
    Ipp8u* y;
    Ipp8u* cb;
    Ipp8u* cr;
    Ipp8u* a;

    rgba = pSrc + i * SrcStep;

    y  = pDst[0] + i * DstStep;
    cb = pDst[1] + i * DstStep;
    cr = pDst[2] + i * DstStep;
    a  = pDst[3] + i * DstStep;

    mfxownpj_CMYKToYCCK_JPEG_8u_C4P4R(rgba, y, cb, cr, a, roiSize.width);
  }

  return ippStsNoErr;
} /* mfxiCMYKToYCCK_JPEG_8u_C4P4R() */
