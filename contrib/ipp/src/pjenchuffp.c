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
//  Purpose:
//    progressive Huffman entropy encoder
//
//  Contents:
//    mfxiGetHuffmanStatistics8x8_DCFirst_JPEG_16s_C1,
//    mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1,
//    mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1,
//    mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1,
//    mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1,
//    mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1,
//    mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#ifndef __PJZIGZAG_H__
#include "pjzigzag.h"
#endif
#ifndef __PJHUFFTBL_H__
#include "pjhufftbl.h"
#endif
#ifndef __PJENCHUFF_H__
#include "pjenchuff.h"
#endif




LOCFUN(int,mfxownpj_get_csize,(
  int data))
{
  int index;
  int ssss;

  index = data < 0 ? -data : data;

  if(index < 256)
  {
    ssss = mfxown_pj_csize[index];
  }
  else
  {
    ssss = mfxown_pj_csize[index >> 8] + 8;
  }

  return ssss;
} /* mfxownpj_get_csize() */

LOCFUN(int,mfxownpj_get_eobsize,(
  int EOBRUN))
{
  int ssss;

  if(EOBRUN < 256)
  {
    ssss = mfxown_pj_eobsize[EOBRUN];
  }
  else
  {
    ssss = mfxown_pj_eobsize[EOBRUN >> 8] + 8;
  }

  return ssss;
} /* mfxownpj_get_eobsize() */


LOCFUN(IppStatus,mfxownpj_count_eobrun,(
  int* pEOBRun,
  int  pAcStatistics[256]))
{
  int I;
  int SSSS;

  if(*pEOBRun == 0)
  {
    return ippStsNoErr;
  }

  if(*pEOBRun == 1)
  {
    pAcStatistics[0]++; /* EOB0 */
  }
  else
  {
    if(*pEOBRun > 32767)
    {
      pAcStatistics[0xe0]++; /* EOB14 */
      *pEOBRun -= 32767;
    }

    SSSS = mfxownpj_get_eobsize(*pEOBRun);

    /* 14 - is max category for EOB codes */
    if(SSSS > 14)
    {
      return ippStsJPEGDCTRangeErr;
    }

    I = SSSS << 4;

    pAcStatistics[I]++;
  }

  *pEOBRun = 0;

  return ippStsNoErr;
} /* mfxownpj_count_eobrun() */

