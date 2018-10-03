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
//    DCT+quantization+level shift+range convert functions (Forward transform)
//
//  Contents:
//    mfxiDCTQuantFwd8x8LS_JPEG_8u16s_C1R
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#ifndef __PS_ANARITH_H__
#include "ps_anarith.h"
#endif
#ifndef __PJQUANT_H__
#include "pjquant.h"
#endif

#if ((_IPP>=_IPP_H9)||(_IPP32E>=_IPP32E_L9))
extern void mfxownDCTQuantFwd8x8LS_JPEG_8u16s_C1R(const Ipp8u* pSrc, int srcStep,
        Ipp16s* pDst, const Ipp16u* pQuantFwdTable);
#endif

#if IPPJ_QNT_OPT || (_IPPXSC >= _IPPXSC_S2)
ASMAPI(void, mfxdct_8x8_fwd_16s, (Ipp16s*, Ipp16s*));
ASMAPI(void, mfxownpj_Sub128_8x8_8u16s, (const Ipp8u*, int, Ipp16s*));
ASMAPI(void, mfxownps_Mul_16u16s_Sfs, (const Ipp16u*, const Ipp16s*, Ipp16s*, int, int));
#endif

#if IPPJ_QNT_OPT
ASMAPI(void, mfxownsMul_16u16s_PosSfs, (const Ipp16u*, const Ipp16s*, Ipp16s*, int, int));
#endif


/* ---------------------- library functions definitions -------------------- */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantFwd8x8LS_JPEG_8u16s_C1R
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

IPPFUN(IppStatus, mfxiDCTQuantFwd8x8LS_JPEG_8u16s_C1R, (
  const Ipp8u*  pSrc,
        int     srcStep,
        Ipp16s* pDst,
  const Ipp16u* pQuantFwdTable))
{
  IPP_BAD_PTR2_RET(pSrc,pDst)
  IPP_BAD_STEP_RET(srcStep)
  IPP_BAD_PTR1_RET(pQuantFwdTable)

#if ((_IPP>=_IPP_H9)||(_IPP32E>=_IPP32E_L9))
    mfxownDCTQuantFwd8x8LS_JPEG_8u16s_C1R(pSrc, srcStep, pDst, pQuantFwdTable);
#else
#if IPPJ_QNT_OPT || (_IPPXSC >= _IPPXSC_S2)
  {
    mfxownpj_Sub128_8x8_8u16s(pSrc,srcStep,pDst);
    mfxdct_8x8_fwd_16s(pDst,pDst);
#if IPPJ_QNT_OPT
    mfxownsMul_16u16s_PosSfs(pQuantFwdTable,(Ipp16s*)pDst,pDst,DCTSIZE2,QUANT_BITS);
#else
    mfxownps_Mul_16u16s_Sfs(pQuantFwdTable,(Ipp16s*)pDst,pDst,DCTSIZE2,QUANT_BITS);
#endif
  }
#else
  {
    IppiSize roi = { 8, 8 };

    mfxownpj_Sub128_JPEG_8u16s_C1R(pSrc,srcStep,pDst,8*sizeof(Ipp16s),roi);
    mfxiDCT8x8Fwd_16s_C1I(pDst);
    mfxsMul_16u16s_Sfs(pQuantFwdTable,pDst,pDst,DCTSIZE2,QUANT_BITS);
  }
#endif
#endif

  return ippStsNoErr;
} /* mfxiDCTQuantFwd8x8LS_JPEG_8u16s_C1R() */
