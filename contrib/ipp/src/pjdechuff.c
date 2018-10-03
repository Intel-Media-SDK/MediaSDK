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
//    Huffman entropy decoder
//
//  Contents:
//    mfxiDecodeHuffmanSpecGetBufSize_JPEG_8u,
//    mfxiDecodeHuffmanSpecInitAlloc_JPEG_8u,
//    mfxiDecodeHuffmanSpecFree_JPEG_8u,
//    mfxiDecodeHuffmanStateGetBufSize_JPEG_8u,
//    mfxiDecodeHuffmanStateInit_JPEG_8u,
//    mfxiDecodeHuffmanStateInitAlloc_JPEG_8u,
//    mfxiDecodeHuffmanStateFree_JPEG_8u,
//    mfxiDecodeHuffman8x8_JPEG_1u16s_C1
//
*/

#include "precomp.h"

#ifndef __OWNJ_H__
#include "ownj.h"
#endif
#ifndef __PSCOPY_H__
#include "pscopy.h"
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


LOCFUN(IppStatus,mfxownpj_DecodeHuffmanSpecInit,(
  const Ipp8u*                   pBits,
  const Ipp8u*                   pVals,
        ownpjDecodeHuffmanSpec*  pDecHuffTable))
{
  int     l;
  int     ctr;
  int     lookbits;
  Ipp32u  i;
  Ipp32u  j;
  Ipp32u  k;
  Ipp32u  L;
  Ipp32u  idx;
  Ipp32u  size;
  Ipp32u  code;
  Ipp32u  huffcode[257];
  Ipp32u  huffsize[257];

  mfxownsZero_8u((Ipp8u*)pDecHuffTable,sizeof(ownpjDecodeHuffmanSpec));
  mfxownsZero_8u((Ipp8u*)&huffcode[0],sizeof(huffcode));
  mfxownsZero_8u((Ipp8u*)&huffsize[0],sizeof(huffsize));

  k = 0;

  for(i = 0; i < 16; i++)
  {
    j = (Ipp32u)pBits[i];

    if((k + j) > 256)
    {
      /* protect against table overrun */
      return ippStsJPEGHuffTableErr;
    }

    while(j--)
    {
      huffsize[k++] = (i + 1);
    }
  }

  huffsize[k] = 0;

  /* Fill huff_table */
  code = 0;

  size = huffsize[0];
  idx  = 0;

  while(huffsize[idx])
  {
    while(huffsize[idx] == size)
    {
      huffcode[idx] = code;
      code++;
      idx++;
      if(idx > 256)
      {
          return ippStsJPEGHuffTableErr;
      }
    }

    code <<= 1;
    size++;
  }

  mfxownsZero_8u((Ipp8u*)pDecHuffTable->valptr,sizeof(pDecHuffTable->valptr));
  mfxownsZero_8u((Ipp8u*)pDecHuffTable->mincode,sizeof(pDecHuffTable->mincode));
  mfxownsZero_8u((Ipp8u*)pDecHuffTable->maxcode,sizeof(pDecHuffTable->maxcode));

  idx = 0;

  for(l = 1; l <= 16; l++)
  {
    k = (Ipp32u)pBits[l-1];

    if(k)
    {
      /* huffval[] index of 1-st symbol of code length l */
      pDecHuffTable->valptr[l]  = (Ipp16u)idx;

      /* minimum code of length l */
      pDecHuffTable->mincode[l] = (Ipp16u)huffcode[idx];

      i = idx + k;

      while(idx < i)
      {
        if(idx >= 256)
        {
          /* protect against table overrun */
          return ippStsJPEGHuffTableErr;
        }

        pDecHuffTable->huffval[idx] = pVals[idx];
        idx++;
      }

      /* maximum code of length l */
      pDecHuffTable->maxcode[l] = (Ipp16s)huffcode[idx-1];
    }
    else
    {
      /* -1 if no codes of this length */
      /* huff_table->maxcode[l] = 0xffff; */
      /* OA: huff_table->maxcode[l] = 0x0000; */
      pDecHuffTable->maxcode[l] = (Ipp16s) -1;
    }
  }

  /* OA: huff_table->maxcode[17] = 0xffff; */
  pDecHuffTable->maxcode[17] = (Ipp16s) -1;

  mfxownsZero_8u((Ipp8u*)pDecHuffTable->huffelem,sizeof(pDecHuffTable->huffelem));

  idx = 0;

  for(l = 1; l <= 8; l++)
  {
    L = (Ipp32u)pBits[l-1];

    for(i = 1; i <= L; i++, idx++)
    {
      lookbits = huffcode[idx] << (8-l);

      for(ctr = 1 << (8-l); ctr > 0; ctr--)
      {
        pDecHuffTable->huffelem[lookbits] = (l<<16) | (pVals[idx]);
        lookbits++;
        if(lookbits > 256)
        {
            /* protect against table overrun */
            return ippStsJPEGHuffTableErr;
        }
      }
    }
  }

  return ippStsNoErr;
} /* mfxownpj_DecodeHuffmanSpecInit() */


