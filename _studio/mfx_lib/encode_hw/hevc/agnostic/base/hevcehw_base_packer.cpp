// Copyright (c) 2020 Intel Corporation
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

#include "hevcehw_base_packer.h"
#include <numeric>

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

const mfxU8 tab_cabacRangeTabLps[128][4] =
{
    { 128, 176, 208, 240 }, { 128, 167, 197, 227 }, { 128, 158, 187, 216 }, { 123, 150, 178, 205 },
    { 116, 142, 169, 195 }, { 111, 135, 160, 185 }, { 105, 128, 152, 175 }, { 100, 122, 144, 166 },
    {  95, 116, 137, 158 }, {  90, 110, 130, 150 }, {  85, 104, 123, 142 }, {  81,  99, 117, 135 },
    {  77,  94, 111, 128 }, {  73,  89, 105, 122 }, {  69,  85, 100, 116 }, {  66,  80,  95, 110 },
    {  62,  76,  90, 104 }, {  59,  72,  86,  99 }, {  56,  69,  81,  94 }, {  53,  65,  77,  89 },
    {  51,  62,  73,  85 }, {  48,  59,  69,  80 }, {  46,  56,  66,  76 }, {  43,  53,  63,  72 },
    {  41,  50,  59,  69 }, {  39,  48,  56,  65 }, {  37,  45,  54,  62 }, {  35,  43,  51,  59 },
    {  33,  41,  48,  56 }, {  32,  39,  46,  53 }, {  30,  37,  43,  50 }, {  29,  35,  41,  48 },
    {  27,  33,  39,  45 }, {  26,  31,  37,  43 }, {  24,  30,  35,  41 }, {  23,  28,  33,  39 },
    {  22,  27,  32,  37 }, {  21,  26,  30,  35 }, {  20,  24,  29,  33 }, {  19,  23,  27,  31 },
    {  18,  22,  26,  30 }, {  17,  21,  25,  28 }, {  16,  20,  23,  27 }, {  15,  19,  22,  25 },
    {  14,  18,  21,  24 }, {  14,  17,  20,  23 }, {  13,  16,  19,  22 }, {  12,  15,  18,  21 },
    {  12,  14,  17,  20 }, {  11,  14,  16,  19 }, {  11,  13,  15,  18 }, {  10,  12,  15,  17 },
    {  10,  12,  14,  16 }, {   9,  11,  13,  15 }, {   9,  11,  12,  14 }, {   8,  10,  12,  14 },
    {   8,   9,  11,  13 }, {   7,   9,  11,  12 }, {   7,   9,  10,  12 }, {   7,   8,  10,  11 },
    {   6,   8,   9,  11 }, {   6,   7,   9,  10 }, {   6,   7,   8,   9 }, {   2,   2,   2,   2 },
    //The same for valMPS=1
    { 128, 176, 208, 240 }, { 128, 167, 197, 227 }, { 128, 158, 187, 216 }, { 123, 150, 178, 205 },
    { 116, 142, 169, 195 }, { 111, 135, 160, 185 }, { 105, 128, 152, 175 }, { 100, 122, 144, 166 },
    {  95, 116, 137, 158 }, {  90, 110, 130, 150 }, {  85, 104, 123, 142 }, {  81,  99, 117, 135 },
    {  77,  94, 111, 128 }, {  73,  89, 105, 122 }, {  69,  85, 100, 116 }, {  66,  80,  95, 110 },
    {  62,  76,  90, 104 }, {  59,  72,  86,  99 }, {  56,  69,  81,  94 }, {  53,  65,  77,  89 },
    {  51,  62,  73,  85 }, {  48,  59,  69,  80 }, {  46,  56,  66,  76 }, {  43,  53,  63,  72 },
    {  41,  50,  59,  69 }, {  39,  48,  56,  65 }, {  37,  45,  54,  62 }, {  35,  43,  51,  59 },
    {  33,  41,  48,  56 }, {  32,  39,  46,  53 }, {  30,  37,  43,  50 }, {  29,  35,  41,  48 },
    {  27,  33,  39,  45 }, {  26,  31,  37,  43 }, {  24,  30,  35,  41 }, {  23,  28,  33,  39 },
    {  22,  27,  32,  37 }, {  21,  26,  30,  35 }, {  20,  24,  29,  33 }, {  19,  23,  27,  31 },
    {  18,  22,  26,  30 }, {  17,  21,  25,  28 }, {  16,  20,  23,  27 }, {  15,  19,  22,  25 },
    {  14,  18,  21,  24 }, {  14,  17,  20,  23 }, {  13,  16,  19,  22 }, {  12,  15,  18,  21 },
    {  12,  14,  17,  20 }, {  11,  14,  16,  19 }, {  11,  13,  15,  18 }, {  10,  12,  15,  17 },
    {  10,  12,  14,  16 }, {   9,  11,  13,  15 }, {   9,  11,  12,  14 }, {   8,  10,  12,  14 },
    {   8,   9,  11,  13 }, {   7,   9,  11,  12 }, {   7,   9,  10,  12 }, {   7,   8,  10,  11 },
    {   6,   8,   9,  11 }, {   6,   7,   9,  10 }, {   6,   7,   8,   9 }, {   2,   2,   2,   2 }
};

/* CABAC trans tables: state (MPS and LPS ) + valMPS in 6th bit */
const mfxU8 tab_cabacTransTbl[2][128] =
{
    {
          1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,
         17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
         33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
         49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  62,  63,
          0,  64,  65,  66,  66,  68,  68,  69,  70,  71,  72,  73,  73,  75,  75,  76,
         77,  77,  79,  79,  80,  80,  82,  82,  83,  83,  85,  85,  86,  86,  87,  88,
         88,  89,  90,  90,  91,  91,  92,  93,  93,  94,  94,  94,  95,  96,  96,  97,
         97,  97,  98,  98,  99,  99,  99, 100, 100, 100, 101, 101, 101, 102, 102, 127
    },
    {
           0,   0,   1,   2,   2,   4,   4,   5,   6,   7,   8,   9,   9,  11,  11,  12,
          13,  13,  15,  15,  16,  16,  18,  18,  19,  19,  21,  21,  22,  22,  23,  24,
          24,  25,  26,  26,  27,  27,  28,  29,  29,  30,  30,  30,  31,  32,  32,  33,
          33,  33,  34,  34,  35,  35,  35,  36,  36,  36,  37,  37,  37,  38,  38,  63,
          65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
          81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,
          97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
         113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 126, 127
    }
};

const CABACContextTable CABACContext::InitVal[3] =
{
    {
        { 154 },            /*CU_TRANSQUANT_BYPASS_FLAG*/
        { 139, 141, 157 },  /*SPLIT_CODING_UNIT_FLAG*/
        { 154, 154, 154 },  /*SKIP_FLAG*/
        { 154 },            /*MERGE_IDX*/
        { 63 }              /*END_OF_SLICE_FLAG*/
    },
    {
        { 154 },            /*CU_TRANSQUANT_BYPASS_FLAG*/
        { 107, 139, 126 },  /*SPLIT_CODING_UNIT_FLAG*/
        { 197, 185, 201 },  /*SKIP_FLAG*/
        { 122 },            /*MERGE_IDX*/
        { 63 }              /*END_OF_SLICE_FLAG*/
    },
    {
        { 154 },            /*CU_TRANSQUANT_BYPASS_FLAG*/
        { 107, 139, 126 },  /*SPLIT_CODING_UNIT_FLAG*/
        { 197, 185, 201 },  /*SKIP_FLAG*/
        { 137 },            /*MERGE_IDX*/
        { 63 }              /*END_OF_SLICE_FLAG*/
    }
};

