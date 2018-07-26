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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "mfx_h265_encode_hw_bs.h"
#include <assert.h>
#include <list>
#include <algorithm>

namespace MfxHwH265Encode
{

const mfxU8 tab_cabacRangeTabLps[128][4] =
{
    { 128, 176, 208, 240 },
    { 128, 167, 197, 227 },
    { 128, 158, 187, 216 },
    { 123, 150, 178, 205 },
    { 116, 142, 169, 195 },
    { 111, 135, 160, 185 },
    { 105, 128, 152, 175 },
    { 100, 122, 144, 166 },
    { 95, 116, 137, 158 },
    { 90, 110, 130, 150 },
    { 85, 104, 123, 142 },
    { 81, 99, 117, 135 },
    { 77, 94, 111, 128 },
    { 73, 89, 105, 122 },
    { 69, 85, 100, 116 },
    { 66, 80, 95, 110 },
    { 62, 76, 90, 104 },
    { 59, 72, 86, 99 },
    { 56, 69, 81, 94 },
    { 53, 65, 77, 89 },
    { 51, 62, 73, 85 },
    { 48, 59, 69, 80 },
    { 46, 56, 66, 76 },
    { 43, 53, 63, 72 },
    { 41, 50, 59, 69 },
    { 39, 48, 56, 65 },
    { 37, 45, 54, 62 },
    { 35, 43, 51, 59 },
    { 33, 41, 48, 56 },
    { 32, 39, 46, 53 },
    { 30, 37, 43, 50 },
    { 29, 35, 41, 48 },
    { 27, 33, 39, 45 },
    { 26, 31, 37, 43 },
    { 24, 30, 35, 41 },
    { 23, 28, 33, 39 },
    { 22, 27, 32, 37 },
    { 21, 26, 30, 35 },
    { 20, 24, 29, 33 },
    { 19, 23, 27, 31 },
    { 18, 22, 26, 30 },
    { 17, 21, 25, 28 },
    { 16, 20, 23, 27 },
    { 15, 19, 22, 25 },
    { 14, 18, 21, 24 },
    { 14, 17, 20, 23 },
    { 13, 16, 19, 22 },
    { 12, 15, 18, 21 },
    { 12, 14, 17, 20 },
    { 11, 14, 16, 19 },
    { 11, 13, 15, 18 },
    { 10, 12, 15, 17 },
    { 10, 12, 14, 16 },
    { 9, 11, 13, 15 },
    { 9, 11, 12, 14 },
    { 8, 10, 12, 14 },
    { 8, 9, 11, 13 },
    { 7, 9, 11, 12 },
    { 7, 9, 10, 12 },
    { 7, 8, 10, 11 },
    { 6, 8, 9, 11 },
    { 6, 7, 9, 10 },
    { 6, 7, 8, 9 },
    { 2, 2, 2, 2 },
    //The same for valMPS=1
    { 128, 176, 208, 240 },
    { 128, 167, 197, 227 },
    { 128, 158, 187, 216 },
    { 123, 150, 178, 205 },
    { 116, 142, 169, 195 },
    { 111, 135, 160, 185 },
    { 105, 128, 152, 175 },
    { 100, 122, 144, 166 },
    { 95, 116, 137, 158 },
    { 90, 110, 130, 150 },
    { 85, 104, 123, 142 },
    { 81, 99, 117, 135 },
    { 77, 94, 111, 128 },
    { 73, 89, 105, 122 },
    { 69, 85, 100, 116 },
    { 66, 80, 95, 110 },
    { 62, 76, 90, 104 },
    { 59, 72, 86, 99 },
    { 56, 69, 81, 94 },
    { 53, 65, 77, 89 },
    { 51, 62, 73, 85 },
    { 48, 59, 69, 80 },
    { 46, 56, 66, 76 },
    { 43, 53, 63, 72 },
    { 41, 50, 59, 69 },
    { 39, 48, 56, 65 },
    { 37, 45, 54, 62 },
    { 35, 43, 51, 59 },
    { 33, 41, 48, 56 },
    { 32, 39, 46, 53 },
    { 30, 37, 43, 50 },
    { 29, 35, 41, 48 },
    { 27, 33, 39, 45 },
    { 26, 31, 37, 43 },
    { 24, 30, 35, 41 },
    { 23, 28, 33, 39 },
    { 22, 27, 32, 37 },
    { 21, 26, 30, 35 },
    { 20, 24, 29, 33 },
    { 19, 23, 27, 31 },
    { 18, 22, 26, 30 },
    { 17, 21, 25, 28 },
    { 16, 20, 23, 27 },
    { 15, 19, 22, 25 },
    { 14, 18, 21, 24 },
    { 14, 17, 20, 23 },
    { 13, 16, 19, 22 },
    { 12, 15, 18, 21 },
    { 12, 14, 17, 20 },
    { 11, 14, 16, 19 },
    { 11, 13, 15, 18 },
    { 10, 12, 15, 17 },
    { 10, 12, 14, 16 },
    { 9, 11, 13, 15 },
    { 9, 11, 12, 14 },
    { 8, 10, 12, 14 },
    { 8, 9, 11, 13 },
    { 7, 9, 11, 12 },
    { 7, 9, 10, 12 },
    { 7, 8, 10, 11 },
    { 6, 8, 9, 11 },
    { 6, 7, 9, 10 },
    { 6, 7, 8, 9 },
    { 2, 2, 2, 2 }
};
/* CABAC trans tables: state (MPS and LPS ) + valMPS in 6th bit */
const mfxU8 tab_cabacTransTbl[2][128] = {
    { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
    27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
    39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
    62, 63, 0, 64, 65, 66, 66, 68, 68, 69, 70, 71,
    72, 73, 73, 75, 75, 76, 77, 77, 79, 79, 80, 80,
    82, 82, 83, 83, 85, 85, 86, 86, 87, 88, 88, 89,
    90, 90, 91, 91, 92, 93, 93, 94, 94, 94, 95, 96,
    96, 97, 97, 97, 98, 98, 99, 99, 99, 100, 100,
    100, 101, 101, 101, 102, 102, 127
    },
    { 64, 0, 1, 2, 2, 4, 4, 5, 6, 7, 8, 9, 9, 11, 11,
    12, 13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21,
    21, 22, 22, 23, 24, 24, 25, 26, 26, 27, 27, 28,
    29, 29, 30, 30, 30, 31, 32, 32, 33, 33, 33, 34,
    34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38,
    63, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
    76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
    88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 126, 127
    }
};

const
    mfxU32 tab_ctxIdxOffset[MAIN_SYNTAX_ELEMENT_NUMBER_HEVC] =
{
    0,      // QT_CBF_HEVC
    10,     // QT_ROOT_CBF_HEVC
    11,     // LAST_X_HEVC
    41,     // LAST_Y_HEVC
    71,     // SIG_COEFF_GROUP_FLAG_HEVC
    75,     // SIG_FLAG_HEVC
    117,    // ONE_FLAG_HEVC
    141,    // ABS_FLAG_HEVC
    147,    // TRANS_SUBDIV_FLAG_HEVC
    150,    // TRANSFORMSKIP_FLAG_HEVC
    152,    // CU_TRANSQUANT_BYPASS_FLAG_CTX
    153,    // SPLIT_CODING_UNIT_FLAG_HEVC
    156,    // SKIP_FLAG_HEVC
    159,    // MERGE_FLAG_HEVC
    160,    // MERGE_IDX_HEVC
    161,    // PART_SIZE_HEVC
    165,    // AMP_SPLIT_POSITION_HEVC
    166,    // PRED_MODE_HEVC
    167,    // INTRA_LUMA_PRED_MODE_HEVC
    168,    // INTRA_CHROMA_PRED_MODE_HEVC
    170,    // INTER_DIR_HEVC
    175,    // MVD_HEVC
    177,    // REF_FRAME_IDX_HEVC
    179,    // DQP_HEVC
    183,    // MVP_IDX_HEVC
    185,    // SAO_MERGE_FLAG_HEVC
    186,    // SAO_TYPE_IDX_HEVC
    187,    // END_OF_SLICE_FLAG_HEVC
};

const mfxU32 tab_ctxIdxSize[MAIN_SYNTAX_ELEMENT_NUMBER_HEVC] =
{
    10,     // QT_CBF_HEVC
    1,      // QT_ROOT_CBF_HEVC
    30,     // LAST_X_HEVC
    30,     // LAST_Y_HEVC
    4,      // SIG_COEFF_GROUP_FLAG_HEVC
    42,     // SIG_FLAG_HEVC
    24,     // ONE_FLAG_HEVC
    6,      // ABS_FLAG_HEVC
    3,      // TRANS_SUBDIV_FLAG_HEVC
    2,      // TRANSFORMSKIP_FLAG_HEVC
    1,      // CU_TRANSQUANT_BYPASS_FLAG_CTX
    3,      // SPLIT_CODING_UNIT_FLAG_HEVC
    3,      // SKIP_FLAG_HEVC
    1,      // MERGE_FLAG_HEVC
    1,      // MERGE_IDX_HEVC
    4,      // PART_SIZE_HEVC
    1,      // AMP_SPLIT_POSITION_HEVC
    1,      // PRED_MODE_HEVC
    1,      // INTRA_LUMA_PRED_MODE_HEVC
    2,      // INTRA_CHROMA_PRED_MODE_HEVC
    5,      // INTER_DIR_HEVC
    2,      // MVD_HEVC
    2,      // REF_FRAME_IDX_HEVC
    4,      // DQP_HEVC
    2,      // MVP_IDX_HEVC
    1,      // SAO_MERGE_FLAG_HEVC
    1,      // SAO_TYPE_IDX_HEVC
    1,      // END_OF_SLICE_FLAG_HEVC
};

const
    mfxU8 initVal_for_transquant_bypass_flag[3][1] =
{
    { 154 },
    { 154 },
    { 154 }
};

//split_coding_unit_flag   INIT_SPLIT_FLAG
const
    mfxU8 initVal_for_split_coding_unit_flag[3][3] =
{
    { 139, 141, 157, },
    { 107, 139, 126, },
    { 107, 139, 126, }
};

//skip_flag  INIT_SKIP_FLAG
const
    mfxU8 initVal_for_skip_flag[3][3] =
{
    { 154, 154, 154, },
    { 197, 185, 201, },
    { 197, 185, 201, }
};

const
    mfxU8 initVal_for_merge_idx[3][1] =
{
    { 154, },
    { 122, },
    { 137, }
};


static
    void InitializeContext(mfxU8 *pContext, mfxU8 initVal, mfxI32 SliceQPy)
{
    SliceQPy = MFX_MIN(MFX_MAX(0, SliceQPy), 51);

    mfxI32 slope = (initVal >> 4) * 5 - 45;
    mfxI32 offset = ((initVal & 15) << 3) - 16;
    mfxI32 initState = MFX_MIN(MFX_MAX(1, (((slope * SliceQPy) >> 4) + offset)), 126);
    mfxU32 mpState = (initState >= 64);
    *pContext = mfxU8(((mpState ? (initState - 64) : (63 - initState)) << 0) + (mpState << 6));
} //void InitializeContext(CABAC_CONTEXT_H265 *pContext, mfxU8 initVal, mfxI32 SliceQPy)


mfxU32 AddEmulationPreventionAndCopy(
    mfxU8* data,
    mfxU32 length,
    mfxU8*                           bsDataStart,
    mfxU8*                           bsEnd,
    bool                             bEmulationByteInsertion)
{
    mfxU8 * sbegin = data;
    mfxU32  len = length;
    assert(mfxU32(bsEnd - bsDataStart) > length);

    if (!bEmulationByteInsertion)
    {
        memcpy_s(bsDataStart, len, sbegin, len);
        return len;
    }

    mfxU32 size = mfxU32(bsEnd - bsDataStart);
    HeaderPacker::PackRBSP(bsDataStart, sbegin, size, length);
    len = size;
    return len;
}


BitstreamWriter::BitstreamWriter(mfxU8* bs, mfxU32 size, mfxU8 bitOffset)
    : m_bsStart(bs)
    , m_bsEnd(bs + size)
    , m_bs(bs)
    , m_bitStart(bitOffset & 7)
    , m_bitOffset(bitOffset & 7)
    , m_codILow(0)// cabac variables
    , m_codIRange(510)
    , m_bitsOutstanding(0)
    , m_BinCountsInNALunits(0)
    , m_firstBitFlag(true)
{
    assert(bitOffset < 8);
    *m_bs &= 0xFF << (8 - m_bitOffset);
}

BitstreamWriter::~BitstreamWriter()
{
}

void BitstreamWriter::Reset(mfxU8* bs, mfxU32 size, mfxU8 bitOffset)
{
    if (bs)
    {
        m_bsStart   = bs;
        m_bsEnd     = bs + size;
        m_bs        = bs;
        m_bitOffset = (bitOffset & 7);
        m_bitStart  = (bitOffset & 7);
    }
    else
    {
        m_bs        = m_bsStart;
        m_bitOffset = m_bitStart;
    }
}

void BitstreamWriter::PutBitsBuffer(mfxU32 n, void* bb, mfxU32 o)
{
    mfxU8* b = (mfxU8*)bb;
    mfxU32 N, B;

    assert(bb);

    if (o)
    {
        N  = o / 8;
        b += N;
        o &= 7;

        if (o)
        {
            N = (n < (8 - o)) ? n : 0;

            PutBits(8 - o, ((b[0] & (0xff >> o)) >> N));

            n -= (N ? N : (8 - o));
            b++;

            if (!n)
                return;
        }
    }

    if (!m_bitOffset)
    {
        N = n / 8;
        n &= 7;

        assert(N + !!n < (m_bsEnd - m_bs));
        memcpy_s(m_bs, N, b, N);

        m_bs += N;

        if (n)
        {
            m_bs[0] = b[N];
            m_bs[0] &= (0xff << (8 - n));
            m_bitOffset = (mfxU8)n;
        }
    }
    else
    {
        assert( (n + 7 - m_bitOffset) / 8 < (m_bsEnd - m_bs));

        while (n >= 24)
        {
            B = ((((mfxU32)b[0] << 24) | ((mfxU32)b[1] << 16) | ((mfxU32)b[2] << 8)) >> m_bitOffset);

            m_bs[0] |= (mfxU8)(B >> 24);
            m_bs[1]  = (mfxU8)(B >> 16);
            m_bs[2]  = (mfxU8)(B >> 8);
            m_bs[3]  = (mfxU8) B;

            m_bs += 3;
            b    += 3;
            n    -= 24;
        }

        while (n >= 8)
        {
            B = ((mfxU32)b[0] << 8) >> m_bitOffset;

            m_bs[0] |= (mfxU8)(B >> 8);
            m_bs[1]  = (mfxU8) B;

            m_bs++;
            b++;
            n -= 8;
        }

        if (n)
            PutBits(n, (b[0] >> (8 - n)));
    }
}

void BitstreamWriter::PutBits(mfxU32 n, mfxU32 b)
{
    assert(n <= sizeof(b) * 8);
    while (n > 24)
    {
        n -= 16;
        PutBits(16, (b >> n));
    }

    b <<= (32 - n);

    if (!m_bitOffset)
    {
        m_bs[0] = (mfxU8)(b >> 24);
        m_bs[1] = (mfxU8)(b >> 16);
    }
    else
    {
        b >>= m_bitOffset;
        n  += m_bitOffset;

        m_bs[0] |= (mfxU8)(b >> 24);
        m_bs[1]  = (mfxU8)(b >> 16);
    }

    if (n > 16)
    {
        m_bs[2] = (mfxU8)(b >> 8);
        m_bs[3] = (mfxU8)b;
    }

    m_bs += (n >> 3);
    m_bitOffset = (n & 7);
}

void BitstreamWriter::PutBit(mfxU32 b)
{
    switch(m_bitOffset)
    {
    case 0:
        m_bs[0] = (mfxU8)(b << 7);
        m_bitOffset = 1;
        break;
    case 7:
        m_bs[0] |= (mfxU8)(b & 1);
        m_bs ++;
        m_bitOffset = 0;
        break;
    default:
        if (b & 1)
            m_bs[0] |= (mfxU8)(1 << (7 - m_bitOffset));
        m_bitOffset ++;
        break;
    }
}

void BitstreamWriter::PutGolomb(mfxU32 b)
{
    if (!b)
    {
        PutBit(1);
    }
    else
    {
        mfxU32 n = 1;

        b ++;

        while (b >> n)
            n ++;

        PutBits(n - 1, 0);
        PutBits(n, b);
    }
}

void BitstreamWriter::PutTrailingBits(bool bCheckAligened)
{
    if ((!bCheckAligened) || m_bitOffset)
        PutBit(1);

    if (m_bitOffset)
    {
        *(++m_bs)   = 0;
        m_bitOffset = 0;
    }
}

void BitstreamWriter::PutBitC(mfxU32 B)
{
    if (m_firstBitFlag)
        m_firstBitFlag = false;
    else
        PutBit(B);

    while (m_bitsOutstanding > 0)
    {
        PutBit(1 - B);
        m_bitsOutstanding--;
    }
}
void BitstreamWriter::RenormE()
{
    while (m_codIRange < 256)
    {
        if (m_codILow < 256)
        {
            PutBitC(0);
        }
        else if (m_codILow >= 512)
        {
            m_codILow -= 512;
            PutBitC(1);
        }
        else
        {
            m_codILow -= 256;
            m_bitsOutstanding++;
        }
        m_codIRange <<= 1;
        m_codILow <<= 1;
    }
}

void BitstreamWriter::EncodeBin(mfxU8 * ctx, mfxU8 binVal)
{
    mfxU8  pStateIdx = (*ctx) & 0x3F;
    mfxU8  valMPS = ((*ctx) >> 6);
    mfxU32 qCodIRangeIdx = (m_codIRange >> 6) & 3;
    mfxU32 codIRangeLPS = tab_cabacRangeTabLps[pStateIdx][qCodIRangeIdx];

    m_codIRange -= codIRangeLPS;

    if (binVal != valMPS)
    {
        m_codILow += m_codIRange;
        m_codIRange = codIRangeLPS;

        if (pStateIdx == 0)
            valMPS = 1 - valMPS;

        pStateIdx = tab_cabacTransTbl[1][pStateIdx];//transIdxLPS[pStateIdx];
    }
    else
    {
        pStateIdx = tab_cabacTransTbl[0][pStateIdx];//transIdxMPS[pStateIdx];
    }
    *ctx = ((valMPS << 6) | pStateIdx);
    //*ctx = tab_cabacTransTbl[binVal][pStateIdx];

    RenormE();
    m_BinCountsInNALunits++;
}

void BitstreamWriter::EncodeBinEP(mfxU8 binVal)
{
    if (binVal == 1) {
        m_codILow += m_codILow + m_codIRange;
    }
    else {
        m_codILow += m_codILow;
    }
    RenormE();
    m_BinCountsInNALunits++;
}

void BitstreamWriter::SliceFinish()
{
    m_codIRange -= 2;
    m_codILow += m_codIRange;
    m_codIRange = 2;

    RenormE();
    PutBitC((m_codILow >> 9) & 1);
    PutBit(m_codILow >> 8);
    PutTrailingBits();

    m_BinCountsInNALunits++;
}

void BitstreamWriter::cabacInit()
{
    m_codILow = 0;
    m_codIRange = 510;
    m_bitsOutstanding = 0;
    m_BinCountsInNALunits = 0;
    m_firstBitFlag = true;
}

BitstreamReader::BitstreamReader(mfxU8* bs, mfxU32 size, mfxU8 bitOffset)
: m_bsStart(bs)
, m_bsEnd(bs + size)
, m_bs(bs)
, m_bitStart(bitOffset & 7)
, m_bitOffset(bitOffset & 7)
, m_emulation(true)
{
}

BitstreamReader::~BitstreamReader()
{
}

void BitstreamReader::Reset(mfxU8* bs, mfxU32 size, mfxU8 bitOffset)
{
    if (bs)
    {
        m_bsStart = bs;
        m_bsEnd = bs + size;
        m_bs = bs;
        m_bitOffset = (bitOffset & 7);
        m_bitStart = (bitOffset & 7);
    }
    else
    {
        m_bs = m_bsStart;
        m_bitOffset = m_bitStart;
    }
}

mfxU32 BitstreamReader::GetBit()
{
    if (m_bs >= m_bsEnd)
    {
        assert(!"end of buffer");
        throw EndOfBuffer();
    }

    mfxU32 b = (*m_bs >> (7 - m_bitOffset)) & 1;

    if (++m_bitOffset == 8)
    {
        ++m_bs;
        m_bitOffset = 0;

        if (   m_emulation
            && m_bs - m_bsStart >= 2
            && (m_bsEnd - m_bs) >= 1
            && *m_bs                == 0x03
            && *(m_bs - 1)          == 0x00
            && *(m_bs - 2)          == 0x00
            && (*(m_bs + 1) & 0xfc) == 0x00)
        {
            ++m_bs;
        }
    }

    return b;
}

mfxU32 BitstreamReader::GetBits(mfxU32 n)
{
    mfxU32 b = 0;

    while (n--)
        b = (b << 1) | GetBit();

    return b;
}

mfxU32 BitstreamReader::GetUE()
{
    mfxU32 lz = 0;

    while (!GetBit())
        lz++;

    return !lz ? 0 : ((1 << lz) | GetBits(lz)) - 1;
}

mfxI32 BitstreamReader::GetSE()
{
    mfxU32 ue = GetUE();
    return (ue & 1) ? (mfxI32)((ue + 1) >> 1) : -(mfxI32)((ue + 1) >> 1);
}

mfxStatus HeaderReader::ReadNALU(BitstreamReader& bs, NALU & nalu)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool emulation = bs.GetEmulation();

