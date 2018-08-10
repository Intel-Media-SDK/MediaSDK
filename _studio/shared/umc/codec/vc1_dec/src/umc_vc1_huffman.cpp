// Copyright (c) 2017-2018 Intel Corporation
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

#include "umc_vc1_huffman.h"

#include <cstdlib>
using namespace UMC;

#define VLC_FORBIDDEN 0xf0f1

static uint32_t bit_mask[33] =
{
    0x0,
    0x01,       0x03,       0x07,       0x0f,
    0x01f,      0x03f,      0x07f,      0x0ff,
    0x01ff,     0x03ff,     0x07ff,     0x0fff,
    0x01fff,    0x03fff,    0x07fff,    0x0ffff,
    0x01ffff,   0x03ffff,   0x07ffff,   0x0fffff,
    0x01fffff,  0x03fffff,  0x07fffff,  0x0ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
};

#define GetNBits(current_data, bit_ptr, nbits, pData,type)              \
{                                                                       \
    uint32_t x;                                                  \
                                                                        \
    bit_ptr -= nbits;                                                   \
                                                                        \
    if (bit_ptr < 0)                                                     \
    {                                                                   \
        bit_ptr += 32;                                                  \
                                                                        \
        x = (current_data)[1] >> (bit_ptr);                             \
        x >>= 1;                                                        \
        x += (current_data)[0] << (31 - bit_ptr);                       \
        (current_data)++;                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        x = (current_data)[0] >> (bit_ptr + 1);                         \
    }                                                                   \
                                                                        \
    pData = (type)(x & bit_mask[nbits]);                                \
}

#define UngetNBits(current_data, bit_ptr, nbits)                        \
{                                                                       \
    bit_ptr += nbits;                                                   \
    if (bit_ptr > 31)                                                    \
    {                                                                   \
        bit_ptr -= 32;                                                  \
        (current_data)--;                                               \
    }                                                                   \
}

static int32_t HuffmanTableSize(int32_t rl, const int32_t *pSrcTable)
{
    typedef struct
    {
        int32_t code;
        int32_t Nc;
    } CodeWord;

    CodeWord *Code;
    int32_t nWord, SubTblLength, CodeLength, n, i, j, s, index, dstTableSize;
    const int32_t *src;

    /* find number of code words */
    src = pSrcTable + pSrcTable[1] + 2;
    for (nWord = 0;;) {
        if ((n = *src++) < 0) break;
        src += n*(rl == 0 ? 2 : 3);
        nWord += n;
    }

    /* allocate temporary buffer */
    Code = (CodeWord *)malloc(nWord * sizeof(CodeWord));
    if (((CodeWord *)0) == Code)
        return 0;

    /* find destination table size */
    src = pSrcTable + pSrcTable[1] + 2;
    nWord = 0;
    dstTableSize = (1 << pSrcTable[2]) + 1;
    for (CodeLength = 1; ; CodeLength++) {
        /* process all code which length is CodeLength */
        if ((n = *src++) < 0) break;

        for (i = 0; i < n; i++) {
            SubTblLength = 0;
            for (s = 0; s < pSrcTable[1]; s++) {/* for all subtables */
                SubTblLength += pSrcTable[2 + s];
                if (CodeLength <= SubTblLength) break;
                /* find already processed code with the same prefix */
                index = *src >> (CodeLength - SubTblLength);
                for (j = 0; j<nWord; j++) {
                    if (Code[j].Nc <= SubTblLength) continue;
                    if ((Code[j].code >> (Code[j].Nc - SubTblLength)) == index) break;
                }
                if (j >= nWord) {
                    /* there is not code words with the same prefix, create new subtable */
                    dstTableSize += (1 << pSrcTable[2 + s + 1]) + 1;
                }
            }
            /* put word in array */
            Code[nWord].code = *src++;
            Code[nWord++].Nc = CodeLength;
            src += rl == 0 ? 1 : 2;
        }
    }

    free(Code);

    return dstTableSize;
}

