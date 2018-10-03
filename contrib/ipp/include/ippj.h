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
//              Intel(R) Integrated Performance Primitives
//              JPEG Processing (ippJP)
// 
// 
*/


#if !defined( __IPPJ_H__ ) || defined( _OWN_BLDPCS )
#define __IPPJ_H__

#if defined (_WIN32_WCE) && defined (_M_IX86) && defined (__stdcall)
  #define _IPP_STDCALL_CDECL
  #undef __stdcall
#endif

#ifndef __IPPDEFS_H__
#include "ippdefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    IppiEncodeHuffmanSpec
//
//  Purpose:
//    Encoder Huffman table in fast-to-use format
//
//  Notes:
//
*/

struct EncodeHuffmanSpec;

typedef struct EncodeHuffmanSpec IppiEncodeHuffmanSpec;


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    Mfxi_DecodeHuffmanSpec
//
//  Purpose:
//    Decoder Huffman table in fast-to-use format
//
//  Notes:
//
*/

struct DecodeHuffmanSpec;

typedef struct DecodeHuffmanSpec IppiDecodeHuffmanSpec;


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    IppiEncodeHuffmanState
//
//  Purpose:
//    Huffman encoder state
//
//  Notes:
//
*/

struct EncodeHuffmanState;

typedef struct EncodeHuffmanState IppiEncodeHuffmanState;


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    Mfxi_DecodeHuffmanState
//
//  Purpose:
//    Huffman decoder state
//
//  Notes:
//
*/

struct DecodeHuffmanState;

typedef struct DecodeHuffmanState IppiDecodeHuffmanState;

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//
//  Purpose:
//    Enumerator of lossless JPEG predictors
//
*/

enum
{
  PRED1 = 1,
  PRED2 = 2,
  PRED3 = 3,
  PRED4 = 4,
  PRED5 = 5,
  PRED6 = 6,
  PRED7 = 7
};




/* ///////////////////////////////////////////////////////////////////////////
//        Huffman Encoding Functions (Baseline)
/////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanRawTableInit_JPEG_8u
//
//  Purpose:
//    Build Huffman "raw" table from counted statistics of
//    Huffman symbols for 8-bit precision
//
//  Parameters:
//    pStatistics - pointer to array of 256 int,
//                  with the one entry for each of possible huffman symbol.
//    pListBits   - pointer to array of 16 bytes.
//    pListVals   - pointer to array of 256 bytes.
//
//  Returns:
//    IppStatus
//
//  Notes:
//    pListBits represents list of BITS and pListVals represent list
//    of HUFFVAL, as specified in ISO/IEC 10918-1, Figure B.7
*/

IPPAPI(IppStatus, mfxiEncodeHuffmanRawTableInit_JPEG_8u, (
  const int    pStatistics[256],
        Ipp8u* pListBits,
        Ipp8u* pListVals))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanSpecGetBufSize_JPEG_8u
//
//  Purpose:
//    get size of IppjEncodeHuffmanSpec struct
//
//  Parameters:
//    size - where write size of buffer
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiEncodeHuffmanSpecGetBufSize_JPEG_8u, (
  int* size))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanSpecInit_JPEG_8u
//
//  Purpose:
//    Build Huffman table for encoder
//    in fast-to-use format from "raw" Huffman table
//
//  Parameters:
//    pListBits    - pointer to array of 16 bytes.
//    pListVals    - pointer to array of up to 256 bytes.
//    pEncHuffSpec - pointer to Huffman table to be initialized
//
//  Returns:
//    IppStatus
//
//  Notes:
//    pListBits represents list of BITS and pListVals represent list
//    of HUFFVAL, as specified in ISO/IEC 10918-1, Figure B.7
*/

IPPAPI(IppStatus, mfxiEncodeHuffmanSpecInit_JPEG_8u, (
  const Ipp8u*                 pListBits,
  const Ipp8u*                 pListVals,
        IppiEncodeHuffmanSpec* pEncHuffSpec))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanStateInit_JPEG_8u
//
//  Purpose:
//    Build EncHuffState struct for encoder
//
//  Parameters:
//    pEncHuffState - pointer to EncHuffState struct to be initialized
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiEncodeHuffmanStateInit_JPEG_8u, (
  IppiEncodeHuffmanState*  pEncHuffState))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffman8x8_JPEG_16s1u_C1
//
//  Purpose:
//    Huffman encode 8x8 block of quantized DCT coefs
//
//  Parameters:
//    pSrc            pointer to 8x8 block of quantized DCT coefficients
//    pDst            pointer to output JPEG bitstream
//    dstLenBytes     bitstream length, in bytes
//    pDstCurrPos     pointer to current byte offset in buffer (in/out)
//    pLastDC         pointer to last DC coefficient from previous block of
//                    the same color component (in/out)
//    pEncHuffState   pointer to Huffman state struct
//    pDcTable        pointer to Huffman DC table
//    pAcTable        pointer to Huffman AC table
//    bFlushState     if non zero - only flush any pending bits
//                    from IppiEncodeHuffmanState to bitstream
//
//  Returns:
//    IppStatus
//
//  Notes:
//    set bFlushState to 1, after processing all MCUs in scan, to
//    flush any pending bits from IppiEncodeHuffmanState to bitstream
//    Encoding perform as defined in ISO/IEC 10918-1.
//    Appendix A - Requirements and guidelines,
//    Annex F, F.1.2 - Baseline Huffman encoding procedures.
*/