/* ---------------------- library functions definitions -------------------- */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiGetHuffmanStatistics8x8_DCFirst_JPEG_16s_C1
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiGetHuffmanStatistics8x8_DCFirst_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
        int      pDcStatistics[256],
        Ipp16s*  pLastDC,
        int      Al))
{
  int pt;
  int DIFF;
  int SSSS;

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_PTR1_RET(pDcStatistics);
  IPP_BAD_PTR1_RET(pLastDC);
  OWN_BADARG_RET(Al < 0 || Al > 13);

  pt = pSrc[0] >> Al;

  DIFF = pt - *pLastDC;

  *pLastDC = (Ipp16s)pt;

  SSSS = mfxownpj_get_csize(DIFF);

  /* 11 - is max category for DC coefficient */
  if(SSSS > 11)
  {
    return ippStsJPEGDCTRangeErr;
  }

  pDcStatistics[SSSS]++;

  return ippStsNoErr;
} /* mfxiGetHuffmanStatistics8x8_DCFirst_JPEG_16s_C1() */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1, (
  const Ipp16s*           pSrc,
  int                     pAcStatistics[256],
  int                     Ss,
  int                     Se,
  int                     Al,
  IppiEncodeHuffmanState* pEncHuffState,
  int                     bFlushState))
{
  int k;
  int r;
  int temp;
  int nBits;
  ownpjEncodeHuffmanState* pState;
  IppStatus status = ippStsNoErr;

  IPP_BAD_PTR1_RET(pAcStatistics);
  IPP_BAD_PTR1_RET(pEncHuffState);

  pState = (ownpjEncodeHuffmanState*)pEncHuffState;

  if(bFlushState != 0)
  {
    /* count EOBRUN */
    status = mfxownpj_count_eobrun(&pState->nEndOfBlockRun,pAcStatistics);

    mfxownpj_EncodeHuffmanStateInit(pState);

    return status;
  }

  IPP_BAD_PTR1_RET(pSrc);
  OWN_BADARG_RET(Ss < 1  || Ss > 63);
  OWN_BADARG_RET(Se < Ss || Se > 63);
  OWN_BADARG_RET(Al < 0  || Al > 13);

  /* Encode the AC coefficients per section G.1.2.2, fig. G.3 */

  r = 0;      /* r = run length of zeros */

  for(k = Ss; k <= Se; k++)
  {
    temp = pSrc[mfxown_pj_izigzag_index[k]];
    if(temp == 0)
    {
      r++;
      continue;
    }

    /* We must apply the point transform by Al. For AC coefficients this    */
    /* is an integer division with rounding towards 0. To do this portably  */
    /* in C, we shift after obtaining the absolute value; so the code is    */
    /* interwoven with finding the abs value (temp) and output bits (temp2) */

    if(temp < 0)
    {
      temp = -temp;   /* temp is abs value of input */
      temp >>= Al;    /* apply the point transform  */
    }
    else
    {
      temp >>= Al;    /* apply the point transform  */
    }

    /* Watch out for case that nonzero coef is zero after point transform */

    if(temp == 0)
    {
      r++;
      continue;
    }

    /* count EOBRUN */
    status = mfxownpj_count_eobrun(&pState->nEndOfBlockRun,pAcStatistics);
    if(ippStsNoErr != status)
    {
      return status;
    }

    /* if run length > 15, must emit special run-length-16 codes (0xF0) */
    while(r > 15)
    {
      pAcStatistics[0xf0]++;
      r -= 16;
    }

    /* Find the number of bits needed for the magnitude of the coefficient */
    /* there must be at least one 1 bit                                    */
    nBits = 1;
    while((temp >>= 1))
    {
      nBits++;
    }

    /* Check for out-of-range coefficient values */
    if(nBits > 10)
    {
      return ippStsJPEGDCTRangeErr;
    }

    /* Count Huffman symbol for run length / number of bits */
    pAcStatistics[(r << 4) + nBits]++;

    /* reset zero run length */
    r = 0;
  }

  if(r > 0)
  {
    /* If there are trailing zeroes, */
    /* count an EOB                  */
    pState->nEndOfBlockRun++;
    if(pState->nEndOfBlockRun == 32767)
    {
      status = mfxownpj_count_eobrun(&pState->nEndOfBlockRun,pAcStatistics);
      if(ippStsNoErr != status)
      {
        return status;
      }
    }
  }

  return status;
} /* mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1() */


