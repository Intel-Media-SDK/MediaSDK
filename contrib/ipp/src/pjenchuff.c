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

//  Purpose:
//    Huffman entropy encoder
//
//  Contents:
//    mfxiEncodeHuffmanRawTableInit_JPEG_8u,
//    mfxiEncodeHuffmanSpecGetBufSize_JPEG_8u,
//    mfxiEncodeHuffmanStateGetBufSize_JPEG_8u,
//    mfxiEncodeHuffmanStateInit_JPEG_8u,
//    mfxiEncodeHuffman8x8_JPEG_16s1u_C1,
//    mfxiGetHuffmanStatistics8x8_JPEG_16s_C1
//

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
#ifndef __PJENCHUFF_H__
#include "pjenchuff.h"
#endif



#define MAX_CODE_LEN 32

LOCFUN(IppStatus,mfxownpj_EncodeHuffmanSpecInit,(
  const Ipp8u*                  pBits,
  const Ipp8u*                  pVals,
        ownpjEncodeHuffmanSpec* pEncHuffTbl))
{
  int i;
  int j;
  int k;
  unsigned int si;
  unsigned int code;
  int lastk;
  unsigned int huffsize[257];
  unsigned int huffcode[257];

  mfxownsZero_8u((Ipp8u*)pEncHuffTbl,sizeof(ownpjEncodeHuffmanSpec));
  mfxownsZero_8u((Ipp8u*)&huffsize[0],sizeof(huffsize));
  mfxownsZero_8u((Ipp8u*)&huffcode[0],sizeof(huffcode));

  /* Generate Size Table */
  /* ISO/IEC 10918-1, Figure C.1 - Generation of table of Huffman code sizes */

  k = 0;

  for(i = 0; i < 16; i++)
  {
    j = pBits[i];

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
  lastk       = k;

  /* Generate Code Table */
  /* ISO/IEC 10918-1, Figure C.2 - Generation of table of Huffman codes */

  k    = 0;
  code = 0;
  si   = huffsize[0];

  while(huffsize[k])
  {
    while(huffsize[k] == si)
    {
      huffcode[k++] = code;
      code++;
    }
    /* code is now 1 more than the last code used for codelength si; but
     * it must still fit in si bits, since no code is allowed to be all ones.
     */
    if(code >= (unsigned int)(1 << si))
    {
      /* corrupted table */
      return ippStsJPEGHuffTableErr;
    }
    code <<= 1;
    si++;
  }

  /* Order Codes */
  /* ISO/IEC 10918-1, Figure C.3 - Ordering procedure for encoding */
  /* procedure code tables */

  for(k = 0; k < lastk; k++)
  {
    i                   = (int)pVals[k];
    pEncHuffTbl->hcs[i] = (huffsize[k] << 16) | (huffcode[k] & 0xffff);
  }

  return ippStsNoErr;
} /* mfxownpj_EncodeHuffmanSpecInit() */


OWNFUN(void,mfxownpj_EncodeHuffmanStateInit,(
  ownpjEncodeHuffmanState* pState))
{
  pState->uBitBuffer     = 0;
  pState->nBitsValid     = 0;

  pState->nBE            = 0;
  pState->nEndOfBlockRun = 0;

  mfxownsZero_8u(pState->pBEBuff,sizeof(pState->pBEBuff));

  return;
} /* mfxownpj_EncodeHuffmanStateInit() */


OWNFUN(IppStatus,mfxownpj_write_bits_jpeg,(
  const unsigned int             uValue,
  const int                      nBits,
        Ipp8u*                   pDst,
        int                      nDstLenBytes,
        int*                     pDstCurrPos,
        ownpjEncodeHuffmanState* pState))
{
  int    nBitsValid;
  Ipp32u uBitBuffer;
  IppStatus status  = ippStsNoErr;

  if(nBits == 0)
  {
    return ippStsJPEGHuffTableErr;
  }

  uBitBuffer = (Ipp32u)(uValue & MASK(nBits));
  nBitsValid = pState->nBitsValid + nBits;

  uBitBuffer <<= 24 - nBitsValid;

  uBitBuffer |= (Ipp32u)pState->uBitBuffer;

  while(nBitsValid >= 8)
  {
    Ipp32u c = (Ipp32u)((uBitBuffer >> 16) & 0xFF);

    if(*pDstCurrPos >= nDstLenBytes)
    {
      status = ippStsJPEGOutOfBufErr;
      goto Exit;
    }

    *(pDst + *pDstCurrPos) = (Ipp8u)c;
    (*pDstCurrPos)++;

    if(c == 0xff)
    {
      if(*pDstCurrPos == nDstLenBytes)
      {
        status = ippStsJPEGOutOfBufErr;
        goto Exit;
      }
      *(pDst + *pDstCurrPos) = 0x00;
      (*pDstCurrPos)++;
    }

    uBitBuffer <<= 8;
    nBitsValid  -= 8;
  }

   /* update state variables */
  pState->uBitBuffer = uBitBuffer;
  pState->nBitsValid = nBitsValid;

Exit:

  return status;
} /* mfxownpj_write_bits_jpeg() */


/* ---------------------- library functions definitions -------------------- */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanRawTableInit_JPEG_8u
//
//  Purpose:
//    Build Huffman "raw" table from counted statistics of
//    Huffman symbols
//
//  Parameters:
//    pStatistics - pointer to Huffman symbols statistics
//    pListBits   - pointer to list of bits for Huffman codes
//    pListVals   - pointer to list of vals for Huffman codes
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffmanRawTableInit_JPEG_8u, (
  const int    pStatistics[256],
        Ipp8u* pListBits,
        Ipp8u* pListVals))
{
  int   i;
  int   j;
  int   k;
  int   v;
  int   v1;
  int   v2;
  int   freq[257];
  int   codesize[257];
  int   others[257];
  Ipp8u bits[MAX_CODE_LEN+1];

  IPP_BAD_PTR1_RET(pStatistics);
  IPP_BAD_PTR2_RET(pListBits,pListVals);

  mfxownsCopy_8u((Ipp8u*)pStatistics,(Ipp8u*)&freq[0],sizeof(freq));
  mfxownsZero_8u((Ipp8u*)&codesize,sizeof(codesize));
  mfxownsZero_8u((Ipp8u*)&bits,sizeof(bits));
  mfxownsSet_32s(-1,others,257);

  freq[256] = 1;

  for(;;)
  {
    /* find v1 for least value of freq[v1] > 0 */
    v1 = -1;
    v = 1000000000L;

    for(i = 0; i <= 256; i++)
    {
      if((freq[i] != 0) && (freq[i] <= v))
      {
        v = freq[i];
        v1 = i;
      }
    }

    /* find v2 for next least value of freq[v2] > 0 */
    v2 = -1;
    v = 1000000000L;
    for(i = 0; i <= 256; i++)
    {
      if((freq[i] != 0) && (freq[i] <= v) && (i != v1))
      {
        v = freq[i];
        v2 = i;
      }
    }

    if(v2 < 0 || v1 < 0)
    {
      break;
    }

    freq[v1] = freq[v1] + freq[v2];
    freq[v2] = 0;

    codesize[v1]++;

    while(others[v1] >= 0)
    {
      v1 = others[v1];
      codesize[v1]++;
    }

    others[v1] = v2;

    codesize[v2]++;

    while(others[v2] >= 0)
    {
      v2 = others[v2];
      codesize[v2]++;
    }
  }


  /* count bits */
  for(i = 0; i <= 256; i++)
  {
    if(codesize[i])
    {
      if(codesize[i] > MAX_CODE_LEN)
      {
        return ippStsJPEGHuffTableErr;
      }

      bits[ codesize[i] ]++;
    }
  }


  /* adjust bits */

  for(i = MAX_CODE_LEN; i > 16; i--)
  {
    while(bits[i] > 0)
    {
      j = i - 2;
      while (bits[j] == 0)
      {
        j--;
      }

      bits[i]   = (Ipp8u)(bits[i] - 2);
      bits[i-1] = (Ipp8u)(bits[i - 1] + 1);
      bits[j+1] = (Ipp8u)(bits[j + 1] + 2);
      bits[j]   = (Ipp8u)(bits[j] - 1);
    }
  }

  while(bits[i] == 0)
  {
    i--;
  }

  bits[i] = (Ipp8u)(bits[i] - 1);


  /* sort input */

  mfxsCopy_8u(&bits[1],pListBits,16);

  k = 0;

  for(i = 1; i <= MAX_CODE_LEN; i++)
  {
    for(j = 0; j <= 255; j++)
    {
      if(codesize[j] == i)
      {
        pListVals[k] = (Ipp8u)j;
        k++;
      }
    }
  }

  return ippStsNoErr;
} /* mfxiEncodeHuffmanRawTableInit_JPEG_8u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanSpecGetBufSize_JPEG_8u
//
//  Purpose:
//    get size of MfxiEncodeHuffmanSpec struct
//
//  Parameters:
//    pEncHuffSpecBufSize - where write size of buffer
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffmanSpecGetBufSize_JPEG_8u, (
  int* pEncHuffSpecBufSize))
{
  IPP_BAD_PTR1_RET(pEncHuffSpecBufSize);

  *pEncHuffSpecBufSize = sizeof(ownpjEncodeHuffmanSpec);

  return ippStsNoErr;
} /* mfxiEncodeHuffmanSpecGetBufSize_JPEG_8u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanSpecInit_JPEG_8u
//
//  Purpose:
//    Build Huffman table for encoder
//    in fast-to-use format from raw Huffman table
//
//  Parameters:
//    pListBits    - pointer to raw Huffman table bits
//    pListVals    - pointer to raw Huffman table vals
//    pEncHuffSpec - pointer to Huffman table to be initialized
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffmanSpecInit_JPEG_8u, (
  const Ipp8u*            pListBits,
  const Ipp8u*            pListVals,
  IppiEncodeHuffmanSpec*  pEncHuffSpec))
{
  ownpjEncodeHuffmanSpec* pEncHuffTbl;

  IPP_BAD_PTR2_RET(pListBits,pListVals);
  IPP_BAD_PTR1_RET(pEncHuffSpec);

  pEncHuffTbl = (ownpjEncodeHuffmanSpec*)pEncHuffSpec;

  return mfxownpj_EncodeHuffmanSpecInit(pListBits,pListVals,pEncHuffTbl);
} /* mfxiEncodeHuffmanSpecInit_JPEG_8u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanStateGetBufSize_JPEG_8u
//
//  Purpose:
//    get size of IppiEncodeHuffmanState struct
//
//  Parameters:
//    pEncHuffStateBufSize - where write size of buffer
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffmanStateGetBufSize_JPEG_8u, (
  int*  pEncHuffStateBufSize))
{
  IPP_BAD_PTR1_RET(pEncHuffStateBufSize);

  *pEncHuffStateBufSize = sizeof(ownpjEncodeHuffmanState);

  return ippStsNoErr;
} /* mfxiEncodeHuffmanStateGetBufSize_JPEG_8u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanStateInit_JPEG_8u
//
//  Purpose:
//    Build IppiEncodeHuffmanState struct
//
//  Parameters:
//    pEncHuffState - pointer to IppiEncodeHuffmanState struct
//                    to be initialized
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffmanStateInit_JPEG_8u, (
  IppiEncodeHuffmanState*  pEncHuffState))
{
  ownpjEncodeHuffmanState* pState;

  IPP_BAD_PTR1_RET(pEncHuffState);

  pState = (ownpjEncodeHuffmanState*)pEncHuffState;

  mfxownpj_EncodeHuffmanStateInit(pState);

  return ippStsNoErr;
} /* mfxiEncodeHuffmanStateInit_JPEG_8u() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffman8x8_JPEG_16s1u_C1
//
//  Purpose:
//    Huffman encode 8x8 block of quantized DCT coefs
//
//  Parameters:
//    pSrc            pointer to quantized DCT coefficients
//    pDst            pointer to output JPEG bitstream
//    DstLenBytes     bitstream length, in bytes
//    pDstCurrPos     current offset in bytes in buffer (in/out)
//    pLastDC         pointer to last DC coefs (in/out)
//    pEncHuffState   pointer to Huffman state struct
//    pDcTable        pointer to Huffman DC table
//    pAcTable        pointer to Huffman AC table
//    bFlushState     if non zero - only flush any pending bits from state
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiEncodeHuffman8x8_JPEG_16s1u_C1, (
  const Ipp16s*                 pSrc,
        Ipp8u*                  pDst,
        int                     nDstLenBytes,
        int*                    pDstCurrPos,
        Ipp16s*                 pLastDC,
  const IppiEncodeHuffmanSpec*  pDcTable,
  const IppiEncodeHuffmanSpec*  pAcTable,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))
{
  int    i;
  int    r;
  int    rs;
  unsigned int cs;
  unsigned int uValue;
  int    nBits;
  int    data;
  int    ssss;
  ownpjEncodeHuffmanSpec*  dc_table = (ownpjEncodeHuffmanSpec*)pDcTable;
  ownpjEncodeHuffmanSpec*  ac_table = (ownpjEncodeHuffmanSpec*)pAcTable;
  ownpjEncodeHuffmanState* pState   = (ownpjEncodeHuffmanState*)pEncHuffState;
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
  IPP_BAD_PTR1_RET(pLastDC);
  IPP_BAD_PTR1_RET(pDcTable);
  IPP_BAD_PTR1_RET(pAcTable);

#if defined (_A6) || ( _IPP >= _IPP_W7 ) || ( _IPP32E >= _IPP32E_M7 )
  status = mfxownpj_EncodeHuffman8x8_JPEG_16s1u_C1(
           pSrc,
           pDst,
           nDstLenBytes,
           pDstCurrPos,
           pLastDC,
           dc_table,
           ac_table,
           pState);

  if(ippStsNoErr == status)
  {
    goto Exit;
  }
#endif

  /* Encode DC Coefficient */

  data = pSrc[0] - *pLastDC;

  *pLastDC = pSrc[0];

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

  cs = dc_table->hcs[ssss];

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

  if(ssss)
  {
    status = mfxownpj_write_bits_jpeg(
      data,ssss,pDst,nDstLenBytes,pDstCurrPos,pState);

    if(ippStsNoErr != status)
    {
      goto Exit;
    }
  }

  /* Encode AC Coefficient(s) */

  r = 0;

  for(i = 1; i < DCTSIZE2; i++)
  {
    data = pSrc[mfxown_pj_izigzag_index[i]];

    if(data == 0)
    {
      r++; /* Increment run-length of zeroes */
    }
    else
    {
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

      rs = (r << 4) + ssss;

      /* Encode run-length of zeros and size of first non-zero AC coeff */

      cs = ac_table->hcs[rs];

      uValue = cs & 0xffff;
      nBits  = cs >> 16;

      status = mfxownpj_write_bits_jpeg(
                 uValue,nBits,pDst,nDstLenBytes,pDstCurrPos,pState);

      if(ippStsNoErr != status)
      {
        goto Exit;
      }

      /* Encode AC coefficient value and sign */

      if(ssss)
      {
        status = mfxownpj_write_bits_jpeg(
          data,ssss,pDst,nDstLenBytes,pDstCurrPos,pState);

        if(ippStsNoErr != status)
        {
          goto Exit;
        }
      }

      r = 0;
    } /* if data != 0 */
  }   /* for i = 1...63 */

  if(r > 0)
  {
    /* Emit End Of Block code */

    cs = ac_table->hcs[0x00];

    uValue = cs & 0xffff;
    nBits  = cs >> 16;

    status = mfxownpj_write_bits_jpeg(
      uValue,nBits,pDst,nDstLenBytes,pDstCurrPos,pState);

    if(ippStsNoErr != status)
    {
      goto Exit;
    }
  }

