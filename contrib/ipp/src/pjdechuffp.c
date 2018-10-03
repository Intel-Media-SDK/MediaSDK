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
//    Huffman entropy decoder (progressive)
//
//  Contents:
//    mfxiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1
//    mfxiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1
//    mfxiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1
//    mfxiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1
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
#ifndef __PJDECHUFF_H__
#include "pjdechuff.h"
#endif





/* ---------------------- library functions definitions -------------------- */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1
//
//  Purpose:
//    Huffman decode DC coefficient of 8x8 block of quantized DCT coefficients
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    SrcLenBytes   bitstream length, in bytes
//    pSrcCurrPos   pointer to current byte offset in buffer (in/out)
//    pDst          pointer to output block of quantized DCT coefficients
//    pLastDC       pointer to last DC coefficients from preivious block
//                  of the same color component
//    pMarker       where to store JPEG marker
//    Al            Approximation bit position low
//    pDcTable      pointer to huffman DC table
//    pDecHuffState pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1, (
  const Ipp8u*                  pSrc,
        int                     nSrcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst,
        Ipp16s*                 pLastDC,
        int*                    pMarker,
        int                     Al,
  const IppiDecodeHuffmanSpec*  pDcTable,
        IppiDecodeHuffmanState* pDecHuffState))
{
  int        s;
  int        diff;
  IppStatus  status = ippStsNoErr;
  ownpjDecodeHuffmanSpec*  dc_table;
  ownpjDecodeHuffmanState* pState;

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_SIZE_RET(nSrcLenBytes);
  IPP_BAD_PTR1_RET(pSrcCurrPos);
  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_PTR1_RET(pLastDC);
  IPP_BAD_PTR1_RET(pMarker);
  OWN_BADARG_RET(Al < 0 || Al > 13);
  IPP_BAD_PTR1_RET(pDcTable);
  IPP_BAD_PTR1_RET(pDecHuffState);

  dc_table = (ownpjDecodeHuffmanSpec*)pDcTable;
  pState   = (ownpjDecodeHuffmanState*)pDecHuffState;

  status = mfxownpj_DecodeHuffSymbol(
    pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,&s,dc_table,pState);
  if(ippStsNoErr > status)
  {
    goto Exit;
  }

  if(s)
  {
    s &= 0xf;  /* Make sure s is in [0,15] */

    if(pState->nBitsValid < s)
    {
      status = mfxownpj_FillBitBuffer(
        pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,s,pState);
      if(ippStsNoErr > status)
      {
        goto Exit;
      }
    }

    diff = mfxownpj_get_bits(s,pState);

    if((diff & (1 << (s-1))) == 0)
    {
      diff += mfxown_pj_lowest_coef[s];
    }

    *pLastDC = (Ipp16s)(*pLastDC + diff);
  }

  pDst[0] = (Ipp16s)(*pLastDC << Al);

Exit:

  return status;
} /* mfxiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1
//
//  Purpose:
//    Refine DC coefficient of 8x8 block of quantized DCT coefficients
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    SrcLenBytes   bitstream length, in bytes
//    pSrcCurrPos   pointer to current byte offset in buffer (in/out)
//    pDst          pointer to output block of quantized DCT coefficients
//    pMarker       where to store JPEG marker
//    Al            Approximation bit position low
//    pState        pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1, (
  const Ipp8u*                   pSrc,
        int                      nSrcLenBytes,
        int*                     pSrcCurrPos,
        Ipp16s*                  pDst,
        int*                     pMarker,
        int                      Al,
        IppiDecodeHuffmanState*  pDecHuffState))
{
  int       p;
  int       diff;
  IppStatus status = ippStsNoErr;
  ownpjDecodeHuffmanState* pState;

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_SIZE_RET(nSrcLenBytes);
  IPP_BAD_PTR1_RET(pSrcCurrPos);
  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_PTR1_RET(pMarker);
  OWN_BADARG_RET(Al < 0 || Al > 13);
  IPP_BAD_PTR1_RET(pDecHuffState);

  pState = (ownpjDecodeHuffmanState*)pDecHuffState;

  p = 1 << Al;

  if(pState->nBitsValid < 1)
  {
    status = mfxownpj_FillBitBuffer(
      pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,1,pState);
    if(ippStsNoErr > status)
    {
      goto Exit;
    }
  }

  diff = mfxownpj_get_bits(1,pState);

  if(diff)
  {
    pDst[0] |= p;
  }

Exit:

  return status;
} /* mfxiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1
//
//  Purpose:
//    Huffman decode AC coefficient of 8x8 block of quantized DCT coefficients
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    SrcLenBytes   bitstream length, in bytes
//    pSrcCurrPos   pointer to current byte offset in buffer (in/out)
//    pDst          pointer to output block of quantized DCT coefficients
//    pMarker       where to store JPEG marker
//    Ss            spectral selection start
//    Se            spectral selection end
//    Al            Approximation bit position low
//    pAcTable      pointer to huffman AC table
//    pDecHuffState pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1, (
  const Ipp8u*                  pSrc,
        int                     nSrcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst,
        int*                    pMarker,
        int                     Ss,
        int                     Se,
        int                     Al,
  const IppiDecodeHuffmanSpec*  pAcTable,
        IppiDecodeHuffmanState* pDecHuffState))
{
#if !defined (_A6) && !( _IPP >= _IPP_W7 ) && !( _IPP32E >= _IPP32E_M7 )
  int        r;
  int        s;
  int        k;
  const int* index;
  IppStatus  status = ippStsNoErr;
  ownpjDecodeHuffmanSpec*  ac_table;
  ownpjDecodeHuffmanState* pState;
#endif

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_SIZE_RET(nSrcLenBytes);
  IPP_BAD_PTR1_RET(pSrcCurrPos);
  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_PTR1_RET(pMarker);
  OWN_BADARG_RET(Ss < 1 || Ss > 63);
  OWN_BADARG_RET(Se < Ss || Se > 63);
  OWN_BADARG_RET(Al < 0 || Al > 13);
  IPP_BAD_PTR1_RET(pAcTable);
  IPP_BAD_PTR1_RET(pDecHuffState);

#if defined (_A6) || ( _IPP >= _IPP_W7 ) || ( _IPP32E >= _IPP32E_M7 )
  return mfxownpj_DecodeHuffman8x8_ACFirst_JPEG_1u16s_C1(
    pSrc,
    nSrcLenBytes,
    pSrcCurrPos,
    pDst,
    pMarker,
    Ss,
    Se,
    Al,
    (ownpjDecodeHuffmanSpec*)pAcTable,
    (ownpjDecodeHuffmanState*)pDecHuffState);
#else
  ac_table = (ownpjDecodeHuffmanSpec*)pAcTable;
  pState   = (ownpjDecodeHuffmanState*)pDecHuffState;

  index = &mfxown_pj_izigzag_index[0];

  if(pState->nEndOfBlockRun > 0)
  {
    pState->nEndOfBlockRun--;
  }
  else
  {
    for(k = Ss; k <= Se; k++)
    {
      status = mfxownpj_DecodeHuffSymbol(
        pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,&s,ac_table,pState);
      if(ippStsNoErr > status)
      {
        goto Exit;
      }

      r  = s >> 4;
      s &= 15;

      if(s)
      {
        k += r;
        if(pState->nBitsValid < s)
        {
          status = mfxownpj_FillBitBuffer(
            pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,s,pState);
          if(ippStsNoErr > status)
          {
            goto Exit;
          }
        }

        r = mfxownpj_get_bits(s,pState);

        if((r & (1 << (s-1))) == 0)
        {
          pDst[index[k]] = (Ipp16s)((r + mfxown_pj_lowest_coef[s]) << Al);
        }
        else
        {
          pDst[index[k]] = (Ipp16s)(r << Al);
        }

      }
      else
      {
        if(r == 15)
        {
          k += 15;
        }
        else
        {
          pState->nEndOfBlockRun = 1 << r;
          if(r)
          {
            if(pState->nBitsValid < r)
            {
              status = mfxownpj_FillBitBuffer(
                pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,r,pState);
              if(ippStsNoErr > status)
              {
                goto Exit;
              }
            }

            r = mfxownpj_get_bits(r,pState);
            pState->nEndOfBlockRun += r;
          }
          pState->nEndOfBlockRun--;
          break;
        }
      }
    } // for(k...)
  }

Exit:

  return status;
#endif
} /* mfxiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1
//
//  Purpose:
//    Refine AC coefficients of 8x8 block of quantized DCT coefficients
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    SrcLenBytes   bitstream length, in bytes
//    pSrcCurrPos   pointer to current byte offset in buffer (in/out)
//    pDst          pointer to output block of quantized DCT coefficients
//    pMarker       where to store JPEG marker
//    Ss            spectral selection start
//    Se            spectral selection end
//    Al            Approximation bit position low
//    pAcTable      Huffman AC table
//    pDecHuffState pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1, (
  const Ipp8u*                  pSrc,
        int                     nSrcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst,
        int*                    pMarker,
        int                     Ss,
        int                     Se,
        int                     Al,
  const IppiDecodeHuffmanSpec*  pAcTable,
        IppiDecodeHuffmanState* pDecHuffState))
{
#if !defined (_A6) && !( _IPP >= _IPP_W7 ) && !( _IPP32E >= _IPP32E_M7 )
  int        r;
  int        s;
  int        k;
  int        p1, m1;
  const int* index;
  Ipp16s*    thiscoef;
  IppStatus  status = ippStsNoErr;
  ownpjDecodeHuffmanSpec*  ac_table;
  ownpjDecodeHuffmanState* pState;
#endif

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_SIZE_RET(nSrcLenBytes);
  IPP_BAD_PTR1_RET(pSrcCurrPos);
  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_PTR1_RET(pMarker);
  OWN_BADARG_RET(Ss < 1 || Ss > 63);
  OWN_BADARG_RET(Se < Ss || Se > 63);
  OWN_BADARG_RET(Al < 0 || Al > 13);
  IPP_BAD_PTR1_RET(pAcTable);
  IPP_BAD_PTR1_RET(pDecHuffState);

#if defined (_A6) || ( _IPP >= _IPP_W7 ) || ( _IPP32E >= _IPP32E_M7 )
    return mfxownpj_DecodeHuffman8x8_ACRefine_JPEG_1u16s_C1(
    pSrc,
    nSrcLenBytes,
    pSrcCurrPos,
    pDst,
    pMarker,
    Ss,
    Se,
    Al,
    (ownpjDecodeHuffmanSpec*)pAcTable,
    (ownpjDecodeHuffmanState*)pDecHuffState);
#else
  ac_table = (ownpjDecodeHuffmanSpec*)pAcTable;
  pState   = (ownpjDecodeHuffmanState*)pDecHuffState;

  p1 =   1  << Al; /*  1 in the bit position which being coded */
  m1 = (-1) << Al; /* -1 in the bit position which being coded */

  index = &mfxown_pj_izigzag_index[0];


  /* always only one block per MCU */

  /* coefficient loop counter is started from first coefficent in band */
  k = Ss;

  if(pState->nEndOfBlockRun == 0)
  {
    for(; k <= Se; k++)
    {
      status = mfxownpj_DecodeHuffSymbol(
        pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,&s,ac_table,pState);
      if(ippStsNoErr > status)
      {
        goto Exit;
      }

      r  = s >> 4;
      s &= 15;

      if(s)
      {
        if(pState->nBitsValid < 1)
        {
          status = mfxownpj_FillBitBuffer(
            pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,1,pState);
          if(ippStsNoErr > status)
          {
            goto Exit;
          }
        }

        s = (mfxownpj_get_bits(1,pState)) ? p1 : m1;
      }
      else
      {
        if(r != 15)
        {
          /* EOBr, run length is 2^r + appended bits */
          pState->nEndOfBlockRun = 1 << r;
          if(r)
          {
            if(pState->nBitsValid < r)
            {
              status = mfxownpj_FillBitBuffer(
                pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,r,pState);
              if(ippStsNoErr > status)
              {
                goto Exit;
              }
            }

            r = mfxownpj_get_bits(r,pState);

            pState->nEndOfBlockRun += r;
          }
          break; /* EOB logic is handled the rest of block */
        }
        /* Note, s = 0 when processing ZRL */
      }

      /*
      // Advance over already-nonzero coefficients
      // and r still-zero coefficients, appending correction bits
      // to the nonzeroes. A correction bit is 1 if the absolute value
      // of the coefficient must be increased.
      */
      do
      {
        thiscoef = pDst + index[k];
        if(thiscoef[0] != 0)
        {
          if(pState->nBitsValid < 1)
          {
            status = mfxownpj_FillBitBuffer(
              pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,1,pState);
            if(ippStsNoErr > status)
            {
              goto Exit;
            }
          }

          if(mfxownpj_get_bits(1,pState))
          {
            if((thiscoef[0] & p1) == 0)
            {
              /* nothing to do if already changed it */
              if(thiscoef[0] >= 0)
              {
                thiscoef[0] = (Ipp16s)(thiscoef[0] + p1);
              }
              else
              {
                thiscoef[0] = (Ipp16s)(thiscoef[0] + m1);
              }
            }
          }
        }
        else
        {
          if(--r < 0)
          {
            break; /* target zero coefficient */
          }
        }

        k++;
      } while(k <= Se);

      if(s)
      {
        /* Output new nonzero coefficient */
        pDst[index[k]] = (Ipp16s)s;
      }
    }
  }

  if(pState->nEndOfBlockRun > 0)
  {
    /*
    // Scanning for any remaining coefficient positions after the end-of-band
    // (the last newly nonzero coefficient, if any). Append a correction bit
    // to each already-nonzero coefficient.
    // A correction bit is 1 if the absolute value of the coefficient
    // must be increased.
    */
    for(; k <= Se; k++)
    {
      thiscoef = pDst + index[k];
      if(thiscoef[0] != 0)
      {
        if(pState->nBitsValid < 1)
        {
          status = mfxownpj_FillBitBuffer(
            pSrc,nSrcLenBytes,pSrcCurrPos,pMarker,1,pState);
          if(ippStsNoErr > status)
          {
            goto Exit;
          }
        }

        if(mfxownpj_get_bits(1,pState))
        {
          if((thiscoef[0] & p1) == 0)
          {
            /* do nothing if already changed it */
            if(thiscoef[0] >= 0)
            {
              thiscoef[0] = (Ipp16s)(thiscoef[0] + p1);
            }
            else
            {
              thiscoef[0] = (Ipp16s)(thiscoef[0] + m1);
            }
          }
        }
      }
    }
    /* Count one block completed in EOB run */
    pState->nEndOfBlockRun--;
  }

Exit:

  return status;
#endif
} /* mfxiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1() */



