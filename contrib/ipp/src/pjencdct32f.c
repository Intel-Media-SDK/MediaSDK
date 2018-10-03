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
//    DCT + quantization+level shift + range convert functions (Forward transform)
//
//  Contents:
//    mfxiDCTQuantFwd8x8LS_JPEG_16u16s_C1R
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif


#if (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)

extern void mfxdct_qnt_fwd_8x8_ls(const Ipp16u* pSrc,int srcStep,Ipp16s* pDst,const Ipp32f* pQntFwdTbl);

#else

static void mfxdct_qnt_fwd_8x8_ls(const Ipp16u* pSrc,int srcStep,Ipp16s* pDst,const Ipp32f* pQntFwdTbl)
{
  int i;
  __ALIGN16 Ipp32f wb[DCTSIZE2];

  for(i = 0; i < DCTSIZE; i++)
    ippsConvert_16u32f((Ipp16u*)((Ipp8u*)pSrc + i*srcStep), wb + i*DCTSIZE, DCTSIZE);

  ippsSubC_32f_I(2048.0, wb, DCTSIZE2);
  mfxiDCT8x8Fwd_32f_C1I(wb);
  ippsMul_32f_I(pQntFwdTbl, wb, DCTSIZE2);

  ippsConvert_32f16s_Sfs(wb, pDst, DCTSIZE2, ippRndNear, 0);

  return;
} /* mfxdct_qnt_fwd_8x8_ls() */

#endif




/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantFwd8x8LS_JPEG_16u16s_C1R
//
//  Purpose:
//    Forward DCT transform, quantization and level shift
//
//  Parameter:
//    pSrc           - pointer to source
//    srcStep        - line offset for source data
//    pDst           - pointer to output array
//    pQuantFwdTable - pointer to Quantization table
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDCTQuantFwd8x8LS_JPEG_16u16s_C1R,(
  const Ipp16u* pSrc,
        int     srcStep,
        Ipp16s* pDst,
  const Ipp32f* pQuantFwdTable))
{
  IPP_BAD_PTR2_RET(pSrc, pDst);
  IPP_BAD_STEP_RET(srcStep);
  IPP_BAD_PTR1_RET(pQuantFwdTable);

  mfxdct_qnt_fwd_8x8_ls(pSrc,srcStep,pDst,pQuantFwdTable);

  return ippStsNoErr;
} /* mfxiDCTQuantFwd8x8LS_JPEG_16u16s_C1R() */