    bs.SetEmulation(false);

    try
    {
        mfxU32 start_code = bs.GetBits(24);
        mfxU32 n = 3;

        while ((start_code & 0x00FFFFFF) != 1)
        {
            start_code <<= 8;
            start_code |= bs.GetBits(8);
            n++;
        }

        if (bs.GetBit())
            return MFX_ERR_INVALID_VIDEO_PARAM;

        nalu.long_start_code = (n > 3) && !(start_code >> 24);
        nalu.nal_unit_type = bs.GetBits(6);
        nalu.nuh_layer_id = bs.GetBits(6);
        nalu.nuh_temporal_id_plus1 = bs.GetBits(3);
    }
    catch (std::exception &)
    {
        sts = MFX_ERR_INVALID_VIDEO_PARAM;
    }

    bs.SetEmulation(emulation);

    return sts;
}

mfxStatus HeaderReader::ReadSPS(BitstreamReader& bs, SPS & sps)
{
    mfxStatus sts = MFX_ERR_NONE;
    NALU nalu = {};

    Zero(sps);

    do
    {
        sts = ReadNALU(bs, nalu);
        MFX_CHECK_STS(sts);
    } while (nalu.nal_unit_type != SPS_NUT);

    try
    {
        sps.video_parameter_set_id = bs.GetBits(4);
        sps.max_sub_layers_minus1 = bs.GetBits(3);
        sps.temporal_id_nesting_flag = bs.GetBit();

        sps.general.profile_space = bs.GetBits(2);
        sps.general.tier_flag = bs.GetBit();
        sps.general.profile_idc = bs.GetBits(5);
        sps.general.profile_compatibility_flags = bs.GetBits(32);
        sps.general.progressive_source_flag = bs.GetBit();
        sps.general.interlaced_source_flag = bs.GetBit();
        sps.general.non_packed_constraint_flag = bs.GetBit();
        sps.general.frame_only_constraint_flag = bs.GetBit();

        //general_reserved_zero_44bits
        if (bs.GetBits(24))
            return MFX_ERR_UNSUPPORTED;
        if (bs.GetBits(20))
            return MFX_ERR_UNSUPPORTED;

        sps.general.level_idc = (mfxU8)bs.GetBits(8);

        for (mfxU32 i = 0; i < sps.max_sub_layers_minus1; i++)
        {
            sps.sub_layer[i].profile_present_flag = bs.GetBit();
            sps.sub_layer[i].level_present_flag = bs.GetBit();
        }

        if (sps.max_sub_layers_minus1 && bs.GetBits(2 * 8)) // reserved_zero_2bits[ i ]
            return MFX_ERR_INVALID_VIDEO_PARAM;

        for (mfxU32 i = 0; i < sps.max_sub_layers_minus1; i++)
        {
            if (sps.sub_layer[i].profile_present_flag)
            {
                sps.sub_layer[i].profile_space = bs.GetBits(2);
                sps.sub_layer[i].tier_flag = bs.GetBit();
                sps.sub_layer[i].profile_idc = bs.GetBits(5);
                sps.sub_layer[i].profile_compatibility_flags = bs.GetBits(32);
                sps.sub_layer[i].progressive_source_flag = bs.GetBit();
                sps.sub_layer[i].interlaced_source_flag = bs.GetBit();
                sps.sub_layer[i].non_packed_constraint_flag = bs.GetBit();
                sps.sub_layer[i].frame_only_constraint_flag = bs.GetBit();

                //general_reserved_zero_44bits
                if (bs.GetBits(24))
                    return MFX_ERR_UNSUPPORTED;
                if (bs.GetBits(20))
                    return MFX_ERR_UNSUPPORTED;
            }

            if (sps.sub_layer[i].level_present_flag)
                sps.sub_layer[i].level_idc = (mfxU8)bs.GetBits(8);
        }

        sps.seq_parameter_set_id = bs.GetUE();
        sps.chroma_format_idc = bs.GetUE();

        if (sps.chroma_format_idc == 3)
            sps.separate_colour_plane_flag = bs.GetBit();

        sps.pic_width_in_luma_samples = bs.GetUE();
        sps.pic_height_in_luma_samples = bs.GetUE();

        sps.conformance_window_flag = bs.GetBit();

        if (sps.conformance_window_flag)
        {
            sps.conf_win_left_offset = bs.GetUE();
            sps.conf_win_right_offset = bs.GetUE();
            sps.conf_win_top_offset = bs.GetUE();
            sps.conf_win_bottom_offset = bs.GetUE();
        }

        sps.bit_depth_luma_minus8 = bs.GetUE();
        sps.bit_depth_chroma_minus8 = bs.GetUE();
        sps.log2_max_pic_order_cnt_lsb_minus4 = bs.GetUE();


        sps.sub_layer_ordering_info_present_flag = bs.GetBit();

        for (mfxU32 i = (sps.sub_layer_ordering_info_present_flag ? 0 : sps.max_sub_layers_minus1);
            i <= sps.max_sub_layers_minus1; i++)
        {
            sps.sub_layer[i].max_dec_pic_buffering_minus1 = bs.GetUE();
            sps.sub_layer[i].max_num_reorder_pics = bs.GetUE();
            sps.sub_layer[i].max_latency_increase_plus1 = bs.GetUE();
        }

        sps.log2_min_luma_coding_block_size_minus3 = bs.GetUE();
        sps.log2_diff_max_min_luma_coding_block_size = bs.GetUE();
        sps.log2_min_transform_block_size_minus2 = bs.GetUE();
        sps.log2_diff_max_min_transform_block_size = bs.GetUE();
        sps.max_transform_hierarchy_depth_inter = bs.GetUE();
        sps.max_transform_hierarchy_depth_intra = bs.GetUE();
        sps.scaling_list_enabled_flag = bs.GetBit();

        if (sps.scaling_list_enabled_flag)
            return MFX_ERR_UNSUPPORTED;

        sps.amp_enabled_flag = bs.GetBit();
        sps.sample_adaptive_offset_enabled_flag = bs.GetBit();
        sps.pcm_enabled_flag = bs.GetBit();

        if (sps.pcm_enabled_flag)
        {
            sps.pcm_sample_bit_depth_luma_minus1 = bs.GetBits(4);
            sps.pcm_sample_bit_depth_chroma_minus1 = bs.GetBits(4);
            sps.log2_min_pcm_luma_coding_block_size_minus3 = bs.GetUE();
            sps.log2_diff_max_min_pcm_luma_coding_block_size = bs.GetUE();
            sps.pcm_loop_filter_disabled_flag = bs.GetBit();
        }

        sps.num_short_term_ref_pic_sets = (mfxU8)bs.GetUE();

        for (mfxU32 idx = 0; idx < sps.num_short_term_ref_pic_sets; idx++)
        {
            STRPS & strps = sps.strps[idx];

            if (idx != 0)
                strps.inter_ref_pic_set_prediction_flag = bs.GetBit();

            if (strps.inter_ref_pic_set_prediction_flag)
            {
                mfxI16 i = 0, j = 0, dPoc = 0;

                strps.delta_rps_sign = bs.GetBit();
                strps.abs_delta_rps_minus1 = (mfxU16)bs.GetUE();

                mfxU32 RefRpsIdx = idx - (strps.delta_idx_minus1 + 1);
                STRPS& ref = sps.strps[RefRpsIdx];
                mfxI16 NumDeltaPocs = ref.num_negative_pics + ref.num_positive_pics;
                mfxI16 deltaRps = (1 - 2 * strps.delta_rps_sign) * (strps.abs_delta_rps_minus1 + 1);

                for (j = 0; j <= NumDeltaPocs; j++)
                {
                    strps.pic[j].use_delta_flag = 1;

                    strps.pic[j].used_by_curr_pic_flag = bs.GetBit();

                    if (!strps.pic[j].used_by_curr_pic_flag)
                        strps.pic[j].use_delta_flag = bs.GetBit();
                }

                i = 0;
                for (j = ref.num_positive_pics - 1; j >= 0; j--)
                {
                    dPoc = ref.pic[j].DeltaPocSX + deltaRps;

                    if (dPoc < 0 && strps.pic[ref.num_negative_pics + j].use_delta_flag)
                    {
                        strps.pic[i].DeltaPocSX = dPoc;
                        strps.pic[i].used_by_curr_pic_sx_flag = strps.pic[ref.num_negative_pics + j].used_by_curr_pic_flag;
                        i++;
                    }
                }

                if (deltaRps < 0 && strps.pic[NumDeltaPocs].use_delta_flag)
                {
                    strps.pic[i].DeltaPocSX = deltaRps;
                    strps.pic[i].used_by_curr_pic_sx_flag = strps.pic[NumDeltaPocs].used_by_curr_pic_flag;
                    i++;
                }

                for (j = 0; j < ref.num_negative_pics; j++)
                {
                    dPoc = ref.pic[j].DeltaPocSX + deltaRps;

                    if (dPoc < 0 && strps.pic[j].use_delta_flag)
                    {
                        strps.pic[i].DeltaPocSX = dPoc;
                        strps.pic[i].used_by_curr_pic_sx_flag = strps.pic[j].used_by_curr_pic_flag;
                        i++;
                    }
                }

                strps.num_negative_pics = i;

                i = 0;
                for (j = ref.num_negative_pics - 1; j >= 0; j--)
                {
                    dPoc = ref.pic[j].DeltaPocSX + deltaRps;

                    if (dPoc > 0 && strps.pic[j].use_delta_flag)
                    {
                        strps.pic[i].DeltaPocSX = dPoc;
                        strps.pic[i].used_by_curr_pic_sx_flag = strps.pic[j].used_by_curr_pic_flag;
                        i++;
                    }
                }

                if (deltaRps > 0 && strps.pic[NumDeltaPocs].use_delta_flag)
                {
                    strps.pic[i].DeltaPocSX = deltaRps;
                    strps.pic[i].used_by_curr_pic_sx_flag = strps.pic[NumDeltaPocs].used_by_curr_pic_flag;
                    i++;
                }

                for (j = 0; j < ref.num_positive_pics; j++)
                {
                    dPoc = ref.pic[j].DeltaPocSX + deltaRps;

                    if (dPoc > 0 && strps.pic[j].use_delta_flag)
                    {
                        strps.pic[i].DeltaPocSX = dPoc;
                        strps.pic[i].used_by_curr_pic_sx_flag = strps.pic[j].used_by_curr_pic_flag;
                        i++;
                    }
                }

                strps.num_positive_pics = i;
            }
            else
            {
                strps.num_negative_pics = bs.GetUE();
                strps.num_positive_pics = bs.GetUE();

                for (mfxU32 i = 0; i < strps.num_negative_pics; i++)
                {
                    strps.pic[i].delta_poc_s0_minus1 = bs.GetUE();
                    strps.pic[i].used_by_curr_pic_s0_flag = bs.GetBit();
                    strps.pic[i].DeltaPocSX = -(mfxI16)(strps.pic[i].delta_poc_sx_minus1 + 1);
                }

                for (mfxU32 i = strps.num_negative_pics; i < mfxU32(strps.num_positive_pics + strps.num_negative_pics); i++)
                {
                    strps.pic[i].delta_poc_s1_minus1 = bs.GetUE();
                    strps.pic[i].used_by_curr_pic_s1_flag = bs.GetBit();
                    strps.pic[i].DeltaPocSX = (mfxI16)(strps.pic[i].delta_poc_sx_minus1 + 1);
                }
            }
        }

        sps.long_term_ref_pics_present_flag = bs.GetBit();

        if (sps.long_term_ref_pics_present_flag)
        {
            sps.num_long_term_ref_pics_sps = bs.GetUE();

            for (mfxU32 i = 0; i < sps.num_long_term_ref_pics_sps; i++)
            {
                sps.lt_ref_pic_poc_lsb_sps[i] = (mfxU16)bs.GetBits(sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
                sps.used_by_curr_pic_lt_sps_flag[i] = (mfxU8)bs.GetBit();
            }
        }

        sps.temporal_mvp_enabled_flag = bs.GetBit();
        sps.strong_intra_smoothing_enabled_flag = bs.GetBit();

        sps.vui_parameters_present_flag = bs.GetBit();

        if (sps.vui_parameters_present_flag)
        {
            VUI & vui = sps.vui;

            vui.aspect_ratio_info_present_flag = bs.GetBit();

            if (vui.aspect_ratio_info_present_flag)
            {
                vui.aspect_ratio_idc = (mfxU8)bs.GetBits(8);
                if (vui.aspect_ratio_idc == 255)
                {
                    vui.sar_width = (mfxU16)bs.GetBits(16);
                    vui.sar_height = (mfxU16)bs.GetBits(16);
                }
            }

            vui.overscan_info_present_flag = bs.GetBit();

            if (vui.overscan_info_present_flag)
                vui.overscan_appropriate_flag = bs.GetBit();

            vui.video_signal_type_present_flag = bs.GetBit();

            if (vui.video_signal_type_present_flag)
            {
                vui.video_format = bs.GetBits(3);
                vui.video_full_range_flag = bs.GetBit();
                vui.colour_description_present_flag = bs.GetBit();

                if (vui.colour_description_present_flag)
                {
                    vui.colour_primaries = (mfxU8)bs.GetBits(8);
                    vui.transfer_characteristics = (mfxU8)bs.GetBits(8);
                    vui.matrix_coeffs = (mfxU8)bs.GetBits(8);
                }
            }

            vui.chroma_loc_info_present_flag = bs.GetBit();

            if (vui.chroma_loc_info_present_flag)
            {
                vui.chroma_sample_loc_type_top_field = bs.GetUE();
                vui.chroma_sample_loc_type_bottom_field = bs.GetUE();
            }

            vui.neutral_chroma_indication_flag = bs.GetBit();
            vui.field_seq_flag = bs.GetBit();
            vui.frame_field_info_present_flag = bs.GetBit();
            vui.default_display_window_flag = bs.GetBit();

            if (vui.default_display_window_flag)
            {
                vui.def_disp_win_left_offset = bs.GetUE();
                vui.def_disp_win_right_offset = bs.GetUE();
                vui.def_disp_win_top_offset = bs.GetUE();
                vui.def_disp_win_bottom_offset = bs.GetUE();
            }

            vui.timing_info_present_flag = bs.GetBit();

            if (vui.timing_info_present_flag)
            {
                vui.num_units_in_tick = bs.GetBits(32);
                vui.time_scale = bs.GetBits(32);
                vui.poc_proportional_to_timing_flag = bs.GetBit();

                if (vui.poc_proportional_to_timing_flag)
                    vui.num_ticks_poc_diff_one_minus1 = bs.GetUE();

                vui.hrd_parameters_present_flag = bs.GetBit();

                if (vui.hrd_parameters_present_flag)
                {
                    HRDInfo & hrd = vui.hrd;

                    hrd.nal_hrd_parameters_present_flag = bs.GetBit();
                    hrd.vcl_hrd_parameters_present_flag = bs.GetBit();

                    if (hrd.nal_hrd_parameters_present_flag
                        || hrd.vcl_hrd_parameters_present_flag)
                    {
                        hrd.sub_pic_hrd_params_present_flag = bs.GetBit();

                        if (hrd.sub_pic_hrd_params_present_flag)
                        {
                            hrd.tick_divisor_minus2 = bs.GetBits(8);
                            hrd.du_cpb_removal_delay_increment_length_minus1 = bs.GetBits(5);
                            hrd.sub_pic_cpb_params_in_pic_timing_sei_flag = bs.GetBit();
                            hrd.dpb_output_delay_du_length_minus1 = bs.GetBits(5);
                        }

                        hrd.bit_rate_scale = bs.GetBits(4);
                        hrd.cpb_size_scale = bs.GetBits(4);

                        if (hrd.sub_pic_hrd_params_present_flag)
                            hrd.cpb_size_du_scale = bs.GetBits(4);

                        hrd.initial_cpb_removal_delay_length_minus1 = bs.GetBits(5);
                        hrd.au_cpb_removal_delay_length_minus1 = bs.GetBits(5);
                        hrd.dpb_output_delay_length_minus1 = bs.GetBits(5);
                    }

                    for (mfxU16 i = 0; i <= sps.max_sub_layers_minus1; i++)
                    {
                        hrd.sl[i].fixed_pic_rate_general_flag = bs.GetBit();

                        if (!hrd.sl[i].fixed_pic_rate_general_flag)
                            hrd.sl[i].fixed_pic_rate_within_cvs_flag = bs.GetBit();

                        if (hrd.sl[i].fixed_pic_rate_within_cvs_flag || hrd.sl[i].fixed_pic_rate_general_flag)
                            hrd.sl[i].elemental_duration_in_tc_minus1 = bs.GetUE();
                        else
                            hrd.sl[i].low_delay_hrd_flag = bs.GetBit();

                        if (!hrd.sl[i].low_delay_hrd_flag)
                            hrd.sl[i].cpb_cnt_minus1 = bs.GetUE();

                        if (hrd.nal_hrd_parameters_present_flag)
                        {
                            for (mfxU16 j = 0; j <= hrd.sl[i].cpb_cnt_minus1; j++)
                            {
                                hrd.sl[i].cpb[j].bit_rate_value_minus1 = bs.GetUE();
                                hrd.sl[i].cpb[j].cpb_size_value_minus1 = bs.GetUE();

                                if (hrd.sub_pic_hrd_params_present_flag)
                                {
                                    hrd.sl[i].cpb[j].cpb_size_du_value_minus1 = bs.GetUE();
                                    hrd.sl[i].cpb[j].bit_rate_du_value_minus1 = bs.GetUE();
                                }

                                hrd.sl[i].cpb[j].cbr_flag = (mfxU8)bs.GetBit();
                            }
                        }

                        if (hrd.vcl_hrd_parameters_present_flag)
                            return MFX_ERR_UNSUPPORTED;
                    }
                }
            }

            vui.bitstream_restriction_flag = bs.GetBit();

            if (vui.bitstream_restriction_flag)
            {
                vui.tiles_fixed_structure_flag = bs.GetBit();
                vui.motion_vectors_over_pic_boundaries_flag = bs.GetBit();
                vui.restricted_ref_pic_lists_flag = bs.GetBit();
                vui.min_spatial_segmentation_idc = bs.GetUE();
                vui.max_bytes_per_pic_denom = bs.GetUE();
                vui.max_bits_per_min_cu_denom = bs.GetUE();
                vui.log2_max_mv_length_horizontal = bs.GetUE();
                vui.log2_max_mv_length_vertical = bs.GetUE();
            }
        }


        if (bs.GetBit()) //sps.extension_flag
            return MFX_ERR_UNSUPPORTED;
    }
    catch (std::exception &)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    return sts;
}

mfxStatus HeaderReader::ReadPPS(BitstreamReader& bs, PPS & pps)
{
    mfxStatus sts = MFX_ERR_NONE;
    NALU nalu = {};

    Zero(pps);

    do
    {
        sts = ReadNALU(bs, nalu);
        MFX_CHECK_STS(sts);
    } while (nalu.nal_unit_type != PPS_NUT);

    try
    {
        pps.pic_parameter_set_id = bs.GetUE();
        pps.seq_parameter_set_id = bs.GetUE();
        pps.dependent_slice_segments_enabled_flag = bs.GetBit();
        pps.output_flag_present_flag = bs.GetBit();
        pps.num_extra_slice_header_bits = bs.GetBits(3);
        pps.sign_data_hiding_enabled_flag = bs.GetBit();
        pps.cabac_init_present_flag = bs.GetBit();
        pps.num_ref_idx_l0_default_active_minus1 = bs.GetUE();
        pps.num_ref_idx_l1_default_active_minus1 = bs.GetUE();
        pps.init_qp_minus26 = bs.GetSE();
        pps.constrained_intra_pred_flag = bs.GetBit();
        pps.transform_skip_enabled_flag = bs.GetBit();
        pps.cu_qp_delta_enabled_flag = bs.GetBit();

        if (pps.cu_qp_delta_enabled_flag)
            pps.diff_cu_qp_delta_depth = bs.GetUE();

        pps.cb_qp_offset = bs.GetSE();
        pps.cr_qp_offset = bs.GetSE();
        pps.slice_chroma_qp_offsets_present_flag = bs.GetBit();
        pps.weighted_pred_flag = bs.GetBit();
        pps.weighted_bipred_flag = bs.GetBit();
        pps.transquant_bypass_enabled_flag = bs.GetBit();
        pps.tiles_enabled_flag = bs.GetBit();
        pps.entropy_coding_sync_enabled_flag = bs.GetBit();

        if (pps.tiles_enabled_flag)
        {
            pps.num_tile_columns_minus1 = (mfxU16)bs.GetUE();
            pps.num_tile_rows_minus1 = (mfxU16)bs.GetUE();
            pps.uniform_spacing_flag = bs.GetBit();

            if (!pps.uniform_spacing_flag)
            {
                for (mfxU32 i = 0; i < pps.num_tile_columns_minus1; i++)
                    pps.column_width[i] = (mfxU16)bs.GetUE() + 1;

                for (mfxU32 i = 0; i < pps.num_tile_rows_minus1; i++)
                    pps.row_height[i] = (mfxU16)bs.GetUE() + 1;
            }

            pps.loop_filter_across_tiles_enabled_flag = bs.GetBit();
        }

        pps.loop_filter_across_slices_enabled_flag = bs.GetBit();
        pps.deblocking_filter_control_present_flag = bs.GetBit();

        if (pps.deblocking_filter_control_present_flag)
        {
            pps.deblocking_filter_override_enabled_flag = bs.GetBit();
            pps.deblocking_filter_disabled_flag = bs.GetBit();

            if (!pps.deblocking_filter_disabled_flag)
            {
                pps.beta_offset_div2 = bs.GetSE();
                pps.tc_offset_div2 = bs.GetSE();
            }
        }

        pps.scaling_list_data_present_flag = bs.GetBit();
        if (pps.scaling_list_data_present_flag)
            return MFX_ERR_UNSUPPORTED;

        pps.lists_modification_present_flag = bs.GetBit();
        pps.log2_parallel_merge_level_minus2 = (mfxU16)bs.GetUE();
        pps.slice_segment_header_extension_present_flag = bs.GetBit();


        if (bs.GetBit()) //pps.extension_flag
            return MFX_ERR_UNSUPPORTED;
    }
    catch (std::exception &)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    return sts;
}

mfxStatus HeaderPacker::PackRBSP(mfxU8* dst, mfxU8* rbsp, mfxU32& dst_size, mfxU32 rbsp_size)
{
    mfxU32 rest_bytes = dst_size - rbsp_size;
    mfxU8* rbsp_end = rbsp + rbsp_size;

    if(dst_size < rbsp_size)
        return MFX_ERR_NOT_ENOUGH_BUFFER;

    //skip start-code
    if (rbsp_size > 3)
    {
        dst[0] = rbsp[0];
        dst[1] = rbsp[1];
        dst[2] = rbsp[2];
        dst  += 3;
        rbsp += 3;
    }

    while (rbsp_end - rbsp > 2)
    {
        *dst++ = *rbsp++;

        if ( !(*(rbsp-1) || *rbsp || (*(rbsp+1) & 0xFC)) )
        {
            if (!--rest_bytes)
                return MFX_ERR_NOT_ENOUGH_BUFFER;

            *dst++ = *rbsp++;
            *dst++ = 0x03;
        }
    }

    while (rbsp_end > rbsp)
        *dst++ = *rbsp++;

    dst_size -= rest_bytes;

    return MFX_ERR_NONE;
}

void HeaderPacker::PackNALU(BitstreamWriter& bs, NALU const & h)
{
    if (   h.nal_unit_type == VPS_NUT
        || h.nal_unit_type == SPS_NUT
        || h.nal_unit_type == PPS_NUT
        || h.nal_unit_type == AUD_NUT
        || h.nal_unit_type == PREFIX_SEI_NUT
        || h.long_start_code)
    {
        bs.PutBits(8, 0); //zero_byte
    }

    bs.PutBits( 24, 0x000001);//start_code

    bs.PutBit(0);
    bs.PutBits(6, h.nal_unit_type);
    bs.PutBits(6, h.nuh_layer_id);
    bs.PutBits(3, h.nuh_temporal_id_plus1);
}

void HeaderPacker::PackPTL(BitstreamWriter& bs, LayersInfo const & ptl, mfxU16 max_sub_layers_minus1)
{
    bs.PutBits(2, ptl.general.profile_space);
    bs.PutBit(ptl.general.tier_flag);
    bs.PutBits(5, ptl.general.profile_idc);
    bs.PutBits(24,(ptl.general.profile_compatibility_flags >> 8));
    bs.PutBits(8 ,(ptl.general.profile_compatibility_flags & 0xFF));
    bs.PutBit(ptl.general.progressive_source_flag);
    bs.PutBit(ptl.general.interlaced_source_flag);
    bs.PutBit(ptl.general.non_packed_constraint_flag);
    bs.PutBit(ptl.general.frame_only_constraint_flag);
    bs.PutBits(24, 0); //general_reserved_zero_44bits
    bs.PutBits(20, 0); //general_reserved_zero_44bits
    bs.PutBits(8, ptl.general.level_idc);

    for (mfxU32 i = 0; i < max_sub_layers_minus1; i++ )
    {
        bs.PutBit(ptl.sub_layer[i].profile_present_flag);
        bs.PutBit(ptl.sub_layer[i].level_present_flag);
    }

    if (max_sub_layers_minus1 > 0)
        for (mfxU32 i = max_sub_layers_minus1; i < 8; i++)
            bs.PutBits(2, 0); // reserved_zero_2bits[ i ]

    for (mfxU32 i = 0; i < max_sub_layers_minus1; i++)
    {
        if (ptl.sub_layer[i].profile_present_flag)
        {
            bs.PutBits(2,  ptl.sub_layer[i].profile_space);
            bs.PutBit(ptl.sub_layer[i].tier_flag);
            bs.PutBits(5,  ptl.sub_layer[i].profile_idc);
            bs.PutBits(24,(ptl.sub_layer[i].profile_compatibility_flags >> 8));
            bs.PutBits(8 ,(ptl.sub_layer[i].profile_compatibility_flags & 0xFF));
            bs.PutBit(ptl.sub_layer[i].progressive_source_flag);
            bs.PutBit(ptl.sub_layer[i].interlaced_source_flag);
            bs.PutBit(ptl.sub_layer[i].non_packed_constraint_flag);
            bs.PutBit(ptl.sub_layer[i].frame_only_constraint_flag);
            bs.PutBits(24, 0); //general_reserved_zero_44bits
            bs.PutBits(20, 0); //general_reserved_zero_44bits
        }

        if (ptl.sub_layer[i].level_present_flag)
            bs.PutBits(8, ptl.sub_layer[i].level_idc);
    }
}

void HeaderPacker::PackSLO(BitstreamWriter& bs, LayersInfo const & slo, mfxU16 max_sub_layers_minus1)
{
    bs.PutBit(slo.sub_layer_ordering_info_present_flag);

    for (mfxU32 i = ( slo.sub_layer_ordering_info_present_flag ? 0 : max_sub_layers_minus1 );
            i  <=  max_sub_layers_minus1; i++ )
    {
        bs.PutUE(slo.sub_layer[i].max_dec_pic_buffering_minus1);
        bs.PutUE(slo.sub_layer[i].max_num_reorder_pics);
        bs.PutUE(slo.sub_layer[i].max_latency_increase_plus1);
    }
}

void HeaderPacker::PackAUD(BitstreamWriter& bs, mfxU8 pic_type)
{
    NALU nalu = {0, AUD_NUT, 0, 1};
    PackNALU(bs, nalu);
    bs.PutBits(3, pic_type);
    bs.PutTrailingBits();
}

void HeaderPacker::PackVPS(BitstreamWriter& bs, VPS const &  vps)
{
    NALU nalu = {0, VPS_NUT, 0, 1};

    PackNALU(bs, nalu);

    bs.PutBits(4, vps.video_parameter_set_id);
    bs.PutBits(2, 3);
    bs.PutBits(6, vps.max_layers_minus1);
    bs.PutBits(3, vps.max_sub_layers_minus1);
    bs.PutBit(vps.temporal_id_nesting_flag);
    bs.PutBits(16, 0xFFFF);

    PackPTL(bs, vps, vps.max_sub_layers_minus1);
    PackSLO(bs, vps, vps.max_sub_layers_minus1);

    bs.PutBits(6, vps.max_layer_id);
    bs.PutUE(vps.num_layer_sets_minus1);

    assert(0 == vps.num_layer_sets_minus1);

    bs.PutBit(vps.timing_info_present_flag);

    if (vps.timing_info_present_flag)
    {
        bs.PutBits(32, vps.num_units_in_tick);
        bs.PutBits(32, vps.time_scale);

        bs.PutBit(vps.poc_proportional_to_timing_flag);

        if (vps.poc_proportional_to_timing_flag)
            bs.PutUE(vps.num_ticks_poc_diff_one_minus1);

        bs.PutUE(vps.num_hrd_parameters);

        assert(0 == vps.num_hrd_parameters);
    }

    bs.PutBit(0); //vps.extension_flag
    bs.PutTrailingBits();
}

void HeaderPacker::PackHRD(
    BitstreamWriter& bs,
    HRDInfo const & hrd,
    bool commonInfPresentFlag,
    mfxU16 maxNumSubLayersMinus1)
{
    if (commonInfPresentFlag)
    {
        bs.PutBit(hrd.nal_hrd_parameters_present_flag);
        bs.PutBit(hrd.vcl_hrd_parameters_present_flag);

        if (   hrd.nal_hrd_parameters_present_flag
            || hrd.vcl_hrd_parameters_present_flag)
        {
            bs.PutBit(hrd.sub_pic_hrd_params_present_flag);

            if (hrd.sub_pic_hrd_params_present_flag)
            {
                bs.PutBits(8, hrd.tick_divisor_minus2);
                bs.PutBits(5, hrd.du_cpb_removal_delay_increment_length_minus1);
                bs.PutBit(hrd.sub_pic_cpb_params_in_pic_timing_sei_flag);
                bs.PutBits(5, hrd.dpb_output_delay_du_length_minus1);
            }

            bs.PutBits(4, hrd.bit_rate_scale);
            bs.PutBits(4, hrd.cpb_size_scale);

            if (hrd.sub_pic_hrd_params_present_flag)
                bs.PutBits(4, hrd.cpb_size_du_scale);

           bs.PutBits(5, hrd.initial_cpb_removal_delay_length_minus1);
           bs.PutBits(5, hrd.au_cpb_removal_delay_length_minus1);
           bs.PutBits(5, hrd.dpb_output_delay_length_minus1);
        }
    }

    for (mfxU16 i = 0; i <= maxNumSubLayersMinus1; i ++)
    {
        bs.PutBit(hrd.sl[i].fixed_pic_rate_general_flag);

        if (!hrd.sl[i].fixed_pic_rate_general_flag)
            bs.PutBit(hrd.sl[i].fixed_pic_rate_within_cvs_flag);

        if (hrd.sl[i].fixed_pic_rate_within_cvs_flag || hrd.sl[i].fixed_pic_rate_general_flag )
            bs.PutUE(hrd.sl[i].elemental_duration_in_tc_minus1);
        else
            bs.PutBit(hrd.sl[i].low_delay_hrd_flag);

        if (!hrd.sl[i].low_delay_hrd_flag)
            bs.PutUE(hrd.sl[i].cpb_cnt_minus1);

        if (hrd.nal_hrd_parameters_present_flag)
        {
            for (mfxU16 j = 0; j <= hrd.sl[i].cpb_cnt_minus1; j ++)
            {
               bs.PutUE(hrd.sl[i].cpb[j].bit_rate_value_minus1);
               bs.PutUE(hrd.sl[i].cpb[j].cpb_size_value_minus1);

                if (hrd.sub_pic_hrd_params_present_flag)
                {
                    bs.PutUE(hrd.sl[i].cpb[j].cpb_size_du_value_minus1);
                    bs.PutUE(hrd.sl[i].cpb[j].bit_rate_du_value_minus1);
                }

                bs.PutBit(hrd.sl[i].cpb[j].cbr_flag);
            }
        }
        assert(hrd.vcl_hrd_parameters_present_flag == 0);
    }
}

void HeaderPacker::PackVUI(BitstreamWriter& bs, VUI const & vui, mfxU16 max_sub_layers_minus1)
{
    bs.PutBit(vui.aspect_ratio_info_present_flag);

    if (vui.aspect_ratio_info_present_flag)
    {
        bs.PutBits(8, vui.aspect_ratio_idc);
        if (vui.aspect_ratio_idc == 255)
        {
            bs.PutBits(16, vui.sar_width);
            bs.PutBits(16, vui.sar_height);
        }
    }

    bs.PutBit(vui.overscan_info_present_flag);

    if (vui.overscan_info_present_flag)
        bs.PutBit(vui.overscan_appropriate_flag);

    bs.PutBit(vui.video_signal_type_present_flag);

    if (vui.video_signal_type_present_flag)
    {
        bs.PutBits(3, vui.video_format);
        bs.PutBit(vui.video_full_range_flag);
        bs.PutBit(vui.colour_description_present_flag);

        if (vui.colour_description_present_flag)
        {
            bs.PutBits(8, vui.colour_primaries);
            bs.PutBits(8, vui.transfer_characteristics);
            bs.PutBits(8, vui.matrix_coeffs);
        }
    }

    bs.PutBit(vui.chroma_loc_info_present_flag);

    if (vui.chroma_loc_info_present_flag)
    {
        bs.PutUE(vui.chroma_sample_loc_type_top_field);
        bs.PutUE(vui.chroma_sample_loc_type_bottom_field);
    }

    bs.PutBit(vui.neutral_chroma_indication_flag );
    bs.PutBit(vui.field_seq_flag                 );
    bs.PutBit(vui.frame_field_info_present_flag  );
    bs.PutBit(vui.default_display_window_flag    );

    if (vui.default_display_window_flag)
    {
        bs.PutUE(vui.def_disp_win_left_offset   );
        bs.PutUE(vui.def_disp_win_right_offset  );
        bs.PutUE(vui.def_disp_win_top_offset    );
        bs.PutUE(vui.def_disp_win_bottom_offset );
    }

    bs.PutBit(vui.timing_info_present_flag);

    if (vui.timing_info_present_flag)
    {
        bs.PutBits(32, vui.num_units_in_tick );
        bs.PutBits(32, vui.time_scale        );
        bs.PutBit(vui.poc_proportional_to_timing_flag);

        if (vui.poc_proportional_to_timing_flag)
            bs.PutUE(vui.num_ticks_poc_diff_one_minus1);

        bs.PutBit(vui.hrd_parameters_present_flag);

        if (vui.hrd_parameters_present_flag)
            PackHRD(bs, vui.hrd, 1, max_sub_layers_minus1);
    }

    bs.PutBit(vui.bitstream_restriction_flag);

    if (vui.bitstream_restriction_flag)
    {
        bs.PutBit(vui.tiles_fixed_structure_flag              );
        bs.PutBit(vui.motion_vectors_over_pic_boundaries_flag );
        bs.PutBit(vui.restricted_ref_pic_lists_flag           );
        bs.PutUE(vui.min_spatial_segmentation_idc  );
        bs.PutUE(vui.max_bytes_per_pic_denom       );
        bs.PutUE(vui.max_bits_per_min_cu_denom     );
        bs.PutUE(vui.log2_max_mv_length_horizontal );
        bs.PutUE(vui.log2_max_mv_length_vertical   );
    }
}

void HeaderPacker::PackSPS(BitstreamWriter& bs, SPS const & sps)
{
    NALU nalu = {0, SPS_NUT, 0, 1};

    PackNALU(bs, nalu);

    bs.PutBits(4, sps.video_parameter_set_id);
    bs.PutBits(3, sps.max_sub_layers_minus1);
    bs.PutBit(sps.temporal_id_nesting_flag);

    PackPTL(bs, sps, sps.max_sub_layers_minus1);

    bs.PutUE(sps.seq_parameter_set_id);
    bs.PutUE(sps.chroma_format_idc);

    if(sps.chroma_format_idc  ==  3)
        bs.PutBit(sps.separate_colour_plane_flag);

    bs.PutUE(sps.pic_width_in_luma_samples);
    bs.PutUE(sps.pic_height_in_luma_samples);
    bs.PutBit(sps.conformance_window_flag);

    if (sps.conformance_window_flag)
    {
        bs.PutUE(sps.conf_win_left_offset);
        bs.PutUE(sps.conf_win_right_offset);
        bs.PutUE(sps.conf_win_top_offset);
        bs.PutUE(sps.conf_win_bottom_offset);
    }

    bs.PutUE(sps.bit_depth_luma_minus8);
    bs.PutUE(sps.bit_depth_chroma_minus8);
    bs.PutUE(sps.log2_max_pic_order_cnt_lsb_minus4);

    PackSLO(bs, sps, sps.max_sub_layers_minus1);

    bs.PutUE(sps.log2_min_luma_coding_block_size_minus3);
    bs.PutUE(sps.log2_diff_max_min_luma_coding_block_size);
    bs.PutUE(sps.log2_min_transform_block_size_minus2);
    bs.PutUE(sps.log2_diff_max_min_transform_block_size);
    bs.PutUE(sps.max_transform_hierarchy_depth_inter);
    bs.PutUE(sps.max_transform_hierarchy_depth_intra);
    bs.PutBit(sps.scaling_list_enabled_flag);

    assert(0 == sps.scaling_list_enabled_flag);

    bs.PutBit(sps.amp_enabled_flag);
    bs.PutBit(sps.sample_adaptive_offset_enabled_flag);
    bs.PutBit(sps.pcm_enabled_flag);

    if (sps.pcm_enabled_flag)
    {
        bs.PutBits(4, sps.pcm_sample_bit_depth_luma_minus1);
        bs.PutBits(4, sps.pcm_sample_bit_depth_chroma_minus1);
        bs.PutUE(sps.log2_min_pcm_luma_coding_block_size_minus3);
        bs.PutUE(sps.log2_diff_max_min_pcm_luma_coding_block_size);
        bs.PutBit(sps.pcm_loop_filter_disabled_flag);
    }

    bs.PutUE(sps.num_short_term_ref_pic_sets);

    for (mfxU32 i = 0; i < sps.num_short_term_ref_pic_sets; i++)
        PackSTRPS(bs, sps.strps, sps.num_short_term_ref_pic_sets, i);


    bs.PutBit(sps.long_term_ref_pics_present_flag);

    if (sps.long_term_ref_pics_present_flag)
    {
        bs.PutUE(sps.num_long_term_ref_pics_sps);

        for (mfxU32 i = 0; i < sps.num_long_term_ref_pics_sps; i++)
        {
            bs.PutBits(sps.log2_max_pic_order_cnt_lsb_minus4 + 4, sps.lt_ref_pic_poc_lsb_sps[i] );
            bs.PutBit(sps.used_by_curr_pic_lt_sps_flag[i] );
        }
    }

    bs.PutBit(sps.temporal_mvp_enabled_flag);
    bs.PutBit(sps.strong_intra_smoothing_enabled_flag);

    bs.PutBit(sps.vui_parameters_present_flag);

    if (sps.vui_parameters_present_flag)
        PackVUI(bs, sps.vui, sps.max_sub_layers_minus1);

    bs.PutBit(0); //sps.extension_flag

    bs.PutTrailingBits();
}

void HeaderPacker::PackPPS(BitstreamWriter& bs, PPS const &  pps)
{
    NALU nalu = {0, PPS_NUT, 0, 1};

    PackNALU(bs, nalu);

    bs.PutUE(pps.pic_parameter_set_id);
    bs.PutUE(pps.seq_parameter_set_id);
    bs.PutBit(pps.dependent_slice_segments_enabled_flag);
    bs.PutBit(pps.output_flag_present_flag);
    bs.PutBits(3, pps.num_extra_slice_header_bits);
    bs.PutBit(pps.sign_data_hiding_enabled_flag);
    bs.PutBit(pps.cabac_init_present_flag);
    bs.PutUE(pps.num_ref_idx_l0_default_active_minus1);
    bs.PutUE(pps.num_ref_idx_l1_default_active_minus1);
    bs.PutSE(pps.init_qp_minus26);
    bs.PutBit(pps.constrained_intra_pred_flag);
    bs.PutBit(pps.transform_skip_enabled_flag);
    bs.PutBit(pps.cu_qp_delta_enabled_flag);

    if (pps.cu_qp_delta_enabled_flag)
        bs.PutUE(pps.diff_cu_qp_delta_depth);

    bs.PutSE(pps.cb_qp_offset);
    bs.PutSE(pps.cr_qp_offset);
    bs.PutBit(pps.slice_chroma_qp_offsets_present_flag);
    bs.PutBit(pps.weighted_pred_flag);
    bs.PutBit(pps.weighted_bipred_flag);
    bs.PutBit(pps.transquant_bypass_enabled_flag);
    bs.PutBit(pps.tiles_enabled_flag);
    bs.PutBit(pps.entropy_coding_sync_enabled_flag);

    if (pps.tiles_enabled_flag )
    {
        bs.PutUE(pps.num_tile_columns_minus1);
        bs.PutUE(pps.num_tile_rows_minus1);
        bs.PutBit(pps.uniform_spacing_flag);

        if(!pps.uniform_spacing_flag)
        {
            for (mfxU32 i = 0; i < pps.num_tile_columns_minus1; i++)
                bs.PutUE(Max<mfxU16>(pps.column_width[i], 1) - 1);

            for (mfxU32 i = 0; i < pps.num_tile_rows_minus1; i++)
                bs.PutUE(Max<mfxU16>(pps.row_height[i], 1) - 1);
        }

        bs.PutBit(pps.loop_filter_across_tiles_enabled_flag);
    }

    bs.PutBit(pps.loop_filter_across_slices_enabled_flag);
    bs.PutBit(pps.deblocking_filter_control_present_flag);

    if(pps.deblocking_filter_control_present_flag)
    {
        bs.PutBit(pps.deblocking_filter_override_enabled_flag);
        bs.PutBit(pps.deblocking_filter_disabled_flag);

        if(!pps.deblocking_filter_disabled_flag)
        {
            bs.PutSE(pps.beta_offset_div2);
            bs.PutSE(pps.tc_offset_div2);
        }
    }

    bs.PutBit(pps.scaling_list_data_present_flag);
    assert(0 == pps.scaling_list_data_present_flag);

    bs.PutBit(pps.lists_modification_present_flag);
    bs.PutUE(pps.log2_parallel_merge_level_minus2);
    bs.PutBit(pps.slice_segment_header_extension_present_flag);

    bs.PutBit(0); //pps.extension_flag

    bs.PutTrailingBits();
}

void HeaderPacker::PackSSH(
    BitstreamWriter& bs,
    NALU  const &    nalu,
    SPS   const &    sps,
    PPS   const &    pps,
    Slice const &    slice,
    mfxU32*          qpd_offset,
    bool             dyn_slice_size,
    mfxU32*          sao_offset,
    mfxU32*          pwt_offset,
    mfxU32*          pwt_length)
{
    const mfxU8 B = 0, P = 1/*, I = 2*/;

    PackNALU(bs, nalu);

    if (!dyn_slice_size) {
        mfxU32 MaxCU = (1<<(sps.log2_min_luma_coding_block_size_minus3 + 3 + sps.log2_diff_max_min_luma_coding_block_size));
        mfxU32 PicSizeInCtbsY = CeilDiv(sps.pic_width_in_luma_samples, MaxCU) * CeilDiv(sps.pic_height_in_luma_samples, MaxCU);

        bs.PutBit(slice.first_slice_segment_in_pic_flag);

        if (nalu.nal_unit_type >=  BLA_W_LP  && nalu.nal_unit_type <=  RSV_IRAP_VCL23)
            bs.PutBit(slice.no_output_of_prior_pics_flag);

        bs.PutUE(slice.pic_parameter_set_id);

        if (!slice.first_slice_segment_in_pic_flag)
        {
            if (pps.dependent_slice_segments_enabled_flag)
                bs.PutBit(slice.dependent_slice_segment_flag);

            bs.PutBits(CeilLog2(PicSizeInCtbsY), slice.segment_address);
        }
    }

    if( !slice.dependent_slice_segment_flag )
    {
        if(pps.num_extra_slice_header_bits)
            bs.PutBits(pps.num_extra_slice_header_bits, slice.reserved_flags);

        bs.PutUE(slice.type);

        if (pps.output_flag_present_flag)
            bs.PutBit(slice.pic_output_flag);

        if (sps.separate_colour_plane_flag == 1)
            bs.PutBits(2, slice.colour_plane_id);

        if (nalu.nal_unit_type != IDR_W_RADL && nalu.nal_unit_type !=  IDR_N_LP)
        {
            bs.PutBits(sps.log2_max_pic_order_cnt_lsb_minus4+4, slice.pic_order_cnt_lsb);
            bs.PutBit(slice.short_term_ref_pic_set_sps_flag);

            if (!slice.short_term_ref_pic_set_sps_flag)
            {
                STRPS strps[65];
                Copy(strps, sps.strps);
                strps[sps.num_short_term_ref_pic_sets] = slice.strps;

                PackSTRPS(bs, strps, sps.num_short_term_ref_pic_sets, sps.num_short_term_ref_pic_sets);
            }
            else
            {
                if (sps.num_short_term_ref_pic_sets > 1)
                    bs.PutBits(CeilLog2(sps.num_short_term_ref_pic_sets), slice.short_term_ref_pic_set_idx);
            }

            if (sps.long_term_ref_pics_present_flag)
            {
                if (sps.num_long_term_ref_pics_sps > 0)
                    bs.PutUE(slice.num_long_term_sps);

                bs.PutUE(slice.num_long_term_pics);

                for (mfxU16 i = 0; i < slice.num_long_term_sps + slice.num_long_term_pics; i++)
                {
                    if (i < slice.num_long_term_sps)
                    {
                        if (sps.num_long_term_ref_pics_sps > 1)
                            bs.PutBits(CeilLog2(sps.num_long_term_ref_pics_sps), slice.lt[i].lt_idx_sps);
                    }
                    else
                    {
                        bs.PutBits(sps.log2_max_pic_order_cnt_lsb_minus4+4, slice.lt[i].poc_lsb_lt);
                        bs.PutBit(slice.lt[i].used_by_curr_pic_lt_flag);
                    }

                    bs.PutBit(slice.lt[i].delta_poc_msb_present_flag);

                    if( slice.lt[i].delta_poc_msb_present_flag)
                        bs.PutUE(slice.lt[i].delta_poc_msb_cycle_lt );
                }
            }

            if (sps.temporal_mvp_enabled_flag)
                bs.PutBit(slice.temporal_mvp_enabled_flag);
        }

        if (sps.sample_adaptive_offset_enabled_flag)
        {
            if (sao_offset)
                *sao_offset = bs.GetOffset();

            bs.PutBit(slice.sao_luma_flag);
            bs.PutBit(slice.sao_chroma_flag);
        }

        if (slice.type == P || slice.type == B)
        {
            mfxU16 NumPocTotalCurr = 0;

            bs.PutBit(slice.num_ref_idx_active_override_flag);

            if (slice.num_ref_idx_active_override_flag)
            {
                bs.PutUE(slice.num_ref_idx_l0_active_minus1 );

                if (slice.type == B)
                    bs.PutUE(slice.num_ref_idx_l1_active_minus1 );
            }

            for (mfxU16 i = 0; i < slice.strps.num_negative_pics; i++ )
                if( slice.strps.pic[i].used_by_curr_pic_s0_flag )
                    NumPocTotalCurr ++;

            for (mfxU16 i = slice.strps.num_negative_pics;
                i < (slice.strps.num_negative_pics + slice.strps.num_positive_pics); i++)
                if( slice.strps.pic[i].used_by_curr_pic_s1_flag )
                    NumPocTotalCurr ++;

            for (mfxU16 i = 0; i < slice.num_long_term_sps + slice.num_long_term_pics; i++ )
                if( slice.lt[i].used_by_curr_pic_lt_flag )
                    NumPocTotalCurr ++;

            if (pps.lists_modification_present_flag && NumPocTotalCurr > 1)
            {
                bs.PutBit(!!slice.ref_pic_list_modification_flag_lx[0]);

                if (slice.ref_pic_list_modification_flag_lx[0])
                    for (mfxU16 i = 0; i <= slice.num_ref_idx_l0_active_minus1; i ++)
                        bs.PutBits(CeilLog2(NumPocTotalCurr), slice.list_entry_lx[0][i]);

                if (slice.type == B)
                {
                    bs.PutBit(!!slice.ref_pic_list_modification_flag_lx[1]);

                    if (slice.ref_pic_list_modification_flag_lx[1])
                        for (mfxU16  i = 0; i  <=  slice.num_ref_idx_l1_active_minus1; i ++)
                        bs.PutBits(CeilLog2(NumPocTotalCurr), slice.list_entry_lx[1][i]);
                }
            }

            if (slice.type  ==  B)
                bs.PutBit(slice.mvd_l1_zero_flag );

            if (pps.cabac_init_present_flag)
                bs.PutBit(slice.cabac_init_flag);

            if (slice.temporal_mvp_enabled_flag)
            {
                //slice.collocated_from_l0_flag  = 1;

                if (slice.type == B)
                    bs.PutBit(1); //slice.collocated_from_l0_flag

                if ((  slice.collocated_from_l0_flag  &&  slice.num_ref_idx_l0_active_minus1 > 0 )  ||
                    ( !slice.collocated_from_l0_flag  &&  slice.num_ref_idx_l1_active_minus1 > 0 ) )
                    bs.PutUE(slice.collocated_ref_idx);
            }

#if defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
            if (pwt_offset)
                *pwt_offset = bs.GetOffset();

            if (   (pps.weighted_pred_flag && slice.type == P)
                || (pps.weighted_bipred_flag && slice.type == B))
            {
                const mfxU16 Y = 0, Cb = 1, Cr = 2, W = 0, O = 1;
                mfxI16
                      WpOffsetHalfRangeC = (1 << 7)
                    , wY = (1 << slice.luma_log2_weight_denom)
                    , wC = (1 << slice.chroma_log2_weight_denom)
                    , l2WDc = slice.chroma_log2_weight_denom;

                bs.PutUE(slice.luma_log2_weight_denom);

                if (sps.chroma_format_idc != 0)
                    bs.PutSE(mfxI32(slice.chroma_log2_weight_denom) - slice.luma_log2_weight_denom);

                for (mfxU32 l = 0; l < mfxU32(1 + (slice.type == B)); l++)
                {
                    mfxU32 sz = l ? slice.num_ref_idx_l1_active_minus1 + 1 : slice.num_ref_idx_l0_active_minus1 + 1;

                    mfxU16 lumaw = 0;
                    mfxU16 chromaw = 0;

                    for (mfxU32 i = 0; i < sz; i++)
                    {
                        lumaw   <<= 1;
                        lumaw   |= !(slice.pwt[l][i][Y][O] == 0 && slice.pwt[l][i][Y][W] == wY);
                        chromaw <<= 1;
                        chromaw |= !(slice.pwt[l][i][Cb][O] == 0 && slice.pwt[l][i][Cb][W] == wC);
                        chromaw |= !(slice.pwt[l][i][Cr][O] == 0 && slice.pwt[l][i][Cr][W] == wC);
                    }

                    bs.PutBits(sz, lumaw);

                    if (sps.chroma_format_idc != 0)
                        bs.PutBits(sz, chromaw);
                    else
                        chromaw = 0;

                    for (mfxU32 i = 0; i < sz; i++)
                    {
                        if (lumaw & (1 << (sz - i - 1)))
                        {
                            bs.PutSE(slice.pwt[l][i][Y][W] - wY);
                            bs.PutSE(slice.pwt[l][i][Y][O]);
                        }

                        if (chromaw & (1 << (sz - i - 1)))
                        {
                            bs.PutSE(slice.pwt[l][i][Cb][W] - wC);
                            bs.PutSE(
                                Clip3(
                                    -4 * WpOffsetHalfRangeC,
                                    4 * WpOffsetHalfRangeC - 1,
                                    ((WpOffsetHalfRangeC * slice.pwt[l][i][Cb][W]) >> l2WDc) + slice.pwt[l][i][Cb][O] - WpOffsetHalfRangeC));

                            bs.PutSE(slice.pwt[l][i][Cr][W] - wC);
                            bs.PutSE(
                                Clip3(
                                    -4 * WpOffsetHalfRangeC,
                                    4 * WpOffsetHalfRangeC - 1,
                                    ((WpOffsetHalfRangeC * slice.pwt[l][i][Cr][W]) >> l2WDc) + slice.pwt[l][i][Cr][O] - WpOffsetHalfRangeC));
                        }
                    }
                }
            }

            if (pwt_length && pwt_offset)
                *pwt_length = bs.GetOffset() - *pwt_offset;
#else
            (void)pwt_offset;
            (void)pwt_length;
            assert(0 == pps.weighted_pred_flag);
            assert(0 == pps.weighted_bipred_flag);
#endif //defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)


            bs.PutUE(slice.five_minus_max_num_merge_cand);
        }

        if (qpd_offset)
            *qpd_offset = bs.GetOffset();

        bs.PutSE(slice.slice_qp_delta);

        if (pps.slice_chroma_qp_offsets_present_flag)
        {
            bs.PutSE(slice.slice_cb_qp_offset);
            bs.PutSE(slice.slice_cr_qp_offset);
        }

        if (pps.deblocking_filter_override_enabled_flag)
            bs.PutBit(slice.deblocking_filter_override_flag);

        if (slice.deblocking_filter_override_flag)
        {
            bs.PutBit(slice.deblocking_filter_disabled_flag);

            if (!slice.deblocking_filter_disabled_flag)
            {
                bs.PutSE(slice.beta_offset_div2);
                bs.PutSE(slice.tc_offset_div2);
            }
        }

        if (   pps.loop_filter_across_slices_enabled_flag
            && (slice.sao_luma_flag
             || slice.sao_chroma_flag
             || !slice.deblocking_filter_disabled_flag))
        {
            bs.PutBit(slice.loop_filter_across_slices_enabled_flag);
        }
    }

    if (pps.tiles_enabled_flag || pps.entropy_coding_sync_enabled_flag)
    {
        assert(slice.num_entry_point_offsets == 0);

        bs.PutUE(slice.num_entry_point_offsets);

        /*if (slice.num_entry_point_offsets > 0)
        {
            bs.PutUE(slice.offset_len_minus1);

            for (mfxU32 i = 0; i < slice.num_entry_point_offsets; i++)
                bs.PutBits(slice.offset_len_minus1+1, slice.entry_point_offset_minus1[i]);
        }*/
    }

    assert(0 == pps.slice_segment_header_extension_present_flag);

    if (!dyn_slice_size)   // no trailing bits for dynamic slice size
        bs.PutTrailingBits();
}

void HeaderPacker::PackSTRPS(BitstreamWriter& bs, const STRPS* sets, mfxU32 num, mfxU32 idx)
{
    STRPS const & strps = sets[idx];

    if (idx != 0)
    {
        bs.PutBit(strps.inter_ref_pic_set_prediction_flag);
    }

    if (strps.inter_ref_pic_set_prediction_flag)
    {

        if(idx == num)
        {
            bs.PutUE(strps.delta_idx_minus1);
        }

        bs.PutBit(strps.delta_rps_sign);
        bs.PutUE(strps.abs_delta_rps_minus1);

        mfxU32 RefRpsIdx = idx - ( strps.delta_idx_minus1 + 1 );
        mfxU32 NumDeltaPocs = sets[RefRpsIdx].num_negative_pics + sets[RefRpsIdx].num_positive_pics;

        for (mfxU32 j = 0; j <=  NumDeltaPocs; j++)
        {
            bs.PutBit(strps.pic[j].used_by_curr_pic_flag);

            if (!strps.pic[j].used_by_curr_pic_flag)
            {
                bs.PutBit(strps.pic[j].use_delta_flag);
            }
        }
    }
    else
    {
        bs.PutUE(strps.num_negative_pics);
        bs.PutUE(strps.num_positive_pics);

        for (mfxU32 i = 0; i < strps.num_negative_pics; i++)
        {
            bs.PutUE(strps.pic[i].delta_poc_s0_minus1);
            bs.PutBit(strps.pic[i].used_by_curr_pic_s0_flag);
        }

        for(mfxU32 i = strps.num_negative_pics; i < mfxU32(strps.num_positive_pics + strps.num_negative_pics); i++)
        {
            bs.PutUE(strps.pic[i].delta_poc_s1_minus1);
            bs.PutBit(strps.pic[i].used_by_curr_pic_s1_flag);
        }
    }
}

void HeaderPacker::PackSEIPayload(
    BitstreamWriter& bs,
    VUI const & vui,
    BufferingPeriodSEI const & bp)
{
    HRDInfo const & hrd = vui.hrd;

    bs.PutUE(bp.seq_parameter_set_id);

    if (!bp.irap_cpb_params_present_flag)
        bs.PutBit(bp.irap_cpb_params_present_flag);

    if (bp.irap_cpb_params_present_flag)
    {
        bs.PutBits(hrd.au_cpb_removal_delay_length_minus1+1, bp.cpb_delay_offset);
        bs.PutBits(hrd.dpb_output_delay_length_minus1+1, bp.dpb_delay_offset);
    }

    bs.PutBit(bp.concatenation_flag);
    bs.PutBits(hrd.au_cpb_removal_delay_length_minus1+1, bp.au_cpb_removal_delay_delta_minus1);

    mfxU16 CpbCnt = hrd.sl[0].cpb_cnt_minus1;

    if (hrd.nal_hrd_parameters_present_flag)
    {
        for (mfxU16 i = 0; i <= CpbCnt; i ++)
        {
            bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1+1, bp.nal[i].initial_cpb_removal_delay  );
            bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1+1, bp.nal[i].initial_cpb_removal_offset );

            if (   hrd.sub_pic_hrd_params_present_flag
                || bp.irap_cpb_params_present_flag)
            {
                bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1+1, bp.nal[i].initial_alt_cpb_removal_delay  );
                bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1+1, bp.nal[i].initial_alt_cpb_removal_offset );
            }
        }
    }

    if (hrd.vcl_hrd_parameters_present_flag)
    {
        for (mfxU16 i = 0; i <= CpbCnt; i ++)
        {
            bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1+1, bp.vcl[i].initial_cpb_removal_delay  );
            bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1+1, bp.vcl[i].initial_cpb_removal_offset );

            if (   hrd.sub_pic_hrd_params_present_flag
                || bp.irap_cpb_params_present_flag)
            {
                bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1+1, bp.vcl[i].initial_alt_cpb_removal_delay  );
                bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1+1, bp.vcl[i].initial_alt_cpb_removal_offset );
            }
        }
    }

    bs.PutTrailingBits(true);
}