IPPAPI(IppStatus, mfxiEncodeHuffman8x8_JPEG_16s1u_C1, (
  const Ipp16s*                 pSrc,
        Ipp8u*                  pDst,
        int                     dstLenBytes,
        int*                    pDstCurrPos,
        Ipp16s*                 pLastDC,
  const IppiEncodeHuffmanSpec*  pDcTable,
  const IppiEncodeHuffmanSpec*  pAcTable,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiGetHuffmanStatistics8x8_JPEG_16s_C1
//
//  Purpose:
//    Calculate statistics of huffman values
//
//  Parameters:
//    pSrc          - pointer to 8x8 block of quantized DCT coefficients
//    pDcStatistics - pointer to 256 elements array
//    pAcStatistics - pointer to 256 elements array
//    pLastDC       - pointer to DC coefficient from previous 8x8 block
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiGetHuffmanStatistics8x8_JPEG_16s_C1, (
  const Ipp16s* pSrc,
        int     pDcStatistics[256],
        int     pAcStatistics[256],
        Ipp16s* pLastDC))




/* ///////////////////////////////////////////////////////////////////////////
//        Huffman Encoding Functions (Progressive)
/////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiGetHuffmanStatistics8x8_DCFirst_JPEG_16s_C1
//
//  Purpose:
//    Calculate statistics of huffman values, progressive coding,
//    first scan
//
//  Parameters:
//    pSrc          - pointer to 8x8 block of quantized DCT coefficients
//    pDcStatistics - pointer to 256 elements array
//    pLastDC       - pointer to DC coefficient from previous 8x8 block
//    Al            - successive approximation bit position low
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiGetHuffmanStatistics8x8_DCFirst_JPEG_16s_C1, (
  const Ipp16s* pSrc,
        int     pDcStatistics[256],
        Ipp16s* pLastDC,
        int     Al))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1
//
//  Purpose:
//    Calculate statistics of huffman values, progressive coding,
//    first scan
//
//  Parameters:
//    pSrc          - pointer to 8x8 block of quantized DCT coefficients
//    pAcStatistics - pointer to 256 elements array
//    Ss            - start of spectral selection
//    Se            - end of spectral selection
//    Al            - successive approximation bit position low
//    pEncHuffState - huffman encoder state
//    bFlushState   - set to 1, after processing all MCUs
//                    to flush EOB counter
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiGetHuffmanStatistics8x8_ACFirst_JPEG_16s_C1, (
  const Ipp16s*                 pSrc,
        int                     pAcStatistics[256],
        int                     Ss,
        int                     Se,
        int                     Al,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))




/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1
//
//  Purpose:
//    Calculate statistics of huffman values, progressive coding,
//    next scans
//
//  Parameters:
//    pSrc          - pointer to 8x8 block of quantized DCT coefficients
//    pAcStatistics - pointer to 256 elements array
//    Ss            - start of spectral selection
//    Se            - end of spectral selection
//    Al            - successive approximation bit position low
//    pEncHuffState - huffman encoder state
//    bFlushState   - set to 1, after processing all MCUs
//                    to flush EOB counter
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiGetHuffmanStatistics8x8_ACRefine_JPEG_16s_C1, (
  const Ipp16s*                 pSrc,
        int                     pAcStatistics[256],
        int                     Ss,
        int                     Se,
        int                     Al,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1
//
//  Purpose:
//    huffman encode DC coefficient from 8x8 block of quantized
//    DCT coefficients. Progressive coding, first scan
//
//  Parameters:
//    pSrc          - pointer to 8x8 block of quantized DCT coefficients
//    pDst          - pointer to output bitstream
//    dstLenBytes   - length of bitstream buffer
//    pDstCurrPos   - corrent byte position in bitstream buffer
//    pLastDC       - pointer to DC coefficient from previous 8x8 block
//    Al            - successive approximation bit position low
//    pDcTable      - pointer to DC huffman table
//    pEncHuffState - pointer to encoder huffman state
//    bFlushState   - set to 1, after processing all MCUs
//                    to flush any pending bits from state
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Encoding perform as defined in ISO/IEC 10918-1.
//    Appendix A - Requirements and guidelines,
//    Annex G, G.1.2 - Progressive encoding procedures with Huffman.
*/

IPPAPI(IppStatus, mfxiEncodeHuffman8x8_DCFirst_JPEG_16s1u_C1, (
  const Ipp16s*                 pSrc,
        Ipp8u*                  pDst,
        int                     dstLenBytes,
        int*                    pDstCurrPos,
        Ipp16s*                 pLastDC,
        int                     Al,
  const IppiEncodeHuffmanSpec*  pDcTable,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1
//
//  Purpose:
//    huffman encode DC coefficient from 8x8 block of quantized
//    DCT coefficients. Progressive coding, next scans
//
//  Parameters:
//    pSrc          - pointer to 8x8 block of quantized DCT coefficients
//    pDst          - pointer to output bitstream
//    dstLenBytes   - length of bitstream
//    pDstCurrPos   - current byte position in bitstream
//    Al            - successive approximation bit position low
//    pEncHuffState - pointer to encoder huffman state
//    bFlushState   - set to 1, after processing all MCUs
//                    to flush any pending bits from state
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Encoding perform as defined in ISO/IEC 10918-1.
//    Appendix A - Requirements and guidelines,
//    Annex G, G.1.2 - Progressive encoding procedures with Huffman.
*/

IPPAPI(IppStatus, mfxiEncodeHuffman8x8_DCRefine_JPEG_16s1u_C1, (
  const Ipp16s*                 pSrc,
        Ipp8u*                  pDst,
        int                     dstLenBytes,
        int*                    pDstCurrPos,
        int                     Al,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1
//
//  Purpose:
//    huffman encode AC coefficients from 8x8 block of quantized
//    DCT coefficients. Progressive coding, first scan
//
//  Parameters:
//    pSrc          - pointer to 8x8 block of quantized DCT coefficients
//    pDst          - pointer to output bitstream buffer
//    dstLenBytes   - length of bitstream buffer
//    pDstCurrPos   - current byte position in bitstream buffer
//    Ss            - start of spectral selection
//    Se            - end of spectral selection
//    Al            - successive approximation bit position low
//    pAcTable      - pointer to encoder haffman AC table
//    pEncHuffState - pointer encoder huffman state
//    bFlushState   - set to 1, after processing all MCUs
//                    to flush any pending bits from state
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Encoding perform as defined in ISO/IEC 10918-1.
//    Appendix A - Requirements and guidelines,
//    Annex G, G.1.2 - Progressive encoding procedures with Huffman.
*/

IPPAPI(IppStatus, mfxiEncodeHuffman8x8_ACFirst_JPEG_16s1u_C1, (
  const Ipp16s*                 pSrc,
        Ipp8u*                  pDst,
        int                     dstLenBytes,
        int*                    pDstCurrPos,
        int                     Ss,
        int                     Se,
        int                     Al,
  const IppiEncodeHuffmanSpec*  pAcTable,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))



/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1
//
//  Purpose:
//    huffman encode AC coefficients from 8x8 block of quantized
//    DCT coefficients. Progressive coding, next scans
//
//  Parameters:
//    pSrc          - pointer to 8x8 block of quantized DCT coefficeints
//    pDst          - pointer to output bitstream buffer
//    dstLenBytes   - length of bitstream buffer
//    pDstCurrPos   - current byte position in bitstream buffer
//    Ss            - start of spectral selection
//    Se            - end of spectral selection
//    Al            - successive approximation bit position low
//    pAcTable      - pointer to encoder huffman AC table
//    pEncHuffState - pointer to encoder huffman state
//    bFlushState   - set to 1, after processing all MCUs
//                    to flush any pending bits from state
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Encoding perform as defined in ISO/IEC 10918-1.
//    Appendix A - Requirements and guidelines,
//    Annex G, G.1.2 - Progressive encoding procedures with Huffman.
*/

IPPAPI(IppStatus, mfxiEncodeHuffman8x8_ACRefine_JPEG_16s1u_C1, (
  const Ipp16s*                 pSrc,
        Ipp8u*                  pDst,
        int                     dstLenBytes,
        int*                    pDstCurrPos,
        int                     Ss,
        int                     Se,
        int                     Al,
  const IppiEncodeHuffmanSpec*  pAcTable,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))



/* ///////////////////////////////////////////////////////////////////////////
//        Huffman Encoding Functions (Lossless)
/////////////////////////////////////////////////////////////////////////// */


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
//    pEncHuffTable pointer to huffman lossless table
//    pEncHuffState pointer to Huffman state struct
//    bFlushState
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiEncodeHuffmanOne_JPEG_16s1u_C1, (
  const Ipp16s*                 pSrc,
        Ipp8u*                  pDst,
        int                     nDstLenBytes,
        int*                    pDstCurrPos,
  const IppiEncodeHuffmanSpec*  pEncHuffTable,
        IppiEncodeHuffmanState* pEncHuffState,
        int                     bFlushState))


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

IPPAPI(IppStatus, mfxiGetHuffmanStatisticsOne_JPEG_16s_C1, (
  const Ipp16s* pSrc,
        int     pHuffStatistics[256]))




/* ///////////////////////////////////////////////////////////////////////////
//        Huffman Decoding Functions (Baseline)
/////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanSpecGetBufSize_JPEG_8u
//
//  Purpose:
//    get size of IppiDecodeHuffmanSpec struct
//
//  Parameters:
//    size - where write size of buffer
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiDecodeHuffmanSpecGetBufSize_JPEG_8u, (
  int*  size))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanSpecInit_JPEG_8u
//
//  Purpose:
//    Build Huffman table for decoder in fast-to-use format
//    from "raw" Huffman table
//
//  Parameters:
//    pListBits    - pointer to array of 16 bytes.
//    pListVals    - pointer to array of up to 256 bytes.
//    pDecHuffSpec - pointer to Huffman table to be initialized
//
//  Returns:
//    IppStatus
//
//  Notes:
//    pListBits represents list of BITS and pListVals represent
//    list of HUFFVAL, as specified in ISO/IEC 10918-1, Figure B.7
*/

IPPAPI(IppStatus, mfxiDecodeHuffmanSpecInit_JPEG_8u, (
  const Ipp8u*                  pListBits,
  const Ipp8u*                  pListVals,
        IppiDecodeHuffmanSpec*  pDecHuffSpec))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanStateGetBufSize_JPEG_8u
//
//  Purpose:
//    get size of IppiEncoderHuffmanState struct
//
//  Parameters:
//    size - where write size of buffer
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiEncodeHuffmanStateGetBufSize_JPEG_8u, (
  int*  size))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiEncodeHuffmanStateInit_JPEG_8u
//
//  Purpose:
//    Build EncHuffState struct for encoder
//
//  Parameters:
//    pEncHuffState - pointer to EncHuffState struct to be initialized
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiEncodeHuffmanStateInit_JPEG_8u, (
  IppiEncodeHuffmanState*  pEncHuffState))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanStateGetBufSize_JPEG_8u
//
//  Purpose:
//    get size of IppiDecodeHuffmanState struct
//
//  Parameters:
//    size - where write size of buffer
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiDecodeHuffmanStateGetBufSize_JPEG_8u, (
  int*  size))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanStateInit_JPEG_8u
//
//  Purpose:
//    Build IppiDecodeHuffmanState struct for decoder
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

IPPAPI(IppStatus, mfxiDecodeHuffmanStateInit_JPEG_8u, (
  IppiDecodeHuffmanState*  pDecHuffState))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffman8x8_JPEG_1u16s_C1
//
//  Purpose:
//    Huffman decode 8x8 block of quantized DCT coefficients
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    srcLenBytes   bitstream length, in bytes
//    pSrcCurrPos   pointer to current byte offset in buffer (in/out)
//    pDst          pointer to output 8x8 block of quantized DCT coefficients
//    pLastDC       pointer to last DC coefficients from preivious block
//                  of the same color component
//    pMarker       where to store marker which can be found
//    pDcTable      pointer to huffman DC table
//    pAcTable      pointer to huffman AC table
//    pDecHuffState pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Decoding perform as specified in ISO/IEC 10918-1,
//    Appendix A - Requirements and guidelines,
//    Annex F, F.2.2 - Baseline Huffman Decoding procedures
*/

IPPAPI(IppStatus, mfxiDecodeHuffman8x8_JPEG_1u16s_C1, (
  const Ipp8u*                  pSrc,
        int                     srcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst,
        Ipp16s*                 pLastDC,
        int*                    pMarker,
  const IppiDecodeHuffmanSpec*  pDcTable,
  const IppiDecodeHuffmanSpec*  pAcTable,
        IppiDecodeHuffmanState* pDecHuffState))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffmanRow_JPEG_1u16s_C1P4
//
//  Purpose:
//    Huffman decode row of lossless coefficents
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    nSrcLenBytes  bitstream length, in bytes
//    pSrcCurrPos   pointer to current offset in buffer in bytes (in/out)
//    pDst          pointer to decoded difference
//    nDstLen       number of mcus in row
//    nDstRows      number of rows (actually color channels)
//    pMarker       where to store JPEG marker
//    pDecHuffTable pointer to huffman table
//    pDecHuffState pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiDecodeHuffmanRow_JPEG_1u16s_C1P4, (
  const Ipp8u*                  pSrc,
        int                     nSrcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst[4],
        int                     nDstLen,
        int                     nDstRows,
        int*                    pMarker,
  const IppiDecodeHuffmanSpec*  pDecHuffTable[4],
        IppiDecodeHuffmanState* pDecHuffState))