#define MAX_CORR_BITS 1000

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1, (
  const Ipp16s*                 pSrc,
        int                     pAcStatistics[256],
        int                     Ss,
        int                     Se,
        int                     Al,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))
{
#if !( _IPP >= _IPP_A6 ) && !( _IPP32E >= _IPP32E_M7 )/**/
  int    r;
  int    k;
  int    temp;
  int    nEOB;
  int    nBE;
  int    nBR;
  /*Ipp8u* pBRBuffer = NULL;*/
  int    absvalues[DCTSIZE2];
#endif

  IppStatus status = ippStsNoErr;
  ownpjEncodeHuffmanState* pState;

  pState = (ownpjEncodeHuffmanState*)pEncHuffState;

  IPP_BAD_PTR1_RET(pAcStatistics);
  IPP_BAD_PTR1_RET(pEncHuffState);

  if(bFlushState != 0)
  {
#if ( _IPP >= _IPP_A6 ) || ( _IPP32E >= _IPP32E_M7 )/**/
    return mfxownpj_GetHuffmanStatistics8x8_ACFlush_JPEG_16s_C1(
             pAcStatistics,pState);
#else
    /* count EOBRUN */
    status = mfxownpj_count_eobrun(&pState->nEndOfBlockRun,pAcStatistics);

    mfxownpj_EncodeHuffmanStateInit(pState);

    goto Exit;
#endif
  }

  IPP_BAD_PTR1_RET(pSrc);
  OWN_BADARG_RET(Ss < 1  || Ss > 63);
  OWN_BADARG_RET(Se < Ss || Se > 63);
  OWN_BADARG_RET(Al < 0  || Al > 13);

  /* It is convenient to make a pre-pass to determine the transformed */
  /* coefficients' absolute values and the EOB position.              */

#if ( _IPP >= _IPP_A6 ) || ( _IPP32E >= _IPP32E_M7 )/**/
    status = mfxownpj_GetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1(
               pSrc,pAcStatistics,Ss,Se,Al,pState);
#else
  nBE  = pState->nBE;
  nEOB = 0;

  for(k = Ss; k <= Se; k++)
  {
    temp = pSrc[mfxown_pj_izigzag_index[k]];

    /* We must apply the point transform by Al. For AC coefficients this   */
    /* is an integer division with rounding towards 0. To do this portably */
    /* in C, we shift after obtaining the absolute value.                  */

    /* temp is abs value of input */
    if(temp < 0)
    {
      temp = -temp;
    }

    /* apply the point transform */
    temp >>= Al;

    /* save abs value for main pass */
    absvalues[k] = temp;

    /* EOB = index of last newly-nonzero coef */
    if(temp == 1)
    {
      nEOB = k;
    }
  }

  /* Encode the AC coefficients per section G.1.2.3, fig. G.7 */

  r         = 0;                     /* r = run length of zeros               */
  nBR       = 0;                     /* BR = count of buffered bits added now */
  /*pBRBuffer = pState->pBEBuff + nBE;*/ /* Append bits to buffer     */

  for(k = Ss; k <= Se; k++)
  {
    temp = absvalues[k];
    if(temp == 0)
    {
      r++;
      continue;
    }

    /* Emit any required ZRLs, but not if they can be folded into EOB */
    while(r > 15 && k <= nEOB)
    {
      /* count EOBRUN */
      status = mfxownpj_count_eobrun(&pState->nEndOfBlockRun,pAcStatistics);
      if(ippStsNoErr != status)
      {
        return status;
      }

      /* Emit any buffered correction bits */
      /* nothing to do                     */
      nBE = 0;

      /* Emit ZRL */
      while(r > 15)
      {
        pAcStatistics[0xf0]++;
        r -= 16;
      }

      /* Emit buffered correction bits that must be associated with ZRL */
      /* nothing to do                                                  */

      /* BE bits are gone now */
      /*pBRBuffer = pState->pBEBuff;*/
      nBR = 0;
    }

    /* If the coef was previously nonzero, it only needs a correction bit.   */
    /* NOTE: a straight translation of the spec's figure G.7 would suggest   */
    /* that we also need to test r > 15. But if r > 15, we can only get here */
    /* if k > EOB, which implies that this coefficient is not 1.             */

    if(temp > 1)
    {
      /* The correction bit is the next bit of the absolute value. */
      /*pBRBuffer[nBR] = (Ipp8u)(temp & 1);*/
      nBR++;
      continue;
    }

    /* count EOBRUN */
    status = mfxownpj_count_eobrun(&pState->nEndOfBlockRun,pAcStatistics);
    if(ippStsNoErr != status)
    {
      return status;
    }

    /* Emit any buffered correction bits */
    /* nothing to do                     */
    nBE = 0;

    /* Count/emit Huffman symbol for run length / number of bits */
    pAcStatistics[(r << 4) + 1]++;

    /* Emit output bit for newly-nonzero coef */
    /* nothing to do                          */

    /* Emit buffered correction bits that must be associated with this code */
    /* nothing to do                                                        */

    /* BE bits are gone now */
    /*pBRBuffer = pState->pBEBuff;*/
    nBR = 0;

    /* reset zero run length */
    r = 0;
  }

  /* If there are trailing zeroes */
  if(r > 0 || nBR > 0)
  {
    /* count an EOB */
    pState->nEndOfBlockRun++;

    /* concat my correction bits to older ones */
    nBE += nBR;

    /* We force out the EOB if we risk either:                         */
    /*   1. overflow of the EOB counter;                               */
    /*   2. overflow of the correction bit buffer during the next MCU. */

    if((pState->nEndOfBlockRun == 32767) ||
       (nBE > (MAX_CORR_BITS - DCTSIZE2 + 1)))
    {
      status = mfxownpj_count_eobrun(&pState->nEndOfBlockRun,pAcStatistics);
      if(ippStsNoErr != status)
      {
        return status;
      }
      /* Emit any buffered correction bits */
      /* nothing to do                     */
      nBE = 0;
    }
  }

  pState->nBE = nBE;

Exit:

#endif

  return status;
} /* mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1() */



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1, (
  const Ipp16s*                  pSrc,
        Ipp8u*                   pDst,
        int                      nDstLenBytes,
        int*                     pDstCurrPos,
        Ipp16s*                  pLastDc,
        int                      Al,
  const IppiEncodeHuffmanSpec*   pDcTable,
        IppiEncodeHuffmanState*  pEncHuffState,
        int                      bFlushState))
{
  int    pt;
  int    data;
  int    ssss;
  unsigned int cs;
  unsigned int uValue;
  int    nBits;
  ownpjEncodeHuffmanSpec*  dc_table;
  ownpjEncodeHuffmanState* pState;
  IppStatus status = ippStsNoErr;

  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_SIZE_RET(nDstLenBytes);
  IPP_BAD_PTR1_RET(pDstCurrPos);
  IPP_BAD_PTR1_RET(pEncHuffState);

  dc_table = (ownpjEncodeHuffmanSpec*)pDcTable;
  pState   = (ownpjEncodeHuffmanState*)pEncHuffState;

  if(bFlushState != 0)
  {
    status = mfxownpj_write_bits_jpeg(
      0x7f,7,pDst,nDstLenBytes,pDstCurrPos,pState);

    mfxownpj_EncodeHuffmanStateInit(pState);

    goto Exit;
  }

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_PTR1_RET(pLastDc);
  OWN_BADARG_RET(Al < 0 || Al > 13);
  IPP_BAD_PTR1_RET(pDcTable);

  /*//////////////////////////////////////////////////////////////////////
  // Encode DC Coefficient
  */

  pt = pSrc[0] >> Al;

  data = pt - *pLastDc;

  *pLastDc = (Ipp16s)pt;

  if(data < 0)
  {
    if(data > -256)
    {
      ssss = mfxown_pj_csize[-data];
    }
    else
    {
      ssss = mfxown_pj_csize[(-data) >> 8] + 8;
    }

    data -= 1;
  }
  else
  {
    if(data < 256)
    {
      ssss = mfxown_pj_csize[data];
    }
    else
    {
      ssss = mfxown_pj_csize[data >> 8] + 8;
    }
  }

  /*//////////////////////////////////////////////////////////////////////
  // Encode Size of DC Coefficient
  */

  /* 11 - is max category for DC coefficient */
  if(ssss > 11)
  {
    status = ippStsJPEGDCTRangeErr;
    goto Exit;
  }

  cs = dc_table->hcs[ssss];

  uValue = cs & 0xffff;
  nBits  = cs >> 16;

  status = mfxownpj_write_bits_jpeg(
    uValue,nBits,pDst,nDstLenBytes,pDstCurrPos,pState);

  if(ippStsNoErr != status)
  {
    goto Exit;
  }

  /*//////////////////////////////////////////////////////////////////////
  // Encode DC Coefficient Value
  // (difference from previous value in this component)
  */

  uValue = data;
  nBits  = ssss;

  if(nBits)
  {
    status = mfxownpj_write_bits_jpeg(
      uValue,nBits,pDst,nDstLenBytes,pDstCurrPos,pState);

    if(ippStsNoErr != status)
    {
      goto Exit;
    }
  }

Exit:

  return status;
} /* mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1, (
  const Ipp16s*                  pSrc,
        Ipp8u*                   pDst,
        int                      nDstLenBytes,
        int*                     pDstCurrPos,
        int                      Al,
        IppiEncodeHuffmanState*  pEncHuffState,
        int                      bFlushState))
{
  ownpjEncodeHuffmanState* pState;
  IppStatus status = ippStsNoErr;

  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_SIZE_RET(nDstLenBytes);
  IPP_BAD_PTR1_RET(pDstCurrPos);
  IPP_BAD_PTR1_RET(pEncHuffState);

  pState = (ownpjEncodeHuffmanState*)pEncHuffState;

  if(bFlushState != 0)
  {
    status = mfxownpj_write_bits_jpeg(
      0x7f,7,pDst,nDstLenBytes,pDstCurrPos,pState);

    mfxownpj_EncodeHuffmanStateInit(pState);

    goto Exit;
  }

  IPP_BAD_PTR1_RET(pSrc);
  OWN_BADARG_RET(Al < 0 || Al > 13);

  status = mfxownpj_write_bits_jpeg(
    pSrc[0] >> Al,1,pDst,nDstLenBytes,pDstCurrPos,pState);

Exit:

  return status;
} /* mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1, (
  const Ipp16s*                 pSrc,
        Ipp8u*                  pDst,
        int                     nDstLenBytes,
        int*                    pDstCurrPos,
        int                     Ss,
        int                     Se,
        int                     Al,
  const IppiEncodeHuffmanSpec*  pAcTable,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))
{
#if !( _IPP >= _IPP_A6 ) && !( _IPP32E >= _IPP32E_M7 )/**/
  int    k;
  int    r;
  int    tmp;
  int    tmp2;
  int    ssss;
  int    i;
  unsigned int cs;
  unsigned int uValue;
  int    nBits;
  ownpjEncodeHuffmanSpec*  ac_table;
  ownpjEncodeHuffmanState* pState;
  IppStatus status = ippStsNoErr;
