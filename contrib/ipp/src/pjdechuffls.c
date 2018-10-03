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
//    lossless Huffman entropy decoder
//
//  Contents:
//    mfxiDecodeHuffmanOne_JPEG_1u16s_C1
//    mfxiDecodeHuffmanRow_JPEG_1u16s_C1P4
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#ifndef __PJHUFFTBL_H__
#include "pjhufftbl.h"
#endif
#ifndef __PJDECHUFF_H__
#include "pjdechuff.h"
#endif




/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanOne_JPEG_1u16s_C1
//
//  Purpose:
//    Huffman decode one lossless coefs
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    nSrcLenBytes  bitstream length, in bytes
//    pSrcCurrPos   pointer to current offset in buffer in bytes (in/out)
//    pDst          pointer to decoded difference
//    pMarker       where to store JPEG marker
//    pDcTable      pointer to huffman DC table
//    pDecHuffState pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffmanOne_JPEG_1u16s_C1, (
  const Ipp8u*                  pSrc,
        int                     nSrcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst,
        int*                    pMarker,
  const IppiDecodeHuffmanSpec*  pDcTable,
        IppiDecodeHuffmanState* pDecHuffState))
{
  int  s;
  int  diff = 0;
  ownpjDecodeHuffmanSpec*  dc_table = (ownpjDecodeHuffmanSpec*)pDcTable;
  ownpjDecodeHuffmanState* pState   = (ownpjDecodeHuffmanState*)pDecHuffState;
  IppStatus status = ippStsNoErr;

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_SIZE_RET(nSrcLenBytes);
  IPP_BAD_PTR1_RET(pSrcCurrPos);
  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_PTR1_RET(pMarker);
  IPP_BAD_PTR1_RET(pDcTable);
  IPP_BAD_PTR1_RET(pDecHuffState);

#if ( _IPP >= _IPP_W7 )
if( ((nSrcLenBytes - *pSrcCurrPos) >= 8) && !(*pMarker) ) {
  if( !mfxownpj_DecodeHuffmanOne_JPEG_1u16s_C1(
             pSrc,
             pDst,
             pSrcCurrPos,
             dc_table,
             pState ) )
  {
    return ippStsNoErr;
  }
}
#endif

#if defined (_I7)
  if( !mfxownpj_DecodeHuffmanOne_JPEG_1u16s_C1(
            pSrc,
            nSrcLenBytes,
            pSrcCurrPos,
            pDst,
            pMarker,
            dc_table,
            pState) )
  {
    return ippStsNoErr;
  }
#endif

  /* decode DC coef */
  status = mfxownpj_DecodeHuffSymbol(
    pSrc,
    nSrcLenBytes,
    pSrcCurrPos,
    pMarker,
    &s,
    dc_table,
    pState);

  if(ippStsNoErr > status)
  {
    goto Exit;
  }

  if(s)
  {
    if(s < 16)
    {
      if(pState->nBitsValid < s)
      {
        status = mfxownpj_FillBitBuffer(
          pSrc,
          nSrcLenBytes,
          pSrcCurrPos,
          pMarker,
          s,
          pState);

        if(ippStsNoErr > status)
        {
          goto Exit;
        }
      }

      diff = mfxownpj_get_bits(s,pState);

      diff = mfxownpj_huff_extend(diff,s);
    }
    else
      diff = 32768;
  }

  pDst[0] = (Ipp16s)diff;


Exit:

  return status;
} /* mfxiDecodeHuffmanOne_JPEG_1u16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanRow_JPEG_1u16s_C1P4
//
//  Purpose:
//    Huffman decode one lossless coefs
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    nSrcLenBytes  bitstream length, in bytes
//    pSrcCurrPos   pointer to current offset in buffer in bytes (in/out)
//    pDst          pointer to decoded difference
//    pMarker       where to store JPEG marker
//    pDcTable      pointer to huffman DC table
//    pDecHuffState pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffmanRow_JPEG_1u16s_C1P4, (
  const Ipp8u*                  pSrc,
        int                     nSrcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst[4],
        int                     nDstLen,
        int                     nDstRows,
        int*                    pMarker,
  const IppiDecodeHuffmanSpec*  pDecHuffTable[4],
        IppiDecodeHuffmanState* pDecHuffState))
{
  int  i, j;
  int  symb;
  int  diff = 0;
  Ipp16s* dst[4];
  ownpjDecodeHuffmanSpec*  pTable[4];
  ownpjDecodeHuffmanState* pState = (ownpjDecodeHuffmanState*)pDecHuffState;
  IppStatus status = ippStsNoErr;

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_SIZE_RET(nSrcLenBytes);
  IPP_BAD_PTR1_RET(pSrcCurrPos);
  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_SIZE_RET(nDstLen);
  IPP_BAD_SIZE_RET(nDstRows);
  IPP_BAD_PTR1_RET(pMarker);
  IPP_BAD_PTR1_RET(pDecHuffTable);
  IPP_BAD_PTR1_RET(pDecHuffState);

  for(i = 0; i < IPP_MIN(nDstRows,4); i++)
  {
    IPP_BAD_PTR2_RET(pDst[i],pDecHuffTable[i]);

    pTable[i] = (ownpjDecodeHuffmanSpec*)pDecHuffTable[i];
    dst[i]    = pDst[i];
  }

  #if (_IPP >= _IPP_V8) || ( _IPP32E >= _IPP32E_U8 )
  status = mfxownpj_DecodeHuffmanRow_JPEG_1u16s_C1P4(
                      pSrc,
                      nSrcLenBytes,
                      pSrcCurrPos,
                      pDst,
                      nDstLen,
                      nDstRows,
                      pMarker,
                      (const ownpjDecodeHuffmanSpec**)pTable,
                      pState);
  if(ippStsNoErr == status)
  {
    return ippStsNoErr;
  }
  #endif

  for(i = 0; i < nDstLen; i++)
  {
    for(j = 0; j < IPP_MIN(nDstRows,4); j++)
    {
      diff = 0;

      /* decode DC coef */
      status = mfxownpj_DecodeHuffSymbol(
                 pSrc,nSrcLenBytes,pSrcCurrPos,
                 pMarker,&symb,pTable[j],pState);

      if(ippStsNoErr > status)
      {
        goto Exit;
      }

      if(symb)
      {
        if(symb < 16)
        {
          if(pState->nBitsValid < symb)
          {
            status = mfxownpj_FillBitBuffer(
                       pSrc,nSrcLenBytes,pSrcCurrPos,
                       pMarker,symb,pState);

            if(ippStsNoErr > status)
            {
              goto Exit;
            }
          }

          diff = mfxownpj_get_bits(symb,pState);

          diff = mfxownpj_huff_extend(diff,symb);
        }
        else
          diff = 32768;
      }

      dst[j][i] = (Ipp16s)diff;
    } // for (nDstRows)
  } // for (nDstLen)

Exit:

  return status;
} /* mfxiDecodeHuffmanRow_JPEG_1u16s_C1P4() */