Exit:

  return status;
} /* mfxiEncodeHuffman8x8_JPEG_16s1u_C1() */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiGetHuffmanStatistics8x8_JPEG_16s_C1
//
//  Purpose:
//    gather huffman symbols statistics
//
//  Parameters:
//    pSrc               - pointer to input DCT coefficients block
//    pDcStatistics[256] - array for DC symbols statistics
//    pAcStatistics[265] - array for AC symbols statistics
//    pLastDC            - pointer to variable to store DC coefficient
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPFUN(IppStatus, mfxiGetHuffmanStatistics8x8_JPEG_16s_C1, (
  const Ipp16s* pSrc,
        int     pDcStatistics[256],
        int     pAcStatistics[256],
        Ipp16s* pLastDC))
{
  int    i;
  int    r;
  int    rs;
  int    data;
  int    ssss;

  IPP_BAD_PTR1_RET(pSrc);
  IPP_BAD_PTR2_RET(pDcStatistics,pAcStatistics);
  IPP_BAD_PTR1_RET(pLastDC);


  /* Encode DC Coefficient */
  data = pSrc[0] - *pLastDC;

  *pLastDC = pSrc[0];

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

  if(ssss > 15)
    return ippStsJPEGDCTRangeErr;

  /* Count huffval Size of DC Coefficient */
  pDcStatistics[ssss]++;


  /* Encode AC Coefficient(s) */
  r = 0;

  for(i = 1; i < DCTSIZE2; i++)
  {
    data = pSrc[mfxown_pj_izigzag_index[i]];

    if(data == 0)
    {
      r++; /* Increment run-length of zeroes */
    }
    else
    {
      while(r > 15)
      {
        pAcStatistics[0xF0]++;

        r -= 16;
      }

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

        /* data -= 1; */
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

      if(ssss > 14)
        return ippStsJPEGDCTRangeErr;

      rs = (r << 4) + ssss;

      /* Count run-length of zeros and size of first non-zero AC coeff */
      pAcStatistics[rs]++;

      r = 0;
    } /* if data != 0 */
  }   /* for i = 1...63 */

  if(r > 0)
  {
    /* Emit End Of Block code */

    pAcStatistics[0]++;
  }

  return ippStsNoErr;
} /* mfxiGetHuffmanStatistics8x8_JPEG_16s_C1() */