void CABACContext::Init(const PPS& pps, const Slice& sh)
{
    mfxU8* pState    = (mfxU8*)&((CABACContextTable&)*this);
    mfxU8* pStateEnd = pState + sizeof(CABACContextTable);
    mfxI32 SliceQpY = mfx::clamp(26 + pps.init_qp_minus26 + sh.slice_qp_delta, 0, 51);
    mfxU32 initType =
          (sh.type == 1)                 * (sh.cabac_init_flag + 1)
        + (sh.type != 2 && sh.type != 1) * (2 - sh.cabac_init_flag);

    const mfxU8* pInitVal = (const mfxU8*)&InitVal[initType];

    while (pState != pStateEnd)
    {
        mfxI32 preCtxState = mfx::clamp(((((pInitVal[0] >> 4) * 5 - 45) * SliceQpY) >> 4) + ((pInitVal[0] & 15) << 3) - 16, 1, 126);

        *pState = mfxU8(
              (preCtxState <= 63) * ((63 - preCtxState) << 1)
            + (preCtxState > 63)  * (((preCtxState - 64) << 1) | 1));

        ++pState;
        ++pInitVal;
    }

    END_OF_SLICE_FLAG[0] = (63 << 1);
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

    auto SkipOffsetBytes = [&]()
    {
        N = o / 8;
        b += N;
        o &= 7;
        return o;
    };
    auto PutBitsAfterOffsetOnes = [&]()
    {
        N = (n < (8 - o)) * n;
        PutBits(8 - o, ((b[0] & (0xff >> o)) >> N));
        n -= (N + !N * (8 - o));
        ++b;
        return n;
    };
    auto PutBytesAligned = [&]()
    {
        N = n / 8;
        n &= 7;

        assert(N + !!n < (m_bsEnd - m_bs));
        std::copy(b, b + N, m_bs);

        m_bs += N;

        return !n;
    };
    auto PutLastByteBitsAligned = [&]()
    {
        m_bs[0] = b[N];
        m_bs[0] &= (0xff << (8 - n));
        m_bitOffset = (mfxU8)n;
        return true;
    };
    auto CopyAlignedToUnaligned = [&]()
    {
        assert((n + 7 - m_bitOffset) / 8 < (m_bsEnd - m_bs));

        while (n >= 24)
        {
            B = ((((mfxU32)b[0] << 24) | ((mfxU32)b[1] << 16) | ((mfxU32)b[2] << 8)) >> m_bitOffset);

            m_bs[0] |= (mfxU8)(B >> 24);
            m_bs[1] = (mfxU8)(B >> 16);
            m_bs[2] = (mfxU8)(B >> 8);
            m_bs[3] = (mfxU8)B;

            m_bs += 3;
            b += 3;
            n -= 24;
        }

        while (n >= 8)
        {
            B = ((mfxU32)b[0] << 8) >> m_bitOffset;

            m_bs[0] |= (mfxU8)(B >> 8);
            m_bs[1] = (mfxU8)B;

            m_bs++;
            b++;
            n -= 8;
        }

        if (n)
            PutBits(n, (b[0] >> (8 - n)));

        return true;
    };
    auto CopyUnalignedPartToAny = [&]()
    {
        return o && SkipOffsetBytes() && PutBitsAfterOffsetOnes();
    };
    auto CopyAlignedToAligned = [&]()
    {
        return !m_bitOffset && (PutBytesAligned() || PutLastByteBitsAligned());
    };

    bool bDone =
           CopyUnalignedPartToAny()
        || CopyAlignedToAligned()
        || CopyAlignedToUnaligned();

    ThrowAssert(!bDone, "BitstreamWriter::PutBitsBuffer failed");
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

void BitstreamWriter::EncodeBin(mfxU8& ctx, mfxU8 binVal)
{
    mfxU8  pStateIdx = (ctx >> 1);
    mfxU8  valMPS = (ctx & 1);
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

    ctx = (pStateIdx << 1) | valMPS;

    RenormE();
    m_BinCountsInNALunits++;
}

void BitstreamWriter::EncodeBinEP(mfxU8 binVal)
{
    m_codILow += m_codILow + m_codIRange * (binVal == 1);
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


void Packer::PackNALU(BitstreamWriter& bs, NALU const & h)
{
    bool bLongSC =
        h.nal_unit_type == VPS_NUT
        || h.nal_unit_type == SPS_NUT
        || h.nal_unit_type == PPS_NUT
        || h.nal_unit_type == AUD_NUT
        || h.nal_unit_type == PREFIX_SEI_NUT
        || h.long_start_code;

    if (bLongSC)
        bs.PutBits(8, 0); //zero_byte

    bs.PutBits( 24, 0x000001);//start_code

    bs.PutBit(0);
    bs.PutBits(6, h.nal_unit_type);
    bs.PutBits(6, h.nuh_layer_id);
    bs.PutBits(3, h.nuh_temporal_id_plus1);
}

void Packer::PackPTL(BitstreamWriter& bs, LayersInfo const & ptl, mfxU16 max_sub_layers_minus1)
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
    bs.PutBit(ptl.general.constraint.max_12bit       );
    bs.PutBit(ptl.general.constraint.max_10bit       );
    bs.PutBit(ptl.general.constraint.max_8bit        );
    bs.PutBit(ptl.general.constraint.max_422chroma   );
    bs.PutBit(ptl.general.constraint.max_420chroma   );
    bs.PutBit(ptl.general.constraint.max_monochrome  );
    bs.PutBit(ptl.general.constraint.intra           );
    bs.PutBit(ptl.general.constraint.one_picture_only);
    bs.PutBit(ptl.general.constraint.lower_bit_rate  );
    bs.PutBits(23, 0);
    bs.PutBits(11, 0);
    bs.PutBit(ptl.general.inbld_flag);
    bs.PutBits(8, ptl.general.level_idc);

    std::for_each(ptl.sub_layer, ptl.sub_layer + max_sub_layers_minus1
        , [&](const LayersInfo::SubLayer& sl)
    {
        bs.PutBit(sl.profile_present_flag);
        bs.PutBit(sl.level_present_flag);
    });

    if (max_sub_layers_minus1 > 0)
        bs.PutBits(2 * (8 - max_sub_layers_minus1), 0); // reserved_zero_2bits[ i ]

    std::for_each(ptl.sub_layer, ptl.sub_layer + max_sub_layers_minus1
        , [&](const LayersInfo::SubLayer& sl)
    {
        if (sl.profile_present_flag)
        {
            bs.PutBits(2, sl.profile_space);
            bs.PutBit(sl.tier_flag);
            bs.PutBits(5, sl.profile_idc);
            bs.PutBits(24, (sl.profile_compatibility_flags >> 8));
            bs.PutBits(8, (sl.profile_compatibility_flags & 0xFF));
            bs.PutBit(sl.progressive_source_flag);
            bs.PutBit(sl.interlaced_source_flag);
            bs.PutBit(sl.non_packed_constraint_flag);
            bs.PutBit(sl.frame_only_constraint_flag);
            bs.PutBit(ptl.general.constraint.max_12bit);
            bs.PutBit(ptl.general.constraint.max_10bit);
            bs.PutBit(ptl.general.constraint.max_8bit);
            bs.PutBit(ptl.general.constraint.max_422chroma);
            bs.PutBit(ptl.general.constraint.max_420chroma);
            bs.PutBit(ptl.general.constraint.max_monochrome);
            bs.PutBit(ptl.general.constraint.intra);
            bs.PutBit(ptl.general.constraint.one_picture_only);
            bs.PutBit(ptl.general.constraint.lower_bit_rate);
            bs.PutBits(23, 0);
            bs.PutBits(11, 0);
            bs.PutBit(sl.inbld_flag);
        }

        if (sl.level_present_flag)
            bs.PutBits(8, sl.level_idc);

    });
}

void Packer::PackSLO(BitstreamWriter& bs, LayersInfo const & slo, mfxU16 max_sub_layers_minus1)
{
    bs.PutBit(slo.sub_layer_ordering_info_present_flag);

    std::for_each(
        slo.sub_layer + (max_sub_layers_minus1 * !slo.sub_layer_ordering_info_present_flag)
        , slo.sub_layer + max_sub_layers_minus1 + 1
        , [&](const LayersInfo::SubLayer& sl)
    {
        bs.PutUE(sl.max_dec_pic_buffering_minus1);
        bs.PutUE(sl.max_num_reorder_pics);
        bs.PutUE(sl.max_latency_increase_plus1);
    });
}

void Packer::PackAUD(BitstreamWriter& bs, mfxU8 pic_type)
{
    NALU nalu = {0, AUD_NUT, 0, 1};
    PackNALU(bs, nalu);
    bs.PutBits(3, pic_type);
    bs.PutTrailingBits();
}

void Packer::PackVPS(BitstreamWriter& bs, VPS const &  vps)
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

void Packer::PackHRD(
    BitstreamWriter& bs,
    HRDInfo const & hrd,
    bool commonInfPresentFlag,
    mfxU16 maxNumSubLayersMinus1)
{
    mfxU32 nSE = 0;

    auto PackSubLayer = [&](const HRDInfo::SubLayer& sl)
    {
        nSE += PutBit(bs, sl.fixed_pic_rate_general_flag);
        nSE += !sl.fixed_pic_rate_general_flag && PutBit(bs, sl.fixed_pic_rate_within_cvs_flag);

        bool bFixedPicRate =
            sl.fixed_pic_rate_within_cvs_flag
            || sl.fixed_pic_rate_general_flag;

        nSE += bFixedPicRate && PutUE(bs, sl.elemental_duration_in_tc_minus1);
        nSE += !bFixedPicRate && PutBit(bs, sl.low_delay_hrd_flag);
        nSE += !sl.low_delay_hrd_flag && PutUE(bs, sl.cpb_cnt_minus1);

        std::for_each(
            sl.cpb
            , sl.cpb + ((sl.cpb_cnt_minus1 + 1) * hrd.nal_hrd_parameters_present_flag)
            , [&](const HRDInfo::SubLayer::CPB& cpb)
        {
            nSE += PutUE(bs, cpb.bit_rate_value_minus1);
            nSE += PutUE(bs, cpb.cpb_size_value_minus1);
            nSE += hrd.sub_pic_hrd_params_present_flag && PutUE(bs, cpb.cpb_size_du_value_minus1);
            nSE += hrd.sub_pic_hrd_params_present_flag && PutUE(bs, cpb.bit_rate_du_value_minus1);
            nSE += PutBit(bs, cpb.cbr_flag);
        });

        assert(hrd.vcl_hrd_parameters_present_flag == 0);
    };

    if (commonInfPresentFlag)
    {
        nSE += PutBit(bs, hrd.nal_hrd_parameters_present_flag);
        nSE += PutBit(bs, hrd.vcl_hrd_parameters_present_flag);

        bool bNalVclHrdPresent =
            hrd.nal_hrd_parameters_present_flag
            || hrd.vcl_hrd_parameters_present_flag;

        if (bNalVclHrdPresent)
        {
            nSE += PutBit(bs, hrd.sub_pic_hrd_params_present_flag);

            if (hrd.sub_pic_hrd_params_present_flag)
            {
                nSE += PutBits(bs, 8, hrd.tick_divisor_minus2);
                nSE += PutBits(bs, 5, hrd.du_cpb_removal_delay_increment_length_minus1);
                nSE += PutBit(bs, hrd.sub_pic_cpb_params_in_pic_timing_sei_flag);
                nSE += PutBits(bs, 5, hrd.dpb_output_delay_du_length_minus1);
            }

            nSE += PutBits(bs, 4, hrd.bit_rate_scale);
            nSE += PutBits(bs, 4, hrd.cpb_size_scale);
            nSE += hrd.sub_pic_hrd_params_present_flag && PutBits(bs, 4, hrd.cpb_size_du_scale);
            nSE += PutBits(bs, 5, hrd.initial_cpb_removal_delay_length_minus1);
            nSE += PutBits(bs, 5, hrd.au_cpb_removal_delay_length_minus1);
            nSE += PutBits(bs, 5, hrd.dpb_output_delay_length_minus1);
        }
    }

    std::for_each(hrd.sl, hrd.sl + maxNumSubLayersMinus1 + 1, PackSubLayer);
}

void Packer::PackVUI(BitstreamWriter& bs, VUI const & vui, mfxU16 max_sub_layers_minus1)
{
    mfxU32 nSE = 0;
    bool bPackSARWH = vui.aspect_ratio_info_present_flag && vui.aspect_ratio_idc == 255;

    nSE += PutBit(bs, vui.aspect_ratio_info_present_flag);
    nSE += vui.aspect_ratio_info_present_flag && PutBits(bs, 8, vui.aspect_ratio_idc);
    nSE += bPackSARWH && PutBits(bs, 16, vui.sar_width);
    nSE += bPackSARWH && PutBits(bs, 16, vui.sar_height);
    nSE += PutBit(bs, vui.overscan_info_present_flag);
    nSE += vui.overscan_info_present_flag && PutBit(bs, vui.overscan_appropriate_flag);
    nSE += PutBit(bs, vui.video_signal_type_present_flag);

    if (vui.video_signal_type_present_flag)
    {
        nSE += PutBits(bs, 3, vui.video_format);
        nSE += PutBit(bs, vui.video_full_range_flag);
        nSE += PutBit(bs, vui.colour_description_present_flag);

        if (vui.colour_description_present_flag)
        {
            nSE += PutBits(bs, 8, vui.colour_primaries);
            nSE += PutBits(bs, 8, vui.transfer_characteristics);
            nSE += PutBits(bs, 8, vui.matrix_coeffs);
        }
    }

    nSE += PutBit(bs, vui.chroma_loc_info_present_flag);
    nSE += vui.chroma_loc_info_present_flag && PutUE(bs, vui.chroma_sample_loc_type_top_field);
    nSE += vui.chroma_loc_info_present_flag && PutUE(bs, vui.chroma_sample_loc_type_bottom_field);
    nSE += PutBit(bs, vui.neutral_chroma_indication_flag );
    nSE += PutBit(bs, vui.field_seq_flag                 );
    nSE += PutBit(bs, vui.frame_field_info_present_flag  );
    nSE += PutBit(bs, vui.default_display_window_flag    );

    if (vui.default_display_window_flag)
    {
        nSE += PutUE(bs, vui.def_disp_win_left_offset   );
        nSE += PutUE(bs, vui.def_disp_win_right_offset  );
        nSE += PutUE(bs, vui.def_disp_win_top_offset    );
        nSE += PutUE(bs, vui.def_disp_win_bottom_offset );
    }

    nSE += PutBit(bs, vui.timing_info_present_flag);

    if (vui.timing_info_present_flag)
    {
        nSE += PutBits(bs, 32, vui.num_units_in_tick );
        nSE += PutBits(bs, 32, vui.time_scale        );
        nSE += PutBit(bs, vui.poc_proportional_to_timing_flag);
        nSE += vui.poc_proportional_to_timing_flag && PutUE(bs, vui.num_ticks_poc_diff_one_minus1);
        nSE += PutBit(bs, vui.hrd_parameters_present_flag);

        if (vui.hrd_parameters_present_flag)
            PackHRD(bs, vui.hrd, 1, max_sub_layers_minus1);
    }

    nSE += PutBit(bs, vui.bitstream_restriction_flag);

    if (vui.bitstream_restriction_flag)
    {
        nSE += PutBit(bs, vui.tiles_fixed_structure_flag              );
        nSE += PutBit(bs, vui.motion_vectors_over_pic_boundaries_flag );
        nSE += PutBit(bs, vui.restricted_ref_pic_lists_flag           );
        nSE += PutUE(bs, vui.min_spatial_segmentation_idc  );
        nSE += PutUE(bs, vui.max_bytes_per_pic_denom       );
        nSE += PutUE(bs, vui.max_bits_per_min_cu_denom     );
        nSE += PutUE(bs, vui.log2_max_mv_length_horizontal );
        nSE += PutUE(bs, vui.log2_max_mv_length_vertical   );
    }

    assert(nSE >= 10);
}

void PackSTRPS(BitstreamWriter& bs, const STRPS* sets, mfxU32 num, mfxU32 idx);

void PackExtension(mfxU8 extFlags, std::function<bool(mfxU8)> Pack)
{
    mfxU8 extId = 0;

    while (extFlags)
    {
        if (extFlags & 0x80)
            ThrowAssert(!Pack(extId), "extension is not supported");

        ++extId;
        extFlags <<= 1;
    }
}

mfxU32 Packer::PackSLD(BitstreamWriter& bs, ScalingList const & scl)
{
    mfxU32 nSE = 0;
    const mfxU32 matrixIds[] = { 0, 1, 2, 3, 4, 5 };
    auto PackDeltaCoef = [&](mfxU32 sizeId, mfxU32 nextCoef, const mfxU8* scalingList)
    {
        mfxU32 coefNum = std::min(64, (1 << (4 + (sizeId << 1))));
        nSE += PutSE(bs, int8_t(scalingList[0] - nextCoef));

        for (mfxU32 i = 1; i < coefNum; i++)
        {
            nSE += PutSE(bs, int8_t(scalingList[i] - scalingList[i - 1]));
            //here casting to int8_t will perform as x -= 256 if x > 127 and x += 256 if x < -128
        }
    };
    auto Pack4x4 = [&](mfxU32 matrixId)
    {
        nSE += PutBit(bs, 0); //scaling_list_pred_mode_flag
        PackDeltaCoef(0, 8, &scl.scalingLists0[matrixId][0]);
    };
    auto Pack8x8 = [&](mfxU32 matrixId)
    {
        nSE += PutBit(bs, 0); //scaling_list_pred_mode_flag
        PackDeltaCoef(1, 8, &scl.scalingLists1[matrixId][0]);
    };
    auto Pack16x16 = [&](mfxU32 matrixId)
    {
        nSE += PutBit(bs, 0); //scaling_list_pred_mode_flag
        auto next = scl.scalingLists2[matrixId][0];
        nSE += PutSE(bs, next - 8);
        PackDeltaCoef(2, next, &scl.scalingLists2[matrixId][0]);
    };
    auto Pack32x32 = [&](mfxU32 matrixId)
    {
        nSE += PutBit(bs, 0); //scaling_list_pred_mode_flag
        auto next = scl.scalingLists3[!!matrixId][0];
        nSE += PutSE(bs, next - 8);
        PackDeltaCoef(3, next, &scl.scalingLists3[!!matrixId][0]);
    };

    std::for_each(std::begin(matrixIds), std::end(matrixIds), Pack4x4);
    std::for_each(std::begin(matrixIds), std::end(matrixIds), Pack8x8);
    std::for_each(std::begin(matrixIds), std::end(matrixIds), Pack16x16);
    Pack32x32(0);
    Pack32x32(3);

    assert(nSE ==
          (1 + sizeof(scl.scalingLists0)
        + (1 + sizeof(scl.scalingLists1))
        + (2 + sizeof(scl.scalingLists2))
        + (2 + sizeof(scl.scalingLists3))));

    return nSE;
}

void Packer::PackSPS(BitstreamWriter& bs, SPS const & sps)
{
    NALU   nalu = {0, SPS_NUT, 0, 1};
    mfxU32 nSE  = 0;
    mfxU32 nLTR = !!sps.long_term_ref_pics_present_flag * sps.num_long_term_ref_pics_sps;

    PackNALU(bs, nalu);

    nSE += PutBits(bs, 4, sps.video_parameter_set_id);
    nSE += PutBits(bs, 3, sps.max_sub_layers_minus1);
    nSE += PutBit(bs, sps.temporal_id_nesting_flag);

    PackPTL(bs, sps, sps.max_sub_layers_minus1);

    nSE += PutUE(bs, sps.seq_parameter_set_id);
    nSE += PutUE(bs, sps.chroma_format_idc);
    nSE += (sps.chroma_format_idc == 3) && PutBit(bs, sps.separate_colour_plane_flag);
    nSE += PutUE(bs, sps.pic_width_in_luma_samples);
    nSE += PutUE(bs, sps.pic_height_in_luma_samples);
    nSE += PutBit(bs, sps.conformance_window_flag);

    if (sps.conformance_window_flag)
    {
        nSE += PutUE(bs, sps.conf_win_left_offset);
        nSE += PutUE(bs, sps.conf_win_right_offset);
        nSE += PutUE(bs, sps.conf_win_top_offset);
        nSE += PutUE(bs, sps.conf_win_bottom_offset);
    }

    nSE += PutUE(bs, sps.bit_depth_luma_minus8);
    nSE += PutUE(bs, sps.bit_depth_chroma_minus8);
    nSE += PutUE(bs, sps.log2_max_pic_order_cnt_lsb_minus4);

    PackSLO(bs, sps, sps.max_sub_layers_minus1);

    nSE += PutUE(bs, sps.log2_min_luma_coding_block_size_minus3);
    nSE += PutUE(bs, sps.log2_diff_max_min_luma_coding_block_size);
    nSE += PutUE(bs, sps.log2_min_transform_block_size_minus2);
    nSE += PutUE(bs, sps.log2_diff_max_min_transform_block_size);
    nSE += PutUE(bs, sps.max_transform_hierarchy_depth_inter);
    nSE += PutUE(bs, sps.max_transform_hierarchy_depth_intra);
    nSE += PutBit(bs, sps.scaling_list_enabled_flag);

    nSE += sps.scaling_list_enabled_flag && PackSLD(bs, sps.scl);

    nSE += PutBit(bs, sps.amp_enabled_flag);
    nSE += PutBit(bs, sps.sample_adaptive_offset_enabled_flag);
    nSE += PutBit(bs, sps.pcm_enabled_flag);

    if (sps.pcm_enabled_flag)
    {
        nSE += PutBits(bs, 4, sps.pcm_sample_bit_depth_luma_minus1);
        nSE += PutBits(bs, 4, sps.pcm_sample_bit_depth_chroma_minus1);
        nSE += PutUE(bs, sps.log2_min_pcm_luma_coding_block_size_minus3);
        nSE += PutUE(bs, sps.log2_diff_max_min_pcm_luma_coding_block_size);
        nSE += PutBit(bs, sps.pcm_loop_filter_disabled_flag);
    }

    nSE += PutUE(bs, sps.num_short_term_ref_pic_sets);

    for (mfxU32 i = 0; i < sps.num_short_term_ref_pic_sets; i++)
        PackSTRPS(bs, sps.strps, sps.num_short_term_ref_pic_sets, i);

    nSE += PutBit(bs, sps.long_term_ref_pics_present_flag);
    nSE += sps.long_term_ref_pics_present_flag && PutUE(bs, sps.num_long_term_ref_pics_sps);

    for (mfxU32 i = 0; i < nLTR; ++i)
    {
        nSE += PutBits(bs, sps.log2_max_pic_order_cnt_lsb_minus4 + 4, sps.lt_ref_pic_poc_lsb_sps[i] );
        nSE += PutBit(bs, sps.used_by_curr_pic_lt_sps_flag[i] );
    }

    nSE += PutBit(bs, sps.temporal_mvp_enabled_flag);
    nSE += PutBit(bs, sps.strong_intra_smoothing_enabled_flag);
    nSE += PutBit(bs, sps.vui_parameters_present_flag);

    if (sps.vui_parameters_present_flag)
        PackVUI(bs, sps.vui, sps.max_sub_layers_minus1);

    nSE += PutBit(bs, sps.extension_flag);
    nSE += sps.extension_flag && PutBit(bs, sps.range_extension_flag);
    nSE += sps.extension_flag && PutBits(bs, 7, sps.ExtensionFlags);

    if (sps.range_extension_flag)
    {
        nSE += PutBit(bs, sps.transform_skip_rotation_enabled_flag);
        nSE += PutBit(bs, sps.transform_skip_context_enabled_flag);
        nSE += PutBit(bs, sps.implicit_rdpcm_enabled_flag);
        nSE += PutBit(bs, sps.explicit_rdpcm_enabled_flag);
        nSE += PutBit(bs, sps.extended_precision_processing_flag);
        nSE += PutBit(bs, sps.intra_smoothing_disabled_flag);
        nSE += PutBit(bs, sps.high_precision_offsets_enabled_flag);
        nSE += PutBit(bs, sps.persistent_rice_adaptation_enabled_flag);
        nSE += PutBit(bs, sps.cabac_bypass_alignment_enabled_flag);
    }

    assert(nSE >= 27);

    PackExtension(sps.ExtensionFlags & 0x7f
        , [&](mfxU8 id)
    {
        auto& PackSpsExt = Glob::PackSpsExt::Get(*m_pGlob);
        return PackSpsExt && PackSpsExt(sps, id, bs);
    });

    bs.PutTrailingBits();
}

void Packer::PackPPS(BitstreamWriter& bs, PPS const &  pps)
{
    NALU   nalu             = {0, PPS_NUT, 0, 1};
    mfxU32 nSE              = 0;
    bool   bNeedDblk        = pps.deblocking_filter_control_present_flag;
    bool   bNeedDblkOffsets = bNeedDblk && !pps.deblocking_filter_disabled_flag;

    PackNALU(bs, nalu);

    nSE += PutUE(bs, pps.pic_parameter_set_id);
    nSE += PutUE(bs, pps.seq_parameter_set_id);
    nSE += PutBit(bs, pps.dependent_slice_segments_enabled_flag);
    nSE += PutBit(bs, pps.output_flag_present_flag);
    nSE += PutBits(bs, 3, pps.num_extra_slice_header_bits);
    nSE += PutBit(bs, pps.sign_data_hiding_enabled_flag);
    nSE += PutBit(bs, pps.cabac_init_present_flag);
    nSE += PutUE(bs, pps.num_ref_idx_l0_default_active_minus1);
    nSE += PutUE(bs, pps.num_ref_idx_l1_default_active_minus1);
    nSE += PutSE(bs, pps.init_qp_minus26);
    nSE += PutBit(bs, pps.constrained_intra_pred_flag);
    nSE += PutBit(bs, pps.transform_skip_enabled_flag);
    nSE += PutBit(bs, pps.cu_qp_delta_enabled_flag);

    nSE += pps.cu_qp_delta_enabled_flag && PutUE(bs, pps.diff_cu_qp_delta_depth);

    nSE += PutSE(bs, pps.cb_qp_offset);
    nSE += PutSE(bs, pps.cr_qp_offset);
    nSE += PutBit(bs, pps.slice_chroma_qp_offsets_present_flag);
    nSE += PutBit(bs, pps.weighted_pred_flag);
    nSE += PutBit(bs, pps.weighted_bipred_flag);
    nSE += PutBit(bs, pps.transquant_bypass_enabled_flag);
    nSE += PutBit(bs, pps.tiles_enabled_flag);
    nSE += PutBit(bs, pps.entropy_coding_sync_enabled_flag);

    if (pps.tiles_enabled_flag)
    {
        auto cwEnd      = pps.column_width + pps.num_tile_columns_minus1 * !pps.uniform_spacing_flag;
        auto rhEnd      = pps.row_height + pps.num_tile_rows_minus1 * !pps.uniform_spacing_flag;
        auto PutTileDim = [&](mfxU16 x) { return PutUE(bs, std::max<mfxU16>(x, 1) - 1); };

        nSE += PutUE(bs, pps.num_tile_columns_minus1);
        nSE += PutUE(bs, pps.num_tile_rows_minus1);
        nSE += PutBit(bs, pps.uniform_spacing_flag);
        nSE += mfxU32(std::count_if(pps.column_width, cwEnd, PutTileDim));
        nSE += mfxU32(std::count_if(pps.row_height, rhEnd, PutTileDim));
        nSE += PutBit(bs, pps.loop_filter_across_tiles_enabled_flag);
    }

    nSE += PutBit(bs, pps.loop_filter_across_slices_enabled_flag);
    nSE += PutBit(bs, pps.deblocking_filter_control_present_flag);
    nSE += bNeedDblk && PutBit(bs, pps.deblocking_filter_override_enabled_flag);
    nSE += bNeedDblk && PutBit(bs, pps.deblocking_filter_disabled_flag);
    nSE += bNeedDblkOffsets && PutSE(bs, pps.beta_offset_div2);
    nSE += bNeedDblkOffsets && PutSE(bs, pps.tc_offset_div2);

    nSE += PutBit(bs, pps.scaling_list_data_present_flag);
    assert(0 == pps.scaling_list_data_present_flag);

    nSE += PutBit(bs, pps.lists_modification_present_flag);
    nSE += PutUE(bs, pps.log2_parallel_merge_level_minus2);
    nSE += PutBit(bs, pps.slice_segment_header_extension_present_flag);
    nSE += PutBit(bs, pps.extension_flag);
    nSE += pps.extension_flag && PutBit(bs, pps.range_extension_flag);
    nSE += pps.extension_flag && PutBits(bs, 7, pps.ExtensionFlags);

    if (pps.range_extension_flag)
    {
        mfxU32 nQpOffsets = pps.chroma_qp_offset_list_enabled_flag * (pps.chroma_qp_offset_list_len_minus1 + 1);

        nSE += pps.transform_skip_enabled_flag && PutUE(bs, pps.log2_max_transform_skip_block_size_minus2);
        nSE += PutBit(bs, pps.cross_component_prediction_enabled_flag);
        nSE += PutBit(bs, pps.chroma_qp_offset_list_enabled_flag);
        nSE += pps.chroma_qp_offset_list_enabled_flag && PutUE(bs, pps.diff_cu_chroma_qp_offset_depth);
        nSE += pps.chroma_qp_offset_list_enabled_flag && PutUE(bs, pps.chroma_qp_offset_list_len_minus1);

        for (mfxU32 i = 0; i < nQpOffsets; i++)
        {
            nSE += PutSE(bs, pps.cb_qp_offset_list[i]);
            nSE += PutSE(bs, pps.cr_qp_offset_list[i]);
        }

        nSE += PutUE(bs, pps.log2_sao_offset_scale_luma);
        nSE += PutUE(bs, pps.log2_sao_offset_scale_chroma);
    }

    PackExtension(pps.ExtensionFlags & 0x7f
        , [&](mfxU8 id)
    {
        auto& PackPpsExt = Glob::PackPpsExt::Get(*m_pGlob);
        return PackPpsExt && PackPpsExt(pps, id, bs);
    });

    bs.PutTrailingBits();

    assert(nSE >= 28);
}

void Packer::PackSSHPartIdAddr(
    BitstreamWriter& bs,
    NALU  const &    nalu,
    SPS   const &    sps,
    PPS   const &    pps,
    Slice const &    slice)
{
    mfxU32 MaxCU = (1<<(sps.log2_min_luma_coding_block_size_minus3 + 3 + sps.log2_diff_max_min_luma_coding_block_size));
    mfxU32 PicSizeInCtbsY = CeilDiv(sps.pic_width_in_luma_samples, MaxCU) * CeilDiv(sps.pic_height_in_luma_samples, MaxCU);

    bs.PutBit(slice.first_slice_segment_in_pic_flag);

    bool bIRAP = nalu.nal_unit_type >= BLA_W_LP && nalu.nal_unit_type <= RSV_IRAP_VCL23;

    if (bIRAP)
        bs.PutBit(slice.no_output_of_prior_pics_flag);

    bs.PutUE(slice.pic_parameter_set_id);

    if (!slice.first_slice_segment_in_pic_flag)
    {
        if (pps.dependent_slice_segments_enabled_flag)
            bs.PutBit(slice.dependent_slice_segment_flag);

        bs.PutBits(CeilLog2(PicSizeInCtbsY), slice.segment_address);
    }
}

void Packer::PackSSHPartNonIDR(
    BitstreamWriter& bs,
    SPS   const &    sps,
    Slice const &    slice)
{
    mfxU32 nSE          = 0;
    bool   bNeedStIdx   = slice.short_term_ref_pic_set_sps_flag && (sps.num_short_term_ref_pic_sets > 1);
    auto   PutSpsLT     = [&](const Slice::LongTerm& lt)
    {
        nSE += (sps.num_long_term_ref_pics_sps > 1) && PutBits(bs, CeilLog2(sps.num_long_term_ref_pics_sps), lt.lt_idx_sps);
        nSE += PutBit(bs, lt.delta_poc_msb_present_flag);
        nSE += lt.delta_poc_msb_present_flag && PutUE(bs, lt.delta_poc_msb_cycle_lt);
    };
    auto   PutSliceLT   = [&](const Slice::LongTerm& lt)
    {
        nSE += PutBits(bs, sps.log2_max_pic_order_cnt_lsb_minus4 + 4, lt.poc_lsb_lt);
        nSE += PutBit(bs, lt.used_by_curr_pic_lt_flag);
        nSE += PutBit(bs, lt.delta_poc_msb_present_flag);
        nSE += lt.delta_poc_msb_present_flag && PutUE(bs, lt.delta_poc_msb_cycle_lt);
    };

    nSE += PutBits(bs, sps.log2_max_pic_order_cnt_lsb_minus4 + 4, slice.pic_order_cnt_lsb);
    nSE += PutBit(bs, slice.short_term_ref_pic_set_sps_flag);

    if (!slice.short_term_ref_pic_set_sps_flag)
    {
        std::vector<STRPS> strps(sps.strps, sps.strps + sps.num_short_term_ref_pic_sets);
        strps.push_back(slice.strps);
        PackSTRPS(bs, strps.data(), sps.num_short_term_ref_pic_sets, sps.num_short_term_ref_pic_sets);
    }

    nSE += bNeedStIdx && PutBits(bs, CeilLog2(sps.num_short_term_ref_pic_sets), slice.short_term_ref_pic_set_idx);

    if (sps.long_term_ref_pics_present_flag)
    {
        nSE += (sps.num_long_term_ref_pics_sps > 0) && PutUE(bs, slice.num_long_term_sps);
        nSE += PutUE(bs, slice.num_long_term_pics);

        std::for_each(slice.lt, slice.lt + slice.num_long_term_sps, PutSpsLT);
        std::for_each(slice.lt, slice.lt + slice.num_long_term_pics, PutSliceLT);
    }

    nSE += sps.temporal_mvp_enabled_flag && PutBit(bs, slice.temporal_mvp_enabled_flag);

    assert(nSE >= 2);
}

bool Packer::PackSSHPWT(
    BitstreamWriter& bs
    , const SPS&     sps
    , const PPS&     pps
    , const Slice&   slice)
{
    constexpr mfxU16 Y = 0, Cb = 1, Cr = 2, W = 0, O = 1, P = 1, B = 0;

    struct AccWFlag : std::pair<mfxU16, mfxI16>
    {
        AccWFlag(mfxU16 idx, mfxI16 w) : std::pair<mfxU16, mfxI16>(idx, w){}
        mfxU16 operator()(mfxU16 wf, const mfxI16(&pwt)[3][2])
        {
            return (wf << 1) + !(pwt[first][O] == 0 && pwt[first][W] == second);
        };
    };

    bool     bNeedY       = (pps.weighted_pred_flag && slice.type == P) || (pps.weighted_bipred_flag && slice.type == B);
    bool     bNeedC       = bNeedY && (sps.chroma_format_idc != 0);
    mfxI16   BdOffsetC    =
        sps.high_precision_offsets_enabled_flag * (sps.bit_depth_chroma_minus8 + 8 - 1)
        + !sps.high_precision_offsets_enabled_flag * 7;
    mfxU32   nSE          = 0;
    mfxI16   WpOffC       = (1 << BdOffsetC);
    mfxI16   wY           = (1 << slice.luma_log2_weight_denom);
    mfxI16   wC           = (1 << slice.chroma_log2_weight_denom);
    mfxI16   l2WDc        = slice.chroma_log2_weight_denom;
    auto     startOffset  = bs.GetOffset();
    auto     PutPwtLX     = [&](const mfxI16(&pwtLX)[16][3][2], mfxU32 sz)
    {
        mfxU32 szY      = sz * bNeedY;
        mfxU32 szC      = sz * bNeedC;
        mfxU16 wfmap    = (1 << (szY - 1));
        mfxU16 lumaw    = 0;
        mfxU16 chromaw  = 0;
        auto   PutWOVal = [&](const mfxI16(&pwt)[3][2])
        {
            bool bPutY = !!(lumaw & wfmap);
            bool bPutC = !!(chromaw & wfmap);

            nSE += bPutY && PutSE(bs, pwt[Y][W] - wY);
            nSE += bPutY && PutSE(bs, pwt[Y][O]);

            nSE += bPutC && PutSE(bs, pwt[Cb][W] - wC);
            nSE += bPutC && PutSE(bs, mfx::clamp(((WpOffC * pwt[Cb][W]) >> l2WDc) + pwt[Cb][O] - WpOffC, -4 * WpOffC, 4 * WpOffC - 1));

            nSE += bPutC && PutSE(bs, pwt[Cb][W] - wC);
            nSE += bPutC && PutSE(bs, mfx::clamp(((WpOffC * pwt[Cr][W]) >> l2WDc) + pwt[Cr][O] - WpOffC, -4 * WpOffC, 4 * WpOffC - 1));

            wfmap >>= 1;
        };

        lumaw    = std::accumulate(pwtLX, pwtLX + szY, mfxU16(0), AccWFlag(Y, wY));
        chromaw  = std::accumulate(pwtLX, pwtLX + szC, mfxU16(0), AccWFlag(Cb, wC));
        chromaw |= std::accumulate(pwtLX, pwtLX + szC, mfxU16(0), AccWFlag(Cr, wC));

        nSE += szY && PutBits(bs, szY, lumaw);
        nSE += szC && PutBits(bs, szC, chromaw);

        std::for_each(pwtLX, pwtLX + szY, PutWOVal);
    };

    bs.AddInfo(PACK_PWTOffset, startOffset);

    nSE += bNeedY && PutUE(bs, slice.luma_log2_weight_denom);
    nSE += bNeedC && PutSE(bs, mfxI32(slice.chroma_log2_weight_denom) - slice.luma_log2_weight_denom);

    PutPwtLX(slice.pwt[0], slice.num_ref_idx_l0_active_minus1 + 1);
    PutPwtLX(slice.pwt[1], (slice.num_ref_idx_l1_active_minus1 + 1) * (slice.type == B));

    bs.AddInfo(PACK_PWTLength, bs.GetOffset() - startOffset);

    return !!nSE;
}

void Packer::PackSSHPartPB(
    BitstreamWriter& bs,
    SPS   const &    sps,
    PPS   const &    pps,
    Slice const &    slice)
{
    auto   IsSTUsed        = [](const STRPSPic& pic) { return !!pic.used_by_curr_pic_sx_flag; };
    auto   IsLTUsed        = [](const Slice::LongTerm& lt) { return !!lt.used_by_curr_pic_lt_flag; };
    bool   bB              = (slice.type == 0);
    mfxU32 nSE             = 0;
    auto   stEnd           = slice.strps.pic + slice.strps.num_negative_pics + slice.strps.num_positive_pics;
    auto   ltEnd           = slice.lt + slice.num_long_term_sps + slice.num_long_term_pics;
    mfxU16 NumPocTotalCurr = mfxU16(std::count_if(slice.strps.pic, stEnd, IsSTUsed) + std::count_if(slice.lt, ltEnd, IsLTUsed));
    bool   bNeedRPLM0      = pps.lists_modification_present_flag && NumPocTotalCurr > 1;
    bool   bNeedRPLM1      = bNeedRPLM0 && bB;
    bool   bNeedCRefIdx0   = (slice.collocated_from_l0_flag && slice.num_ref_idx_l0_active_minus1 > 0);
    bool   bNeedCRefIdx1   = (!slice.collocated_from_l0_flag && slice.num_ref_idx_l1_active_minus1 > 0);
    bool   bNeedCRefIdx    = slice.temporal_mvp_enabled_flag && (bNeedCRefIdx0 || bNeedCRefIdx1);
    auto   PackRPLM        = [&](mfxU8 lx, mfxU8 num)
    {
        nSE += PutBit(bs, !!slice.ref_pic_list_modification_flag_lx[lx]);

        num *= !!slice.ref_pic_list_modification_flag_lx[lx];

        std::for_each(slice.list_entry_lx[lx], slice.list_entry_lx[lx] + num
            , [&](mfxU8 entry)
        {
            nSE += PutBits(bs, CeilLog2(NumPocTotalCurr), entry);
        });
    };

    nSE += PutBit(bs, slice.num_ref_idx_active_override_flag);
    nSE += slice.num_ref_idx_active_override_flag && PutUE(bs, slice.num_ref_idx_l0_active_minus1);
    nSE += slice.num_ref_idx_active_override_flag && bB && PutUE(bs, slice.num_ref_idx_l1_active_minus1);

    if (bNeedRPLM0)
    {
        PackRPLM(0, slice.num_ref_idx_l0_active_minus1 + 1);
    }

    if (bNeedRPLM1)
    {
        PackRPLM(1, slice.num_ref_idx_l1_active_minus1 + 1);
    }

    nSE += bB && PutBit(bs, slice.mvd_l1_zero_flag);
    nSE += pps.cabac_init_present_flag && PutBit(bs, slice.cabac_init_flag);
    nSE += slice.temporal_mvp_enabled_flag && bB && PutBit(bs, slice.collocated_from_l0_flag);
    nSE += bNeedCRefIdx && PutUE(bs, slice.collocated_ref_idx);

    PackSSHPWT(bs, sps, pps, slice);

    nSE += PutUE(bs, slice.five_minus_max_num_merge_cand);

    assert(nSE >= 2);
}

void Packer::PackSSHPartIndependent(
    BitstreamWriter& bs,
    NALU  const &    nalu,
    SPS   const &    sps,
    PPS   const &    pps,
    Slice const &    slice)
{
    const mfxU8 I = 2;
    mfxU32 nSE = 0;

    nSE += PutBits(bs, pps.num_extra_slice_header_bits, slice.reserved_flags);
    nSE += PutUE(bs, slice.type);
    nSE += pps.output_flag_present_flag && PutBit(bs, slice.pic_output_flag);
    nSE += (sps.separate_colour_plane_flag == 1) && PutBits(bs, 2, slice.colour_plane_id);

    bool bNonIDR = nalu.nal_unit_type != IDR_W_RADL && nalu.nal_unit_type != IDR_N_LP;

    if (bNonIDR)
        PackSSHPartNonIDR(bs, sps, slice);

    if (sps.sample_adaptive_offset_enabled_flag)
    {
        bs.AddInfo(PACK_SAOOffset, bs.GetOffset());

        nSE += PutBit(bs, slice.sao_luma_flag);
        nSE += PutBit(bs, slice.sao_chroma_flag);
    }

    if (slice.type != I)
        PackSSHPartPB(bs, sps, pps, slice);

    bs.AddInfo(PACK_QPDOffset, bs.GetOffset());

    nSE += PutSE(bs, slice.slice_qp_delta);
    nSE += pps.slice_chroma_qp_offsets_present_flag && PutSE(bs, slice.slice_cb_qp_offset);
    nSE += pps.slice_chroma_qp_offsets_present_flag && PutSE(bs, slice.slice_cr_qp_offset);
    nSE += pps.deblocking_filter_override_enabled_flag && PutBit(bs, slice.deblocking_filter_override_flag);
    nSE += slice.deblocking_filter_override_flag && PutBit(bs, slice.deblocking_filter_disabled_flag);

    bool bPackDblkOffsets = slice.deblocking_filter_override_flag && !slice.deblocking_filter_disabled_flag;
    nSE += bPackDblkOffsets && PutSE(bs, slice.beta_offset_div2);
    nSE += bPackDblkOffsets && PutSE(bs, slice.tc_offset_div2);

    bool bPackSliceLF =
        (pps.loop_filter_across_slices_enabled_flag
            && (slice.sao_luma_flag
                || slice.sao_chroma_flag
                || !slice.deblocking_filter_disabled_flag));

    nSE += bPackSliceLF && PutBit(bs, slice.loop_filter_across_slices_enabled_flag);

    assert(nSE >= 2);
}

void Packer::PackSSH(
    BitstreamWriter& bs,
    NALU  const &    nalu,
    SPS   const &    sps,
    PPS   const &    pps,
    Slice const &    slice,
    bool             dyn_slice_size)
{
    PackNALU(bs, nalu);

    if (!dyn_slice_size)
        PackSSHPartIdAddr(bs, nalu, sps, pps, slice);

    if (!slice.dependent_slice_segment_flag)
        PackSSHPartIndependent(bs, nalu, sps, pps, slice);

    if (pps.tiles_enabled_flag || pps.entropy_coding_sync_enabled_flag)
    {
        assert(slice.num_entry_point_offsets == 0);

        bs.PutUE(slice.num_entry_point_offsets);
    }

    assert(0 == pps.slice_segment_header_extension_present_flag);

    if (!dyn_slice_size)   // no trailing bits for dynamic slice size
        bs.PutTrailingBits();
}

void PackSTRPS(BitstreamWriter& bs, const STRPS* sets, mfxU32 num, mfxU32 idx)
{
    STRPS const & strps = sets[idx];

    if (idx != 0)
        bs.PutBit(strps.inter_ref_pic_set_prediction_flag);

    if (strps.inter_ref_pic_set_prediction_flag)
    {
        if(idx == num)
            bs.PutUE(strps.delta_idx_minus1);

        bs.PutBit(strps.delta_rps_sign);
        bs.PutUE(strps.abs_delta_rps_minus1);

        mfxU32 RefRpsIdx = idx - ( strps.delta_idx_minus1 + 1 );
        mfxU32 NumDeltaPocs = sets[RefRpsIdx].num_negative_pics + sets[RefRpsIdx].num_positive_pics;

        std::for_each(
            strps.pic
            , strps.pic + NumDeltaPocs + 1
            , [&](const STRPS::Pic& pic)
        {
            bs.PutBit(pic.used_by_curr_pic_flag);

            if (!pic.used_by_curr_pic_flag)
                bs.PutBit(pic.use_delta_flag);
        });
    }
    else
    {
        bs.PutUE(strps.num_negative_pics);
        bs.PutUE(strps.num_positive_pics);

        std::for_each(
            strps.pic
            , strps.pic + strps.num_positive_pics + strps.num_negative_pics
            , [&](const STRPS::Pic& pic)
        {
            bs.PutUE(pic.delta_poc_sx_minus1);
            bs.PutBit(pic.used_by_curr_pic_sx_flag);
        });
    }
}

void Packer::PackSEIPayload(
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
    bool bAltCpb = hrd.sub_pic_hrd_params_present_flag || bp.irap_cpb_params_present_flag;

    auto PackCPB = [&](const BufferingPeriodSEI::CPB& cpb)
    {
        bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1 + 1, cpb.initial_cpb_removal_delay);
        bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1 + 1, cpb.initial_cpb_removal_offset);

        if (bAltCpb)
        {
            bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1 + 1, cpb.initial_alt_cpb_removal_delay);
            bs.PutBits(hrd.initial_cpb_removal_delay_length_minus1 + 1, cpb.initial_alt_cpb_removal_offset);
        }
    };

    std::for_each(
        bp.nal
        , bp.nal + (CpbCnt + 1) * hrd.nal_hrd_parameters_present_flag
        , PackCPB);

    std::for_each(
        bp.vcl
        , bp.vcl + (CpbCnt + 1) * hrd.vcl_hrd_parameters_present_flag
        , PackCPB);

    bs.PutTrailingBits(true);
}