/* ///////////////////////////////////////////////////////////////////////////
//        Huffman Decoding Functions (Progressive)
/////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1
//
//  Purpose:
//    Huffman decode DC coefficient of 8x8 block of quantized DCT coefficients
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    srcLenBytes   bitstream length, in bytes
//    pSrcCurrPos   pointer to current byte offset in buffer (in/out)
//    pDst          pointer to output 8x8 block of quantized DCT coefficients
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
//    Decoding perform as defined in ISO/IEC 10918-1.
//    Appendix A - Requirements and guidelines,
//    Annex G, G.2 - Progressive decoding of the DCT.
*/

IPPAPI(IppStatus, mfxiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1, (
  const Ipp8u*                  pSrc,
        int                     srcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst,
        Ipp16s*                 pLastDC,
        int*                    pMarker,
        int                     Al,
  const IppiDecodeHuffmanSpec*  pDcTable,
        IppiDecodeHuffmanState* pDecHuffState))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1
//
//  Purpose:
//    Refine DC coefficient of 8x8 block of quantized DCT coefficients
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    srcLenBytes   bitstream length, in bytes
//    pSrcCurrPos   pointer to current byte offset in buffer (in/out)
//    pDst          pointer to output 8x8 block of quantized DCT coefficients
//    pMarker       where to store JPEG marker
//    Al            Approximation bit position low
//    pDecHuffState pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Decoding perform as defined in ISO/IEC 10918-1.
//    Appendix A - Requirements and guidelines,
//    Annex G, G.2 - Progressive decoding of the DCT.
*/

