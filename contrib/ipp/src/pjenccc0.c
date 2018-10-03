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
//    mfxiRGBToYCbCr_JPEG_8u_P3R
//    mfxiRGBToYCbCr_JPEG_8u_C3P3R
//    mfxiBGRToYCbCr_JPEG_8u_C3P3R
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

LOCFUN(void, mfxownpj_RGBToYCbCr_JPEG_8u_P3R, (
  const Ipp8u* r,
  const Ipp8u* g,
  const Ipp8u* b,
        Ipp8u* y,
        Ipp8u* cb,
        Ipp8u* cr,
        int    width))
{
  int i;
  int w4;
  int rv;
  int gv;
  int bv;

  w4 = 0;

  for(i = w4; i < width; i++)
  {
    rv = r[i];
    gv = g[i];
    bv = b[i];

    y[i]  = (Ipp8u)((mfxcc_table[rv + R_Y_OFFS] +
                     mfxcc_table[gv + G_Y_OFFS] +
                     mfxcc_table[bv + B_Y_OFFS] + 3) >> 16);

    cb[i] = (Ipp8u)((mfxcc_table[rv + R_CB_OFF] +
                     mfxcc_table[gv + G_CB_OFF] +
                     mfxcc_table[bv + B_CB_OFF] + 3) >> 16);

    cr[i] = (Ipp8u)((mfxcc_table[rv + R_CR_OFF] +
                     mfxcc_table[gv + G_CR_OFF] +
                     mfxcc_table[bv + B_CR_OFF] + 3) >> 16);
  }

  return;
} /* mfxownpj_RGBToYCbCr_JPEG_8u_P3R() */


#if !((IPPJ_ENCCC_OPT) || (_IPPLRB>=_IPPLRB_B1))

LOCFUN(void, mfxownpj_RGBToYCbCr_JPEG_8u_C3P3R, (
  const Ipp8u* rgb,
        Ipp8u* y,
        Ipp8u* cb,
        Ipp8u* cr,
        int    width))
{
  int i;
  int r, g, b;

  for(i = 0; i < width; i++)
  {
    r = rgb[0];
    g = rgb[1];
    b = rgb[2];

    rgb += 3;

    y[i]  = (Ipp8u)((mfxcc_table[r + R_Y_OFFS] +
                     mfxcc_table[g + G_Y_OFFS] +
                     mfxcc_table[b + B_Y_OFFS] + 3) >> 16);

    cb[i] = (Ipp8u)((mfxcc_table[r + R_CB_OFF] +
                     mfxcc_table[g + G_CB_OFF] +
                     mfxcc_table[b + B_CB_OFF] + 3) >> 16);

    cr[i] = (Ipp8u)((mfxcc_table[r + R_CR_OFF] +
                     mfxcc_table[g + G_CR_OFF] +
                     mfxcc_table[b + B_CR_OFF] + 3) >> 16);
  }

  return;
} /* mfxownpj_RGBToYCbCr_JPEG_8u_C3P3R() */


LOCFUN(void, mfxownpj_BGRToYCbCr_JPEG_8u_C3P3R, (
  const Ipp8u* bgr,
        Ipp8u* y,
        Ipp8u* cb,
        Ipp8u* cr,
        int    width))
{
  int i;
  int r, g, b;

  for(i = 0; i < width; i++)
  {
    r = bgr[2];
    g = bgr[1];
    b = bgr[0];

    bgr += 3;

    y[i]  = (Ipp8u)((mfxcc_table[r + R_Y_OFFS] +
                     mfxcc_table[g + G_Y_OFFS] +
                     mfxcc_table[b + B_Y_OFFS] + 3) >> 16);

    cb[i] = (Ipp8u)((mfxcc_table[r + R_CB_OFF] +
                     mfxcc_table[g + G_CB_OFF] +
                     mfxcc_table[b + B_CB_OFF] + 3) >> 16);

    cr[i] = (Ipp8u)((mfxcc_table[r + R_CR_OFF] +
                     mfxcc_table[g + G_CR_OFF] +
                     mfxcc_table[b + B_CR_OFF] + 3) >> 16);
  }

  return;
} /* mfxownpj_BGRToYCbCr_JPEG_8u_C3P3R() */

#endif
#endif



/* ---------------------- library functions definitions -------------------- */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiRGBToYCbCr_JPEG_8u_P3R
//
//  Purpose:
//    RGB to YCbCr color convert
//
//  Parameter:
//    pSrc      pointer to pointers to input data
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
//      Y  =  0.29900*R + 0.58700*G + 0.11400*B
//      Cb = -0.16874*R - 0.33126*G + 0.50000*B + 128.0
//      Cr =  0.50000*R - 0.41869*G - 0.08131*B + 128.0
*/