void Packer::PackSEIPayload(
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

    bool bHRD = hrd.nal_hrd_parameters_present_flag || hrd.vcl_hrd_parameters_present_flag;
    if (bHRD)
    {
       bs.PutBits(hrd.au_cpb_removal_delay_length_minus1+1, pt.au_cpb_removal_delay_minus1);
       bs.PutBits(hrd.dpb_output_delay_length_minus1+1, pt.pic_dpb_output_delay);

       assert(hrd.sub_pic_hrd_params_present_flag == 0);
    }

    bs.PutTrailingBits(true);
}

mfxU32 Packer::PackRBSP(mfxU8* dst, mfxU8* rbsp, mfxU32 dst_size, mfxU32 rbsp_size)
{
    mfxU32 rest_bytes = dst_size - rbsp_size;
    mfxU8* rbsp_end = rbsp + rbsp_size;

    if(dst_size < rbsp_size)
        return 0;

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

        bool bNeedEPB = !(rbsp[-1] || rbsp[0] || (rbsp[1] & 0xFC));
        if (bNeedEPB)
        {
            if (!--rest_bytes)
                return 0;

            *dst++ = *rbsp++;
            *dst++ = 0x03;
        }
    }

    while (rbsp_end > rbsp)
        *dst++ = *rbsp++;

    dst_size -= rest_bytes;

    return dst_size;
}