IPPAPI(IppStatus, mfxiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1, (
  const Ipp8u*                   pSrc,
        int                      srcLenBytes,
        int*                     pSrcCurrPos,
        Ipp16s*                  pDst,
        int*                     pMarker,
        int                      Al,
        IppiDecodeHuffmanState*  pDecHuffState))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1
//
//  Purpose:
//    Huffman decode AC coefficient of 8x8 block of quantized DCT coefficients
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    srcLenBytes   bitstream length, in bytes
//    pSrcCurrPos   pointer to current byte offset in buffer (in/out)
//    pDst          pointer to output 8x8 block of quantized DCT coefficients
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
//    Decoding perform as defined in ISO/IEC 10918-1.
//    Appendix A - Requirements and guidelines,
//    Annex G, G.2 - Progressive decoding of the DCT.
*/

IPPAPI(IppStatus, mfxiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1, (
  const Ipp8u*                  pSrc,
        int                     srcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst,
        int*                    pMarker,
        int                     Ss,
        int                     Se,
        int                     Al,
  const IppiDecodeHuffmanSpec*  pAcTable,
        IppiDecodeHuffmanState* pDecHuffState))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1
//
//  Purpose:
//    Refine AC coefficients of 8x8 block of quantized DCT coefficients
//
//  Parameters:
//    pSrc          pointer to input JPEG bitstream
//    srcLenBytes   bitstream length, in bytes
//    pSrcCurrPos   pointer to current byte offset in buffer (in/out)
//    pDst          pointer to output 8x8 block of quantized DCT coefficients
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
//    Decoding perform as defined in ISO/IEC 10918-1.
//    Appendix A - Requirements and guidelines,
//    Annex G, G.2 - Progressive decoding of the DCT.
*/

