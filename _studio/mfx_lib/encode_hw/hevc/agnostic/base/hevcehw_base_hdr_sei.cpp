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

#include "hevcehw_base_hdr_sei.h"
#include "hevcehw_base_packer.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void HdrSei::SetSupported(ParamSupport& blocks)
{
    blocks.m_ebCopySupported[MFX_EXTBUFF_CONTENT_LIGHT_LEVEL_INFO].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        auto& src = *(const mfxExtContentLightLevelInfo*)pSrc;
        auto& dst = *(mfxExtContentLightLevelInfo*)pDst;

        dst.InsertPayloadToggle     = src.InsertPayloadToggle;
        dst.MaxContentLightLevel    = src.MaxContentLightLevel;
        dst.MaxPicAverageLightLevel = src.MaxPicAverageLightLevel;
    });

    blocks.m_ebCopySupported[MFX_EXTBUFF_MASTERING_DISPLAY_COLOUR_VOLUME].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        auto& src = *(const mfxExtMasteringDisplayColourVolume*)pSrc;
        auto& dst = *(mfxExtMasteringDisplayColourVolume*)pDst;

        dst.InsertPayloadToggle = src.InsertPayloadToggle;
        std::copy_n(src.DisplayPrimariesX, Size(src.DisplayPrimariesX), dst.DisplayPrimariesX);
        std::copy_n(src.DisplayPrimariesY, Size(src.DisplayPrimariesY), dst.DisplayPrimariesY);
        dst.WhitePointX = src.WhitePointX;
        dst.WhitePointY = src.WhitePointY;
        dst.MaxDisplayMasteringLuminance = src.MaxDisplayMasteringLuminance;
        dst.MinDisplayMasteringLuminance = src.MinDisplayMasteringLuminance;
    });
}

void PackSEIPayload(
    BitstreamWriter& bs
    , const mfxExtMasteringDisplayColourVolume & DisplayColour)
{
    mfxU32 size = CeilDiv(bs.GetOffset(), 8u);
    mfxU8* pl = bs.GetStart() + size; // payload start

    bs.PutBits(8, 137);  //payload type
    bs.PutBits(8, 0xff); //place for payload  size
    size += 2;

    for (int i = 0; i < 3; i++)
    {
        bs.PutBits(16, DisplayColour.DisplayPrimariesX[i]);
        bs.PutBits(16, DisplayColour.DisplayPrimariesY[i]);
    }
    bs.PutBits(16, DisplayColour.WhitePointX);
    bs.PutBits(16, DisplayColour.WhitePointY);
    bs.PutBits(32, DisplayColour.MaxDisplayMasteringLuminance);
    bs.PutBits(32, DisplayColour.MinDisplayMasteringLuminance);

    size = CeilDiv(bs.GetOffset(), 8u) - size;

    assert(size < 256);
    pl[1] = (mfxU8)size; //payload size
}

void PackSEIPayload(
    BitstreamWriter& bs
    , const mfxExtContentLightLevelInfo & LightLevel)
{
    mfxU32 size = CeilDiv(bs.GetOffset(), 8u);
    mfxU8* pl = bs.GetStart() + size; // payload start

    bs.PutBits(8, 144);  //payload type
    bs.PutBits(8, 0xff); //place for payload  size
    size += 2;

    bs.PutBits(16, LightLevel.MaxContentLightLevel);
    bs.PutBits(16, LightLevel.MaxPicAverageLightLevel);

    size = CeilDiv(bs.GetOffset(), 8u) - size;

    assert(size < 256);
    pl[1] = (mfxU8)size; //payload size
}

void HdrSei::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckAndFixMDCV
        , [](const mfxVideoParam& /*in*/, mfxVideoParam& par, StorageW& /*global*/) -> mfxStatus
    {
        mfxExtMasteringDisplayColourVolume* pMDCV = ExtBuffer::Get(par);
        MFX_CHECK(pMDCV, MFX_ERR_NONE);
        mfxU32 changed = 0;

        changed += CheckOrZero<mfxU16, MFX_PAYLOAD_OFF, MFX_PAYLOAD_IDR>(pMDCV->InsertPayloadToggle);
        changed += CheckMaxOrClip(pMDCV->WhitePointX, 50000u);
        changed += CheckMaxOrClip(pMDCV->WhitePointY, 50000u);
        changed += CheckMaxOrClip(pMDCV->DisplayPrimariesX[0], 50000u);
        changed += CheckMaxOrClip(pMDCV->DisplayPrimariesX[1], 50000u);
        changed += CheckMaxOrClip(pMDCV->DisplayPrimariesX[2], 50000u);
        changed += CheckMaxOrClip(pMDCV->DisplayPrimariesY[0], 50000u);
        changed += CheckMaxOrClip(pMDCV->DisplayPrimariesY[1], 50000u);
        changed += CheckMaxOrClip(pMDCV->DisplayPrimariesY[2], 50000u);

        return changed ? MFX_WRN_INCOMPATIBLE_VIDEO_PARAM : MFX_ERR_NONE;
    });
    Push(BLK_CheckAndFixCLLI
        , [](const mfxVideoParam& /*in*/, mfxVideoParam& par, StorageW& /*global*/) -> mfxStatus
    {
        mfxExtContentLightLevelInfo* pCLLI = ExtBuffer::Get(par);
        MFX_CHECK(pCLLI, MFX_ERR_NONE);
        mfxU32 changed = 0;

        changed += CheckOrZero<mfxU16, MFX_PAYLOAD_OFF, MFX_PAYLOAD_IDR>(pCLLI->InsertPayloadToggle);

        return changed ? MFX_WRN_INCOMPATIBLE_VIDEO_PARAM : MFX_ERR_NONE;
    });
}

