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


#ifndef __PJDECHUFF_H__
#define __PJDECHUFF_H__

#ifndef __OWNJ_H__
#include "ownj.h"
#endif


/* minimum allowable value */
#define HUFF_MIN_GET_BITS 25
#define HUFF_LOOKAHEAD     8


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxownpjDecodeHuffmanSpec
//
//  Purpose:
//    Decoder Huffman table in fast-to-use format
//
//  Notes:
//
*/

typedef struct _ownpjDecodeHuffmanSpec
{
  Ipp16u huffval[256];
  Ipp32u huffelem[256];
  Ipp16u mincode[18];
  Ipp16s maxcode[18];
  Ipp16u valptr[18];
} ownpjDecodeHuffmanSpec;


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxownpjDecodeHuffmanState
//
//  Purpose:
//    Huffman decoder state
//
//  Fields:
//
//  Notes:
//
*/

typedef struct _ownpjDecodeHuffmanState
{
  Ipp64u  uBitBuffer;
  int     nBitsValid;
  int     nEndOfBlockRun;
  int     lastNonZeroNo;
} ownpjDecodeHuffmanState;


#define MASK(nBits) \
  ( (1 << (nBits)) - 1 )

#define mfxownpj_peek_bits(nBits,pState) \
  ( ((int)((Ipp32u)(pState)->uBitBuffer >> \
          ( (pState)->nBitsValid - (nBits) ))) & MASK(nBits) )

#define mfxownpj_get_bits(nBits,pState) \
  ( ( (pState)->nBitsValid -= (nBits) ), \
    ((int)((Ipp32u)(pState)->uBitBuffer >> \
          ( (pState)->nBitsValid ))) & MASK(nBits) )


#if !defined (_I7)

OWNAPI(IppStatus,mfxownpj_FillBitBuffer,(
  const Ipp8u*                   pSrc,
        int                      SrcLenBytes,
        int*                     pSrcCurrPos,
        int*                     pMarker,
        int                      nBits,
        ownpjDecodeHuffmanState* pState));

#endif


OWNAPI(IppStatus,mfxownpj_DecodeHuffSymbol,(
  const Ipp8u*                   pSrc,
        int                      nSrcLenBytes,
        int*                     pSrcCurrPos,
        int*                     pMarker,
        int*                     pResult,
  const ownpjDecodeHuffmanSpec*  pDecHuffTable,
        ownpjDecodeHuffmanState* pDecHuffState));


#if (defined (_A6) || ( _IPP >= _IPP_W7 ) || ( _IPP32E >= _IPP32E_M7 )) || ((_IPP_ARCH ==_IPP_ARCH_LRB) && (_IPPLRB == _IPPLRB_B1))

ASMAPI(IppStatus,mfxownpj_DecodeHuffman8x8_JPEG_1u16s_C1,(
  const Ipp8u*                   pSrc,
        int                      nSrcLenBytes,
        int*                     pSrcCurrPos,
        Ipp16s*                  pDst,
        Ipp16s*                  pLastDC,
        int*                     pMarker,
  const ownpjDecodeHuffmanSpec*  pDcTable,
  const ownpjDecodeHuffmanSpec*  pAcTable,
        ownpjDecodeHuffmanState* pDecHuffState));

ASMAPI(IppStatus,mfxownpj_DecodeHuffman8x8_Direct_JPEG_1u16s_C1,(
  const Ipp8u*                  pSrc,
        int*                    pSrcBitsLen,
        Ipp16s*                 pDst,
        Ipp16s*                 pLastDC,
        int*                    pMarker,
        Ipp32u*                 pPrefetchedBits,
        int*                    pNumValidPrefetchedBits,
  const ownpjDecodeHuffmanSpec* pDCTable,
  const ownpjDecodeHuffmanSpec* pACTable));

ASMAPI(IppStatus, mfxownpj_DecodeHuffman8x8_ACFirst_JPEG_1u16s_C1,(
    const Ipp8u*                   pSrc,
          int                      nSrcLenBytes,
          int*                     pSrcCurrPos,
          Ipp16s*                  pDst,
          int*                     pMarker,
          int                      Ss,
          int                      Se,
          int                      Al,
    const ownpjDecodeHuffmanSpec*  pAcTable,
          ownpjDecodeHuffmanState* pDecHuffState));

ASMAPI(IppStatus, mfxownpj_DecodeHuffman8x8_ACRefine_JPEG_1u16s_C1,(
    const Ipp8u*                   pSrc,
          int                      SrcLenBytes,
          int*                     pSrcCurrPos,
          Ipp16s*                  pDst,
          int*                     pMarker,
          int                      Ss,
          int                      Se,
          int                      Al,
    const ownpjDecodeHuffmanSpec*  pAcTable,
          ownpjDecodeHuffmanState* pDecHuffState));

#endif


#if defined (_I7)

ASMAPI(IppStatus,mfxownpj_FillBitBuffer,(
  const Ipp8u*                   pSrc,
        int                      nSrcLenBytes,
        int*                     pSrcCurrPos,
        int*                     pMarker,
        int                      nBits,
        ownpjDecodeHuffmanState* pDecHuffState));

ASMAPI(IppStatus,mfxownpj_DecodeHuffmanOne_JPEG_1u16s_C1,(
           const Ipp8u*                    pSrc,
                 int                       nSrcLenBytes,
                 int*                      pSrcCurrPos,
                 Ipp16s*                   pDst,
                 int*                      pMarker,
           const ownpjDecodeHuffmanSpec*   pDcTable,
                 ownpjDecodeHuffmanState*  pDecHuffState));

#endif


#if ( _IPP >= _IPP_W7 )

ASMAPI(IppStatus,mfxownpj_DecodeHuffmanOne_JPEG_1u16s_C1,(
          const Ipp8u*                    pSrc,
                Ipp16s*                   pDst,
                int*                      pSrcCurrPos,
          const ownpjDecodeHuffmanSpec*   pDcTable,
                ownpjDecodeHuffmanState*  pDecHuffState));

#endif

#if (_IPP >= _IPP_V8) || ( _IPP32E >= _IPP32E_U8 )

ASMAPI(IppStatus,mfxownpj_DecodeHuffmanRow_JPEG_1u16s_C1P4,(
    const Ipp8u*                   pSrc,
          int                      SrcLenBytes,
          int*                     pSrcCurrPos,
          Ipp16s*                  pDst[4],
          int                      nDstLen,
          int                      nDstRows,
          int*                     pMarker,
    const ownpjDecodeHuffmanSpec*  pTable[4],
          ownpjDecodeHuffmanState* pDecHuffState));

#endif


#endif /* __PJDECHUFF_H__ */