IPPAPI(IppStatus, mfxiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1, (
  const Ipp8u*                  pSrc,
        int                     srcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst,
        int*                    pMarker,
        int                     Ss,
        int                     Se,
        int                     Al,
  const IppiDecodeHuffmanSpec*  pAcTable,
        IppiDecodeHuffmanState* pDecHuffState))

/* ///////////////////////////////////////////////////////////////////////////
//        Huffman Decoding Functions (Lossless)
/////////////////////////////////////////////////////////////////////////// */


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
//    pDecHuffTable pointer to huffman table
//    pDecHuffState pointer to Huffman state struct
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiDecodeHuffmanOne_JPEG_1u16s_C1, (
  const Ipp8u*                  pSrc,
        int                     nSrcLenBytes,
        int*                    pSrcCurrPos,
        Ipp16s*                 pDst,
        int*                    pMarker,
  const IppiDecodeHuffmanSpec*  pDecHuffTable,
        IppiDecodeHuffmanState* pDecHuffState))

/* ///////////////////////////////////////////////////////////////////////////
//        Quantization Functions for encoder
/////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiQuantFwdRawTableInit_JPEG_8u
//
//  Purpose:
//    Adjust raw quantization table according quality factor
//
//  Parameters:
//    pQuantRawTable - pointer to "raw" Quantization table
//    qualityFactor - JPEG quality factor (valid range 1...100)
//
//  Returns:
//    IppStatus
//
//  Notes:
//    "raw" quantization table is array of 64 bytes (Q0..Q63), as specified
//    in ISO/IEC 10918-1, Figure B.6
*/

IPPAPI(IppStatus, mfxiQuantFwdRawTableInit_JPEG_8u, (
  Ipp8u* pQuantRawTable,
  int    qualityFactor))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiQuantFwdTableInit_JPEG_8u16u
//
//  Purpose:
//    Build 8-bit precision Quantization table for encoder
//    in fast-to-use format from "raw" Quantization table
//
//  Parameters:
//    pQuantRawTable - pointer to "raw" Quantization table
//    pQuantFwdTable - pointer to Quantization table to be initialized
//
//  Returns:
//    IppStatus
//
//  Notes:
//    "raw" quantization table is array of 64 bytes (Q0..Q63), as specified
//    in ISO/IEC 10918-1, Figure B.6 in zigzag order
*/

IPPAPI(IppStatus, mfxiQuantFwdTableInit_JPEG_8u16u, (
  const Ipp8u*  pQuantRawTable,
        Ipp16u* pQuantFwdTable))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiQuantInvTableInit_JPEG_8u16u
//
//  Purpose:
//    Build Quantization table for decoder in fast-to-use format
//    from "raw" Quantization table
//
//  Parameters:
//    pQuantRawTable - pointer to "raw" Quantization table
//    pQuantInvTable - pointer to Quantization table to be initialized
//
//  Returns:
//    IppStatus
//
//  Notes:
//    "raw" quantization table is array of 64 bytes (Q0..Q63), as specified
//    in ISO/IEC 10918-1, Figure B.6 in zigzag order
*/

IPPAPI(IppStatus, mfxiQuantInvTableInit_JPEG_8u16u, (
  const Ipp8u*  pQuantRawTable,
        Ipp16u* pQuantInvTable))




/* ///////////////////////////////////////////////////////////////////////////
//        Functions for color conversion for encoder
/////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiRGBToYCbCr_JPEG_8u_P3R
//
//  Purpose:
//    RGB to YCbCr color conversion
//
//  Parameter:
//    pSrcRGB   pointer to pointers to the input data.
//              pSrc[0] is pointer to RR..RR plane
//              pSrc[1] is pointer to GG..GG plane, and
//              pSrc[2] is pointer to BB..BB plane
//    srcStep   line offset in input data
//    pDstYCbCr pointer to pointers to the output data.
//              pDst[0] is pointer to YY..YY plane
//              pDst[1] is pointer to CbCb..CbCb plane, and
//              pDst[2] is pointer to CrCr..CrCr plane
//    dstStep   line offset in output data
//    roiSize   ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Y  =  0.29900*R + 0.58700*G + 0.11400*B
//    Cb = -0.16874*R - 0.33126*G + 0.50000*B + 128
//    Cr =  0.50000*R - 0.41869*G - 0.08131*B + 128
*/

IPPAPI(IppStatus, mfxiRGBToYCbCr_JPEG_8u_P3R, (
  const Ipp8u*   pSrcRGB[3],
        int      srcStep,
        Ipp8u*   pDstYCbCr[3],
        int      dstStep,
        IppiSize roiSize))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiRGBToY_JPEG_8u_C3C1R
//    mfxiBGRToY_JPEG_8u_C3C1R
//
//  Purpose:
//    RGB to Y color conversion
//
//  Parameter:
//    pSrcRGB   pointer to the input data RGBRGB...RGBRGB
//    srcStep   line offset in input data
//    pDstY     pointer to the output data YY..YY
//    dstStep   line offset in output data
//    roiSize   ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Y = 0.299*R + 0.587*G + 0.114*B
//
*/

