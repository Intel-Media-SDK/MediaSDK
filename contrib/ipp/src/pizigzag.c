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
//    reorder zigzag to natural
//
//  Contents:
//    IppStatus mfxiZigzagInv8x8_16s_C1(const Ipp16s pSrc, Ipp16s pDst)
//    IppStatus mfxiZigzagFwd8x8_16s_C1(const Ipp16s pSrc, Ipp16s pDst)
//
*/

#include "precomp.h"
#include "owni.h"

static int zigzag_index_Fwd [64] ={
   0,   1,  5,  6, 14, 15, 27, 28,
   2,   4,  7, 13, 16, 26, 29, 42,
   3,   8, 12, 17, 25, 30, 41, 43,
   9,  11, 18, 24, 31, 40, 44, 53,
   10, 19, 23, 32, 39, 45, 52, 54,
   20, 22, 33, 38, 46, 51, 55, 60,
   21, 34, 37, 47, 50, 56, 59, 61,
   35, 36, 48, 49, 57, 58, 62, 63 };

static int zigzag_index_Inv [64] = {
    0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63 };

/* ---------------------- library functions definitions -------------------- */

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiZigzagInv8x8_16s_C1
//
//  Purpose:
//    reorder natural to zigzag 8x8 block
//
//  Parameter:
//    pSrc - pointer to source block
//    pDst - pointer to destination block
//
//  Returns:
//    IppStatus
//
//
*/

#if !(( _IPP >= _IPP_H9 ) || ( _IPP32E >= _IPP32E_L9 ))
IPPFUN(IppStatus, mfxiZigzagInv8x8_16s_C1,(const Ipp16s* pSrc, Ipp16s* pDst)) {

  int i;

  IPP_BAD_PTR2_RET(pSrc,pDst);

  for(i = 0; i < 64; i++)
    pDst[i] = pSrc[ zigzag_index_Fwd[i] ];

  return ippStsNoErr;
} /* mfxiZigzagInv8x8_16s_C1() */
#endif

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    mfxiZigzagFwd8x8_16s_C1
//
//  Purpose:
//    reorder zigzag to natural 8x8 block
//
//  Parameter:
//    pSrc - pointer to source block
//    pDst - pointer to destination block
//
//  Returns:
//    IppStatus
//
//
*/

IPPFUN(IppStatus, mfxiZigzagFwd8x8_16s_C1, (const Ipp16s* pSrc, Ipp16s* pDst)){

   int i;

   IPP_BAD_PTR2_RET(pSrc,pDst)

   for (i = 0; i < 64; i++)
       pDst[i] = pSrc[ zigzag_index_Inv[i] ];

   return ippStsNoErr;
} /* mfxiZigzagFwd8x8_16s_C1() */