LOCFUN(void,mfxownpj_DecodeHuffmanStateInit,(
  ownpjDecodeHuffmanState* pDecHuffState))
{
  pDecHuffState->uBitBuffer     = 0;
  pDecHuffState->nBitsValid     = 0;
  pDecHuffState->nEndOfBlockRun = 0;
  pDecHuffState->lastNonZeroNo  = 0;
  return;
} /* mfxownpj_DecodeHuffmanStateInit() */




#if !defined (_I7)
/*F*
////////////////////////////////////////////////////////////////////////////
//  Name
//    mfxownpj_FillBitBuffer
//
//  Purpose
//
//  Context
//
//  Returns
//    Valid error code, or 0 for OK.
//
//  Parameters
//    pSrc
//    SrcLenBytes
//    pSrcCurrPos
//    pMarker
//    nbits
//    state    Pointer to current IJL state structure.
//
////////////////////////////////////////////////////////////////////////////
*F*/

OWNFUN(IppStatus,mfxownpj_FillBitBuffer,(
  const Ipp8u*                   pSrc,
        int                      nSrcLenBytes,
        int*                     pSrcCurrPos,
        int*                     pMarker,
        int                      nBits,
        ownpjDecodeHuffmanState* pDecHuffState))
{
  int    byte;
  int    marker;
  int    currPos = *pSrcCurrPos;
  Ipp8u* maxPtr = (Ipp8u*)pSrc + nSrcLenBytes;
  Ipp8u* curPtr = (Ipp8u*)pSrc + currPos;
  IppStatus status = ippStsNoErr;

  if(*pMarker)
  {
    /* !dudnik: data is corrupted if there is no enough bits  */
    /* !dudnik: 18.05.2006 status changed to ippStsJPEGDCTRangeErr */
    return ((pDecHuffState->nBitsValid >= nBits) ? (ippStsNoErr) :
                                                   (ippStsJPEGDCTRangeErr));
  }

  /* HUFF_MIN_GET_BITS = 25 */
  while(pDecHuffState->nBitsValid < HUFF_MIN_GET_BITS)
  {
    if(curPtr >= maxPtr)
    {
      /* reach end of bitstream, need to update output variables */
      *pSrcCurrPos = (int)(curPtr - pSrc);

      /* !dudnik: no enough bits in bit buffer */
      if(pDecHuffState->nBitsValid < nBits)
        status = ippStsJPEGOutOfBufErr;

      return status;
    }

    byte = *curPtr;
    curPtr++;

    while(byte == 0xFF)
    {
      if(curPtr >= maxPtr)
      {
        /* !dudnik: buffer too small... update output variables */
        *pSrcCurrPos = (int)(curPtr - pSrc);
        return ippStsJPEGOutOfBufErr;
      }

      marker = *curPtr;
      curPtr++;

      if(marker == 0)
        break;

      if(marker != 0xFF)
      {
        /* detect JPEG marker, stop reading bytes, update output variables */
        /* !dudnik: 18.05.2006 removed status for marker detection */
        pMarker[0] = marker;
        *pSrcCurrPos = (int)(curPtr - pSrc);
        return ippStsNoErr;
      }
    }

    pDecHuffState->uBitBuffer = ((Ipp32u)pDecHuffState->uBitBuffer << 8) | byte;
    pDecHuffState->nBitsValid += 8;
  } /* while */

  *pSrcCurrPos = (int)(curPtr - pSrc);

  return ippStsNoErr;
} /* mfxownpj_FillBitBuffer() */
#endif


