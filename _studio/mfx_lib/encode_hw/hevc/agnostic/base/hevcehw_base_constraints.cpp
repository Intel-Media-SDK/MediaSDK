// Copyright (c) 2019-2020 Intel Corporation
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

#include "hevcehw_base.h"
#include "hevcehw_base_constraints.h"
#include <algorithm>
#include <math.h>
#include <cmath>

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

const mfxU32 TableA1[][6] =
{
//  Level   |MaxLumaPs |    MaxCPB    | MaxSlice  | MaxTileRows | MaxTileCols
//                                      Segments  |
//                                      PerPicture|
/*  1   */ {   36864,      350,    350,        16,            1,           1},
/*  2   */ {  122880,     1500,   1500,        16,            1,           1},
/*  2.1 */ {  245760,     3000,   3000,        20,            1,           1},
/*  3   */ {  552960,     6000,   6000,        30,            2,           2},
/*  3.1 */ {  983040,    10000,  10000,        40,            3,           3},
/*  4   */ { 2228224,    12000,  30000,        75,            5,           5},
/*  4.1 */ { 2228224,    20000,  50000,        75,            5,           5},
/*  5   */ { 8912896,    25000, 100000,       200,           11,          10},
/*  5.1 */ { 8912896,    40000, 160000,       200,           11,          10},
/*  5.2 */ { 8912896,    60000, 240000,       200,           11,          10},
/*  6   */ {35651584,    60000, 240000,       600,           22,          20},
/*  6.1 */ {35651584,   120000, 480000,       600,           22,          20},
/*  6.2 */ {35651584,   240000, 800000,       600,           22,          20},
};

const mfxU32 TableA2[][4] =
{
//  Level   | MaxLumaSr |      MaxBR   | MinCr
/*  1    */ {    552960,    128,    128,    2},
/*  2    */ {   3686400,   1500,   1500,    2},
/*  2.1  */ {   7372800,   3000,   3000,    2},
/*  3    */ {  16588800,   6000,   6000,    2},
/*  3.1  */ {  33177600,  10000,  10000,    2},
/*  4    */ {  66846720,  12000,  30000,    4},
/*  4.1  */ { 133693440,  20000,  50000,    4},
/*  5    */ { 267386880,  25000, 100000,    6},
/*  5.1  */ { 534773760,  40000, 160000,    8},
/*  5.2  */ {1069547520,  60000, 240000,    8},
/*  6    */ {1069547520,  60000, 240000,    8},
/*  6.1  */ {2139095040, 120000, 480000,    8},
/*  6.2  */ {4278190080, 240000, 800000,    6},
};

const mfxU16 MaxLidx = (sizeof(TableA1) / sizeof(TableA1[0])) - 1;

mfxU16 LevelIdx(mfxU16 mfx_level)
{
    static const std::map<mfxU16, mfxU16> LevelMFX2STD =
    {
          {(mfxU16)MFX_LEVEL_HEVC_1 , (mfxU16) 0}
        , {(mfxU16)MFX_LEVEL_HEVC_2 , (mfxU16) 1}
        , {(mfxU16)MFX_LEVEL_HEVC_21, (mfxU16) 2}
        , {(mfxU16)MFX_LEVEL_HEVC_3 , (mfxU16) 3}
        , {(mfxU16)MFX_LEVEL_HEVC_31, (mfxU16) 4}
        , {(mfxU16)MFX_LEVEL_HEVC_4 , (mfxU16) 5}
        , {(mfxU16)MFX_LEVEL_HEVC_41, (mfxU16) 6}
        , {(mfxU16)MFX_LEVEL_HEVC_5 , (mfxU16) 7}
        , {(mfxU16)MFX_LEVEL_HEVC_51, (mfxU16) 8}
        , {(mfxU16)MFX_LEVEL_HEVC_52, (mfxU16) 9}
        , {(mfxU16)MFX_LEVEL_HEVC_6 , (mfxU16)10}
        , {(mfxU16)MFX_LEVEL_HEVC_61, (mfxU16)11}
        , {(mfxU16)MFX_LEVEL_HEVC_62, (mfxU16)12}
    };
    return LevelMFX2STD.at(mfx_level & 0xFF);
}

mfxU16 TierIdx(mfxU16 mfx_level)
{
    return !!(mfx_level & MFX_TIER_HEVC_HIGH);
}

mfxU16 GetMaxDpbSize(mfxU32 PicSizeInSamplesY, mfxU32 MaxLumaPs, mfxU16 maxDpbPicBuf)
{
    if (PicSizeInSamplesY <= (MaxLumaPs >> 2))
        return std::min<mfxU16>(4 * maxDpbPicBuf, 16);
    else if (PicSizeInSamplesY <= (MaxLumaPs >> 1))
        return std::min<mfxU16>(2 * maxDpbPicBuf, 16);
    else if (PicSizeInSamplesY <= ((3 * MaxLumaPs) >> 2))
        return std::min<mfxU16>((4 * maxDpbPicBuf) / 3, 16U);

    return maxDpbPicBuf;
}

mfxU16 HEVCEHW::Base::GetMaxDpbSizeByLevel(mfxVideoParam const & par, mfxU32 PicSizeInSamplesY)
{
    assert(par.mfx.CodecLevel != 0);
    return GetMaxDpbSize(PicSizeInSamplesY, TableA1[LevelIdx(par.mfx.CodecLevel)][0], 6);
}