void Packer::PackBPPayload(
    BitstreamWriter& rbsp
    , const TaskCommonPar & task
    , const SPS& sps)
{
    BufferingPeriodSEI bp = {};

    bp.seq_parameter_set_id = sps.seq_parameter_set_id;
    bp.nal[0].initial_cpb_removal_delay = bp.vcl[0].initial_cpb_removal_delay = task.initial_cpb_removal_delay;
    bp.nal[0].initial_cpb_removal_offset = bp.vcl[0].initial_cpb_removal_offset = task.initial_cpb_removal_offset;

    mfxU32 size = CeilDiv(rbsp.GetOffset(), 8u);
    mfxU8* pl = rbsp.GetStart() + size; // payload start

    rbsp.PutBits(8, 0);    //payload type
    rbsp.PutBits(8, 0xff); //place for payload  size
    size += 2;

    Packer::PackSEIPayload(rbsp, sps.vui, bp);

    size = CeilDiv(rbsp.GetOffset(), 8u) - size;

    assert(size < 256);
    pl[1] = (mfxU8)size; //payload size
}

void Packer::PackPTPayload(
    BitstreamWriter& rbsp
    , const ExtBuffer::Param<mfxVideoParam> & par
    , const TaskCommonPar & task
    , const SPS& sps)
{
    PicTimingSEI pt = {};

    static const std::map<mfxU16, std::array<mfxU8,2>> PicStructMFX2STD[2] =
    {
        {
              {mfxU16(MFX_PICSTRUCT_FIELD_TFF), {mfxU8(3), mfxU8(0)}}
            , {mfxU16(MFX_PICSTRUCT_FIELD_BFF), {mfxU8(4), mfxU8(0)}}
            , {mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FIELD_TFF), {mfxU8(3), mfxU8(1)}}
            , {mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FIELD_BFF), {mfxU8(4), mfxU8(1)}}
            , {mfxU16(MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_REPEATED), {mfxU8(5), mfxU8(0)}}
            , {mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_REPEATED), {mfxU8(5), mfxU8(1)}}
            , {mfxU16(MFX_PICSTRUCT_FIELD_BFF | MFX_PICSTRUCT_FIELD_REPEATED), {mfxU8(6), mfxU8(0)}}
            , {mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FIELD_BFF | MFX_PICSTRUCT_FIELD_REPEATED), {mfxU8(6), mfxU8(1)}}
            , {mfxU16(0), {mfxU8(0), mfxU8(2)}}
        }
        , {//progressive only
              {mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FRAME_DOUBLING), {mfxU8(7), mfxU8(1)}}
            , {mfxU16(MFX_PICSTRUCT_PROGRESSIVE | MFX_PICSTRUCT_FRAME_TRIPLING), {mfxU8(8), mfxU8(1)}}
            , {mfxU16(0), {mfxU8(0), mfxU8(1)}}
        }
    };

    bool bProg = !!(par.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE);
    mfxU16 key = task.pSurfIn->Info.PicStruct;
    key *= mfxU16(!!PicStructMFX2STD[bProg].count(key));
    auto ps = PicStructMFX2STD[bProg].at(key);

    pt.pic_struct = ps[0];
    pt.source_scan_type = ps[1];
    pt.duplicate_flag = 0;

    pt.au_cpb_removal_delay_minus1 = std::max<mfxU32>(task.cpb_removal_delay, 1) - 1;
    pt.pic_dpb_output_delay =
        task.DisplayOrder
        + sps.sub_layer[sps.max_sub_layers_minus1].max_num_reorder_pics
        - task.EncodedOrder;

    mfxU32 size = CeilDiv(rbsp.GetOffset(), 8u);
    mfxU8* pl = rbsp.GetStart() + size; // payload start

    rbsp.PutBits(8, 1);    //payload type
    rbsp.PutBits(8, 0xFF); //place for payload  size
    size += 2;

    Packer::PackSEIPayload(rbsp, sps.vui, pt);

    size = CeilDiv(rbsp.GetOffset(), 8u) - size;

    assert(size < 256);
    pl[1] = (mfxU8)size; //payload size
}