/*F*
////////////////////////////////////////////////////////////////////////////
//  Name
//    mfxownpj_DecodeHuffLongCodes
//
//  Purpose
//
//  Context
//
//  Returns
//    Valid error code, or 0 for OK.
//
//  Parameters
//
////////////////////////////////////////////////////////////////////////////
*F*/

LOCFUN(IppStatus,mfxownpj_DecodeHuffLongCodes,(
  const Ipp8u*                   pSrc,
        int                      nSrcLenBytes,
        int*                     pSrcCurrPos,
        int*                     pMarker,
        int                      nMinBits,
        int*                     pResult,
  const ownpjDecodeHuffmanSpec*  pDecHuffTable,
        ownpjDecodeHuffmanState* pDecHuffState))
{
  int       max;
  int       code;
  int       index;
  IppStatus status;

  if(pDecHuffState->nBitsValid < nMinBits)
  {
    status = mfxownpj_FillBitBuffer(
               pSrc,nSrcLenBytes,pSrcCurrPos,
               pMarker,nMinBits,pDecHuffState);
    if(ippStsNoErr > status)
    {
      return status;
    }
  }

  code = mfxownpj_get_bits(nMinBits,pDecHuffState);

  /* !dudnik: 18.05.2006 limit search for huffman code to 16 bits */
  for( ; nMinBits <= 16; nMinBits++)
  {
    max = pDecHuffTable->maxcode[nMinBits];

    if((max & 0x8000) && (max != -1))
    {
      max = (unsigned short)pDecHuffTable->maxcode[nMinBits];
    }

    if(code <= max)
      break;

    code <<= 1;

    if(pDecHuffState->nBitsValid < 1)
    {
      status = mfxownpj_FillBitBuffer(
                 pSrc,nSrcLenBytes,pSrcCurrPos,
                 pMarker,1,pDecHuffState);
      if(ippStsNoErr > status)
      {
        return status;
      }
    }

    code |= mfxownpj_get_bits(1,pDecHuffState);
  }

  if(nMinBits > 16)
  {
    /* JPEG does not allow huffman codes with more then 16 bits length */
    /* !dudnik: 18.05.2006 add signaling for corrupted data */
    pResult[0] = 0;
    return ippStsJPEGDCTRangeErr;
  }

  index = pDecHuffTable->valptr[nMinBits] +
            ((int)(code - pDecHuffTable->mincode[nMinBits]));
  pResult[0] = pDecHuffTable->huffval[index];

  return ippStsNoErr;
} /* mfxownpj_DecodeHuffLongCodes() */


/*F*
////////////////////////////////////////////////////////////////////////////
//  Name
//    mfxownpj_DecodeHuffSymbol
//
//  Purpose
//
//  Context
//
//  Returns
//    Valid error code, or 0 for OK.
//
//  Parameters
//
////////////////////////////////////////////////////////////////////////////
*F*/

OWNFUN(IppStatus,mfxownpj_DecodeHuffSymbol,(
  const Ipp8u*                   pSrc,
        int                      nSrcLenBytes,
        int*                     pSrcCurrPos,
        int*                     pMarker,
        int*                     pResult,
  const ownpjDecodeHuffmanSpec*  pDecHuffTable,
        ownpjDecodeHuffmanState* pDecHuffState))
{
  int       nBits;
  int       look;
  IppStatus status;

  /* HUFF_LOOKAHEAD = 8 */
  if(pDecHuffState->nBitsValid < HUFF_LOOKAHEAD)
  {
    status = mfxownpj_FillBitBuffer(
               pSrc,nSrcLenBytes,pSrcCurrPos,
               pMarker,0,pDecHuffState);
    if(ippStsNoErr > status)
    {
      return status;
    }

    if(pDecHuffState->nBitsValid < HUFF_LOOKAHEAD)
    {
      nBits = 1;
      goto slowlabel;
    }
  }

  look = mfxownpj_peek_bits(HUFF_LOOKAHEAD,pDecHuffState);

  nBits = pDecHuffTable->huffelem[look] >> 16;

  if(nBits != 0)
  {
    pDecHuffState->nBitsValid -= nBits;
    pResult[0] = pDecHuffTable->huffelem[look] & 0xFFFF;
  }
  else
  {
    nBits = HUFF_LOOKAHEAD + 1;

slowlabel:

    status = mfxownpj_DecodeHuffLongCodes(
               pSrc,nSrcLenBytes,pSrcCurrPos,
               pMarker,nBits,pResult,pDecHuffTable,pDecHuffState);
    if(ippStsNoErr > status)
    {
      return status;
    }
  }

  return ippStsNoErr;
} /* mfxownpj_DecodeHuffSymbol() */