#endif

  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_SIZE_RET(nDstLenBytes);
  IPP_BAD_PTR1_RET(pDstCurrPos);
  IPP_BAD_PTR1_RET(pAcTable);
  IPP_BAD_PTR1_RET(pEncHuffState);

#if !( _IPP >= _IPP_A6 ) && !( _IPP32E >= _IPP32E_M7 )/**/
  ac_table = (ownpjEncodeHuffmanSpec*)pAcTable;
  pState   = (ownpjEncodeHuffmanState*)pEncHuffState;
#endif

  if(bFlushState != 0)
  {
#if ( _IPP >= _IPP_A6 ) || ( _IPP32E >= _IPP32E_M7 )/**/
    return mfxownpj_EncodeHuffman8x8_ACFlush_JPEG_16s1u_C1(
             pDst,
             nDstLenBytes,
             pDstCurrPos,
             (ownpjEncodeHuffmanSpec*)pAcTable,
             (ownpjEncodeHuffmanState*)pEncHuffState);
#else
    status = mfxownpj_encode_eobrun(
      pDst,nDstLenBytes,pDstCurrPos,ac_table,pState);
    if(ippStsNoErr != status)
    {
      goto Flush;
    }

    /* and the BE correction bits */
    status = mfxownpj_AppendBits(
      pState->pBEBuff,&pState->nBE,pDst,nDstLenBytes,pDstCurrPos,pState);
    if(ippStsNoErr != status)
    {
      goto Flush;
    }

Flush:

    status = mfxownpj_write_bits_jpeg(
      0x7f,7,pDst,nDstLenBytes,pDstCurrPos,pState);

    mfxownpj_EncodeHuffmanStateInit(pState);

    goto Exit;
#endif
  }

  IPP_BAD_PTR1_RET(pSrc);
  OWN_BADARG_RET(Ss < 1 || Ss > 63);
  OWN_BADARG_RET(Se < Ss || Se > 63);
  OWN_BADARG_RET(Al < 0 || Al > 13);

  /*////////////////////////////////////////////////////////////////////////
  // Encoding of the AC coefficients, see section G.1.2.2, fig. G.3
  */

