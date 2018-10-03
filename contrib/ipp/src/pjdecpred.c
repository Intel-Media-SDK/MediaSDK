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
//    reconstruct sample from differences
//
//  Contents:
//    mfxiReconstructPredFirstRow_JPEG_16s_C1
//    mfxiReconstructPredRow_JPEG_16s_C1
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif




#if defined (_I7) || ( _IPP >= _IPP_W7 ) || ( _IPP32E >= _IPP32E_M7 )

ASMAPI(void,mfxownpj_ReconstructPredFirstRow_JPEG_16s_C1,(
            const Ipp16s*  pSrc,
                  Ipp16s*  pDst,
                  int      width,
                  int      P,
                  int      Pt));

ASMAPI(void,mfxownpj_ReconstructRow_PRED1_JPEG_16s_C1,(
            const Ipp16s*  pSrc,
            const Ipp16s*  pPrevRow,
                  Ipp16s*  pDst,
                  int      width));
#else

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxownpj_ReconstructRow_PRED1_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc
//    pPrevRow
//    pDst
//    width
//    Pt
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

LOCFUN(IppStatus, mfxownpj_ReconstructRow_PRED1_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
  const Ipp16s*  pPrevRow,
        Ipp16s*  pDst,
        int      width))
{
  int i;

  pDst[0] = (Ipp16s)(pSrc[0] + pPrevRow[0]);

  for(i = 1; i < width; i++)
  {
    pDst[i] = (Ipp16s)(pSrc[i] + pDst[i-1]);
  }

  return ippStsNoErr;
} /* mfxownpj_ReconstructRow_PRED1_JPEG_16s_C1() */

#endif


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxownpj_ReconstructRow_PRED2_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc
//    pPrevRow
//    pDst
//    width
//    Pt
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

