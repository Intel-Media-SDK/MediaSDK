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
//   Inverse DCT transform with scale image size and
//   de-quantization and level shift
//
//  Contents:
//   mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R
//   mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R
//
//
*/


#ifndef __OWNJ_H__
#include "ownj.h"
#endif

#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)

extern void mfxdct_quant_inv8x8To2x2_ls(const Ipp16s* pSrc,Ipp8u* pDst,int dstStep,const Ipp16u* pQntInvTbl);
extern void mfxdct_quant_inv8x8To4x4_ls(const Ipp16s* pSrc,Ipp8u* pDst,int dstStep,const Ipp16u* pQntInvTbl);

#else

static void mfxdct_quant_inv8x8To4x4_ls(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable)
{
  Ipp16s*   workbuf;
  Ipp8u     buf[DCTSIZE2 * sizeof(Ipp16s) + CPU_CACHE_LINE-1];
  IppiSize  roi = { 4, 4 };

  workbuf = (Ipp16s*)IPP_ALIGNED_PTR(&buf[0],CPU_CACHE_LINE);

  mfxsMul_16s(pSrc,(const Ipp16s*)pQuantInvTable,workbuf,DCTSIZE2);

  mfxiDCT8x8To4x4Inv_16s_C1I(workbuf);

  mfxiAdd128_JPEG_16s8u_C1R(workbuf, 4*sizeof(Ipp16s), pDst, dstStep, roi);

  return;
} /* mfxdct_quant_inv8x8To4x4_ls() */


static void mfxdct_quant_inv8x8To2x2_ls(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable)
{
  Ipp16s*   workbuf;
  Ipp8u     buf[DCTSIZE2 * sizeof(Ipp16s) + CPU_CACHE_LINE-1];
  IppiSize  roi = { 2, 2 };

  workbuf = (Ipp16s*)IPP_ALIGNED_PTR(&buf[0],CPU_CACHE_LINE);

  mfxsMul_16s(pSrc,(const Ipp16s*)pQuantInvTable,workbuf,DCTSIZE2);

  mfxiDCT8x8To2x2Inv_16s_C1I(workbuf);

  mfxiAdd128_JPEG_16s8u_C1R(workbuf, 2*sizeof(Ipp16s), pDst, dstStep, roi);

  return;
} /* mfxdct_quant_inv8x8To2x2_ls() */

#endif


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R
//
//  Purpose:
//    Inverse DCT transform with scale image to 1/2 size and
//    de-quantization and level shift
//
//  Parameter:
//    pSrc           - pointer to source
//    pDst           - pointer to output array
//    dstStep        - line offset for output data
//    pQuantInvTable - pointer to Quantization table
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R,(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable))
{
  IPP_BAD_PTR2_RET(pSrc, pDst)
  IPP_BAD_STEP_RET(dstStep)
  IPP_BAD_PTR1_RET(pQuantInvTable)

  mfxdct_quant_inv8x8To4x4_ls(pSrc, pDst, dstStep, pQuantInvTable);

  return  ippStsNoErr;
} /* mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R
//
//  Purpose:
//    Inverse DCT transform with scale image to 1/4 size and
//    de-quantization and level shift
//
//  Parameter:
//    pSrc           - pointer to source
//    pDst           - pointer to output array
//    dstStep        - line offset for output data
//    pQuantInvTable - pointer to Quantization table
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R,(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable))
{
  IPP_BAD_PTR2_RET(pSrc, pDst)
  IPP_BAD_STEP_RET(dstStep)
  IPP_BAD_PTR1_RET(pQuantInvTable)

  mfxdct_quant_inv8x8To2x2_ls(pSrc, pDst, dstStep, pQuantInvTable);

  return  ippStsNoErr;
} /* mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R() */

