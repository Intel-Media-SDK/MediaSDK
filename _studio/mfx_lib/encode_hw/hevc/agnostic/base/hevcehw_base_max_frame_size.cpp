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

#include "hevcehw_base_max_frame_size.h"
#include "hevcehw_base_legacy.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void MaxFrameSize::SetSupported(ParamSupport& blocks)
{
    blocks.m_ebCopySupported[MFX_EXTBUFF_CODING_OPTION2].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        auto& src = *(const mfxExtCodingOption2*)pSrc;
        auto& dst = *(mfxExtCodingOption2*)pDst;

        dst.MaxFrameSize = src.MaxFrameSize;
    });
}

void MaxFrameSize::SetInherited(ParamInheritance& par)
{
    par.m_ebInheritDefault[MFX_EXTBUFF_CODING_OPTION2].emplace_back(
        [](const mfxVideoParam& /*parInit*/
            , const mfxExtBuffer* pSrc
            , const mfxVideoParam& /*parReset*/
            , mfxExtBuffer* pDst)
    {
        auto& src = *(const mfxExtCodingOption2*)pSrc;
        auto& dst = *(mfxExtCodingOption2*)pDst;

        InheritOption(src.MaxFrameSize, dst.MaxFrameSize);
    });
}

void MaxFrameSize::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckAndFix
        , [](const mfxVideoParam& /*in*/, mfxVideoParam& par, StorageW& global) -> mfxStatus
    {
        mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);
        MFX_CHECK(pCO2 && pCO2->MaxFrameSize, MFX_ERR_NONE);

        auto& caps = Glob::EncodeCaps::Get(global);
        auto& MaxFrameSize = pCO2->MaxFrameSize;

        mfxU32 changed = 0;
        bool   bSupported = (Legacy::IsSWBRC(par, pCO2) || caps.UserMaxFrameSizeSupport)
            && !Check(par.mfx.RateControlMethod, mfxU16(MFX_RATECONTROL_VBR), mfxU16(MFX_RATECONTROL_QVBR));

        mfxU32 MinValid = 0;
        mfxU32 MaxValid = MaxFrameSize * bSupported;

        if (par.mfx.FrameInfo.FrameRateExtN && par.mfx.FrameInfo.FrameRateExtD)
        {
            MinValid = mfxU32(TargetKbps(par.mfx) * 1000 * par.mfx.FrameInfo.FrameRateExtD / par.mfx.FrameInfo.FrameRateExtN / 8);
            MaxValid = std::max<mfxU32>(MaxValid, MinValid) * bSupported;
        }

        changed += CheckMinOrClip(MaxFrameSize, MinValid);
        changed += CheckMaxOrZero(MaxFrameSize, MaxValid);

        return changed ? MFX_WRN_INCOMPATIBLE_VIDEO_PARAM : MFX_ERR_NONE;
    });
}

void MaxFrameSize::SetDefaults(const FeatureBlocks& /*blocks*/, TPushSD Push)
{
    Push(BLK_SetDefaults
        , [](mfxVideoParam& par, StorageW& /*strg*/, StorageRW&)
    {
        mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

        MFX_CHECK(pCO3 && pCO2 && IsOn(pCO3->LowDelayBRC), MFX_ERR_NONE);

        mfxF64 frameRate = mfxF64(par.mfx.FrameInfo.FrameRateExtN) / par.mfx.FrameInfo.FrameRateExtD;
        mfxU32 avgFrameSizeInBytes = mfxU32(MaxKbps(par.mfx) * 1000 / frameRate / 8);

        SetDefault<mfxU32>(pCO2->MaxFrameSize, 2 * avgFrameSizeInBytes);

        return MFX_ERR_NONE;
    });
}

void MaxFrameSize::Reset(const FeatureBlocks& /*blocks*/, TPushR Push)
{
    Push(BLK_Reset
        , [](
            const mfxVideoParam& /*par*/
            , StorageRW& global
            , StorageRW& /*local*/) -> mfxStatus
    {
        auto& parOld = Glob::VideoParam::Get(Glob::RealState::Get(global));
        auto& parNew = Glob::VideoParam::Get(global);
        mfxExtCodingOption2& CO2Old = ExtBuffer::Get(parOld);
        mfxExtCodingOption2& CO2New = ExtBuffer::Get(parNew);
        auto& hint = Glob::ResetHint::Get(global);

        hint.Flags |= RF_BRC_RESET *
            ((     parOld.mfx.RateControlMethod == MFX_RATECONTROL_CBR
                || parOld.mfx.RateControlMethod == MFX_RATECONTROL_VBR
                || parOld.mfx.RateControlMethod == MFX_RATECONTROL_VCM)
            && (CO2Old.MaxFrameSize != CO2New.MaxFrameSize));

        return MFX_ERR_NONE;
    });
}


#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