void HeaderPacker::PackSEIPayload(
    BitstreamWriter& bs,
    VUI const & vui,
    PicTimingSEI const & pt)
{
    HRDInfo const & hrd = vui.hrd;

    if (vui.frame_field_info_present_flag)
    {
        bs.PutBits(4, pt.pic_struct);
        bs.PutBits(2, pt.source_scan_type);
        bs.PutBit(pt.duplicate_flag);
    }

    if (   hrd.nal_hrd_parameters_present_flag
        || hrd.vcl_hrd_parameters_present_flag)
    {
       bs.PutBits(hrd.au_cpb_removal_delay_length_minus1+1, pt.au_cpb_removal_delay_minus1);
       bs.PutBits(hrd.dpb_output_delay_length_minus1+1, pt.pic_dpb_output_delay);

       assert(hrd.sub_pic_hrd_params_present_flag == 0);
    }

    bs.PutTrailingBits(true);
}

HeaderPacker::HeaderPacker()
    : m_sz_vps(0)
    , m_sz_sps(0)
    , m_sz_pps(0)
    , m_sz_ssh(0)
    , m_par(0)
    , m_bs(m_bs_ssh, sizeof(m_bs_ssh))
{
    m_sz_vps = 0;
    m_sz_sps = 0;
    m_sz_pps = 0;
    m_sz_ssh = 0;

    for (mfxU8 i = 0; i < 3; i ++)
    {
        BitstreamWriter bs(m_bs_aud[i], AUD_BS_SIZE);
        PackAUD(bs, i);
    }
}

