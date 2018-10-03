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
//    lossless Huffman entropy encoder
//
//  Contents:
//    mfxiGetHuffmanStatisticsOne_JPEG_16s_C1
//    mfxiEncodeHuffmanOne_JPEG_16s1u_C1
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#ifndef __PJHUFFTBL_H__
#include "pjhufftbl.h"
#endif
#ifndef __PJENCHUFF_H__
#include "pjenchuff.h"
#endif




#undef CATEGORY_FROM_FPU

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiGetHuffmanStatisticsOne_JPEG_16s_C1
//
//  Purpose:
//    count statistics for huffman values for particular differences
//
//  Parameters:
//    pSrc             pointer to input differences
//    pHuffStatistics  pointer to accumulated huffman values statistics
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiGetHuffmanStatisticsOne_JPEG_16s_C1, (
  const Ipp16s* pSrc,
        int     pHuffStatistics[256]))
{
  int    data;
  int    ssss;

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_PTR1_RET(pHuffStatistics);

  /* Encode sample */

  data = pSrc[0];

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

  /* Count huffval Size of DC Coefficient */
  if(ssss > 16)
    return ippStsRangeErr;

  pHuffStatistics[ssss]++;


  return ippStsNoErr;
} /* mfxiGetHuffmanStatisticsOne_JPEG_16s_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanOne_JPEG_16s1u_C1
//
//  Purpose:
//    Huffman encode one lossless coefs
//
//  Parameters:
//    pSrc          pointer to input differences
//    pDst          pointer to output JPEG bitstream
//    nDstLenBytes  bitstream length, in bytes
//    pDstCurrPos   pointer to current offset in buffer in bytes (in/out)
//    pHuffTable    pointer to huffman lossless table
//    pDecHuffState pointer to Huffman state struct
//    bFlushState
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffmanOne_JPEG_16s1u_C1, (
  const Ipp16s*                 pSrc,
        Ipp8u*                  pDst,
        int                     nDstLenBytes,
        int*                    pDstCurrPos,
  const IppiEncodeHuffmanSpec*  pHuffTable,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))
{
  unsigned int cs;
  unsigned int uValue;
  int    nBits;
  int    data;
  int    ssss;
  ownpjEncodeHuffmanSpec*  huff_table = (ownpjEncodeHuffmanSpec*)pHuffTable;
  ownpjEncodeHuffmanState* pState     = (ownpjEncodeHuffmanState*)pEncHuffState;
  IppStatus status = ippStsNoErr;

  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_SIZE_RET(nDstLenBytes);
  IPP_BAD_PTR1_RET(pDstCurrPos);
  IPP_BAD_PTR1_RET(pEncHuffState);

  if(bFlushState != 0)
  {
    status = mfxownpj_write_bits_jpeg(0x7f,7,pDst,nDstLenBytes,pDstCurrPos,pState);

    mfxownpj_EncodeHuffmanStateInit(pState);

    goto Exit;
  }

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_PTR1_RET(pHuffTable);

#if ( _IPP >= _IPP_W7 )
  if( (nDstLenBytes - *pDstCurrPos) >= 9 ) {
    if( !mfxownpj_EecodeHuffmanOne_JPEG_16s1u_C1(
               pSrc,
               pDst,
               pDstCurrPos,
               huff_table,
               pState ) )
    {
      return ippStsNoErr;
    }
  }
#endif

#if defined (_I7)
  if( !mfxownpj_EecodeHuffmanOne_JPEG_16s1u_C1(
             pSrc,
             pDst,
             nDstLenBytes,
             pDstCurrPos,
             huff_table,
             pState ) )
  {
    return ippStsNoErr;
  }
#endif

  data = pSrc[0];

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

  /* Encode Size of DC Coefficient */

  cs = huff_table->hcs[ssss];

  uValue = cs & 0xffff;
  nBits  = cs >> 16;

  status = mfxownpj_write_bits_jpeg(
    uValue,nBits,pDst,nDstLenBytes,pDstCurrPos,pState);

  if(ippStsNoErr != status)
  {
    goto Exit;
  }

  /* Encode DC Coefficient Value                        */
  /* (difference from previous value in this component) */

  if(ssss != 0 && ssss < 16)
  {
    status = mfxownpj_write_bits_jpeg(
      data,ssss,pDst,nDstLenBytes,pDstCurrPos,pState);

    if(ippStsNoErr != status)
    {
      goto Exit;
    }
  }

Exit:

  return status;
} /* mfxiEncodeHuffmanOne_JPEG_16s1u_C1() */