#if ( _IPP >= _IPP_A6 ) || ( _IPP32E >= _IPP32E_M7 )/**/
    return mfxownpj_EncodeHuffman8x8_ACFirst_JPEG_16s1u_C1(
             pSrc,
             pDst,
             nDstLenBytes,
             pDstCurrPos,
             Ss,
             Se,
             Al,
             (ownpjEncodeHuffmanSpec*)pAcTable,
             (ownpjEncodeHuffmanState*)pEncHuffState);
#else
  /* r = run length of zeros */
  r = 0;

  for(k = Ss; k <= Se; k++)
  {
    tmp = pSrc[mfxown_pj_izigzag_index[k]];
    if(tmp == 0)
    {
      r++;
      continue;
    }

    /*
    // We need to apply the point transform by Al. It is an integer division
    // with rounding towards 0 for AC coefficients. We do this by shifting
    // after obtaining the absolute value to make it portable in C;
    */

    if(tmp < 0)
    {
      tmp = -tmp;   /* tmp is abs value of input  */
      tmp >>= Al;   /* do the point transform     */
      /* tmp2 = bitwise complement of abs(coef) for a negative coefficients */
      tmp2 = ~tmp;
    }
    else
    {
      tmp >>= Al;   /* do the point transform */
      tmp2 = tmp;
    }

    /* when nonzero coefficient becames zero after point transform */
    if(tmp == 0)
    {
      r++;
      continue;
    }

    /* if there is any pending EOBRUN code emit it */
    status = mfxownpj_encode_eobrun(
      pDst,nDstLenBytes,pDstCurrPos,ac_table,pState);
    if(ippStsNoErr != status)
    {
      goto Exit;
    }

    /* if run length > 15, must emit special run-length-16 codes (0xf0) */
    while(r > 15)
    {
      cs = ac_table->hcs[0xf0];

      uValue = cs & 0xffff;
      nBits  = cs >> 16;

      status = mfxownpj_write_bits_jpeg(
        uValue,nBits,pDst,nDstLenBytes,pDstCurrPos,pState);

      if(ippStsNoErr != status)
      {
        goto Exit;
      }

      r -= 16;
    }

    /* Find the number of bits needed for the magnitude of the coefficient */
    /* there must be at least one 1 bit */
    ssss = 1;
    while((tmp >>= 1))
      ssss++;

    /* Check for out-of-range coefficient values */
    if(ssss > 11)
    {
      status = ippStsJPEGDCTRangeErr;
      goto Exit;
    }

    i = (r << 4) + ssss;

    /* 256 entries in tables */
    if(i > 256)
    {
      status = ippStsErr;
      goto Exit;
    }

    cs = ac_table->hcs[i];

    uValue = cs & 0xffff;
    nBits  = cs >> 16;

    status = mfxownpj_write_bits_jpeg(
      uValue,nBits,pDst,nDstLenBytes,pDstCurrPos,pState);

    if(ippStsNoErr != status)
    {
      goto Exit;
    }

    uValue = tmp2;
    nBits  = ssss;

    if(nBits)
    {
      status = mfxownpj_write_bits_jpeg(
        uValue,nBits,pDst,nDstLenBytes,pDstCurrPos,pState);

      if(ippStsNoErr != status)
      {
        goto Exit;
      }
    }

    /* reset zero run length */
    r = 0;
  }

  if(r > 0)
  {
    /* If there are trailing zeroes, count an EOB */
    pState->nEndOfBlockRun++;
    if(pState->nEndOfBlockRun == 0x7fff)
    {
      status = mfxownpj_encode_eobrun(
        pDst,nDstLenBytes,pDstCurrPos,ac_table,pState);
      if(ippStsNoErr != status)
      {
        goto Exit;
      }
    }
  }