HeaderPacker::~HeaderPacker()
{
}

mfxStatus HeaderPacker::Reset(MfxVideoParam const & par)
{
    mfxStatus sts = MFX_ERR_NONE;
    BitstreamWriter rbsp(m_rbsp, sizeof(m_rbsp));

    m_sz_vps = sizeof(m_bs_vps);
    m_sz_sps = sizeof(m_bs_sps);
    m_sz_pps = sizeof(m_bs_pps);
    m_sz_ssh = sizeof(m_bs_ssh);

    PackVPS(rbsp, par.m_vps);
    sts = PackRBSP(m_bs_vps, m_rbsp, m_sz_vps, CeilDiv(rbsp.GetOffset(), 8));
    assert(!sts);
    if (sts)
        return sts;

    rbsp.Reset();

    PackSPS(rbsp, par.m_sps);
    sts = PackRBSP(m_bs_sps, m_rbsp, m_sz_sps, CeilDiv(rbsp.GetOffset(), 8));
    assert(!sts);
    if (sts)
        return sts;

    rbsp.Reset();

    PackPPS(rbsp, par.m_pps);
    sts = PackRBSP(m_bs_pps, m_rbsp, m_sz_pps, CeilDiv(rbsp.GetOffset(), 8));
    assert(!sts);

    m_par = &par;

    return sts;
}