mfxU32 HEVCEHW::Base::GetMaxKbpsByLevel(mfxVideoParam const & par)
{
    assert(par.mfx.CodecLevel != 0);
    return TableA2[LevelIdx(par.mfx.CodecLevel)][1 + TierIdx(par.mfx.CodecLevel)] * 1100 / 1000;
}

mfxF64 HEVCEHW::Base::GetMaxFrByLevel(mfxVideoParam const & par, mfxU32 PicSizeInSamplesY)
{
    assert(par.mfx.CodecLevel != 0);

    mfxU32 MaxLumaSr = TableA2[LevelIdx(par.mfx.CodecLevel)][0];

    return (mfxF64)MaxLumaSr / PicSizeInSamplesY;
}

mfxU32 HEVCEHW::Base::GetMaxCpbInKBByLevel(mfxVideoParam const & par)
{
    assert(par.mfx.CodecLevel != 0);

    return TableA1[LevelIdx(par.mfx.CodecLevel)][1 + TierIdx(par.mfx.CodecLevel)] * 1100 / 8000;
}

const mfxU16 LevelIdxToMfx[] =
{
    MFX_LEVEL_HEVC_1 ,
    MFX_LEVEL_HEVC_2 ,
    MFX_LEVEL_HEVC_21,
    MFX_LEVEL_HEVC_3 ,
    MFX_LEVEL_HEVC_31,
    MFX_LEVEL_HEVC_4 ,
    MFX_LEVEL_HEVC_41,
    MFX_LEVEL_HEVC_5 ,
    MFX_LEVEL_HEVC_51,
    MFX_LEVEL_HEVC_52,
    MFX_LEVEL_HEVC_6 ,
    MFX_LEVEL_HEVC_61,
    MFX_LEVEL_HEVC_62,
};

inline mfxU16 MaxTidx(mfxU16 l) { return (LevelIdxToMfx[std::min<mfxU16>(l, MaxLidx)] >= MFX_LEVEL_HEVC_4); }

mfxU16 MfxLevel(mfxU16 l, mfxU16 t) { return LevelIdxToMfx[std::min<mfxU16>(l, MaxLidx)] | (MFX_TIER_HEVC_HIGH * !!t); }

mfxU16 HEVCEHW::Base::GetMinLevel(
      mfxU32 frN
    , mfxU32 frD
    , mfxU16 PicWidthInLumaSamples
    , mfxU16 PicHeightInLumaSamples
    , mfxU16 MinRef
    , mfxU16 NumTileColumns
    , mfxU16 NumTileRows
    , mfxU32 NumSlice
    , mfxU32 BufferSizeInKB
    , mfxU32 MaxKbps
    , mfxU16 StartLevel)
{
    bool bInvalid = frN == 0 || frD == 0;
    if (bInvalid)
        return 0;

    mfxU32 CpbBrNalFactor    = 1100;
    mfxU32 PicSizeInSamplesY = (mfxU32)PicWidthInLumaSamples * PicHeightInLumaSamples;
    mfxU32 LumaSr            = mfxU32(std::ceil(mfxF64(PicSizeInSamplesY) * frN / frD));

    mfxU16 lidx = LevelIdx(StartLevel);
    mfxU16 tidx = TierIdx(StartLevel);
    bool bSearch = true;

    while (bSearch)
    {
        mfxU32 MaxLumaPs   = TableA1[lidx][0];
        mfxU32 MaxCPB      = TableA1[lidx][1+tidx];
        mfxU32 MaxSSPP     = TableA1[lidx][3];
        mfxU32 MaxTileRows = TableA1[lidx][4];
        mfxU32 MaxTileCols = TableA1[lidx][5];
        mfxU32 MaxLumaSr   = TableA2[lidx][0];
        mfxU32 MaxBR       = TableA2[lidx][1+tidx];
        //mfxU32 MinCR       = TableA2[lidx][3];
        mfxU16 MaxDpbSize  = GetMaxDpbSize(PicSizeInSamplesY, MaxLumaPs, 6);

        bool bNextTier =
            BufferSizeInKB * 8000 > CpbBrNalFactor * MaxCPB
            || LumaSr > MaxLumaSr
            || MaxKbps * 1000 > CpbBrNalFactor * MaxBR;
        bool bMaxTier = bNextTier && tidx >= MaxTidx(lidx);

        bool bNextLevel =
            PicSizeInSamplesY > MaxLumaPs
            || PicWidthInLumaSamples > sqrt((mfxF64)MaxLumaPs * 8)
            || PicHeightInLumaSamples > sqrt((mfxF64)MaxLumaPs * 8)
            || MinRef + 1 > MaxDpbSize
            || NumTileColumns > MaxTileCols
            || NumTileRows > MaxTileRows
            || NumSlice > MaxSSPP
            || bMaxTier;

        bNextTier &= !bNextLevel;

        tidx *= !bMaxTier;
        tidx += bNextTier;
        lidx += bNextLevel;

        bSearch = (lidx <= MaxLidx) && (bNextTier || bNextLevel);
    }

    return MfxLevel(lidx, tidx);
}



#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)