Exit:

  return status;
#endif
} /* mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1
//
//  Purpose:
//
//  Parameters:
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1, (
  const Ipp16s*                 pSrc,
        Ipp8u*                  pDst,
        int                     nDstLenBytes,
        int*                    pDstCurrPos,
        int                     Ss,
        int                     Se,
        int                     Al,
  const IppiEncodeHuffmanSpec*  pAcTable,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))
{
#if !( _IPP >= _IPP_A6 ) && !( _IPP32E >= _IPP32E_M7 )/**/
  int    r, k;
  unsigned int cs;
  unsigned int uValue;
  int    nBits;
  int    tmp;
  int    nEOB;
  int    nBE;
  int    nBR;
  int    absvalues[DCTSIZE2];
  ownpjEncodeHuffmanSpec*  ac_table;
  ownpjEncodeHuffmanState* pState;
  IppStatus status = ippStsNoErr;
  IppStatus status1 = ippStsNoErr;
  Ipp8u* pBRBuffer = NULL;
#endif

  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_SIZE_RET(nDstLenBytes);
  IPP_BAD_PTR1_RET(pDstCurrPos);
  IPP_BAD_PTR1_RET(pAcTable);
  IPP_BAD_PTR1_RET(pEncHuffState);

#if !( _IPP >= _IPP_A6 ) && !( _IPP32E >= _IPP32E_M7 )/**/
  ac_table = (ownpjEncodeHuffmanSpec*)pAcTable;
  pState   = (ownpjEncodeHuffmanState*)pEncHuffState;

  nBE = pState->nBE;