mfxStatus HeaderPacker::ResetPPS(MfxVideoParam const & par)
{
    BitstreamWriter rbsp(m_rbsp, sizeof(m_rbsp));

    m_sz_pps = sizeof(m_bs_pps);

    PackPPS(rbsp, par.m_pps);
    mfxStatus sts = PackRBSP(m_bs_pps, m_rbsp, m_sz_pps, CeilDiv(rbsp.GetOffset(), 8));
    assert(!sts);

    m_par = &par;

    return sts;
}


void PackBPPayload(BitstreamWriter& rbsp, MfxVideoParam const & par, Task const & task)
{
    BufferingPeriodSEI bp = {};

    bp.seq_parameter_set_id = par.m_sps.seq_parameter_set_id;
    bp.nal[0].initial_cpb_removal_delay  = bp.vcl[0].initial_cpb_removal_delay  = task.m_initial_cpb_removal_delay;
    bp.nal[0].initial_cpb_removal_offset = bp.vcl[0].initial_cpb_removal_offset = task.m_initial_cpb_removal_offset;

    mfxU32 size = CeilDiv(rbsp.GetOffset(), 8);
    mfxU8* pl = rbsp.GetStart() + size; // payload start

    rbsp.PutBits(8, 0);    //payload type
    rbsp.PutBits(8, 0xff); //place for payload  size
    size += 2;

    HeaderPacker::PackSEIPayload(rbsp, par.m_sps.vui, bp);

    size = CeilDiv(rbsp.GetOffset(), 8) - size;

    assert(size < 256);
    pl[1] = (mfxU8)size; //payload size
}