IPPAPI(IppStatus, mfxiRGBToY_JPEG_8u_C3C1R, (
  const Ipp8u*   pSrcRGB,
        int      srcStep,
        Ipp8u*   pDstY,
        int      dstStep,
        IppiSize roiSize))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiRGBToYCbCr_JPEG_8u_C3P3R
//    mfxiBGRToYCbCr_JPEG_8u_C3P3R
//
//  Purpose:
//    RGB(BGR) to YCbCr color conversion
//
//  Parameter:
//    pSrcRGB   pointer to input data RGBRGB..RGBRGB
//    srcStep   line offset in input data
//    pDstYCbCr pointer to pointers to the output data.
//                pDstYCbCr[0] is pointer to YY..YY plane
//                pDstYCbCr[1] is pointer to CbCb..CbCb plane, and
//                pDstYCbCr[2] is pointer to CrCr..CrCr plane
//    dstStep   line offset in output data
//    roiSize   ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Y  =  0.29900*R + 0.58700*G + 0.11400*B
//    Cb = -0.16874*R - 0.33126*G + 0.50000*B + 128
//    Cr =  0.50000*R - 0.41869*G - 0.08131*B + 128
*/

IPPAPI(IppStatus, mfxiRGBToYCbCr_JPEG_8u_C3P3R, (
  const Ipp8u*   pSrcRGB,
        int      srcStep,
        Ipp8u*   pDstYCbCr[3],
        int      dstStep,
        IppiSize roiSize))

IPPAPI(IppStatus, mfxiBGRToYCbCr_JPEG_8u_C3P3R, (
  const Ipp8u*   pSrcBGR,
        int      srcStep,
        Ipp8u*   pDstYCbCr[3],
        int      dstStep,
        IppiSize roiSize))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiRGBToYCbCr_JPEG_8u_C4P3R
//
//  Purpose:
//    RGBA/BGRA to YCbCr color conversion (ignore alpha channel)
//
//  Parameter:
//    pSrcRGBA  pointer to input data RGBARGBA..RGBARGBA
//    srcStep   line offset in input data
//    pDstYCbCr pointer to pointers to the output data.
//                pDstYCbCr[0] is pointer to YY..YY plane
//                pDstYCbCr[1] is pointer to CbCb..CbCb plane, and
//                pDstYCbCr[2] is pointer to CrCr..CrCr plane
//    dstStep   line offset in output data
//    roiSize   ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Y  =  0.29900*R + 0.58700*G + 0.11400*B
//    Cb = -0.16874*R - 0.33126*G + 0.50000*B + 128
//    Cr =  0.50000*R - 0.41869*G - 0.08131*B + 128
*/

IPPAPI(IppStatus,mfxiRGBToYCbCr_JPEG_8u_C4P3R,(
  const Ipp8u*   pRGBA,
        int      srcStep,
        Ipp8u*   pYCbCr[3],
        int      dstStep,
        IppiSize roiSize))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiCMYKToYCCK_JPEG_8u_C4P4R
//
//  Purpose:
//    CMYK to YCCK color conversion
//
//  Parameter:
//    pSrcCMYK  pointer to input data CMYKCMYK..CMYKCMYK
//    srcStep   line offset in input data
//    pDstYCCK  pointer to pointers to output arrays
//              pDst[0] is pointer to YY..YY plane
//              pDst[1] is pointer to CC..CC plane, and
//              pDst[2] is pointer to CC..CC plane
//              pDst[3] is pointer to KK..KK plane
//    dstStep   line offset in output data
//    roiSize   ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//    This version handles Adobe-style CMYK->YCCK conversion,
//    where R = 1-C, G = 1-M, and B = 1-Y are converted to YCbCr,
//    while K (black) channel is unchanged.
*/

IPPAPI(IppStatus, mfxiCMYKToYCCK_JPEG_8u_C4P4R, (
  const Ipp8u*   pSrcCMYK,
        int      srcStep,
        Ipp8u*   pDstYCCK[4],
        int      dstStep,
        IppiSize roiSize))




/* ///////////////////////////////////////////////////////////////////////////
//        Downsampling functions for encoder
/////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleDownH2V1_JPEG_8u_C1R
//
//  Purpose:
//    Sample down by horizontal 2:1
//
//  Parameters:
//    pSrc    - pointer to source data to be sampled down
//    srcStep - line offset in input data
//    srcSize - ROI size
//    pDst    - pointer to sampled down output data
//    dstStep - line offset in input data
//    dstSize - ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiSampleDownH2V1_JPEG_8u_C1R, (
  const Ipp8u*   pSrc,
        int      srcStep,
        IppiSize srcRoiSize,
        Ipp8u*   pDst,
        int      dstStep,
        IppiSize dstRoiSize))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleDownH2V2_JPEG_8u_C1R
//
//  Purpose:
//    Sample down by horizontal 2:1 and vertical 2:1
//
//  Parameters:
//    pSrc    - pointer to source data to be sampled down
//    srcStep - line offset in input data
//    srcSize - ROI size
//    pDst    - pointer to sampled down output data
//    dstStep - line offset in input data
//    dstSize - ROI size
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiSampleDownH2V2_JPEG_8u_C1R, (
  const Ipp8u*   pSrc,
        int      srcStep,
        IppiSize srcRoiSize,
        Ipp8u*   pDst,
        int      dstStep,
        IppiSize dstRoiSize))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleDownRowH2V1_Box_JPEG_8u_C1
//
//  Purpose:
//    Sample down by horizontal 2:1
//
//  Parameters:
//    pSrc     - pointer to source row
//    srcWidth - width of source row
//    pDst     - pointer to sampled down output row
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Downsampling is performed as simple "Box" filter
*/