#endif

  if(bFlushState != 0)
  {
#if ( _IPP >= _IPP_A6 ) || ( _IPP32E >= _IPP32E_M7 )/**/
    return mfxownpj_EncodeHuffman8x8_ACFlush_JPEG_16s1u_C1(
             pDst,
             nDstLenBytes,
             pDstCurrPos,
             (ownpjEncodeHuffmanSpec*)pAcTable,
             (ownpjEncodeHuffmanState*)pEncHuffState);
#else
    status = mfxownpj_encode_eobrun(
      pDst,nDstLenBytes,pDstCurrPos,ac_table,pState);
    if(ippStsNoErr != status)
    {
      goto Flush;
    }

    /* and the BE correction bits */
    status = mfxownpj_AppendBits(
      pState->pBEBuff,&pState->nBE,pDst,nDstLenBytes,pDstCurrPos,pState);
    if(ippStsNoErr != status)
    {
      goto Flush;
    }

Flush:

    status1 = mfxownpj_write_bits_jpeg(
      0x7f,7,pDst,nDstLenBytes,pDstCurrPos,pState);
    if(ippStsNoErr != status1)
    {
      /* 21.05.02: returns the last error */
      status = status1;
    }

    mfxownpj_EncodeHuffmanStateInit(pState);

    return status;
#endif
  }

  IPP_BAD_PTR1_RET(pSrc);
  OWN_BADARG_RET(Ss < 1 || Ss > 63);
  OWN_BADARG_RET(Se < Ss || Se > 63);
  OWN_BADARG_RET(Al < 0 || Al > 13);

  /* It is convenient to make a pre-pass to determine the transformed */
  /* coefficients' absolute values and the EOB position.              */

#if ( _IPP >= _IPP_A6 ) || ( _IPP32E >= _IPP32E_M7 )/**/
    return mfxownpj_EncodeHuffman8x8_ACRefine_JPEG_16s1u_C1(
             pSrc,
             pDst,
             nDstLenBytes,
             pDstCurrPos,
             Ss,
             Se,
             Al,
             (ownpjEncodeHuffmanSpec*)pAcTable,
             (ownpjEncodeHuffmanState*)pEncHuffState);