void PackPTPayload(BitstreamWriter& rbsp, MfxVideoParam const & par, Task const & task)
{
    PicTimingSEI pt = {};

    if (par.isField())
    {
        switch (task.m_surf->Info.PicStruct)
        {
        case mfxU16(MFX_PICSTRUCT_FIELD_TOP):
            pt.pic_struct = 1;
            pt.source_scan_type = 0;
            break;
        case mfxU16(MFX_PICSTRUCT_FIELD_BOTTOM):
            pt.pic_struct = 2;
            pt.source_scan_type = 0;
            break;
        case mfxU16(MFX_PICSTRUCT_FIELD_TOP| MFX_PICSTRUCT_FIELD_PAIRED_PREV):
            pt.pic_struct = 9;
            pt.source_scan_type = 0;
            break;
        case mfxU16(MFX_PICSTRUCT_FIELD_TOP|MFX_PICSTRUCT_FIELD_PAIRED_NEXT):
            pt.pic_struct = 11;
            pt.source_scan_type = 0;
            break;
        case mfxU16(MFX_PICSTRUCT_FIELD_BOTTOM| MFX_PICSTRUCT_FIELD_PAIRED_PREV):
            pt.pic_struct = 10;
            pt.source_scan_type = 0;
            break;
        case mfxU16(MFX_PICSTRUCT_FIELD_BOTTOM| MFX_PICSTRUCT_FIELD_PAIRED_NEXT):
            pt.pic_struct = 12;
            pt.source_scan_type = 0;
            break;
         default:
            pt.pic_struct = ((!!(par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_BFF)) == task.m_secondField) ? 1 : 2;
            pt.source_scan_type = 0;
            break;
        }
    }
    else if (par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE)
    {
        switch (task.m_surf->Info.PicStruct)
        {
        case mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FRAME_DOUBLING):
            pt.pic_struct = 7;
            pt.source_scan_type = 1;
            break;
        case mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FRAME_TRIPLING):
            pt.pic_struct = 8;
            pt.source_scan_type = 1;
            break;
        default:
            pt.pic_struct = 0;
            pt.source_scan_type = 1;
            break;
        }
    }
    else
    {
        switch (task.m_surf->Info.PicStruct)
        {
        case mfxU16(MFX_PICSTRUCT_FIELD_TFF):
            pt.pic_struct = 3;
            pt.source_scan_type = 0;
            break;
        case mfxU16(MFX_PICSTRUCT_FIELD_BFF):
            pt.pic_struct = 4;
            pt.source_scan_type = 0;
            break;
        case mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FIELD_TFF):
            pt.pic_struct = 3;
            pt.source_scan_type = 1;
            break;
        case mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FIELD_BFF):
            pt.pic_struct = 4;
            pt.source_scan_type = 1;
            break;
        case mfxU16(MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_REPEATED):
            pt.pic_struct = 5;
            pt.source_scan_type = 0;
            break;
        case mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_REPEATED):
            pt.pic_struct = 5;
            pt.source_scan_type = 1;
            break;
        case mfxU16(MFX_PICSTRUCT_FIELD_BFF | MFX_PICSTRUCT_FIELD_REPEATED):
            pt.pic_struct = 6;
            pt.source_scan_type = 0;
            break;
        case mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FIELD_BFF | MFX_PICSTRUCT_FIELD_REPEATED):
            pt.pic_struct = 6;
            pt.source_scan_type = 1;
            break;

        default:
            pt.pic_struct = 0;
            pt.source_scan_type = 2;
            break;
        }
    }

    pt.duplicate_flag = 0;

    pt.au_cpb_removal_delay_minus1 = Max(task.m_cpb_removal_delay, 1U) - 1;
    pt.pic_dpb_output_delay        = task.m_dpb_output_delay;

    mfxU32 size = CeilDiv(rbsp.GetOffset(), 8);
    mfxU8* pl = rbsp.GetStart() + size; // payload start

    rbsp.PutBits(8, 1);    //payload type
    rbsp.PutBits(8, 0xFF); //place for payload  size
    size += 2;

    HeaderPacker::PackSEIPayload(rbsp, par.m_sps.vui, pt);

    size = CeilDiv(rbsp.GetOffset(), 8) - size;

    assert(size < 256);
    pl[1] = (mfxU8)size; //payload size
}