static int HuffmanInitAlloc(int32_t rl, const int32_t* pSrcTable, int32_t** ppDstSpec)
{
    int32_t          i, k, l, j, commLen = 0;
    int32_t          nCodes, code, szblen, offset, shift, mask, value1, value2 = 0, size;

    int32_t          *szBuff;
    int32_t          *table;

    if (!pSrcTable || !ppDstSpec)
        return -1;

    size = HuffmanTableSize(rl, pSrcTable);
    if (0 == size)
        return -1;

    szblen = pSrcTable[1];
    offset = (1 << pSrcTable[2]) + 1;
    szBuff = (int32_t*)&pSrcTable[2];

    table = (int32_t*)malloc(size * sizeof(int32_t));
    if (0 == table)
        return -1;
    *ppDstSpec = table;

    for (i = 0; i < size; i++)
    {
        table[i] = (VLC_FORBIDDEN << 8) | 1;
    }
    table[0] = pSrcTable[2];

    for (i = 1, k = 2 + pSrcTable[1]; pSrcTable[k] >= 0; i++)
    {
        nCodes = pSrcTable[k++] * (rl == 0 ? 2 : 3);

        for (nCodes += k; k < nCodes; k += (rl == 0 ? 2 : 3))
        {
            commLen = 0;
            table = *ppDstSpec;
            for (l = 0; l < szblen; l++)
            {
                commLen += szBuff[l];
                if (commLen >= i)
                {
                    mask = ((1 << (i - commLen + szBuff[l])) - 1);
                    shift = (commLen - i);
                    code = (pSrcTable[k] & mask) << shift;
                    value1 = pSrcTable[k + 1];
                    if (rl == 1) value2 = pSrcTable[k + 2];

                    for (j = 0; j < (1 << (commLen - i)); j++)
                        if (rl == 0) table[code + j + 1] = (value1 << 8) | (commLen - i);
                        else      table[code + j + 1] = ((int16_t)value2 << 16) | ((uint8_t)value1 << 8) | (commLen - i);

                    break;
                }
                else
                {
                    mask = (1 << szBuff[l]) - 1;
                    shift = (i - commLen);
                    code = (pSrcTable[k] >> shift) & mask;

                    if (table[code + 1] != (long)((VLC_FORBIDDEN << 8) | 1))
                    {
                        if (((table[code + 1] & 0xff) == 0x80) && (rl == 1 || (table[code + 1] & 0xff00)))
                        {
                            table = (int32_t*)*ppDstSpec + (table[code + 1] >> 8);
                        }
                    }
                    else
                    {
                        table[code + 1] = (offset << 8) | 0x80;
                        table = (*ppDstSpec) + offset;
                        table[0] = szBuff[l + 1];
                        offset += (1 << szBuff[l + 1]) + 1;
                    }

                }
            }
        }
    }

    return 0;
}

int DecodeHuffmanOne(uint32_t**  pBitStream, int* pOffset,
    int32_t*  pDst, const int32_t* pDecodeTable)
{
    uint32_t table_bits, code_len;
    uint32_t  pos;
    int32_t  val;

    if (!pBitStream || !pOffset || !pDecodeTable || !*pBitStream || !pDst)
        return -1;

    table_bits = *pDecodeTable;
    GetNBits((*pBitStream), (*pOffset), table_bits, pos, uint32_t)

    val = pDecodeTable[pos + 1];
    code_len = val & 0xff;
    val = val >> 8;

    while (code_len & 0x80)
    {
        table_bits = pDecodeTable[val];
        GetNBits((*pBitStream), (*pOffset), table_bits, pos, uint32_t)

        val = pDecodeTable[pos + val + 1];
        code_len = val & 0xff;
        val = val >> 8;
    }

    if (val == VLC_FORBIDDEN)
    {
        *pDst = val;
        return -1;
    }

    UngetNBits(*pBitStream, *pOffset, code_len)

    *pDst = val;

    return 0;
}

int DecodeHuffmanPair(uint32_t **pBitStream, int32_t *pBitOffset, const int32_t *pTable,
    int8_t *pFirst, int16_t *pSecond)
{
    int32_t val;
    uint32_t table_bits, pos;
    uint8_t code_len;
    uint32_t *tmp_pbs = 0;
    int32_t tmp_offs = 0;

    /* check error(s) */
    if (!pBitStream || !pBitOffset || !pTable || !pFirst || !pSecond || !*pBitStream)
        return -1;

    tmp_pbs = *pBitStream;
    tmp_offs = *pBitOffset;
    table_bits = *pTable;
    GetNBits((*pBitStream), (*pBitOffset), table_bits, pos, uint32_t);
    val = pTable[pos + 1];
    code_len = (uint8_t)(val);

    while (code_len & 0x80)
    {
        val = val >> 8;
        table_bits = pTable[val];
        GetNBits((*pBitStream), (*pBitOffset), table_bits, pos, uint32_t);
        val = pTable[pos + val + 1];
        code_len = (uint8_t)(val);
    }

    UngetNBits((*pBitStream), (*pBitOffset), code_len);

    if ((val >> 8) == VLC_FORBIDDEN)
    {
        *pBitStream = tmp_pbs;
        *pBitOffset = tmp_offs;
        return -1;
    }

    *pFirst = (int8_t)((val >> 8) & 0xff);
    *pSecond = (int16_t)((val >> 16) & 0xffff);

    return 0;
}

int HuffmanTableInitAlloc(const int32_t* pSrcTable, int32_t** ppDstSpec)
{
    return HuffmanInitAlloc(0, pSrcTable, ppDstSpec);
}

int HuffmanRunLevelTableInitAlloc(const int32_t* pSrcTable, int32_t** ppDstSpec)
{
    return HuffmanInitAlloc(1, pSrcTable, ppDstSpec);
}

void HuffmanTableFree(int32_t *pDecodeTable)
{
    free((void*)pDecodeTable);
}


#endif // #if defined (MFX_ENABLE_VC1_VIDEO_DECODE)
