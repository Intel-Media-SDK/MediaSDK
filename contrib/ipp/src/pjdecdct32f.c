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
//    DCT + quantization+level shift and
//    range convert functions (Inverse transform)
//
//  Contents:
//    mfxiDCTQuantInv8x8LS_JPEG_16s16u_C1
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif


#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)

extern void mfxdct_qnt_inv_8x8_ls(const Ipp16s* pSrc,Ipp16u* pDst,int dstStep,const Ipp32f* pQntInvTbl);

#else

static void mfxdct_qnt_inv_8x8_ls(const Ipp16s* pSrc,Ipp16u* pDst,int dstStep,const Ipp32f* pQntInvTbl)
{
  int i;
  IppiSize roi = { DCTSIZE, DCTSIZE };
  __ALIGN16 Ipp32f wb[DCTSIZE2];

  mfxsConvert_16s32f(pSrc, wb, DCTSIZE2);

  mfxsMul_32f_I(pQntInvTbl, wb, DCTSIZE2);

  mfxiDCT8x8Inv_32f_C1I(wb);

  mfxsAddC_32f_I(2048.0, wb, DCTSIZE2);

  for(i = 0; i < DCTSIZE; i++)
    mfxsConvert_32f16u_Sfs(wb + i*DCTSIZE, (Ipp16u*)((Ipp8u*)pDst + i*dstStep), DCTSIZE, ippRndNear, 0);

  mfxiThreshold_16u_C1IR(pDst,dstStep,roi,4095,ippCmpGreater);

  return;
} /* mfxdct_qnt_inv_8x8_ls() */

#endif


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantInv8x8LS_JPEG_16s16u_C1R
//
//  Purpose:
//    Inverse DCT transform, de-quantization and level shift
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

IPPFUN(IppStatus, mfxiDCTQuantInv8x8LS_JPEG_16s16u_C1R,(
  const Ipp16s* pSrc,
        Ipp16u* pDst,
        int     dstStep,
  const Ipp32f* pQuantInvTable))
{
  IPP_BAD_PTR2_RET(pSrc, pDst)
  IPP_BAD_STEP_RET(dstStep)
  IPP_BAD_PTR1_RET(pQuantInvTable)

  mfxdct_qnt_inv_8x8_ls(pSrc,pDst,dstStep,pQuantInvTable);

  return ippStsNoErr;
} /* mfxiDCTQuantInv8x8LS_JPEG_16s16u_C1R() */