struct PLTypeEq
{
    mfxU16 m_type;
    PLTypeEq(mfxU16 type) { m_type = type; }
    inline bool operator() (const mfxPayload* pl) { return (pl && (pl->Type == m_type)); }
};

void HeaderPacker::GetPrefixSEI(Task const & task, mfxU8*& buf, mfxU32& sizeInBytes)
{
    NALU nalu = {1, PREFIX_SEI_NUT, 0, 1};
    BitstreamWriter rbsp(m_rbsp, sizeof(m_rbsp));
    std::list<const mfxPayload*> prefixPL;
    std::list<const mfxPayload*>::iterator plIt;
    mfxU32 prevNALUBytes = 0;
    mfxPayload BPSEI = {}, PTSEI = {};
    bool insertBP = false, insertPT = false;


    for (mfxU16 i = 0; i < task.m_ctrl.NumPayload; i++)
    {
        if (!(task.m_ctrl.Payload[i]->CtrlFlags & MFX_PAYLOAD_CTRL_SUFFIX))
            prefixPL.push_back(task.m_ctrl.Payload[i]);
    }

    if (m_par->mfx.RateControlMethod != MFX_RATECONTROL_CQP)
    {
        prefixPL.remove_if(PLTypeEq(0)); //buffering_period
        prefixPL.remove_if(PLTypeEq(1)); //pic_timing
    }

    if (prefixPL.empty() && !(task.m_insertHeaders & INSERT_SEI))
    {
        buf         = 0;
        sizeInBytes = 0;
        goto exit;
    }

    buf         = m_bs_ssh;
    sizeInBytes = sizeof(m_bs_ssh);

    /* An SEI NAL unit containing an active parameter sets SEI message shall contain only
    one active parameter sets SEI message and shall not contain any other SEI messages. */
    /* When an SEI NAL unit containing an active parameter sets SEI message is present
    in an access unit, it shall be the first SEI NAL unit that follows the prevVclNalUnitInAu
    of the SEI NAL unit and precedes the nextVclNalUnitInAu of the SEI NAL unit. */
    plIt = std::find_if(prefixPL.begin(), prefixPL.end(), PLTypeEq(129));

    if (plIt != prefixPL.end())
    {
        const mfxPayload& pl = **plIt;

        PackNALU(rbsp, nalu);
        rbsp.PutBitsBuffer(pl.NumBit, pl.Data);
        rbsp.PutTrailingBits(true);
        rbsp.PutTrailingBits();

        prefixPL.remove_if(PLTypeEq(129));

        if (MFX_ERR_NONE != PackRBSP(buf, m_rbsp, sizeInBytes, CeilDiv(rbsp.GetOffset(), 8)))
        {
            buf         = 0;
            sizeInBytes = 0;
        }

        if (!buf || (prefixPL.empty() && !(task.m_insertHeaders & INSERT_SEI)))
            goto exit;

        buf += sizeInBytes;
        prevNALUBytes += sizeInBytes;
        sizeInBytes = sizeof(m_bs_ssh) - prevNALUBytes;

        rbsp.Reset();
    }
    
    plIt = std::find_if(prefixPL.begin(), prefixPL.end(), PLTypeEq(1));
    insertPT = plIt != prefixPL.end();

    if ((task.m_insertHeaders & INSERT_PTSEI) && !insertPT)
    {
        PTSEI.Type   = 1;
        PTSEI.NumBit = rbsp.GetOffset();
        PTSEI.Data   = rbsp.GetStart() + PTSEI.NumBit / 8;

        PackPTPayload(rbsp, *m_par, task);

        PTSEI.NumBit  = rbsp.GetOffset() - PTSEI.NumBit;
        PTSEI.BufSize = (mfxU16)CeilDiv(PTSEI.NumBit, 8);

        prefixPL.push_front(&PTSEI);
        rbsp.PutTrailingBits(true);

        insertPT = true;
    }

    plIt = std::find_if(prefixPL.begin(), prefixPL.end(), PLTypeEq(0));
    insertBP = plIt != prefixPL.end();

    if ((task.m_insertHeaders & INSERT_BPSEI) && !insertBP)
    {
        BPSEI.Type   = 0;
        BPSEI.NumBit = rbsp.GetOffset();
        BPSEI.Data   = rbsp.GetStart() + BPSEI.NumBit / 8;

        PackBPPayload(rbsp, *m_par, task);

        BPSEI.NumBit  = rbsp.GetOffset() - BPSEI.NumBit;
        BPSEI.BufSize = (mfxU16)CeilDiv(BPSEI.NumBit, 8);

        prefixPL.push_front(&BPSEI);
        rbsp.PutTrailingBits(true);

        insertBP = true;
    }

    /* When an SEI NAL unit contains a non-nested buffering period SEI message,
    a non-nested picture timing SEI message, or a non-nested decoding unit information
    SEI message, the SEI NAL unit shall not contain any other SEI message with payloadType
    not equal to 0 (buffering period), 1 (picture timing) or 130 (decoding unit information). */
    if (insertBP || insertPT || prefixPL.end() != std::find_if(prefixPL.begin(), prefixPL.end(), PLTypeEq(130)))
    {
        mfxU32 rbspOffset = rbsp.GetOffset() / 8;
        mfxU8* rbspStart  = rbsp.GetStart() + rbspOffset;

        PackNALU(rbsp, nalu);

        for (plIt = prefixPL.begin(); plIt != prefixPL.end(); plIt++)
        {
            if (!*plIt)
                continue;

            const mfxPayload& pl = **plIt;

            if (pl.Type == 0 || pl.Type == 1 || pl.Type == 130)
            {
                rbsp.PutBitsBuffer(pl.NumBit, pl.Data);
                rbsp.PutTrailingBits(true);
            }
        }

        prefixPL.remove_if(PLTypeEq(0));
        prefixPL.remove_if(PLTypeEq(1));
        prefixPL.remove_if(PLTypeEq(130));
        rbsp.PutTrailingBits();

        if (MFX_ERR_NONE != PackRBSP(buf, rbspStart, sizeInBytes, CeilDiv(rbsp.GetOffset(), 8) - rbspOffset))
        {
            buf         = 0;
            sizeInBytes = 0;
        }

        if (!buf || prefixPL.empty())
            goto exit;

        buf += sizeInBytes;
        prevNALUBytes += sizeInBytes;
        sizeInBytes = sizeof(m_bs_ssh) - prevNALUBytes;

        rbsp.Reset();
    }

    PackNALU(rbsp, nalu);

    for (plIt = prefixPL.begin(); plIt != prefixPL.end(); plIt++)
    {
        if (!*plIt)
            continue;

        rbsp.PutBitsBuffer((*plIt)->NumBit, (*plIt)->Data);
        rbsp.PutTrailingBits(true);
    }

    rbsp.PutTrailingBits();


    if (MFX_ERR_NONE != PackRBSP(buf, m_rbsp, sizeInBytes, CeilDiv(rbsp.GetOffset(), 8)))
    {
        buf         = 0;
        sizeInBytes = 0;
    }

exit:
    if (buf)
    {
        buf          = m_bs_ssh;
        sizeInBytes += prevNALUBytes;
    }

    m_bs.Reset(m_bs_ssh + sizeInBytes, sizeof(m_bs_ssh) - sizeInBytes);
}