IPPAPI(IppStatus, mfxiSampleDownRowH2V1_Box_JPEG_8u_C1, (
  const Ipp8u*   pSrc,
        int      srcWidth,
        Ipp8u*   pDst))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleDownRowH2V2_Box_JPEG_8u_C1
//
//  Purpose:
//    Sample down by horizontal 2:1 and vertical 2:1
//
//  Parameters:
//    pSrc1    - pointer to the source row
//    pSrc2    - pointer to the next(adjacent) row
//    srcWidth - width of source rows
//    pDst     - pointer to sampled down output row
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Downsampling is performed as simple "Box" filter
*/

IPPAPI(IppStatus, mfxiSampleDownRowH2V2_Box_JPEG_8u_C1, (
  const Ipp8u*   pSrc1,
  const Ipp8u*   pSrc2,
        int      srcWidth,
        Ipp8u*   pDst))




/* ///////////////////////////////////////////////////////////////////////////
//        Upsampling functions for decoder
/////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1
//
//  Purpose:
//    sample up horizontally 1:2
//
//  Parameters:
//    pSrc     - pointer to the source row
//    srcWidth - width of source row
//    pDst     - pointer to sampled up output row
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Triangle filter used there (3/4 * nearer pixel + 1/4 * further pixel)
*/

IPPAPI(IppStatus, mfxiSampleUpRowH2V1_Triangle_JPEG_8u_C1, (
  const Ipp8u*   pSrc,
        int      srcWidth,
        Ipp8u*   pDst))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1
//
//  Purpose:
//    sample up horizontally 1:2 and vertically 1:2
//
//  Parameters:
//    pSrc1    - pointer to the source row
//    pSrc2    - pointer to the next(adjacent) source row
//    srcWidth - width of source rows
//    pDst     - pointer to sampled up output row
//
//  Returns:
//    IppStatus
//
//  Notes:
//    Triangle filter used there (3/4 * nearer pixel + 1/4 * further pixel),
*/

IPPAPI(IppStatus, mfxiSampleUpRowH2V2_Triangle_JPEG_8u_C1, (
  const Ipp8u*   pSrc1,
  const Ipp8u*   pSrc2,
        int      srcWidth,
        Ipp8u*   pDst))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiYCbCrToBGR_JPEG_8u_P3C4R
//
//  Purpose:
//    YCbCr to RGBA/BGRA color conversion with filling alpha channel by
//    specified value
//
//  Parameter:
//    pRGB      pointer to RGB color space
//    srcStep   line offset in RGB color space
//    pYCbCr    pointer to pointers to the YCbCr data.
//                pYCC[0] is pointer to YY..YY plane
//                pYCC[1] is pointer to CbCb..CbCb plane, and
//                pYCC[2] is pointer to CrCr..CrCr plane
//    dstStep   line offset in YCbCr color space
//    roiSize   ROI size
//    aval      alpha value
//
//  Returns:
//    IppStatus
//
//  Notes:
//    R = Y +              1.40200*Cr - 179.456
//    G = Y - 0.34414*Cb - 0.71414*Cr + 135.45984
//    B = Y + 1.77200*Cb              - 226.816
*/

IPPAPI(IppStatus,mfxiYCbCrToBGR_JPEG_8u_P3C4R,(
  const Ipp8u*   pYCbCr[3],
        int      srcStep,
        Ipp8u*   pBGR,
        int      dstStep,
        IppiSize roiSize,
        Ipp8u    aval))

/* ///////////////////////////////////////////////////////////////////////////
//        DCT + Quantization + Level Shift Functions for encoder
/////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantFwd8x8LS_JPEG_8u16s_C1R
//
//  Purpose:
//    Forward DCT transform, quantization and level shift
//
//  Parameter:
//    pSrc           - pointer to source color component data
//    srcStep        - line offset for source data
//    pDst           - pointer to output 8x8 block
//                     of quantized DCT coefficients
//    pQuantFwdTable - pointer to Quantization table
//
//  Returns:
//    IppStatus
//
//  Notes:
//    DCT is defined in ISO/IEC 10918-1,
//    Appendix A - Requrements and guidelines, Annex A, A.3.3 FDCT and IDCT.
//    Quantization is defined in ISO/IEC 10918-1,
//    Appendix A - Requrements and guidelines,
//    Annex A, A.3.4 DCT coefficient quantization and dequantization.
//    Level Shift is defined in ISO/IEC 10918-1,
//    Appendix A - Requrements and guidelines, Annex A, A.3.1 - Level Shift.
*/

IPPAPI(IppStatus, mfxiDCTQuantFwd8x8LS_JPEG_8u16s_C1R, (
  const Ipp8u*  pSrc,
        int     srcStep,
        Ipp16s* pDst,
  const Ipp16u* pQuantFwdTable))


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantFwd8x8LS_JPEG_16u16s_C1R
//
//  Purpose:
//    Forward DCT transform, quantization and level shift
//    for 12-bit data
//
//  Parameter:
//    pSrc           - pointer to source color component data
//    srcStep        - line offset for source data
//    pDst           - pointer to output 8x8 block
//                     of quantized DCT coefficients
//    pQuantFwdTable - pointer to Quantization table
//
//  Returns:
//    IppStatus
*/

IPPAPI(IppStatus, mfxiDCTQuantFwd8x8LS_JPEG_16u16s_C1R, (
  const Ipp16u* pSrc,
        int     srcStep,
        Ipp16s* pDst,
  const Ipp32f* pQuantFwdTable))