template<mfxU16 PT>
bool PLTypeEq(const mfxPayload* pl)
{
    return (pl && pl->Type == PT);
}

mfxU32 Packer::GetPrefixSEI(
    const ExtBuffer::Param<mfxVideoParam> & par
    , const TaskCommonPar & task
    , const SPS& sps
    , mfxU8* buf
    , mfxU32 sizeInBytes)
{
    NALU nalu = {1, PREFIX_SEI_NUT, 0, 1};
    BitstreamWriter rbsp(m_rbsp.data(), (mfxU32)m_rbsp.size());
    std::list<const mfxPayload*> prefixPL, tmpPL;
    mfxPayload BPSEI = {}, PTSEI = {};
    auto pBegin = buf;

    auto PutPLIntoRBSP = [&rbsp](const mfxPayload* pPL)
    {
        rbsp.PutBitsBuffer(pPL->NumBit, pPL->Data);
        rbsp.PutTrailingBits(true);
    };
    auto PackSeiNalu = [&](std::list<const mfxPayload*>& pls)
    {
        PackNALU(rbsp, nalu);

        std::for_each(pls.begin(), pls.end(), PutPLIntoRBSP);

        rbsp.PutTrailingBits();

        auto bytesPacked = PackRBSP(buf, rbsp.GetStart(), sizeInBytes, CeilDiv(rbsp.GetOffset(), 8u));

        buf += bytesPacked;
        sizeInBytes -= bytesPacked;

        rbsp.Reset();

        return bytesPacked;
    };

    //external payloads
    prefixPL.assign(task.ctrl.Payload, task.ctrl.Payload + (task.ctrl.NumPayload * !!task.ctrl.Payload));

    if (par.mfx.RateControlMethod != MFX_RATECONTROL_CQP)
    {
        prefixPL.remove_if(PLTypeEq<0>); //buffering_period
        prefixPL.remove_if(PLTypeEq<1>); //pic_timing
    }

    //internal payloads
    std::transform(task.PLInternal.begin(), task.PLInternal.end(), std::back_inserter(prefixPL)
        , [](const mfxPayload& pl) { return &pl; });

    prefixPL.remove_if([](const mfxPayload* pPL) { return !pPL || (pPL->CtrlFlags & MFX_PAYLOAD_CTRL_SUFFIX); });

    bool bNeedSEI = !prefixPL.empty() || (task.InsertHeaders & INSERT_SEI);

    if (!bNeedSEI)
        return 0;

    /* An SEI NAL unit containing an active parameter sets SEI message shall contain only
    one active parameter sets SEI message and shall not contain any other SEI messages. */
    /* When an SEI NAL unit containing an active parameter sets SEI message is present
    in an access unit, it shall be the first SEI NAL unit that follows the prevVclNalUnitInAu
    of the SEI NAL unit and precedes the nextVclNalUnitInAu of the SEI NAL unit. */
    MoveIf(prefixPL, tmpPL, PLTypeEq<129>);

    bool bPackError = !tmpPL.empty() && !PackSeiNalu(tmpPL);

    if (bPackError)
        return 0;
    
    bool bNeedOwnPT = (task.InsertHeaders & INSERT_PTSEI)
        && !std::any_of(prefixPL.begin(), prefixPL.end(), PLTypeEq<1>);
    if (bNeedOwnPT)
    {
        PTSEI.Type   = 1;
        PTSEI.NumBit = rbsp.GetOffset();
        PTSEI.Data   = rbsp.GetStart() + PTSEI.NumBit / 8;

        PackPTPayload(rbsp, par, task, sps);

        PTSEI.NumBit  = rbsp.GetOffset() - PTSEI.NumBit;
        PTSEI.BufSize = (mfxU16)CeilDiv<mfxU32>(PTSEI.NumBit, 8);

        prefixPL.push_front(&PTSEI);
        rbsp.PutTrailingBits(true);

        auto pNewRBSPStart = rbsp.GetStart() + CeilDiv(rbsp.GetOffset(), 8u);
        rbsp.Reset(pNewRBSPStart, mfxU32(rbsp.GetEnd() - pNewRBSPStart));
    }

    bool bNeedOwnBP = (task.InsertHeaders & INSERT_BPSEI)
        && !std::any_of(prefixPL.begin(), prefixPL.end(), PLTypeEq<0>);
    if (bNeedOwnBP)
    {
        BPSEI.Type   = 0;
        BPSEI.NumBit = rbsp.GetOffset();
        BPSEI.Data   = rbsp.GetStart() + BPSEI.NumBit / 8;

        PackBPPayload(rbsp, task, sps);

        BPSEI.NumBit  = rbsp.GetOffset() - BPSEI.NumBit;
        BPSEI.BufSize = (mfxU16)CeilDiv<mfxU32>(BPSEI.NumBit, 8);

        prefixPL.push_front(&BPSEI);
        rbsp.PutTrailingBits(true);

        auto pNewRBSPStart = rbsp.GetStart() + CeilDiv(rbsp.GetOffset(), 8u);
        rbsp.Reset(pNewRBSPStart, mfxU32(rbsp.GetEnd() - pNewRBSPStart));
    }

    /* When an SEI NAL unit contains a non-nested buffering period SEI message,
    a non-nested picture timing SEI message, or a non-nested decoding unit information
    SEI message, the SEI NAL unit shall not contain any other SEI message with payloadType
    not equal to 0 (buffering period), 1 (picture timing) or 130 (decoding unit information). */
    tmpPL.clear();
    MoveIf(prefixPL, tmpPL, PLTypeEq<0>);
    MoveIf(prefixPL, tmpPL, PLTypeEq<1>);
    MoveIf(prefixPL, tmpPL, PLTypeEq<130>);

    bPackError =
           (!tmpPL.empty() && !PackSeiNalu(tmpPL))
        || (!prefixPL.empty() && !PackSeiNalu(prefixPL));

    return mfxU32(!bPackError * (buf - pBegin));
}