LOCFUN(IppStatus, mfxownpj_ReconstructRow_PRED2_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
  const Ipp16s*  pPrevRow,
        Ipp16s*  pDst,
        int      width))
{
  int i;

  pDst[0] = (Ipp16s)(pSrc[0] + pPrevRow[0]);

  for(i = 0; i < width; i++)
  {
    pDst[i] = (Ipp16s)(pSrc[i] + pPrevRow[i]);
  }

  return ippStsNoErr;
} /* mfxownpj_ReconstructRow_PRED2_JPEG_16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxownpj_ReconstructRow_PRED3_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc
//    pPrevRow
//    pDst
//    width
//    Pt
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

LOCFUN(IppStatus, mfxownpj_ReconstructRow_PRED3_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
  const Ipp16s*  pPrevRow,
        Ipp16s*  pDst,
        int      width))
{
  int i;

  pDst[0] = (Ipp16s)(pSrc[0] + pPrevRow[0]);

  for(i = 1; i < width; i++)
  {
    pDst[i] = (Ipp16s)(pSrc[i] + pPrevRow[i-1]);
  }

  return ippStsNoErr;
} /* mfxownpj_ReconstructRow_PRED3_JPEG_16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxownpj_ReconstructRow_PRED4_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc
//    pPrevRow
//    pDst
//    width
//    Pt
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

LOCFUN(IppStatus, mfxownpj_ReconstructRow_PRED4_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
  const Ipp16s*  pPrevRow,
        Ipp16s*  pDst,
        int      width))
{
  int i;

  pDst[0] = (Ipp16s)(pSrc[0] + pPrevRow[0]);

  for(i = 1; i < width; i++)
  {
    pDst[i] = (Ipp16s)(pSrc[i] + (pDst[i-1] + pPrevRow[i] - pPrevRow[i-1]));
  }

  return ippStsNoErr;
} /* mfxownpj_ReconstructRow_PRED4_JPEG_16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxownpj_ReconstructRow_PRED5_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc
//    pPrevRow
//    pDst
//    width
//    Pt
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

LOCFUN(IppStatus, mfxownpj_ReconstructRow_PRED5_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
  const Ipp16s*  pPrevRow,
        Ipp16s*  pDst,
        int      width))
{
  int i;

  pDst[0] = (Ipp16s)(pSrc[0] + pPrevRow[0]);

  for(i = 1; i < width; i++)
  {
    pDst[i] = (Ipp16s)(pSrc[i] + (pDst[i-1] + ((pPrevRow[i] - pPrevRow[i-1]) >> 1)));
  }

  return ippStsNoErr;
} /* mfxownpjReconstructRow_PRED5_JPEG_16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxownpj_ReconstructRow_PRED6_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc
//    pPrevRow
//    pDst
//    width
//    Pt
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

LOCFUN(IppStatus, mfxownpj_ReconstructRow_PRED6_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
  const Ipp16s*  pPrevRow,
        Ipp16s*  pDst,
        int      width))
{
  int i;

  pDst[0] = (Ipp16s)(pSrc[0] + pPrevRow[0]);

  for(i = 1; i < width; i++)
  {
    pDst[i] = (Ipp16s)(pSrc[i] + (pPrevRow[i] + ((pDst[i-1] - pPrevRow[i-1]) >> 1)));
  }

  return ippStsNoErr;
} /* mfxownpj_ReconstructRow_PRED6_JPEG_16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxownpj_ReconstructRow_PRED7_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc
//    pPrevRow
//    pDst
//    width
//    Pt
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

LOCFUN(IppStatus, mfxownpj_ReconstructRow_PRED7_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
  const Ipp16s*  pPrevRow,
        Ipp16s*  pDst,
        int      width))
{
  int i;

  pDst[0] = (Ipp16s)(pSrc[0] + pPrevRow[0]);

  for(i = 1; i < width; i++)
  {
    pDst[i] = (Ipp16s)(pSrc[i] + ((pDst[i-1] + pPrevRow[i]) >> 1));
  }

  return ippStsNoErr;
} /* mfxownpj_ReconstructRow_PRED7_JPEG_16s_C1() */



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiReconstructPredFirstRow_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc
//    pDst
//    width
//    P
//    Pt
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiReconstructPredFirstRow_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
        Ipp16s*  pDst,
        int      width,
        int      P,
        int      Pt))
{
#if !defined (_I7) && !( _IPP >= _IPP_W7 ) && !( _IPP32E >= _IPP32E_M7 )
  int i;
#endif

  IPP_BAD_PTR2_RET(pSrc,pDst);
  IPP_BAD_SIZE_RET(width);
  OWN_BADARG_RET(P < 2 || P > 16);
  OWN_BADARG_RET(Pt < 0);

#if defined (_I7) || ( _IPP >= _IPP_W7 ) || ( _IPP32E >= _IPP32E_M7 )
  mfxownpj_ReconstructPredFirstRow_JPEG_16s_C1(pSrc, pDst, width, P, Pt);
#else
  pDst[0] = (Ipp16s)(pSrc[0] + (1 << (P - Pt - 1)));

  for(i = 1; i < width; i++)
  {
    pDst[i] = (Ipp16s)(pSrc[i] + pDst[i-1]);
  }
#endif

  return ippStsNoErr;
} /* mfxiReconstructPredFirstRow_JPEG_16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiReconstructPredRow_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc
//    pPrevRow
//    pDst
//    width
//    predictor
//    Pt
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiReconstructPredRow_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
  const Ipp16s*  pPrevRow,
        Ipp16s*  pDst,
        int      width,
        int      predictor))
{
  IPP_BAD_PTR3_RET(pSrc,pPrevRow,pDst);
  IPP_BAD_SIZE_RET(width);

  switch(predictor)
  {
  case PRED1:
    mfxownpj_ReconstructRow_PRED1_JPEG_16s_C1(pSrc,pPrevRow,pDst,width);
    break;

  case PRED2:
    mfxownpj_ReconstructRow_PRED2_JPEG_16s_C1(pSrc,pPrevRow,pDst,width);
    break;

  case PRED3:
    mfxownpj_ReconstructRow_PRED3_JPEG_16s_C1(pSrc,pPrevRow,pDst,width);
    break;

  case PRED4:
    mfxownpj_ReconstructRow_PRED4_JPEG_16s_C1(pSrc,pPrevRow,pDst,width);
    break;

  case PRED5:
    mfxownpj_ReconstructRow_PRED5_JPEG_16s_C1(pSrc,pPrevRow,pDst,width);
    break;

  case PRED6:
    mfxownpj_ReconstructRow_PRED6_JPEG_16s_C1(pSrc,pPrevRow,pDst,width);
    break;

  case PRED7:
    mfxownpj_ReconstructRow_PRED7_JPEG_16s_C1(pSrc,pPrevRow,pDst,width);
    break;

  default:
    return ippStsBadArgErr;
  }

  return ippStsNoErr;
} /* mfxiReconstructPredRow_JPEG_16s_C1() */