/* ---------------------- library functions definitions -------------------- */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanSpecGetBufSize_JPEG_8u
//
//  Purpose:
//    get size of IppiDecodeHuffmanSpec struct
//
//  Parameters:
//    pDecHuffSpecbufSize - where write size of buffer
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffmanSpecGetBufSize_JPEG_8u, (
  int*  pDecHuffSpecBufSize))
{
  IPP_BAD_PTR1_RET(pDecHuffSpecBufSize);

  *pDecHuffSpecBufSize = sizeof(ownpjDecodeHuffmanSpec);

  return ippStsNoErr;
} /* mfxiDecodeHuffmanSpecGetBufSize_JPEG_8u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanSpecInit_JPEG_8u
//
//  Purpose:
//    Build Huffman table for decoder
//    in fast-to-use format from raw Huffman table
//
//  Parameters:
//    pDecHuffSpec - pointer to Huffman table to be initialized
//    pListBits - pointer to raw Huffman table bits
//    pListVals - pointer to raw Huffman table vals
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffmanSpecInit_JPEG_8u, (
  const Ipp8u*                  pListBits,
  const Ipp8u*                  pListVals,
        IppiDecodeHuffmanSpec*  pDecHuffSpec))
{
  ownpjDecodeHuffmanSpec* pDecHuffTbl;

  IPP_BAD_PTR2_RET(pListBits,pListVals);
  IPP_BAD_PTR1_RET(pDecHuffSpec);

  pDecHuffTbl = (ownpjDecodeHuffmanSpec*)pDecHuffSpec;

  return mfxownpj_DecodeHuffmanSpecInit(pListBits,pListVals,pDecHuffTbl);
} /* mfxiDecodeHuffmanSpecInit_JPEG_8u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanStateGetBufSize_JPEG_8u
//
//  Purpose:
//    get size of IppiDecodeHuffmanState struct
//
//  Parameters:
//    pDecHuffStateBufSize - where write size of buffer
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffmanStateGetBufSize_JPEG_8u, (
  int* pDecHuffStateBufSize))
{
  IPP_BAD_PTR1_RET(pDecHuffStateBufSize);

  *pDecHuffStateBufSize = sizeof(ownpjDecodeHuffmanState);

  return ippStsNoErr;
} /* mfxiDecodeHuffmanStateGetBufSize_JPEG_8u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanStateInit_JPEG_8u
//
//  Purpose:
//    Build IppiDecodeHuffmanState struct
//
//  Parameters:
//    pDecHuffState - pointer to IppiDecodeHuffmanState struct
//                    to be initialized
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffmanStateInit_JPEG_8u, (
  IppiDecodeHuffmanState* pDecHuffState))
{
  ownpjDecodeHuffmanState* pState;

  IPP_BAD_PTR1_RET(pDecHuffState);

  pState = (ownpjDecodeHuffmanState*)pDecHuffState;

  mfxownpj_DecodeHuffmanStateInit(pState);

  return ippStsNoErr;
} /* mfxiDecodeHuffmanStateInit_JPEG_8u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffman8x8_JPEG_1u16s_C1
//
//  Purpose:
//    Huffman decode 8x8 block of quantized DCT coefs
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    nSrcLenBytes  bitstream length, in bytes
//    pSrcCurrPos   pointer to current offset in buffer in bytes (in/out)
//    pDst          pointer to quantized DCT coefficients
//    pLastDC       pointer to last DC coefs
//    pMarker       where to store JPEG marker
//    pDcTable      pointer to huffman DC table
//    pAcTable      pointer to huffman AC table
//    pDecHuffState pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiDecodeHuffman8x8_JPEG_1u16s_C1, (
  const Ipp8u*                  pSrc,
        int                     nSrcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst,
        Ipp16s*                 pLastDC,
        int*                    pMarker,
  const IppiDecodeHuffmanSpec*  pDcTable,
  const IppiDecodeHuffmanSpec*  pAcTable,
        IppiDecodeHuffmanState* pDecHuffState))
{
  int  n, r, s;
  int  diff;
  int  zz;
  int* zz_index_ptr;
  ownpjDecodeHuffmanSpec*  dc_table = (ownpjDecodeHuffmanSpec*)pDcTable;
  ownpjDecodeHuffmanSpec*  ac_table = (ownpjDecodeHuffmanSpec*)pAcTable;
  ownpjDecodeHuffmanState* pState   = (ownpjDecodeHuffmanState*)pDecHuffState;
  IppStatus status = ippStsNoErr;

  IPP_BAD_PTR1_RET(pSrc);
  OWN_BADARG_RET(nSrcLenBytes < 0);
  IPP_BAD_PTR1_RET(pSrcCurrPos);
  IPP_BAD_PTR1_RET(pDst);
  IPP_BAD_PTR1_RET(pLastDC);
  IPP_BAD_PTR1_RET(pMarker);
  IPP_BAD_PTR1_RET(pDcTable);
  IPP_BAD_PTR1_RET(pAcTable);
  IPP_BAD_PTR1_RET(pDecHuffState);

  n = DCTSIZE2;

#if ( defined (_A6) || ( _IPP >= _IPP_W7 ) || ( _IPP32E >= _IPP32E_M7 )) || ((_IPP_ARCH ==_IPP_ARCH_LRB) && (_IPPLRB == _IPPLRB_B1))
  status = mfxownpj_DecodeHuffman8x8_JPEG_1u16s_C1(
             pSrc,nSrcLenBytes,pSrcCurrPos,
             pDst,pLastDC,pMarker,dc_table,ac_table,pState);

  if(ippStsNoErr == status)
  {
    return ippStsNoErr;
  }
#endif

  /* Clear output buffer */
  mfxownsZero_8u((Ipp8u*)pDst,DCTSIZE2*sizeof(Ipp16s));

  /* decode DC coef */
  status = mfxownpj_DecodeHuffSymbol(
             pSrc,nSrcLenBytes,pSrcCurrPos,
             pMarker,&s,dc_table,pState);

  if(ippStsNoErr > status)
  {
    goto Exit;
  }

  if(s)
  {
    s &= 0x0f; /* s should be in [0,15] */

    if(pState->nBitsValid < s)
    {
      status = mfxownpj_FillBitBuffer(
                 pSrc,nSrcLenBytes,pSrcCurrPos,
                 pMarker,s,pState);

      if(ippStsNoErr > status)
      {
        goto Exit;
      }
    }

    diff = mfxownpj_get_bits(s,pState);

    if(( diff & ( 1 << (s - 1) ) ) == 0)
    {
      diff += mfxown_pj_lowest_coef[s];
    }

    *pLastDC = (Ipp16s)(*pLastDC + diff);
  }

  pDst[0] = *pLastDC;


  zz_index_ptr = (int*)&mfxown_pj_izigzag_index[1];

  /* decode 63 AC coefs */
  for(n = DCTSIZE2-1; n > 0; )
  {
    /* !dudnik: decode category */
    status = mfxownpj_DecodeHuffSymbol(
               pSrc,nSrcLenBytes,pSrcCurrPos,
               pMarker,&s,ac_table,pState);

    if(ippStsNoErr > status)
    {
      goto Exit;
    }

    r  = (s >> 4) & 0x0f;
    s &= 0x0f;

    if(s)
    {
      n            -= r + 1;
      zz_index_ptr += r;

      if(pState->nBitsValid < s)
      {
        status = mfxownpj_FillBitBuffer(
                   pSrc,nSrcLenBytes,pSrcCurrPos,
                   pMarker,s,pState);

        if(ippStsNoErr > status)
        {
          goto Exit;
        }
      }

      r = mfxownpj_get_bits(s,pState);

      zz = *zz_index_ptr++;

      /* !dudnik: 031222 fixed zz overflow */
      if(zz > 63 || zz < 0)
        return ippStsErr;

      if((r & (1 << (s - 1))) == 0)
      {
        pDst[zz] = (Ipp16s)(r + mfxown_pj_lowest_coef[s]);
      }
      else
      {
        pDst[zz] = (Ipp16s)r;
      }
    }
    else if(r == 15)
    {
      n            -= 16;
      zz_index_ptr += 16;
    }
    else
      break;

  } /* decode 63 AC coefficient */

Exit:

  /* !dudnik: 18.05.2006, return index of last non-zero coefficient */
  pState->lastNonZeroNo = DCTSIZE2 - n;

  return status;
} /* mfxiDecodeHuffman8x8_JPEG_1u16s_C1() */