mfxU32 Packer::GetSuffixSEI(
    const TaskCommonPar & task
    , mfxU8* buf
    , mfxU32 sizeInBytes)
{
    NALU nalu = {0, SUFFIX_SEI_NUT, 0, 1};
    BitstreamWriter rbsp(m_rbsp.data(), (mfxU32)m_rbsp.size());
    std::list<const mfxPayload*> suffixPL, prefixPL;
    std::list<const mfxPayload*>::iterator plIt;

    suffixPL.assign(task.ctrl.Payload, task.ctrl.Payload + (task.ctrl.NumPayload * !!task.ctrl.Payload));
    suffixPL.remove_if([](const mfxPayload* pPL) { return !pPL || !(pPL->CtrlFlags & MFX_PAYLOAD_CTRL_SUFFIX); });
    prefixPL.assign(task.ctrl.Payload, task.ctrl.Payload + (task.ctrl.NumPayload * !!task.ctrl.Payload));
    prefixPL.remove_if([](const mfxPayload* pPL) { return !pPL || (pPL->CtrlFlags & MFX_PAYLOAD_CTRL_SUFFIX); });

     /* It is a requirement of bitstream conformance that when a prefix SEI message with payloadType
     equal to 17 (progressive refinement segment end) or 22 (post-filter hint) is present in an access unit,
     a suffix SEI message with the same value of payloadType shall not be present in the same access unit.*/
    if (prefixPL.end() != std::find_if(prefixPL.begin(), prefixPL.end(), PLTypeEq<17>))
        suffixPL.remove_if(PLTypeEq<17>);
    if (prefixPL.end() != std::find_if(prefixPL.begin(), prefixPL.end(), PLTypeEq<22>))
        suffixPL.remove_if(PLTypeEq<22>);

    if (suffixPL.empty())
        return 0;

    PackNALU(rbsp, nalu);

    std::for_each(suffixPL.begin(), suffixPL.end()
        , [&](const mfxPayload* pPL)
    {
        rbsp.PutBitsBuffer(pPL->NumBit, pPL->Data);
        rbsp.PutTrailingBits(true);
    });

    rbsp.PutTrailingBits();

    return PackRBSP(buf, rbsp.GetStart(), sizeInBytes, CeilDiv(rbsp.GetOffset(), 8u));
}