void HdrSei::SetDefaults(const FeatureBlocks& /*blocks*/, TPushSD Push)
{
    Push(BLK_SetDefaultsMDCV
        , [](mfxVideoParam& par, StorageW& /*strg*/, StorageRW&)
    {
        mfxExtMasteringDisplayColourVolume* pMDCV = ExtBuffer::Get(par);
        MFX_CHECK(pMDCV, MFX_ERR_NONE);

        SetDefault<mfxU16>(pMDCV->InsertPayloadToggle, MFX_PAYLOAD_OFF);

        return MFX_ERR_NONE;
    });
    Push(BLK_SetDefaultsCLLI
        , [](mfxVideoParam& par, StorageW& /*strg*/, StorageRW&)
    {
        mfxExtContentLightLevelInfo* pCLLI = ExtBuffer::Get(par);
        MFX_CHECK(pCLLI, MFX_ERR_NONE);

        SetDefault<mfxU16>(pCLLI->InsertPayloadToggle, MFX_PAYLOAD_OFF);

        return MFX_ERR_NONE;
    });
}

void HdrSei::SubmitTask(const FeatureBlocks& /*blocks*/, TPushST Push)
{
    Push(BLK_InsertPayloads
        , [this](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(global);
        auto& task = Task::Common::Get(s_task);

        const mfxExtMasteringDisplayColourVolume* pMDCV = ExtBuffer::Get(task.ctrl);
        const mfxExtContentLightLevelInfo* pCLLI = ExtBuffer::Get(task.ctrl);

        bool bInsertMDCV = !!pMDCV;
        bool bInsertCLLI = !!pCLLI;

        SetDefault(pMDCV, (const mfxExtMasteringDisplayColourVolume*)ExtBuffer::Get(par));
        SetDefault(pCLLI, (const mfxExtContentLightLevelInfo*)ExtBuffer::Get(par));

        bInsertMDCV |= (pMDCV->InsertPayloadToggle == MFX_PAYLOAD_IDR) && (task.FrameType & MFX_FRAMETYPE_IDR);
        bInsertCLLI |= (pCLLI->InsertPayloadToggle == MFX_PAYLOAD_IDR) && (task.FrameType & MFX_FRAMETYPE_IDR);

        MFX_CHECK(bInsertMDCV || bInsertCLLI, MFX_ERR_NONE);

        BitstreamWriter bs(m_buf.data(), (mfxU32)m_buf.size(), 0);

        if (bInsertMDCV)
        {
            mfxPayload pl = {};

            pl.Type     = 137;
            pl.NumBit   = bs.GetOffset();
            pl.BufSize  = (mfxU16)CeilDiv(pl.NumBit, 8u);
            pl.Data     = bs.GetStart() + pl.BufSize;

            PackSEIPayload(bs, *pMDCV);

            pl.NumBit   = bs.GetOffset() - pl.NumBit;
            pl.BufSize  = (mfxU16)CeilDiv(pl.NumBit, 8u);

            task.PLInternal.push_back(pl);

            task.InsertHeaders |= INSERT_DCVSEI;
        }

        if (bInsertCLLI)
        {
            mfxPayload pl = {};

            pl.Type     = 144;
            pl.NumBit   = bs.GetOffset();
            pl.BufSize  = (mfxU16)CeilDiv(pl.NumBit, 8u);
            pl.Data     = bs.GetStart() + pl.BufSize;

            PackSEIPayload(bs, *pCLLI);

            pl.NumBit   = bs.GetOffset() - pl.NumBit;
            pl.BufSize  = (mfxU16)CeilDiv(pl.NumBit, 8u);

            task.PLInternal.push_back(pl);

            task.InsertHeaders |= INSERT_LLISEI;
        }

        return MFX_ERR_NONE;
    });
}


#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
