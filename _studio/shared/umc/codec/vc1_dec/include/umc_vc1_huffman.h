// Copyright (c) 2017 Intel Corporation
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

#include "umc_defs.h"

#if defined (MFX_ENABLE_VC1_VIDEO_DECODE)

#ifndef __UMC_VC1_HUFFMAN_H__
#define __UMC_VC1_HUFFMAN_H__

// return value 0 means ok, != 0 - failed.

int DecodeHuffmanOne(uint32_t**  pBitStream, int* pOffset,
    int32_t*  pDst, const int32_t* pDecodeTable);

int DecodeHuffmanPair(uint32_t **pBitStream, int32_t *pBitOffset,
    const int32_t *pTable, int8_t *pFirst, int16_t *pSecond);

int HuffmanTableInitAlloc(const int32_t* pSrcTable, int32_t** ppDstSpec);
int HuffmanRunLevelTableInitAlloc(const int32_t* pSrcTable, int32_t** ppDstSpec);
void HuffmanTableFree(int32_t *pDecodeTable);

#endif // __UMC_VC1_HUFFMAN_H__
#endif // #if defined (MFX_ENABLE_VC1_VIDEO_DECODE)