static void PackSkipCTU(
    mfxU32 xCtu
    , mfxU32 yCtu
    , mfxU32 log2CtuSize
    , BitstreamWriter& bs
    , const SPS& sps
    , const PPS& pps
    , const Slice& slice
    , mfxU32 x0
    , mfxU32 y0
    , CABACContext& ctx)
{
    bool mbA = //is left MB available
        ((yCtu == y0) && xCtu > x0)
        || ((yCtu != y0) && xCtu > 0);
    bool mbB = //is above MB available
        (yCtu != y0)
        && ((xCtu >= x0 && yCtu > y0)
            || (xCtu < x0 && yCtu > (y0 + (1 << log2CtuSize))));
    bool boundary =
        ((xCtu + (1 << log2CtuSize) > sps.pic_width_in_luma_samples)
            || (yCtu + (1 << log2CtuSize) > sps.pic_height_in_luma_samples))
        && (log2CtuSize > (sps.log2_min_luma_coding_block_size_minus3 + 3));
    mfxU8 split_flag = boundary && (log2CtuSize > (sps.log2_min_luma_coding_block_size_minus3 + 3));

    if (!boundary)
        bs.EncodeBin(ctx.SPLIT_CODING_UNIT_FLAG[0], split_flag);

    if (split_flag)
    {
        mfxU32 x1 = xCtu + (1 << (log2CtuSize - 1));
        mfxU32 y1 = yCtu + (1 << (log2CtuSize - 1));
        struct SplitMap
        {
            mfxU32 x;
            mfxU32 y;
            bool   bPackCTU;
        };
        auto PackSplitCTU = [&](const SplitMap& s)
        {
            PackSkipCTU(s.x, s.y, log2CtuSize - 1, bs, sps, pps, slice, x0, y0, ctx);
        };
        std::list<SplitMap> splitMap({
            {xCtu, yCtu, true},
            {x1  , yCtu, x1 < sps.pic_width_in_luma_samples},
            {xCtu, y1  , y1 < sps.pic_height_in_luma_samples},
            {x1  , y1  , x1 < sps.pic_width_in_luma_samples && y1 < sps.pic_height_in_luma_samples}
        });

        splitMap.remove_if([](const SplitMap& s) { return !s.bPackCTU; });

        std::for_each(std::begin(splitMap), std::end(splitMap), PackSplitCTU);
    }
    else
    {
        if (pps.transquant_bypass_enabled_flag)
            bs.EncodeBin(ctx.CU_TRANSQUANT_BYPASS_FLAG[0], 0);

        bs.EncodeBin(ctx.SKIP_FLAG[mbA + mbB], 1);

        if (slice.five_minus_max_num_merge_cand < 4)
            bs.EncodeBin(ctx.MERGE_IDX[0], 0);
    }
}

void Packer::PackSkipSSD(
    BitstreamWriter& bs
    , SPS   const &     sps
    , PPS   const &     pps
    , Slice const &     sh
    , mfxU32 NumLCU)
{
    CABACContext ctx;
    const mfxU32 CtbLog2SizeY = sps.log2_min_luma_coding_block_size_minus3 + 3 + sps.log2_diff_max_min_luma_coding_block_size;
    const mfxU32 CtbSizeY = (1 << CtbLog2SizeY);
    const mfxU32 PicWidthInCtbsY = CeilDiv(sps.pic_width_in_luma_samples, CtbSizeY);

    ctx.Init(pps, sh);

    mfxU32 xCtu0 = (sh.segment_address % PicWidthInCtbsY) << CtbLog2SizeY;
    mfxU32 yCtu0 = (sh.segment_address / PicWidthInCtbsY) << CtbLog2SizeY;
    mfxU32 ctuAdrrEnd = sh.segment_address + NumLCU - 1;

    for (mfxU32 ctuAdrr = sh.segment_address; ctuAdrr < ctuAdrrEnd; ctuAdrr++)
    {
        mfxU32 xCtu = (ctuAdrr % PicWidthInCtbsY) << CtbLog2SizeY;
        mfxU32 yCtu = (ctuAdrr / PicWidthInCtbsY) << CtbLog2SizeY;

        PackSkipCTU(xCtu, yCtu, CtbLog2SizeY, bs, sps, pps, sh, xCtu0, yCtu0, ctx);

        bs.EncodeBin(ctx.END_OF_SLICE_FLAG[0], 0);
    }

    mfxU32 ctu = ctuAdrrEnd;//PicSizeInCtbsY - 1;
    mfxU32 xCtu = (ctu % PicWidthInCtbsY) << CtbLog2SizeY;
    mfxU32 yCtu = (ctu / PicWidthInCtbsY) << CtbLog2SizeY;

    PackSkipCTU(xCtu, yCtu, CtbLog2SizeY, bs, sps, pps, sh, xCtu0, yCtu0, ctx);

    bs.SliceFinish();
}

void Packer::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_Init
        , [this](
            StorageRW& global
            , StorageRW&) -> mfxStatus
    {
        auto ph = make_unique<MakeStorable<PackedHeaders>>(PackedHeaders{});

        m_pGlob = &global;

        mfxStatus sts = Reset(
            Glob::VPS::Get(global)
            , Glob::SPS::Get(global)
            , Glob::PPS::Get(global)
            , Glob::SliceInfo::Get(global)
            , *ph);
        MFX_CHECK_STS(sts);

        auto&                     vpar   = Glob::VideoParam::Get(global);
        mfxExtCodingOptionVPS&    vps    = ExtBuffer::Get(vpar);
        mfxExtCodingOptionSPSPPS& spspps = ExtBuffer::Get(vpar);

        vps.VPSBuffer     = ph->VPS.pData;
        vps.VPSBufSize    = mfxU16(CeilDiv(ph->VPS.BitLen, 8u));
        spspps.SPSBuffer  = ph->SPS.pData;
        spspps.SPSBufSize = mfxU16(CeilDiv(ph->SPS.BitLen, 8u));
        spspps.PPSBuffer  = ph->PPS.pData;
        spspps.PPSBufSize = mfxU16(CeilDiv(ph->PPS.BitLen, 8u));

        global.Insert(Glob::PackedHeaders::Key, std::move(ph));

        return MFX_ERR_NONE;
    });
}

void Packer::ResetState(const FeatureBlocks& /*blocks*/, TPushRS Push)
{
    Push(BLK_Reset
        , [this](
            StorageRW& global
            , StorageRW&) -> mfxStatus
    {
        auto& initState = Glob::RealState::Get(global);
        auto& ph = Glob::PackedHeaders::Get(initState);

        mfxStatus sts = Reset(
            Glob::VPS::Get(global)
            , Glob::SPS::Get(global)
            , Glob::PPS::Get(global)
            , Glob::SliceInfo::Get(global)
            , ph);
        MFX_CHECK_STS(sts);
        
        auto&                     vpar   = Glob::VideoParam::Get(global);
        mfxExtCodingOptionVPS&    vps    = ExtBuffer::Get(vpar);
        mfxExtCodingOptionSPSPPS& spspps = ExtBuffer::Get(vpar);

        vps.VPSBuffer     = ph.VPS.pData;
        vps.VPSBufSize    = mfxU16(CeilDiv(ph.VPS.BitLen, 8u));
        spspps.SPSBuffer  = ph.SPS.pData;
        spspps.SPSBufSize = mfxU16(CeilDiv(ph.SPS.BitLen, 8u));
        spspps.PPSBuffer  = ph.PPS.pData;
        spspps.PPSBufSize = mfxU16(CeilDiv(ph.PPS.BitLen, 8u));

        auto& vparInit = Glob::VideoParam::Get(initState);
        ((mfxExtCodingOptionVPS&)ExtBuffer::Get(vparInit)) = vps;
        ((mfxExtCodingOptionSPSPPS&)ExtBuffer::Get(vparInit)) = spspps;

        return MFX_ERR_NONE;
    });
}