/* ///////////////////////////////////////////////////////////////////////////
//        DCT + Dequantization + Level Shift Functions for decoder
/////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R
//    mfxiDCTQuantInv8x8LS_4x4_JPEG_16s8u_C1R
//    mfxiDCTQuantInv8x8LS_2x2_JPEG_16s8u_C1R
//    mfxiDCTQuantInv8x8LS_1x1_JPEG_16s8u_C1R
//
//  Purpose:
//    Inverse DCT transform, de-quantization and level shift for
//      1) the whole 8x8 block
//      2) top-left 4x4 quadrant, in case when rest of coefs are zero
//      3) top-left 2x2 part of 8x8 block, in case when rest of coefs are zero
//      4) only DC coef is not zero in 8x8 block
//
//
//  Parameter:
//    pSrc           - pointer to input 8x8 block of quantized DCT coefficients
//    pDst           - pointer to output color component data
//    dstStep        - line offset for destination data
//    pQuantInvTable - pointer to Quantization table
//
//  Returns:
//    IppStatus
//
//  Notes:
//    DCT is defined in ISO/IEC 10918-1,
//    Appendix A - Requrements and guidelines, Annex A, A.3.3 FDCT and IDCT.
//    Quantization is defined in ISO/IEC 10918-1,
//    Appendix A - Requrements and guidelines,
//    Annex A, A.3.4 DCT coefficient quantization and dequantization.
//    Level Shift is defined in ISO/IEC 10918-1,
//    Appendix A - Requrements and guidelines, Annex A, A.3.1 - Level Shift.
*/

IPPAPI(IppStatus, mfxiDCTQuantInv8x8LS_JPEG_16s8u_C1R, (
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable))

IPPAPI(IppStatus, mfxiDCTQuantInv8x8LS_4x4_JPEG_16s8u_C1R,(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable))

IPPAPI(IppStatus, mfxiDCTQuantInv8x8LS_2x2_JPEG_16s8u_C1R,(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable))
IPPAPI(IppStatus, mfxiDCTQuantInv8x8LS_1x1_JPEG_16s8u_C1R,(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R
//    mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R
//
//  Purpose:
//    Inverse DCT transform, de-quantization and level shift and
//    DCT downsampling for 1/2 and 1/4 scale factor
//
//
//  Parameter:
//    pSrc           - pointer to input 8x8 block of quantized DCT coefficients
//    pDst           - pointer to output color component data
//    dstStep        - line offset for destination data
//    pQuantInvTable - pointer to Quantization table
//
//  Returns:
//    IppStatus
//
//  Notes:
//    DCT is defined in ISO/IEC 10918-1,
//    Appendix A - Requrements and guidelines, Annex A, A.3.3 FDCT and IDCT.
//    Quantization is defined in ISO/IEC 10918-1,
//    Appendix A - Requrements and guidelines,
//    Annex A, A.3.4 DCT coefficient quantization and dequantization.
//    Level Shift is defined in ISO/IEC 10918-1,
//    Appendix A - Requrements and guidelines, Annex A, A.3.1 - Level Shift.
*/

IPPAPI(IppStatus, mfxiDCTQuantInv8x8To4x4LS_JPEG_16s8u_C1R,(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable))

IPPAPI(IppStatus, mfxiDCTQuantInv8x8To2x2LS_JPEG_16s8u_C1R,(
  const Ipp16s* pSrc,
        Ipp8u*  pDst,
        int     dstStep,
  const Ipp16u* pQuantInvTable))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiDCTQuantInv8x8LS_JPEG_16s16u_C1R
//
//  Purpose:
//    Inverse DCT transform, de-quantization and level shift
//    for 12-bit data
//
//  Parameter:
//    pSrc           - pointer to input 8x8 block of quantized DCT coefficients
//    pDst           - pointer to output color component data
//    dstStep        - line offset for destination data
//    pQuantInvTable - pointer to Quantization table
//
//  Returns:
//    IppStatus
*/

IPPAPI(IppStatus, mfxiDCTQuantInv8x8LS_JPEG_16s16u_C1R, (
  const Ipp16s* pSrc,
        Ipp16u* pDst,
        int     dstStep,
  const Ipp32f* pQuantInvTable))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiReconstructPredRow_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc       - pointer to input row of differences
//    pPrevRow   - pointer to previous row of reconstructed samples
//    pDst       - pointer to output row of samples
//    width      - width, in elements
//    predictor  - predictor
//    Pt         - point transform parameter
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiReconstructPredRow_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
  const Ipp16s*  pPrevRow,
        Ipp16s*  pDst,
        int      width,
        int      predictor))

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiReconstructPredFirstRow_JPEG_16s_C1
//
//  Purpose:
//    undifference row
//
//  Parameters:
//    pSrc   - pointer to input row of differences
//    pDst   - pointer to output row of samples
//    width  - width, in elements
//    P      - precision of samples, in bits
//    Pt     - point transform parameter
//
//  Returns:
//    IppStatus
//
//  Notes:
//
*/

IPPAPI(IppStatus, mfxiReconstructPredFirstRow_JPEG_16s_C1, (
  const Ipp16s*  pSrc,
        Ipp16s*  pDst,
        int      width,
        int      P,
        int      Pt))

#ifdef __cplusplus
}
#endif

#if defined (_IPP_STDCALL_CDECL)
  #undef  _IPP_STDCALL_CDECL
  #define __stdcall __cdecl
#endif

#endif /* __IPPJ_H__ */
