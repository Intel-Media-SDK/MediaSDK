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

/* ///////////////////////////// "pjenchuff.h" ////////////////////////////////
//
//  Purpose:
//
*/

#ifndef __PJENCHUFF_H__
#define __PJENCHUFF_H__

#ifndef __OWNJ_H__
#include "ownj.h"
#endif




#define MASK(nBits) \
  ( (1 << (nBits)) - 1 )


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ownpjEncodeHuffmanSpec
//
//  Purpose:
//    Encoder Huffman table in fast-to-use format
//
//  Notes:
//    each hcs entry contain composite value.
//    Huffman code size (in bits) in high order 16 bit word
//    Huffman code in low order 16 bit word
//
*/

typedef struct _ownpjEncodeHuffmanSpec
{
  Ipp32u hcs[256];
} ownpjEncodeHuffmanSpec;


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ownpjEncodeHuffmanState
//
//  Purpose:
//    Huffman encoder state
//
//  Fields:
//
//  Notes:
//
*/

typedef struct _ownpjEncodeHuffmanState
{
  Ipp64u   uBitBuffer;
  int      nBitsValid;
  int      nEndOfBlockRun;
  int      nBE;
  Ipp8u    pBEBuff[1024];
} ownpjEncodeHuffmanState;



OWNAPI(IppStatus,mfxownpj_write_bits_jpeg,(
  const unsigned int             uValue,
  const int                      nBits,
        Ipp8u*                   pDst,
        int                      nDstLenBytes,
        int*                     pDstCurrPos,
        ownpjEncodeHuffmanState* pState));

OWNAPI(void,mfxownpj_EncodeHuffmanStateInit,(
  ownpjEncodeHuffmanState* pState));


#if defined (_A6) || ( _IPP >= _IPP_W7 ) || ( _IPP32E >= _IPP32E_M7 )

ASMAPI(IppStatus,mfxownpj_EncodeHuffman8x8_JPEG_16s1u_C1,(
  const Ipp16s*                  pSrc,
        Ipp8u*                   pDst,
        int                      nDstLenBytes,
        int*                     pDstCurrPos,
        Ipp16s*                  pLastDC,
  const ownpjEncodeHuffmanSpec*  pDcTable,
  const ownpjEncodeHuffmanSpec*  pAcTable,
        ownpjEncodeHuffmanState* pEncHuffState));

ASMAPI(IppStatus,mfxownpj_EncodeHuffman8x8_ACFirst_JPEG_16s1u_C1,(
  const Ipp16s*                  pSrc,
        Ipp8u*                   pDst,
        int                      nDstLenBytes,
        int*                     pDstCurrPos,
        int                      Ss,
        int                      Se,
        int                      Al,
  const ownpjEncodeHuffmanSpec*  pAcTable,
        ownpjEncodeHuffmanState* pEncHuffState));

ASMAPI(IppStatus,mfxownpj_EncodeHuffman8x8_ACFlush_JPEG_16s1u_C1,(
        Ipp8u*                   pDst,
        int                      nDstLenBytes,
        int*                     pDstCurrPos,
  const ownpjEncodeHuffmanSpec*  pAcTable,
        ownpjEncodeHuffmanState* pEncHuffState));

ASMAPI(IppStatus,mfxownpj_GetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1,(
  const Ipp16s*                  pSrc,
        int                      pAcStatistics[256],
        int                      Ss,
        int                      Se,
        int                      Al,
        ownpjEncodeHuffmanState* pEncHuffState));

ASMAPI(IppStatus,mfxownpj_GetHuffmanStatistics8x8_ACFlush_JPEG_16s_C1,(
        int                     pAcStatistics[256],
        ownpjEncodeHuffmanState* pEncHuffState));

ASMAPI(IppStatus,mfxownpj_EncodeHuffman8x8_ACRefine_JPEG_16s1u_C1,(
  const Ipp16s*                  pSrc,
        Ipp8u*                   pDst,
        int                      nDstLenBytes,
        int*                     pDstCurrPos,
        int                      Ss,
        int                      Se,
        int                      Al,
  const ownpjEncodeHuffmanSpec*  pAcTable,
        ownpjEncodeHuffmanState* pEncHuffState));

#endif

#endif /* __PJENCHUFF_H__ */