mfxStatus Packer::PackHeader(BitstreamWriter& rbsp, mfxU8* pESBegin, mfxU8* pESEnd, PackedData& d)
{
    d.BitLen = PackRBSP(pESBegin, rbsp.GetStart(), mfxU32(pESEnd - pESBegin), CeilDiv(rbsp.GetOffset(), 8u));
    MFX_CHECK(d.BitLen, MFX_ERR_NOT_ENOUGH_BUFFER);

    d.pData   = pESBegin;
    d.bLongSC = true;
    d.bHasEP  = true;
    d.BitLen  *= 8;

    rbsp.Reset();

    return MFX_ERR_NONE;
};

mfxStatus Packer::Reset(
    const VPS& vps
    , const SPS& sps
    , const PPS& pps
    , const std::vector<SliceInfo>& si
    , PackedHeaders& ph)
{
    MFX_CHECK(m_es.data(), MFX_ERR_UNKNOWN);
    MFX_CHECK(m_rbsp.data(), MFX_ERR_UNKNOWN);

    mfxStatus       sts      = MFX_ERR_NONE;
    auto            pESBegin = m_es.data();
    auto            pESEnd   = pESBegin + m_es.size();
    BitstreamWriter rbsp(m_rbsp.data(), (mfxU32)m_rbsp.size());

    for (mfxU8 i = 0; i < 3; i++)
    {
        BitstreamWriter es(pESBegin, mfxU32(pESEnd - pESBegin));
        PackAUD(es, i);

        auto& d = ph.AUD[i];
        d.pData = es.GetStart();
        d.BitLen = es.GetOffset();
        d.bLongSC = true;
        d.bHasEP = true;
        pESBegin += CeilDiv(d.BitLen, 8u);
    }

    PackVPS(rbsp, vps);
    sts = PackHeader(rbsp, pESBegin, pESEnd, ph.VPS);
    MFX_CHECK_STS(sts);
    pESBegin += ph.VPS.BitLen / 8;

    PackSPS(rbsp, sps);
    sts = PackHeader(rbsp, pESBegin, pESEnd, ph.SPS);
    MFX_CHECK_STS(sts);
    pESBegin += ph.SPS.BitLen / 8;

    PackPPS(rbsp, pps);
    sts = PackHeader(rbsp, pESBegin, pESEnd, ph.PPS);
    MFX_CHECK_STS(sts);
    pESBegin += ph.PPS.BitLen / 8;

    ph.SSH.resize(si.size());

    m_pRTBufBegin = pESBegin;
    m_pRTBufEnd   = pESEnd;

    return MFX_ERR_NONE;
}

mfxU32 Packer::GetPSEIAndSSH(
    const ExtBuffer::Param<mfxVideoParam>& par
    , const TaskCommonPar& task
    , const SPS& sps
    , const PPS& pps
    , Slice sh
    , const std::vector<SliceInfo>& si
    , PackedHeaders& ph
    , mfxU8* buf
    , mfxU32 sizeInBytes)
{
    BitstreamWriter rbsp(m_rbsp.data(), (mfxU32)m_rbsp.size());
    mfxU8*          pBegin   = buf;
    mfxU8*          pEnd     = pBegin + sizeInBytes;
    bool            bSkip    = !!(task.SkipCMD & SKIPCMD_NeedSkipSliceGen);
    bool            bDSS     = !bSkip && !!((const mfxExtCodingOption2&)ExtBuffer::Get(par)).MaxSliceSize;
    bool            bNeedSEI = (task.InsertHeaders & INSERT_SEI)
                                || std::any_of(task.ctrl.Payload, task.ctrl.Payload + task.ctrl.NumPayload,
                                    [](const mfxPayload* pPL) { return pPL && !(pPL->CtrlFlags & MFX_PAYLOAD_CTRL_SUFFIX); });
    auto            PutSSH   = [&](PackedData& d, NALU& nalu)
    {
        rbsp.Reset(pBegin, mfxU32(pEnd - pBegin));
        rbsp.SetInfo(&d.PackInfo);

        PackSSH(rbsp, nalu, sps, pps, sh, bDSS);

        d.BitLen = rbsp.GetOffset();
        pBegin += CeilDiv(d.BitLen, 8u);

        return d.BitLen;
    };
    auto            PutSkip  = [&](PackedData& d, NALU& nalu, mfxU32 nLCU)
    {
        rbsp.Reset();

        PackSSH(rbsp, nalu, sps, pps, sh, bDSS);
        PackSkipSSD(rbsp, sps, pps, sh, nLCU);

        d.BitLen = PackRBSP(pBegin, rbsp.GetStart(), mfxU32(pEnd - pBegin), CeilDiv(rbsp.GetOffset(), 8u));
        pBegin   += d.BitLen;
        d.BitLen *= 8;

        return d.BitLen;
    };

    ph.PrefixSEI.BitLen = 0;

    if (bNeedSEI)
    {
        ph.PrefixSEI.BitLen  = GetPrefixSEI(par, task, sps, pBegin, mfxU32(pEnd - pBegin));
        ph.PrefixSEI.pData   = pBegin;
        pBegin              += ph.PrefixSEI.BitLen;
        ph.PrefixSEI.BitLen *= 8;
        ph.PrefixSEI.bHasEP  = true;
        ph.PrefixSEI.bLongSC = true;

        if (!ph.PrefixSEI.BitLen)
        {
            return 0;
        }
    }

    for (mfxU32 i = 0; i < si.size(); i++)
    {
        auto&   d               = ph.SSH.at(i);
        bool    bLongStartCode  = (!i && !task.InsertHeaders) || task.bForceLongStartCode;
        NALU    nalu            = { mfxU16(bLongStartCode), task.SliceNUT, 0, mfxU16(task.TemporalID + 1) };

        d.pData   = pBegin;
        d.bLongSC = bLongStartCode;
        d.bHasEP  = bSkip;

        sh.first_slice_segment_in_pic_flag  = !i;
        sh.segment_address                  = si.at(i).SegmentAddress;

        bool bFailed =
            !(     (bSkip && PutSkip(d, nalu, si.at(i).NumLCU))
                || (!bSkip && PutSSH(d, nalu)));

        if (bFailed)
        {
            return 0;
        }
    }

    return mfxU32(pBegin - buf);
}

void Packer::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_SubmitTask
        , [this](
            StorageW& global
            , StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);
        OnExit cleanup([&]() { task.PLInternal.clear(); });

        if (task.SkipCMD & SKIPCMD_NeedDriverCall)
        {
            auto& ph  = Glob::PackedHeaders::Get(global);
            auto& pps = Glob::PPS::Get(global);

            if (task.RepackHeaders & INSERT_PPS)
            {
                BitstreamWriter rbsp(m_rbsp.data(), (mfxU32)m_rbsp.size());
                auto pPpsEsBegin = ph.SPS.pData + ph.SPS.BitLen / 8;

                PackPPS(rbsp, pps);

                ThrowAssert(pPpsEsBegin < m_es.data() || pPpsEsBegin > (m_es.data() + m_es.size())
                    , "pPpsEsBegin is out of Packer ES buffer (m_es)");

                auto sts = PackHeader(rbsp, pPpsEsBegin, m_pRTBufEnd, ph.PPS);
                MFX_CHECK_STS(sts);

                m_pRTBufBegin = ph.PPS.pData + ph.PPS.BitLen / 8;
            }

            mfxU32 sz = GetPSEIAndSSH(
                Glob::VideoParam::Get(global)
                , task
                , Glob::SPS::Get(global)
                , pps
                , Task::SSH::Get(s_task)
                , Glob::SliceInfo::Get(global)
                , ph
                , m_pRTBufBegin
                , mfxU32(m_pRTBufEnd - m_pRTBufBegin));
            MFX_CHECK(sz, MFX_ERR_NOT_ENOUGH_BUFFER);
        }
        else if (task.SkipCMD & SKIPCMD_NeedSkipSliceGen)
        {
            FrameLocker bsData(Glob::VideoCore::Get(global), task.BS.Mid);
            auto& ph                = Glob::PackedHeaders::Get(global);
            auto  bsInfo            = Glob::AllocBS::Get(global).GetInfo();
            auto& ssh               = Task::SSH::Get(s_task);
            mfxU32 BsBytesAvailable = bsInfo.Width * bsInfo.Height;
            mfxStatus sts           = MFX_ERR_NONE;

            MFX_CHECK(bsData.Y, MFX_ERR_LOCK_MEMORY);

            auto BSInsert = [&](const PackedData& d) -> mfxStatus
            {
                mfxU32 sz = CeilDiv(d.BitLen, 8u);
                MFX_CHECK(sz <= BsBytesAvailable, MFX_ERR_NOT_ENOUGH_BUFFER);
                MFX_CHECK(d.bHasEP, MFX_ERR_UNDEFINED_BEHAVIOR);

                std::copy(d.pData, d.pData + sz, bsData.Y + task.BsDataLength);

                task.BsDataLength += sz;
                BsBytesAvailable  -= sz;

                return MFX_ERR_NONE;
            };

            bool bErr =
                   ((task.InsertHeaders & INSERT_AUD) && Res2Bool(sts, BSInsert(ph.AUD[mfx::clamp(2 - ssh.type, 0, 2)])))
                || ((task.InsertHeaders & INSERT_VPS) && Res2Bool(sts, BSInsert(ph.VPS)))
                || ((task.InsertHeaders & INSERT_SPS) && Res2Bool(sts, BSInsert(ph.SPS)))
                || ((task.InsertHeaders & INSERT_PPS) && Res2Bool(sts, BSInsert(ph.PPS)));
            MFX_CHECK(!bErr, sts);

            mfxU32 sz = GetPSEIAndSSH(
                Glob::VideoParam::Get(global)
                , task
                , Glob::SPS::Get(global)
                , Glob::PPS::Get(global)
                , ssh
                , Glob::SliceInfo::Get(global)
                , ph
                , bsData.Y + task.BsDataLength
                , BsBytesAvailable);
            MFX_CHECK(sz, MFX_ERR_NOT_ENOUGH_BUFFER);

            task.BsDataLength += sz;
        }

        return MFX_ERR_NONE;
    });
}

void Packer::QueryTask(const FeatureBlocks& /*blocks*/, TPushQT Push)
{
    Push(BLK_InsertSuffixSEI
        , [this](
            StorageW& /*global*/
            , StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);

        if (task.ctrl.NumPayload)
        {
            mfxU32 sz = GetSuffixSEI(task, task.pBsData + task.BsDataLength, task.BsBytesAvailable);
            MFX_CHECK(!sz || !task.bDontPatchBS, MFX_ERR_UNDEFINED_BEHAVIOR);

            task.BsDataLength     += sz;
            task.BsBytesAvailable -= sz;
        }

        return MFX_ERR_NONE;
    });
}


#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