#else
  nEOB = 0;

  for(k = Ss; k <= Se; k++)
  {
    tmp = pSrc[mfxown_pj_izigzag_index[k]];

    /* Apply the point transform by Al. For AC coefficients this */
    /* is an integer division with rounding towards 0. */

    /* tmp is abs value of input */
    if(tmp < 0)
    {
      tmp = -tmp;
    }

    /* do the point transform */
    tmp >>= Al;

    /* save abs value for main pass */
    absvalues[k] = tmp;

    /* EOB = index of last newly-nonzero coef */
    if(tmp == 1)
    {
      nEOB = k;
    }
  }

  /* Encoding of the AC coefficients,see section G.1.2.3, fig. G.7 */

  r         = 0;                     /* r = run length of zeros               */
  nBR       = 0;                     /* BR = count of buffered bits added now */
  nBE       = pState->nBE;
  pBRBuffer = pState->pBEBuff + nBE; /* Append bits to buffer */

  for(k = Ss; k <= Se; k++)
  {
    tmp = absvalues[k];
    if(tmp == 0)
    {
      r++;
      continue;
    }

    /* Emit required ZRLs if any, but not if they can be folded into EOB */
    while(r > 15 && k <= nEOB)
    {
      /* emit pending EOBRUN if any */
      status = mfxownpj_encode_eobrun(
        pDst,nDstLenBytes,pDstCurrPos,ac_table,pState);
      if(ippStsNoErr != status)
      {
        goto Exit;
      }

      /* and the BE correction bits */
      status = mfxownpj_AppendBits(
        pState->pBEBuff,&nBE,pDst,nDstLenBytes,pDstCurrPos,pState);
      if(ippStsNoErr != status)
      {
        goto Exit;
      }

      nBE = 0;

      /* Emit ZRL */
      cs = ac_table->hcs[0xf0];

      uValue = cs & 0xffff;
      nBits  = cs >> 16;

      status = mfxownpj_write_bits_jpeg(
        uValue,nBits,pDst,nDstLenBytes,pDstCurrPos,pState);
      if(ippStsNoErr != status)
      {
        goto Exit;
      }

      r -= 16;

      /* Emit buffered correction bits which must be associated with ZRL */
      status = mfxownpj_AppendBits(
        pBRBuffer,&nBR,pDst,nDstLenBytes,pDstCurrPos,pState);
      if(ippStsNoErr != status)
      {
        goto Exit;
      }

      pBRBuffer = pState->pBEBuff;
      nBR       = 0;
    }

    /*
    // If the coefficient was previously nonzero, it only needs a correction
    // bit.
    */

    if(tmp > 1)
    {
      /* correction bit is the next bit of the absolute value. */
      pBRBuffer[nBR] = (Ipp8u)(tmp & 1);
      nBR++;
      continue;
    }

    /* Emit pending EOBRUN if any */
    status = mfxownpj_encode_eobrun(pDst,nDstLenBytes,pDstCurrPos,ac_table,pState);
    if(ippStsNoErr != status)
    {
      goto Exit;
    }

    /* Emit the BE correction bits */
    status = mfxownpj_AppendBits(
      pState->pBEBuff,&nBE,pDst,nDstLenBytes,pDstCurrPos,pState);
    if(ippStsNoErr != status)
    {
      goto Exit;
    }

    nBE = 0;

    /* Emit Huffman symbol for run length / number of bits */
    cs = ac_table->hcs[(r << 4) + 1];

    uValue = cs & 0xffff;
    nBits  = cs >> 16;

    status = mfxownpj_write_bits_jpeg(
      uValue,nBits,pDst,nDstLenBytes,pDstCurrPos,pState);

    if(ippStsNoErr != status)
    {
      goto Exit;
    }

    /* Emit output bit for newly-nonzero coefficient */
    tmp = (pSrc[mfxown_pj_izigzag_index[k]] < 0) ? 0 : 1;

    status = mfxownpj_write_bits_jpeg(tmp,1,pDst,nDstLenBytes,pDstCurrPos,pState);
    if(ippStsNoErr != status)
    {
      goto Exit;
    }

    /* Emit buffered correction bits which must be associated with this code */
    status = mfxownpj_AppendBits(
      pBRBuffer,&nBR,pDst,nDstLenBytes,pDstCurrPos,pState);
    if(ippStsNoErr != status)
    {
      goto Exit;
    }

    /* BE bits are gone now */
    pBRBuffer = pState->pBEBuff;
    nBR = 0;

    /* reset zero run length */
    r = 0;
  }

  /* If there are trailing zeroes */
  if(r > 0 || nBR > 0)
  {
    /* count an EOB */
    pState->nEndOfBlockRun++;

    /* concat my correction bits to older ones */
    nBE += nBR;

    /* Force the EOB generation if one of the following conditions: */
    /*   1. overflowing of the EOB counter;                               */
    /*   2. overflowing of the correction bit buffer during the next MCU. */

    if((pState->nEndOfBlockRun == 0x7FFF) ||
       (nBE > (MAX_CORR_BITS-DCTSIZE2+1)))
    {
      status = mfxownpj_encode_eobrun(
        pDst,nDstLenBytes,pDstCurrPos,ac_table,pState);
      if(ippStsNoErr != status)
      {
        goto Exit;
      }

      /* append BE bits */
      status = mfxownpj_AppendBits(
        pState->pBEBuff,&nBE,pDst,nDstLenBytes,pDstCurrPos,pState);
      if(ippStsNoErr != status)
      {
        goto Exit;
      }

      nBE = 0;
    }
  }

Exit:

  pState->nBE = nBE;

  return status;
#endif
} /* mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1() */