void HeaderPacker::GetSuffixSEI(Task const & task, mfxU8*& buf, mfxU32& sizeInBytes)
{
    NALU nalu = {0, SUFFIX_SEI_NUT, 0, 1};
    BitstreamWriter rbsp(m_rbsp, sizeof(m_rbsp));
    std::list<const mfxPayload*> suffixPL, prefixPL;
    std::list<const mfxPayload*>::iterator plIt;

    for (mfxU16 i = 0; i < task.m_ctrl.NumPayload; i++)
    {
        if (task.m_ctrl.Payload[i]->CtrlFlags & MFX_PAYLOAD_CTRL_SUFFIX)
            suffixPL.push_back(task.m_ctrl.Payload[i]);
        else
            prefixPL.push_back(task.m_ctrl.Payload[i]);
    }

     /* It is a requirement of bitstream conformance that when a prefix SEI message with payloadType
     equal to 17 (progressive refinement segment end) or 22 (post-filter hint) is present in an access unit,
     a suffix SEI message with the same value of payloadType shall not be present in the same access unit.*/
    if (prefixPL.end() != std::find_if(prefixPL.begin(), prefixPL.end(), PLTypeEq(17)))
        suffixPL.remove_if(PLTypeEq(17));
    if (prefixPL.end() != std::find_if(prefixPL.begin(), prefixPL.end(), PLTypeEq(22)))
        suffixPL.remove_if(PLTypeEq(22));

    if (suffixPL.empty())
    {
        buf         = 0;
        sizeInBytes = 0;
        return;
    }

    PackNALU(rbsp, nalu);

    for (plIt = suffixPL.begin(); plIt != suffixPL.end(); plIt++)
    {
        if (!*plIt)
            continue;

        rbsp.PutBitsBuffer((*plIt)->NumBit, (*plIt)->Data);
        rbsp.PutTrailingBits(true);
    }

    rbsp.PutTrailingBits();

    buf         = m_bs.GetStart() + CeilDiv(m_bs.GetOffset(), 8);
    sizeInBytes = sizeof(m_bs_ssh) - mfxU32(buf - m_bs.GetStart());

    if (MFX_ERR_NONE != PackRBSP(buf, m_rbsp, sizeInBytes, CeilDiv(rbsp.GetOffset(), 8)))
    {
        buf         = 0;
        sizeInBytes = 0;
    }
}

void HeaderPacker::GetSSH(
    Task const & task,
    mfxU32 id,
    mfxU8*& buf,
    mfxU32& sizeInBits,
    mfxU32* qpd_offset,
    bool dyn_slice_size,
    mfxU32* sao_offset,
    mfxU16* ssh_start_len,
    mfxU32* ssh_offset,
    mfxU32* pwt_offset,
    mfxU32* pwt_length)
{
    BitstreamWriter& rbsp = m_bs;
    bool LongStartCode = (id == 0 && task.m_insertHeaders == 0) || IsOn(m_par->m_ext.DDI.LongStartCodes);
    NALU nalu = { mfxU16(LongStartCode), task.m_shNUT, 0, mfxU16(task.m_tid + 1) };

    assert(m_par);
    assert(id < m_par->m_slice.size());

    if (id == 0 && !(task.m_insertHeaders & INSERT_SEI) && task.m_ctrl.NumPayload == 0)
        rbsp.Reset(m_bs_ssh, sizeof(m_bs_ssh));

    Slice sh = task.m_sh;
    sh.first_slice_segment_in_pic_flag = (id == 0);
    sh.segment_address = m_par->m_slice[id].SegmentAddress;

    buf = m_bs.GetStart() + CeilDiv(rbsp.GetOffset(), 8);

    PackSSH(rbsp, nalu, m_par->m_sps, m_par->m_pps, sh, qpd_offset, dyn_slice_size, sao_offset, pwt_offset, pwt_length);
    if (dyn_slice_size) {
        *ssh_start_len = (nalu.long_start_code) ? 48 : 40;
        *ssh_offset = 0;
    }

    mfxU32 SEIbits = (mfxU32)(buf - m_bs.GetStart()) * 8;
    if (qpd_offset)
        *qpd_offset -= SEIbits;
    if (sao_offset)
        *sao_offset -= SEIbits;
    if (pwt_offset)
        *pwt_offset -= SEIbits;

    sizeInBits = rbsp.GetOffset() - (mfxU32)(buf - m_bs.GetStart()) * 8;
}

void HeaderPacker::codingTree(mfxU32 xCtu, mfxU32 yCtu, mfxU32 log2CtuSize, BitstreamWriter& bs, const Slice& slice, mfxU32 x0, mfxU32 y0, mfxU8* tabl)
{
    bool mbA = (yCtu == y0) ? xCtu > x0 : xCtu > 0; //is left MB available
    bool mbB = (yCtu == y0) ? false : ((xCtu >= x0) ? yCtu > y0 : yCtu > y0 + (1 << log2CtuSize));   //is above MB available
    bool boundary = ((xCtu + (1 << log2CtuSize) > m_par->m_sps.pic_width_in_luma_samples) || (yCtu + (1 << log2CtuSize) > m_par->m_sps.pic_height_in_luma_samples)) && (log2CtuSize > m_par->m_sps.log2_min_luma_coding_block_size_minus3 + 3);
    mfxU32 ctx = 0;
    mfxU8 split_flag = 0;
    mfxU8 symbol = 0;

    if (!boundary){
        bs.EncodeBin(CONTEXT(tabl, SPLIT_CODING_UNIT_FLAG_HEVC) + ctx, split_flag);
    }
    else
    {
        split_flag = (log2CtuSize > m_par->m_sps.log2_min_luma_coding_block_size_minus3 + 3) ? 1 : 0;
    }

    if (split_flag)
    {
        mfxU32 x1 = xCtu + (1 << (log2CtuSize - 1));
        mfxU32 y1 = yCtu + (1 << (log2CtuSize - 1));
        codingTree(xCtu, yCtu, log2CtuSize - 1, bs, slice, x0, y0, tabl);
        if (x1 < m_par->m_sps.pic_width_in_luma_samples)
        {
            codingTree(x1, yCtu, log2CtuSize - 1, bs, slice, x0, y0, tabl);
        }
        if (y1 < m_par->m_sps.pic_height_in_luma_samples)
        {
            codingTree(xCtu, y1, log2CtuSize - 1, bs, slice, x0, y0, tabl);
        }
        if (x1 < m_par->m_sps.pic_width_in_luma_samples && y1 < m_par->m_sps.pic_height_in_luma_samples)
        {
            codingTree(x1, y1, log2CtuSize - 1, bs, slice, x0, y0, tabl);
        }
    }
    else
    {
        if (m_par->m_pps.transquant_bypass_enabled_flag)
        {
            symbol = 0;
            bs.EncodeBin(CONTEXT(tabl, CU_TRANSQUANT_BYPASS_FLAG_CTX), symbol);
        }

        symbol = 1;
        mfxU32 ctx_skip = mbA ? 1 : 0;
        ctx_skip += mbB ? 1 : 0;

        bs.EncodeBin(CONTEXT(tabl, SKIP_FLAG_HEVC) + ctx_skip, symbol);

        mfxU32 num_cand = 5 - slice.five_minus_max_num_merge_cand;
        mfxU32 unary_idx = 0;
        if (num_cand > 1)
        {
            for (mfxU32 i = 0; i < num_cand - 1; ++i)
            {
                symbol = i == unary_idx ? 0 : 1;
                if (i == 0)
                {
                    bs.EncodeBin(CONTEXT(tabl, MERGE_IDX_HEVC), symbol);
                }
                else
                {
                    bs.EncodeBinEP(symbol);
                }
                if (symbol == 0)
                {
                    break;
                }
            }
        }
    }
}


void HeaderPacker::GetSkipSlice(Task const & task, mfxU32 id, mfxU8*& buf, mfxU32& sizeInBytes, mfxU32* qpd_offset)
{
    BitstreamWriter& rbsp = m_bs;
    bool LongStartCode = (id == 0 && task.m_insertHeaders == 0) || IsOn(m_par->m_ext.DDI.LongStartCodes);
    NALU nalu = { mfxU16(LongStartCode), task.m_shNUT, 0, mfxU16(task.m_tid + 1) };

    assert(m_par);
    assert(id < m_par->m_slice.size());

    if (id == 0 && !(task.m_insertHeaders & INSERT_SEI) && task.m_ctrl.NumPayload == 0)
        rbsp.Reset(m_bs_ssh, sizeof(m_bs_ssh));

    Slice sh = task.m_sh;
    sh.first_slice_segment_in_pic_flag = (id == 0);
    sh.segment_address = m_par->m_slice[id].SegmentAddress;

    buf = m_bs.GetStart() + CeilDiv(rbsp.GetOffset(), 8);

    PackSSH(rbsp, nalu, m_par->m_sps, m_par->m_pps, sh, qpd_offset);
    const mfxU32 MaxCU = (1 << (m_par->m_sps.log2_min_luma_coding_block_size_minus3 + 3 + m_par->m_sps.log2_diff_max_min_luma_coding_block_size));
    const mfxU32 picWidthInMB = CeilDiv(m_par->m_sps.pic_width_in_luma_samples, MaxCU);
    //cabac init
    mfxI8 SliceQPy = mfxI8(m_par->m_pps.init_qp_minus26 + 26 + sh.slice_qp_delta);//task.m_qpY;

    mfxU32 l = 0;
    SliceQPy = MFX_MAX(0, SliceQPy);
    m_bs.cabacInit();
    mfxU8 frameType = 0;
    switch (sh.type){
    case 2:
    {
        frameType = 0;
        break;
    }
    case 1:
    {
        frameType = 1;
        break;
    }
    case 0:
    {
        frameType = 2;
        break;
    }
    default:
        frameType = 2;
    }
    mfxU8 initializationType = mfxU8(frameType);

    mfxU8 context_array[NUM_CABAC_CONTEXT];

    //transquant_bypass_flag
    for (l = 0; l < tab_ctxIdxSize[CU_TRANSQUANT_BYPASS_FLAG_CTX]; l++)
    {
        InitializeContext(context_array + tab_ctxIdxOffset[CU_TRANSQUANT_BYPASS_FLAG_CTX] + l,
            initVal_for_transquant_bypass_flag[initializationType][l], SliceQPy);
    }

    //split_coding_unit_flag
    for (l = 0; l < tab_ctxIdxSize[SPLIT_CODING_UNIT_FLAG_HEVC]; l++)
    {
        InitializeContext(context_array + tab_ctxIdxOffset[SPLIT_CODING_UNIT_FLAG_HEVC] + l,
            initVal_for_split_coding_unit_flag[initializationType][l], SliceQPy);
    }

    //skip_flag
    for (l = 0; l < tab_ctxIdxSize[SKIP_FLAG_HEVC]; l++)
    {
        InitializeContext(context_array + tab_ctxIdxOffset[SKIP_FLAG_HEVC] + l,
            initVal_for_skip_flag[initializationType][l], SliceQPy);
    }

    //merge_idx
    for (l = 0; l < tab_ctxIdxSize[MERGE_IDX_HEVC]; l++)
    {
        InitializeContext(context_array + tab_ctxIdxOffset[MERGE_IDX_HEVC] + l,
            initVal_for_merge_idx[initializationType][l], SliceQPy);
    }

    context_array[tab_ctxIdxOffset[END_OF_SLICE_FLAG_HEVC]] = 63;
    const mfxU32 log2CtuSize = m_par->m_sps.log2_min_luma_coding_block_size_minus3 + 3 + m_par->m_sps.log2_diff_max_min_luma_coding_block_size;
    mfxU32 xCtu0 = (m_par->m_slice[id].SegmentAddress % picWidthInMB) << log2CtuSize;
    mfxU32 yCtu0 = (m_par->m_slice[id].SegmentAddress / picWidthInMB) << log2CtuSize;
    mfxU32 ctuAdrr = m_par->m_slice[id].SegmentAddress + m_par->m_slice[id].NumLCU;
    for (mfxU32 ctu = m_par->m_slice[id].SegmentAddress; ctu < ctuAdrr - 1; ++ctu){
        mfxU32 xCtu = (ctu % picWidthInMB) << log2CtuSize;
        mfxU32 yCtu = (ctu / picWidthInMB) << log2CtuSize;

        codingTree(xCtu, yCtu, log2CtuSize, m_bs, sh, xCtu0, yCtu0, &context_array[0]);

        m_bs.EncodeBin(CONTEXT(&context_array[0], END_OF_SLICE_FLAG_HEVC), 0);
    }
    mfxU32 ctu = ctuAdrr - 1;//PicSizeInCtbsY - 1;
    mfxU32 xCtu = (ctu % picWidthInMB) << log2CtuSize;
    mfxU32 yCtu = (ctu / picWidthInMB) << log2CtuSize;

    codingTree(xCtu, yCtu, log2CtuSize, m_bs, sh, xCtu0, yCtu0, &context_array[0]);

    m_bs.SliceFinish();

    if (qpd_offset)
        *qpd_offset -= (mfxU32)(buf - m_bs.GetStart()) * 8;

    sizeInBytes = CeilDiv(rbsp.GetOffset(), 8) - (mfxU32)(buf - m_bs.GetStart());
}

} //MfxHwH265Encode
#endif