IPPFUN(IppStatus, mfxiRGBToYCbCr_JPEG_8u_P3R, (
  const Ipp8u*   pSrc[3],
        int      SrcStep,
        Ipp8u*   pDst[3],
        int      DstStep,
        IppiSize roiSize))
{
  int   i;

  IPP_BAD_ENC_CC_P3P3_RET()

#ifdef _OPENMP
#pragma omp parallel for IPP_OMP_NUM_THREADS() \
  shared(pSrc,pDst,SrcStep,DstStep,roiSize) \
  private(i) default(none) \
  if((roiSize.height*roiSize.width) > (OMP_BOUNDARY))
#endif

  for(i = 0; i < roiSize.height; i++)
  {
    const Ipp8u* r;
    const Ipp8u* g;
    const Ipp8u* b;
    Ipp8u* y;
    Ipp8u* cb;
    Ipp8u* cr;

    r = pSrc[0] + i * SrcStep;
    g = pSrc[1] + i * SrcStep;
    b = pSrc[2] + i * SrcStep;

    y  = pDst[0] + i * DstStep;
    cb = pDst[1] + i * DstStep;
    cr = pDst[2] + i * DstStep;

    mfxownpj_RGBToYCbCr_JPEG_8u_P3R(r, g, b, y, cb, cr, roiSize.width);
  }

  return ippStsNoErr;
} /* mfxiRGBToYCbCr_JPEG_8u_P3R() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiRGBToYCbCr_JPEG_8u_C3P3R
//
//  Purpose:
//    RGB to YCbCr color convert
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
//      Y  =  0.29900*R + 0.58700*G + 0.11400*B
//      Cb = -0.16874*R - 0.33126*G + 0.50000*B + 128.0
//      Cr =  0.50000*R - 0.41869*G - 0.08131*B + 128.0
*/

IPPFUN(IppStatus, mfxiRGBToYCbCr_JPEG_8u_C3P3R, (
  const Ipp8u*   pSrc,
        int      SrcStep,
        Ipp8u*   pDst[3],
        int      DstStep,
        IppiSize roiSize))
{
  int   i;

  IPP_BAD_ENC_CC_C3P3_RET()

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
    Ipp8u* cb;
    Ipp8u* cr;

    rgb = pSrc + i * SrcStep;

    y  = pDst[0] + i * DstStep;
    cb = pDst[1] + i * DstStep;
    cr = pDst[2] + i * DstStep;

    mfxownpj_RGBToYCbCr_JPEG_8u_C3P3R(rgb, y, cb, cr, roiSize.width);
  }

  return ippStsNoErr;
} /* mfxiRGBToYCbCr_JPEG_8u_C3P3R() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiBGRToYCbCr_JPEG_8u_C3P3R
//
//  Purpose:
//    BGR to YCbCr color convert
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
//      Y  =  0.29900*R + 0.58700*G + 0.11400*B
//      Cb = -0.16874*R - 0.33126*G + 0.50000*B + 128.0
//      Cr =  0.50000*R - 0.41869*G - 0.08131*B + 128.0
*/

IPPFUN(IppStatus, mfxiBGRToYCbCr_JPEG_8u_C3P3R, (
  const Ipp8u*   pSrc,
        int      SrcStep,
        Ipp8u*   pDst[3],
        int      DstStep,
        IppiSize roiSize))
{
  int   i;

  IPP_BAD_ENC_CC_C3P3_RET()

#ifdef _OPENMP
#pragma omp parallel for IPP_OMP_NUM_THREADS() \
  shared(pSrc,pDst,SrcStep,DstStep,roiSize) \
  private(i) default(none) \
  if((roiSize.height*roiSize.width) > (OMP_BOUNDARY))
#endif

  for(i = 0; i < roiSize.height; i++)
  {
    const Ipp8u* bgr;
    Ipp8u* y;
    Ipp8u* cb;
    Ipp8u* cr;

    bgr = pSrc + i * SrcStep;

    y  = pDst[0] + i * DstStep;
    cb = pDst[1] + i * DstStep;
    cr = pDst[2] + i * DstStep;

    mfxownpj_BGRToYCbCr_JPEG_8u_C3P3R(bgr, y, cb, cr, roiSize.width);
  }

  return ippStsNoErr;
} /* mfxiBGRToYCbCr_JPEG_8u_C3P3R() */
