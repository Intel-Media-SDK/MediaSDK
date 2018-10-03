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
//    Quantization functions (Inverse transform)
//
//  Contents:
//    mfxiQuantInvTableInit_JPEG_16s
//    mfxiQuant8x8Inv_JPEG_16s_C1I
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#ifndef __PJQUANT_H__
#include "pjquant.h"
#endif



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiQuantInvTableInit_JPEG_8u16u
//
//  Purpose:
//    Build Quantization table for decoder
//    in fast-to-use format from raw Quantization table
//
//  Parameter:
//    pQuantInvTable - pointer to pointer on Quantization table
//                     to be initialized
//    pQuantRawTable - pointer to raw Quantization table
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiQuantInvTableInit_JPEG_8u16u, (
  const Ipp8u*  pQuantRawTable,
        Ipp16u* pQuantInvTable))
{
  IppiSize roi;
  IppStatus status;
  Ipp16u wb[DCTSIZE2];

  IPP_BAD_PTR1_RET(pQuantInvTable);
  IPP_BAD_PTR1_RET(pQuantRawTable);

  roi.width  = 8;
  roi.height = 8;

  status = mfxiConvert_8u16u_C1R(pQuantRawTable,8,&wb[0],8*2,roi);
  if(ippStsNoErr != status)
  {
    return status;
  }

  status = mfxiZigzagInv8x8_16s_C1((Ipp16s*)&wb[0],(Ipp16s*)pQuantInvTable);
  if(ippStsNoErr != status)
  {
    return status;
  }

  return ippStsNoErr;
} /* mfxiQuantInvTableInit_JPEG_8u16u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiQuantInv8x8_JPEG_16s_C1I
//
//  Purpose:
//    dequantize 8x8 block of DCT coefs
//
//  Parameters:
//    pSrcDst        - pointer to 8x8 block of DCT coefficients
//    pQuantInvTable - pointer to Quantization table
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiQuantInv8x8_JPEG_16s_C1I, (
  Ipp16s*        pSrcDst,
  const Ipp16u*  pQuantInvTable))
{
#if (_IPP == _IPP_A6) || (_IPP64 >= _IPP64_I7) || (_IPPXSC >= _IPPXSC_S2)
  IPP_BAD_PTR2_RET(pSrcDst,pQuantInvTable);
  mfxownpj_QuantInv_8x8_16s(pSrcDst,pSrcDst,pQuantInvTable);
  return ippStsNoErr;
#elif (_IPP >= _IPP_W7) || (_IPP32E >= _IPP32E_M7)
  IPP_BAD_PTR2_RET(pSrcDst,pQuantInvTable);
  if(IPP_UINT_PTR(pSrcDst) & 0x01 || IPP_UINT_PTR(pQuantInvTable) & 0x01)
    mfxsMul_16s_I((Ipp16s*)pQuantInvTable,pSrcDst,DCTSIZE2);
  else
    mfxownpj_QuantInv_8x8_16s_I(pSrcDst,pQuantInvTable);
  return ippStsNoErr;
#else
  return mfxsMul_16s_I((Ipp16s*)pQuantInvTable,pSrcDst,DCTSIZE2);
#endif
} /* mfxiQuantInv8x8_JPEG_16s_C1I() */
