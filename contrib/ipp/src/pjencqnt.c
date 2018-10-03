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
//    Quantization functions (Forward transform)
//
//  Contents:
//    mfxiQuantFwdRawTableInit_JPEG_8u
//    mfxiQuantFwdTableInit_JPEG_16s
//    mfxiQuantFwd8x8_JPEG_16s_C1I
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



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiQuantFwdRawTableInit_JPEG_8u
//
//  Purpose:
//    Adjust raw quantization table according quality factor
//
//  Parameters:
//    pRawQuantTable - pointer to raw Quantization table
//    qualityFactor  - JPEG quality factor (valid range 1...100)
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiQuantFwdRawTableInit_JPEG_8u, (
  Ipp8u* pQuantRawTable,
  int    qualityFactor))
{
  int i;
  int val;

  IPP_BAD_PTR1_RET(pQuantRawTable);
  IPP_BADARG_RET(qualityFactor <= 0,ippStsBadArgErr);

  // The basic table is used as-is (scaling 100) for a quality of 50.
  // Qualities 50..100 are converted to scaling percentage 200 - 2*Q;
  // note that at Q=100 the scaling is 0, which will cause
  // all the table entries 1 (hence, minimum quantization loss).
  // Qualities 1..50 are converted to scaling percentage 5000/Q.

  if(qualityFactor < 50)
    qualityFactor = 5000 / qualityFactor;
  else
    qualityFactor = 200 - (qualityFactor * 2);

  for(i = 0; i < DCTSIZE2; i++)
  {
    val = (pQuantRawTable[i] * qualityFactor + 50) / 100;
    if(val < 1)
    {
      pQuantRawTable[i] = (Ipp8u)1;
    }
    else if(val > 255)
    {
      pQuantRawTable[i] = (Ipp8u)255;
    }
    else
    {
      pQuantRawTable[i] = (Ipp8u)val;
    }
  }

  return ippStsNoErr;
} /* mfxiQuantFwdRawTableInit_JPEG_8u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiQuantFwdTableInit_JPEG_8u16u
//
//  Purpose:
//    Build Quantization table for encoder
//    in fast-to-use format from raw Quantization table
//
//  Parameter:
//    pQuantRawTable - pointer to "raw" Quantization table
//    pQuantFwdTable - pointer to Quantization table
//                     to be initialized
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiQuantFwdTableInit_JPEG_8u16u, (
  const Ipp8u*  pQuantRawTable,
        Ipp16u* pQuantFwdTable))
{
  IppiSize roi;
  IppStatus status;
  Ipp16u wb[DCTSIZE2];

  IPP_BAD_PTR2_RET(pQuantRawTable,pQuantFwdTable);

  roi.width  = 8;
  roi.height = 8;

  status = mfxiConvert_8u16u_C1R(pQuantRawTable,8,&wb[0],8*2,roi);
  if(ippStsNoErr != status)
  {
    return status;
  }

  status = mfxiZigzagInv8x8_16s_C1((const Ipp16s*)&wb[0],(Ipp16s*)pQuantFwdTable);
  if(ippStsNoErr != status)
  {
    return status;
  }

  status = mfxsDivCRev_16u_I((1 << QUANT_BITS),pQuantFwdTable,DCTSIZE2);
  if(ippStsNoErr != status)
  {
    return status;
  }

  return ippStsNoErr;
} /* mfxiQuantFwdTableInit_JPEG_8u16u() */

