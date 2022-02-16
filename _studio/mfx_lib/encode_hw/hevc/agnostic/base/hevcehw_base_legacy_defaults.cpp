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

#include "hevcehw_base_legacy.h"
#include "hevcehw_base_data.h"
#include "hevcehw_base_constraints.h"
#include <numeric>
#include <set>

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

class TemporalLayers
{
public:
    TemporalLayers()
        : m_numTL(1)
    {
        m_TL[0].Scale = 1;
    }
    TemporalLayers(const mfxExtAvcTemporalLayers& tl)
    {
        SetTL(tl);
    }

    ~TemporalLayers() {}

    static mfxU16 CountTL(const mfxExtAvcTemporalLayers& tl);

    void SetTL(mfxExtAvcTemporalLayers const & tl)
    {
        m_numTL = 0;
        memset(&m_TL, 0, sizeof(m_TL));
        m_TL[0].Scale = 1;

        for (mfxU8 i = 0; i < 7; i++)
        {
            if (tl.Layer[i].Scale)
            {
                m_TL[m_numTL].TId = i;
                m_TL[m_numTL].Scale = (mfxU8)tl.Layer[i].Scale;
                m_numTL++;
            }
        }

        m_numTL = std::max<mfxU8>(m_numTL, 1);
    }

    mfxU8 NumTL() const { return m_numTL; }

    mfxU8 GetTId(mfxU32 frameOrder) const
    {
        mfxU16 i;

        if (m_numTL < 1 || m_numTL > 8)
            return 0;

        for (i = 0; i < m_numTL && (frameOrder % (m_TL[m_numTL - 1].Scale / m_TL[i].Scale)); i++);

        return (i < m_numTL) ? m_TL[i].TId : 0;
    }

    mfxU8 HighestTId() const
    {
        mfxU8 htid = m_TL[m_numTL - 1].TId;
        return mfxU8(htid + !htid * -1);
    }

private:
    mfxU8 m_numTL;

    struct
    {
        mfxU8 TId = 0;
        mfxU8 Scale = 0;
    }m_TL[8];
};

mfxU16 TemporalLayers::CountTL(const mfxExtAvcTemporalLayers& tl)
{
    typedef decltype(tl.Layer[0]) TRLayer;
    return std::max<mfxU16>(1, (mfxU16)std::count_if(
        tl.Layer, tl.Layer + Size(tl.Layer), [](TRLayer l) { return !!l.Scale; }));
}

class GetDefault
{
public:
    static mfxU16 CodedPicAlignment(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        return mfxU16((2 - (par.hw >= MFX_HW_CNL)) * 8);
    }

    static mfxU16 CodedPicWidth(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtHEVCParam* pHEVC = ExtBuffer::Get(par.mvp);

        if (pHEVC && pHEVC->PicWidthInLumaSamples)
        {
            return pHEVC->PicWidthInLumaSamples;
        }

        auto&   fi          = par.mvp.mfx.FrameInfo;
        bool    bCropsValid = fi.CropW > 0;
        mfxU16  W           = mfxU16(bCropsValid * (fi.CropW + fi.CropX) + !bCropsValid * fi.Width);

        return mfx::align2_value(W, par.base.GetCodedPicAlignment(par));
    }

    static mfxU16 CodedPicHeight(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtHEVCParam* pHEVC = ExtBuffer::Get(par.mvp);

        if (pHEVC && pHEVC->PicHeightInLumaSamples)
        {
            return pHEVC->PicHeightInLumaSamples;
        }

        auto&   fi          = par.mvp.mfx.FrameInfo;
        bool    bCropsValid = fi.CropH > 0;
        mfxU16  H           = mfxU16(bCropsValid * (fi.CropH + fi.CropY) + !bCropsValid * fi.Height);

        return mfx::align2_value(H, par.base.GetCodedPicAlignment(par));
    }

    static mfxU16 MaxDPB(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        if (!par.mvp.mfx.CodecLevel)
        {
            return 16;
        }
        auto W = par.base.GetCodedPicWidth(par);
        auto H = par.base.GetCodedPicHeight(par);
        return GetMaxDpbSizeByLevel(par.mvp, W * H);
    }

    static mfxU16 GopPicSize(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        if (par.mvp.mfx.GopPicSize)
        {
            return par.mvp.mfx.GopPicSize;
        }
        if (par.mvp.mfx.CodecProfile == MFX_PROFILE_HEVC_MAINSP)
        {
            return 1;
        }
        return mfxU16(GOP_INFINITE);
    }

    static mfxU16 GopRefDist(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        if (par.mvp.mfx.GopRefDist)
        {
            return par.mvp.mfx.GopRefDist;
        }
        auto GopPicSize = par.base.GetGopPicSize(par);
        const mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par.mvp);
        bool bNoB =
            (pCO2 && pCO2->IntRefType)
            || par.base.GetNumTemporalLayers(par) > 1
            || par.caps.SliceIPOnly
            || GopPicSize < 3
            || par.mvp.mfx.NumRefFrame == 1;
        bool bCQP =
            par.mvp.mfx.RateControlMethod == MFX_RATECONTROL_CQP
            || Legacy::IsSWBRC(par.mvp, ExtBuffer::Get(par.mvp));

        if (bNoB)
        {
            return 1;
        }

        return std::min<mfxU16>(GopPicSize - 1, 4 * (1 + bCQP));
    }

    static mfxU16 NumBPyramidLayers(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        mfxU16 refB = (par.base.GetGopRefDist(par) - 1) / 2;
        mfxU16 x    = refB;

        while (x > 2)
        {
            x = (x - 1) / 2;
            refB -= x;
        }

        return refB + 1;
    }

    static mfxU16 NumRefBPyramid(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        mfxU16 NumRefActiveP[8], NumRefActiveBL0[8], NumRefActiveBL1[8];
        mfxU16 NumLayers = par.base.GetNumBPyramidLayers(par);
        mfxU16 NumRefFrame = par.base.GetMinRefForBPyramid(par);
        bool bExternalNRef = par.base.GetNumRefActive(par, &NumRefActiveP, &NumRefActiveBL0, &NumRefActiveBL1);

        SetIf(NumRefFrame, bExternalNRef, [&]() -> mfxU16
        {
            auto maxBL0idx = std::max_element(NumRefActiveBL0, NumRefActiveBL0 + NumLayers) - NumRefActiveBL0;
            auto maxBL1idx = std::max_element(NumRefActiveBL1, NumRefActiveBL1 + NumLayers) - NumRefActiveBL1;
            mfxU16 maxBL0 = mfxU16((NumRefActiveBL0[maxBL0idx] + maxBL0idx + 1) * (maxBL0idx < NumLayers));
            mfxU16 maxBL1 = mfxU16((NumRefActiveBL1[maxBL1idx] + maxBL1idx + 1) * (maxBL1idx < NumLayers));
            return std::max<mfxU16>({ NumRefFrame, NumRefActiveP[0], maxBL0, maxBL1 });
        });

        return NumRefFrame;
    }

    static mfxU16 NumRefPPyramid(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        mfxU16 NumRefActiveP[8];
        par.base.GetNumRefActive(par, &NumRefActiveP, nullptr, nullptr);
        mfxU16 RefActiveP = *std::max_element(NumRefActiveP, NumRefActiveP + Size(NumRefActiveP));
        return std::min<mfxU16>(DEFAULT_PPYR_INTERVAL, RefActiveP);
    }

    static mfxU16 NumRefNoPyramid(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        mfxU16 NumRefActiveP[8], NumRefActiveBL0[8];
        par.base.GetNumRefActive(par, &NumRefActiveP, &NumRefActiveBL0, nullptr);
        mfxU16 RefActiveP = *std::max_element(NumRefActiveP, NumRefActiveP + Size(NumRefActiveP));
        mfxU16 RefActiveBL0 = *std::max_element(NumRefActiveBL0, NumRefActiveBL0 + Size(NumRefActiveBL0));
        return mfxU16(std::max<mfxU16>(RefActiveP, RefActiveBL0) + (par.base.GetGopRefDist(par) > 1) * RefActiveBL0);
    }

    static mfxU16 NumRefFrames(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        auto NumRefBPyramid = [&]() { return par.base.GetNumRefBPyramid(par); };
        auto NumRefPPyramid = [&]() { return par.base.GetNumRefPPyramid(par); };
        auto NumRefDefault = [&]() { return par.base.GetNumRefNoPyramid(par); };

        bool bUsrValue = !!par.mvp.mfx.NumRefFrame;
        bool bBPyramid = !bUsrValue && (par.base.GetBRefType(par) == MFX_B_REF_PYRAMID);
        bool bPPyramyd = !bUsrValue && !bBPyramid && (par.base.GetPRefType(par) == MFX_P_REF_PYRAMID);
        bool bDefault = !bUsrValue && !bBPyramid && !bPPyramyd;

        mfxU16 NumRefFrame = par.mvp.mfx.NumRefFrame;
        SetIf(NumRefFrame, bBPyramid, NumRefBPyramid);
        SetIf(NumRefFrame, bPPyramyd, NumRefPPyramid);
        SetIf(NumRefFrame, bDefault, NumRefDefault);

        SetIf(NumRefFrame, !bUsrValue, [&]()
        {
            return std::min<mfxU16>(
                par.base.GetMaxDPB(par) - 1
                , std::max<mfxU16>(par.base.GetNumTemporalLayers(par) - 1, NumRefFrame));
        });

        return NumRefFrame;
    }

    static mfxU16 MinRefForBPyramid(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        return par.base.GetNumBPyramidLayers(par) + 1;
    }

    static mfxU16 MinRefForBNoPyramid(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& /*par*/)
    {
        return 2;
    }

    static bool NumRefActive(
        Defaults::TGetNumRefActive::TExt
        , const Defaults::Param& par
        , mfxU16(*pP)[8]
        , mfxU16(*pBL0)[8]
        , mfxU16(*pBL1)[8])
    {
        bool bExternal = false;
        mfxU16 maxP = 0, maxBL0 = 0, maxBL1 = 0;
        std::tie(maxP, maxBL0, maxBL1) = par.base.GetMaxNumRef(par);

        auto SetDefaultNRef =
            [](const mfxU16(*extRef)[8], mfxU16 defaultRef, mfxU16(*NumRefActive)[8])
        {
            bool bExternal = false;
            bool bDone = false;

            bDone |= !NumRefActive;
            bDone |= !bDone && !extRef && std::fill_n(*NumRefActive, 8, defaultRef);
            bDone |= !bDone && std::transform(
                *extRef
                , std::end(*extRef)
                , *NumRefActive
                , [&](mfxU16 ext)
            {
                bExternal |= SetIf(defaultRef, !!ext, ext);
                return defaultRef;
            });

            return bExternal;
        };

        const mfxU16(*extRefP  )[8] = nullptr;
        const mfxU16(*extRefBL0)[8] = nullptr;
        const mfxU16(*extRefBL1)[8] = nullptr;

        if (const mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par.mvp))
        {
            extRefP   = &pCO3->NumRefActiveP;
            extRefBL0 = &pCO3->NumRefActiveBL0;
            extRefBL1 = &pCO3->NumRefActiveBL1;
        }

        bExternal |= SetDefaultNRef(extRefP, maxP, pP);
        bExternal |= SetDefaultNRef(extRefBL0, maxBL0, pBL0);
        bExternal |= SetDefaultNRef(extRefBL1, maxBL1, pBL1);

        return bExternal;
    }

    static mfxU16 BRefType(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par.mvp);
        bool bSet = pCO2 && pCO2->BRefType;

        if (bSet)
            return pCO2->BRefType;

        const mfxU16 BPyrCand[2] = { mfxU16(MFX_B_REF_OFF), mfxU16(MFX_B_REF_PYRAMID) };
        bool bValid =
            par.base.GetGopRefDist(par) > 3
            && (par.mvp.mfx.NumRefFrame == 0
                || par.base.GetMinRefForBPyramid(par) <= par.mvp.mfx.NumRefFrame);

        return BPyrCand[bValid];
    }

    static mfxU16 PRefType(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par.mvp);
        bool bSet = pCO3 && pCO3->PRefType;
        if (bSet)
            return pCO3->PRefType;

        static const mfxU16 PRefType[3] = { mfxU16(MFX_P_REF_DEFAULT), mfxU16(MFX_P_REF_SIMPLE), mfxU16(MFX_P_REF_PYRAMID) };
        const mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par.mvp);
        auto GopRefDist = par.base.GetGopRefDist(par);
        mfxI32 idx = GopRefDist == 1;
        idx += idx && (par.mvp.mfx.RateControlMethod == MFX_RATECONTROL_CQP || Legacy::IsSWBRC(par.mvp, pCO2));

        return PRefType[idx];
    }

    static std::tuple<mfxU32, mfxU32> FrameRate(
        Defaults::TChain<std::tuple<mfxU32, mfxU32>>::TExt
        , const Defaults::Param& par)
    {
        auto& fi = par.mvp.mfx.FrameInfo;

        if (fi.FrameRateExtN && fi.FrameRateExtD)
        {
            return std::make_tuple(fi.FrameRateExtN, fi.FrameRateExtD);
        }

        mfxU32 frN = 30, frD = 1;
        mfxF64 frMax = 30.;

        if (par.mvp.mfx.CodecLevel)
        {
            auto W = par.base.GetCodedPicWidth(par);
            auto H = par.base.GetCodedPicHeight(par);
            frMax = GetMaxFrByLevel(par.mvp, W * H);
        }

        bool bFrByLevel = frN > (frMax * frD);
        frN = mfxU32(frN * !bFrByLevel + (frMax * 1001) * bFrByLevel);
        frD = mfxU32(frD * !bFrByLevel + 1001 * bFrByLevel);

        return std::make_tuple(frN, frD);
    }

    static mfxU16 MaxBitDepthByFourCC(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        auto FourCC = par.mvp.mfx.FrameInfo.FourCC;
        bool b4CCMax10 =
            FourCC == MFX_FOURCC_A2RGB10
            || FourCC == MFX_FOURCC_P010
            || FourCC == MFX_FOURCC_P210
            || FourCC == MFX_FOURCC_Y210
            || FourCC == MFX_FOURCC_Y410;
        return mfxU16(8 + 2 * b4CCMax10);
    }

    static mfxU16 MaxChromaByFourCC(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        auto fcc = par.mvp.mfx.FrameInfo.FourCC;
        bool bMax420 =
            fcc == MFX_FOURCC_NV12
            || fcc == MFX_FOURCC_P010;

        bool bMax422 =
            fcc == MFX_FOURCC_YUY2
            || fcc == MFX_FOURCC_Y210
            || fcc == MFX_FOURCC_P210;
        bMax422 &= !bMax420;

        bool bMax444 = !(bMax422 || bMax420);

        return mfxU16(
            bMax420 * MFX_CHROMAFORMAT_YUV420
            + bMax422 * MFX_CHROMAFORMAT_YUV422
            + bMax444 * MFX_CHROMAFORMAT_YUV444);
    }

    static mfxU16 MaxBitDepth(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        auto profile = par.mvp.mfx.CodecProfile;
        const mfxExtHEVCParam* pHEVC = ExtBuffer::Get(par.mvp);
        bool bCheckConstraints = profile >= MFX_PROFILE_HEVC_REXT && pHEVC;
        bool bMax8 =
            profile == MFX_PROFILE_HEVC_MAIN
            || profile == MFX_PROFILE_HEVC_MAINSP
            || (bCheckConstraints && (pHEVC->GeneralConstraintFlags & MFX_HEVC_CONSTR_REXT_MAX_8BIT));
        bool bMax10 =
            profile == MFX_PROFILE_HEVC_MAIN10
            || (bCheckConstraints && (pHEVC->GeneralConstraintFlags & MFX_HEVC_CONSTR_REXT_MAX_10BIT));
        bool bMax12 =
            (bCheckConstraints && (pHEVC->GeneralConstraintFlags & MFX_HEVC_CONSTR_REXT_MAX_12BIT));

        std::list<mfxU16> maxBD({
            mfxU16(8 + !!par.caps.MaxEncodedBitDepth * (1 << par.caps.MaxEncodedBitDepth))
            , mfxU16(bMax8 * 8)
            , mfxU16(bMax10 * 10)
            , mfxU16(bMax12 * 12)
            , par.mvp.mfx.FrameInfo.BitDepthLuma
            , par.base.GetMaxBitDepthByFourCC(par)
            });

        maxBD.sort();
        maxBD.remove(0);

        return maxBD.front();
    }

    static mfxU16 MaxChroma(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtHEVCParam* pHEVC = ExtBuffer::Get(par.mvp);
        mfxU32 profile               = par.mvp.mfx.CodecProfile;
        mfxU16 maxBy4CC              = par.base.GetMaxChromaByFourCC(par);
        bool   bCheckConstraints     = profile >= MFX_PROFILE_HEVC_REXT && pHEVC;
        bool   bMax420 =
            profile == MFX_PROFILE_HEVC_MAIN
            || profile == MFX_PROFILE_HEVC_MAINSP
            || profile == MFX_PROFILE_HEVC_MAIN10
            || (bCheckConstraints && (pHEVC->GeneralConstraintFlags & MFX_HEVC_CONSTR_REXT_MAX_420CHROMA))
            || !(par.caps.YUV422ReconSupport || par.caps.YUV444ReconSupport)
            || maxBy4CC == MFX_CHROMAFORMAT_YUV420;

        bool bMax422 =
            ((bCheckConstraints && (pHEVC->GeneralConstraintFlags & MFX_HEVC_CONSTR_REXT_MAX_422CHROMA))
                || maxBy4CC == MFX_CHROMAFORMAT_YUV422)
            && (par.caps.YUV422ReconSupport && maxBy4CC >= MFX_CHROMAFORMAT_YUV422);
        bMax422 &= !bMax420;

        bool bMax444 = !(bMax422 || bMax420) && (par.caps.YUV444ReconSupport && maxBy4CC == MFX_CHROMAFORMAT_YUV444);

        bMax420 |= !(bMax422 || bMax444);

        mfxU16 maxCf = mfxU16(
            bMax420 * MFX_CHROMAFORMAT_YUV420
            + bMax422 * MFX_CHROMAFORMAT_YUV422
            + bMax444 * MFX_CHROMAFORMAT_YUV444);

        return maxCf;
    }

    static mfxU16 TargetBitDepthLuma(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par.mvp);

        if (pCO3 && pCO3->TargetBitDepthLuma)
        {
            return pCO3->TargetBitDepthLuma;
        }
        return par.base.GetMaxBitDepth(par);
    }

    static mfxU16 TargetChromaFormat(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par.mvp);

        if (pCO3 && pCO3->TargetChromaFormatPlus1)
        {
            return pCO3->TargetChromaFormatPlus1;
        }
        //For RGB4 use illogical default 420 for backward compatibility
        if (par.mvp.mfx.FrameInfo.FourCC == MFX_FOURCC_RGB4)
        {
            return mfxU16(MFX_CHROMAFORMAT_YUV420 + 1);
        }
        return mfxU16(par.base.GetMaxChroma(par) + 1);
    }

    static mfxU32 TargetKbps(
        Defaults::TChain<mfxU32>::TExt
        , const Defaults::Param& par)
    {
        auto& mfx = par.mvp.mfx;

        if (mfx.TargetKbps)
        {
            return mfx.TargetKbps * std::max<const mfxU32>(1, mfx.BRCParamMultiplier);
        }

        mfxU32 frN, frD, maxBR = 0xffffffff;

        SetIf(maxBR, !!mfx.CodecLevel, [&]() { return GetMaxKbpsByLevel(par.mvp); });

        mfxU16 W = par.base.GetCodedPicWidth(par);
        mfxU16 H = par.base.GetCodedPicHeight(par);
        std::tie(frN, frD) = par.base.GetFrameRate(par);

        mfxU16 bd = par.base.GetTargetBitDepthLuma(par);
        mfxU16 cf = par.base.GetTargetChromaFormat(par) - 1;
        mfxU32 rawBits = (Legacy::GetRawBytes(W, H, cf, bd) << 3);

        return std::min<mfxU32>(maxBR, rawBits * frN / frD / 150000);
    }

    static mfxU32 MaxKbps(
        Defaults::TChain<mfxU32>::TExt
        , const Defaults::Param& par)
    {
        auto& mfx = par.mvp.mfx;

        if (mfx.MaxKbps && mfx.RateControlMethod != MFX_RATECONTROL_CQP && mfx.RateControlMethod != MFX_RATECONTROL_CBR)
        {
            return mfx.MaxKbps * std::max<const mfxU32>(1, mfx.BRCParamMultiplier);
        }
        return par.base.GetTargetKbps(par);
    }

    static mfxU32 BufferSizeInKB(
        Defaults::TChain<mfxU32>::TExt
        , const Defaults::Param& par)
    {
        auto& mfx = par.mvp.mfx;

        if (mfx.BufferSizeInKB)
        {
            return mfx.BufferSizeInKB * std::max<const mfxU32>(1, mfx.BRCParamMultiplier);
        }

        bool bUseMaxKbps =
            mfx.RateControlMethod == MFX_RATECONTROL_CBR
            || mfx.RateControlMethod == MFX_RATECONTROL_VBR
            || mfx.RateControlMethod == MFX_RATECONTROL_QVBR
            || mfx.RateControlMethod == MFX_RATECONTROL_VCM
            || mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT;
        mfxU32 maxCPB             = 0xffffffff;
        mfxU32 minCPB             = bUseMaxKbps * InitialDelayInKB(par.mvp.mfx);
        mfxU32 defaultCPB         = 0;
        auto   GetMaxCPBByLevel   = [&]() { return GetMaxCpbInKBByLevel(par.mvp); };
        auto   GetCPBFromMaxKbps  = [&]() { return par.base.GetMaxKbps(par) / 4; };
        auto   GetCPBFromRawBytes = [&]()
        {
            mfxU16 bd = par.base.GetTargetBitDepthLuma(par);
            mfxU16 W  = par.base.GetCodedPicWidth(par);
            mfxU16 H  = par.base.GetCodedPicHeight(par);
            mfxU16 cf = par.base.GetTargetChromaFormat(par) - 1;
            return Legacy::GetRawBytes(W, H, cf, bd) / 1000;
        };

        SetIf(maxCPB, !!mfx.CodecLevel, GetMaxCPBByLevel);
        SetIf(defaultCPB, bUseMaxKbps, GetCPBFromMaxKbps);
        SetDefault(defaultCPB, GetCPBFromRawBytes);

        return std::max<mfxU32>(minCPB, std::min<mfxU32>(maxCPB, defaultCPB));
    }

    static std::tuple<mfxU16, mfxU16, mfxU16> MaxNumRef(
        Defaults::TChain<std::tuple<mfxU16, mfxU16, mfxU16>>::TExt
        , const Defaults::Param& par)
    {
        mfxU16 tu = par.mvp.mfx.TargetUsage;
        CheckRangeOrSetDefault<mfxU16>(tu, 1, 7, 4);
        bool bBase  = (par.hw < MFX_HW_CNL);
        bool bLP    = IsOn(par.mvp.mfx.LowPower);
        bool bTU7   = (tu == 7);
        --tu;

        static const mfxU16 nRefs[2][2][7] =
        {
            {// VME
                  { 4, 4, 3, 3, 3, 1, 1 }
                , { 2, 2, 1, 1, 1, 1, 1 }
            }
            , {// VDENC
                  { 3, 3, 2, 2, 2, 1, 1 }
                , { 3, 3, 2, 2, 2, 1, 1 }
            }
        };

        mfxU16 nRefL0 = mfxU16(
            bBase * (par.caps.MaxNum_Reference0 * !bTU7 + bTU7)
            + !bBase * nRefs[bLP][0][tu]);
        mfxU16 nRefL1 = mfxU16(
            bBase * (par.caps.MaxNum_Reference1 * !bTU7 + bTU7)
            + !bBase * nRefs[bLP][1][tu]);

        mfxU16 numRefFrame = par.mvp.mfx.NumRefFrame + !par.mvp.mfx.NumRefFrame * 16;

        return std::make_tuple(
            std::min<mfxU16>({ nRefL0, par.caps.MaxNum_Reference0, numRefFrame })
            , std::min<mfxU16>({ nRefL0, par.caps.MaxNum_Reference0, numRefFrame })
            , std::min<mfxU16>({ nRefL1, par.caps.MaxNum_Reference1, numRefFrame }));
    }

    static mfxU16 RateControlMethod(
        Defaults::TChain< mfxU16>::TExt
        , const Defaults::Param& par)
    {
        if (par.mvp.mfx.RateControlMethod)
        {
            return par.mvp.mfx.RateControlMethod;
        }
        return mfxU16(MFX_RATECONTROL_CQP);
    }

    static mfxU8 MinQPMFX(
        Defaults::TChain< mfxU16>::TExt
        , const Defaults::Param& par)
    {
        // negative QP for CQP VME only;
        bool bPositive =
            !par.caps.NegativeQPSupport
            || IsOn(par.mvp.mfx.LowPower) // remove when caps.NegativeQPSupport is correctly set in driver
            || par.mvp.mfx.RateControlMethod != MFX_RATECONTROL_CQP;
        bool bMin10 = IsOn(par.mvp.mfx.LowPower) && !Legacy::IsSWBRC(par.mvp, ExtBuffer::Get(par.mvp)); // 10 is min QP for VDENC

        return mfxU8(std::max(1, (bMin10 * 10) + (bPositive * (6 * (par.base.GetTargetBitDepthLuma(par) - 8)))));
    }

    static mfxU8 MaxQPMFX(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        return 51 + 6 * mfxU8(std::max<mfxU16>(8, par.base.GetTargetBitDepthLuma(par)) - 8);
    }

    static std::tuple<mfxU16, mfxU16, mfxU16> QPMFX(
        Defaults::TChain<std::tuple<mfxU16, mfxU16, mfxU16>>::TExt
        , const Defaults::Param& par)
    {
        bool bCQP = (par.base.GetRateControlMethod(par) == MFX_RATECONTROL_CQP);

        mfxU16 QPI = bCQP * par.mvp.mfx.QPI;
        mfxU16 QPP = bCQP * par.mvp.mfx.QPP;
        mfxU16 QPB = bCQP * par.mvp.mfx.QPB;

        bool bValid = (QPI && QPP && QPB);

        if (bValid)
            return std::make_tuple(QPI, QPP, QPB);

        auto maxQP = par.base.GetMaxQPMFX(par);
        auto minQP = par.base.GetMinQPMFX(par);

        SetDefault(QPI, mfxU16(std::max<mfxI32>(QPP - 2, minQP) * !!QPP));
        SetDefault(QPI, mfxU16(std::max<mfxI32>(QPB - 4, minQP) * !!QPB));
        SetDefault(QPI, std::max<mfxU16>(minQP, (maxQP + 1) / 2));
        SetDefault(QPP, std::min<mfxU16>(QPI + 2, maxQP));
        SetDefault(QPB, std::min<mfxU16>(QPP + 2, maxQP));

        return std::make_tuple(QPI, QPP, QPB);
    }

    static mfxU16 QPOffset(
        Defaults::TGetQPOffset::TExt
        , const Defaults::Param& par
        , mfxI16(*pQPOffset)[8])
    {
        mfxU16 EnableQPOffset = 0;

        if (const mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par.mvp))
        {
            EnableQPOffset = pCO3->EnableQPOffset;
        }

        if (!EnableQPOffset)
        {
            bool bCQP  = par.base.GetRateControlMethod(par) == MFX_RATECONTROL_CQP;
            bool bBPyr = bCQP && par.base.GetBRefType(par) == MFX_B_REF_PYRAMID;
            bool bPPyr = bCQP && !bBPyr && par.base.GetPRefType(par) == MFX_P_REF_PYRAMID;

            EnableQPOffset = Bool2CO(bBPyr || bPPyr);

            bool bGenerate = IsOn(EnableQPOffset) && pQPOffset;

            if (bGenerate)
            {
                mfxI16 QPX = 0, i = 0, maxQPOffset = 0, minQPOffset = 0;

                SetIf(QPX, bBPyr, [&]() { return std::get<2>(par.base.GetQPMFX(par)); });
                SetIf(QPX, bPPyr, [&]() { return std::get<1>(par.base.GetQPMFX(par)); });

                maxQPOffset = mfxI16(par.base.GetMaxQPMFX(par) - QPX);
                minQPOffset = mfxI16(par.base.GetMinQPMFX(par) - QPX);

                std::generate_n(*pQPOffset, 8
                    , [&]() { return mfx::clamp<mfxI16>(i++ + bBPyr, minQPOffset, maxQPOffset); });
            }
        }

        bool bZeroOffset = IsOff(EnableQPOffset) && pQPOffset;
        std::fill_n(*pQPOffset, 8 * bZeroOffset, mfxI16(0));

        return EnableQPOffset;
    }

    static mfxU16 Profile(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        bool bPassThrough = !!par.mvp.mfx.CodecProfile;

        mfxU16 bd = 0, cf = 0;
        SetIf(bd, !bPassThrough, [&]() { return par.base.GetTargetBitDepthLuma(par); });
        SetIf(cf, !bPassThrough, [&]() { return mfxU16(par.base.GetTargetChromaFormat(par) - 1); });

        bool bRext = !bPassThrough && (cf != MFX_CHROMAFORMAT_YUV420 || bd > 10);
        bool bMain10 = !bPassThrough && !bRext && bd == 10;
        bool bMain = !bPassThrough && !bRext && !bMain10;

        return
            bPassThrough * par.mvp.mfx.CodecProfile
            + bRext * MFX_PROFILE_HEVC_REXT
            + bMain10 * MFX_PROFILE_HEVC_MAIN10
            + bMain * MFX_PROFILE_HEVC_MAIN;
    }

    static bool GUID(
        Defaults::TGetGUID::TExt
        , const Defaults::Param& par
        , ::GUID& guid)
    {
        const std::map<mfxU16, std::map<mfxU16, ::GUID>> GUIDSupported[2] =
        {
            //LowPower = OFF
            {
                {
                    mfxU16(8),
                    {
                          {mfxU16(MFX_CHROMAFORMAT_YUV420), DXVA2_Intel_Encode_HEVC_Main}
                        , {mfxU16(MFX_CHROMAFORMAT_YUV422), DXVA2_Intel_Encode_HEVC_Main422}
                        , {mfxU16(MFX_CHROMAFORMAT_YUV444), DXVA2_Intel_Encode_HEVC_Main444}
                    }
                }
                , {
                    mfxU16(10),
                    {
                          {mfxU16(MFX_CHROMAFORMAT_YUV420), DXVA2_Intel_Encode_HEVC_Main10}
                        , {mfxU16(MFX_CHROMAFORMAT_YUV422), DXVA2_Intel_Encode_HEVC_Main422_10}
                        , {mfxU16(MFX_CHROMAFORMAT_YUV444), DXVA2_Intel_Encode_HEVC_Main444_10}
                    }
                }
            }
            //LowPower = ON
            ,{
                {
                    mfxU16(8),
                    {
                          {mfxU16(MFX_CHROMAFORMAT_YUV420), DXVA2_Intel_LowpowerEncode_HEVC_Main}
                        , {mfxU16(MFX_CHROMAFORMAT_YUV422), DXVA2_Intel_LowpowerEncode_HEVC_Main422}
                        , {mfxU16(MFX_CHROMAFORMAT_YUV444), DXVA2_Intel_LowpowerEncode_HEVC_Main444}
                    }
                }
                , {
                    mfxU16(10),
                    {
                          {mfxU16(MFX_CHROMAFORMAT_YUV420), DXVA2_Intel_LowpowerEncode_HEVC_Main10}
                        , {mfxU16(MFX_CHROMAFORMAT_YUV422), DXVA2_Intel_LowpowerEncode_HEVC_Main422_10}
                        , {mfxU16(MFX_CHROMAFORMAT_YUV444), DXVA2_Intel_LowpowerEncode_HEVC_Main444_10}
                    }
                }
            }
        };
        const mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par.mvp);

        bool bBDInvalid = pCO3 && Check<mfxU16, 10, 8, 0>(pCO3->TargetBitDepthLuma);
        bool bCFInvalid = pCO3 && Check<mfxU16
            , MFX_CHROMAFORMAT_YUV420 + 1
            , MFX_CHROMAFORMAT_YUV422 + 1
            , MFX_CHROMAFORMAT_YUV444 + 1>
            (pCO3->TargetChromaFormatPlus1);

        bool bLowPower = IsOn(par.mvp.mfx.LowPower);
        mfxU16 BitDepth = 8 * (par.hw < MFX_HW_KBL || par.base.GetProfile(par) == MFX_PROFILE_HEVC_MAIN);
        mfxU16 ChromaFormat = MFX_CHROMAFORMAT_YUV420 * (par.hw < MFX_HW_ICL);

        auto mvpCopy = par.mvp;
        mvpCopy.NumExtParam = 0;

        Defaults::Param parCopy(mvpCopy, par.caps, par.hw, par.base);
        auto pParForBD = &par;
        auto pParForCF = &par;

        SetIf(pParForBD, bBDInvalid, &parCopy);
        SetIf(pParForCF, bCFInvalid, &parCopy);

        SetIf(BitDepth, !BitDepth, [&]() { return pParForBD->base.GetTargetBitDepthLuma(*pParForBD); });
        SetIf(ChromaFormat, !ChromaFormat, [&]() { return mfxU16(pParForCF->base.GetTargetChromaFormat(*pParForCF) - 1); });

        bool bSupported =
            GUIDSupported[bLowPower].count(BitDepth)
            && GUIDSupported[bLowPower].at(BitDepth).count(ChromaFormat);

        SetIf(guid, bSupported, [&]() { return GUIDSupported[bLowPower].at(BitDepth).at(ChromaFormat); });

        return bSupported;
    }

    static mfxU16 MBBRC(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par.mvp);
        bool bPassThrough               = pCO2 && pCO2->MBBRC;
        bool bON                        = bPassThrough && IsOn(pCO2->MBBRC);
        bool bOFF                       = bPassThrough && IsOff(pCO2->MBBRC);

        bOFF |= !bPassThrough
            && (   par.base.GetRateControlMethod(par) == MFX_RATECONTROL_CQP
                || Legacy::IsSWBRC(par.mvp, pCO2)
                || IsOn(par.mvp.mfx.LowPower));

        return mfxU16(
            bON * MFX_CODINGOPTION_ON
            + bOFF * MFX_CODINGOPTION_OFF);
    }

    static mfxU16 AsyncDepth(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        return par.mvp.AsyncDepth + !par.mvp.AsyncDepth * 5;
    }

    static mfxU16 NumSlices(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        if (par.mvp.mfx.NumSlice)
            return par.mvp.mfx.NumSlice;

        std::vector<SliceInfo> slices;
        return par.base.GetSlices(par, slices);
    }

    static mfxU16 LCUSize(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtHEVCParam* pHEVC = ExtBuffer::Get(par.mvp);

        if (pHEVC && pHEVC->LCUSize)
        {
            return pHEVC->LCUSize;
        }

        bool   bForce64   = par.hw >= MFX_HW_CNL && IsOn(par.mvp.mfx.LowPower);
        bool   bMaxByCaps = par.hw >= MFX_HW_CNL && !IsOn(par.mvp.mfx.LowPower);
        bool   bForce32   = !bForce64 && !bMaxByCaps;
        mfxU16 LCUSize    =
            bForce64 * 64
            + bForce32 * 32
            + bMaxByCaps * (1 << (CeilLog2(par.caps.LCUSizeSupported + 1) + 3));

        assert((LCUSize >> 4) & par.caps.LCUSizeSupported);

        return LCUSize;
    }

    static bool HRDConformanceON(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtCodingOption* pCO = ExtBuffer::Get(par.mvp);
        return (!pCO || !(IsOff(pCO->NalHrdConformance) || IsOff(pCO->VuiNalHrdParameters)))
            && (   par.mvp.mfx.RateControlMethod == MFX_RATECONTROL_CBR
                || par.mvp.mfx.RateControlMethod == MFX_RATECONTROL_VBR
                || par.mvp.mfx.RateControlMethod == MFX_RATECONTROL_VCM
                || par.mvp.mfx.RateControlMethod == MFX_RATECONTROL_QVBR);
    }

    static mfxU16 PicTimingSEI(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtCodingOption* pCO = ExtBuffer::Get(par.mvp);
        bool bForceON = pCO && IsOn(pCO->PicTimingSEI);
        bool bForceOFF = pCO && IsOff(pCO->PicTimingSEI);
        return Bool2CO(bForceON || (!bForceOFF && par.base.GetHRDConformanceON(par)));
    }

    static mfxU16 PPyrInterval(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        return mfxU16(
            !!par.mvp.mfx.NumRefFrame * std::min<mfxU16>(DEFAULT_PPYR_INTERVAL, par.mvp.mfx.NumRefFrame)
            + !par.mvp.mfx.NumRefFrame * DEFAULT_PPYR_INTERVAL);
    }

    static mfxU8 PLayer(
        Defaults::TGetPLayer::TExt
        , const Defaults::Param& par
        , mfxU32 fo)
    {
        const mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par.mvp);
        auto num = par.base.GetPPyrInterval(par);
        mfxU32 i = fo % num;

        bool bForce0 = 
            (pCO3 && pCO3->PRefType != MFX_P_REF_PYRAMID)
            || i == 0
            || i >= num;

        if (bForce0)
            return 0;

        mfxU32 level = 1;
        mfxU32 begin = 0;
        mfxU32 end = num;
        mfxU32 t = (begin + end + 1) / 2;

        while (t != i)
        {
            ++level;
            SetIf(begin, i > t, t);
            SetIf(end, i <= t, t);
            t = (begin + end + 1) / 2;
        }

        return (mfxU8)std::min<mfxU32>(level, 7);
    }

    static mfxU8 TId(
        Defaults::TGetTId::TExt
        , const Defaults::Param& par
        , mfxU32 fo)
    {
        const mfxExtAvcTemporalLayers* pTL = ExtBuffer::Get(par.mvp);
        if (!pTL)
            return 0;
        return TemporalLayers(*pTL).GetTId(fo);
    }

    static mfxU16 NumTemporalLayers(
        Defaults::TChain<mfxU16>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtAvcTemporalLayers* pTL = ExtBuffer::Get(par.mvp);
        if (!pTL)
            return 1;
        return TemporalLayers::CountTL(*pTL);
    }

    static mfxU8 HighestTId(
        Defaults::TChain<mfxU8>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtAvcTemporalLayers* pTL = ExtBuffer::Get(par.mvp);
        if (!pTL)
            return mfxU8(-1);
        return TemporalLayers(*pTL).HighestTId();
    }

    static mfxU8 NumReorderFrames(
        Defaults::TChain<mfxU8>::TExt
        , const Defaults::Param& par)
    {
        mfxU8 BFrameRate = mfxU8(par.base.GetGopRefDist(par) - 1);
        bool  bBPyramid  = (par.base.GetBRefType(par) == MFX_B_REF_PYRAMID);
        mfxU8 n          = !!BFrameRate;

        if (bBPyramid && n--)
        {
            while (BFrameRate)
            {
                BFrameRate >>= 1;
                n++;
            }
        }

        return n;
    }

    static bool NonStdReordering(
        Defaults::TChain<mfxU8>::TExt
        , const Defaults::Param& par)
    {
        return
            par.mvp.mfx.EncodedOrder
            && par.mvp.mfx.NumRefFrame > 2
            && (par.base.GetBRefType(par) == MFX_B_REF_PYRAMID)
            && par.base.GetMinRefForBPyramid(par) > par.mvp.mfx.NumRefFrame;
    }

    static mfxU16 FrameType(
        Defaults::TGetFrameType::TExt
        , const Defaults::Param& par
        , mfxU32 displayOrder
        , mfxU32 lastIDR)
    {
        mfxU32 gopOptFlag = par.mvp.mfx.GopOptFlag;
        mfxU32 gopPicSize = par.mvp.mfx.GopPicSize;
        mfxU32 gopRefDist = par.mvp.mfx.GopRefDist;
        mfxU32 idrPicDist = gopPicSize * (par.mvp.mfx.IdrInterval);

        //infinite GOP
        SetIf(idrPicDist, gopPicSize == 0xffff, 0xffffffff);
        SetIf(gopPicSize, gopPicSize == 0xffff, 0xffffffff);

        mfxU32 fo = displayOrder - lastIDR;
        bool bIdr = fo == 0 || (idrPicDist && (fo % idrPicDist == 0));
        bool bIRef = !bIdr && (fo % gopPicSize == 0);
        bool bPRef =
            !bIdr && !bIRef
            && (   (fo % gopPicSize % gopRefDist == 0)
                || ((fo + 1) % gopPicSize == 0 && (gopOptFlag & MFX_GOP_CLOSED))
                || (idrPicDist && (fo + 1) % idrPicDist == 0));
        bool bB = !(bPRef || bIdr || bPRef || bIRef);

        mfxU16 ft =
            bIdr * (MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR)
            + bIRef * (MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF)
            + bPRef * (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF)
            + bB * (MFX_FRAMETYPE_B);

        bool bForceNonRef = IsRef(ft) && par.base.GetTId(par, fo) == par.base.GetHighestTId(par);
        ft &= ~(bForceNonRef * MFX_FRAMETYPE_REF);

        return ft;
    }

    static std::tuple<mfxU8, mfxU8> RPLFromExt(
        Defaults::TGetRPLFromExt::TExt
        , const Defaults::Param&
        , const DpbArray &DPB
        , mfxU16 maxL0
        , mfxU16 maxL1
        , const mfxExtAVCRefLists& extRPL
        , mfxU8(&RPL)[2][MAX_DPB_SIZE])
    {
        typedef decltype(*extRPL.RefPicList0) TRExtRPL0Entry;
        typedef decltype(*extRPL.RefPicList1) TRExtRPL1Entry;

        mfxU8 l0 = 0, l1 = 0;

        std::for_each(extRPL.RefPicList0
            , extRPL.RefPicList0 + extRPL.NumRefIdxL0Active
            , [&](TRExtRPL0Entry ref)
        {
            mfxU8 idx = Legacy::GetDPBIdxByFO(DPB, (mfxI32)ref.FrameOrder);
            RPL[0][l0] = idx;
            l0 += (idx < MAX_DPB_SIZE) && (l0 < maxL0);
        });

        std::for_each(extRPL.RefPicList1
            , extRPL.RefPicList1 + extRPL.NumRefIdxL1Active
            , [&](TRExtRPL1Entry ref)
        {
            mfxU8 idx = Legacy::GetDPBIdxByFO(DPB, (mfxI32)ref.FrameOrder);
            RPL[1][l1] = idx;
            l1 += (idx < MAX_DPB_SIZE) && (l1 < maxL1);
        });

        l0 = std::min(l0, mfxU8(maxL0));
        l1 = std::min(l1, mfxU8(maxL1));

        std::fill(RPL[0] + l0, RPL[0] + Size(RPL[0]), IDX_INVALID);
        std::fill(RPL[1] + l1, RPL[1] + Size(RPL[1]), IDX_INVALID);

        return std::make_tuple(l0, l1);
    }

    static std::tuple<mfxU8, mfxU8> RPLMod(
        Defaults::TGetRPL::TExt
        , const Defaults::Param& par
        , const DpbArray &/*DPB*/
        , mfxU16 /*maxL0*/
        , mfxU16 maxL1
        , const FrameBaseInfo& cur
        , mfxU8(&RPL)[2][MAX_DPB_SIZE])
    {
        mfxU8 l0 = mfxU8(Size(RPL[0]) - std::count(RPL[0], RPL[0] + Size(RPL[0]), IDX_INVALID));
        mfxU8 l1 = mfxU8(Size(RPL[1]) - std::count(RPL[1], RPL[1] + Size(RPL[1]), IDX_INVALID));
        bool isB = IsB(cur.FrameType) && !cur.isLDB;

        bool bDefaultL1 = isB && !l1 && l0;

        if (bDefaultL1)
        {
            RPL[1][l1++] = RPL[0][l0 - 1];
        }

        // on VDENC for LDB frames L1 must be completely identical to L0
        mfxU8 maxLdbL1 = (mfxU8)std::max<mfxU32>(maxL1, l0 * IsOn(par.mvp.mfx.LowPower));

        //ignore l1 != l0 in pExtLists for LDB (unsupported by HW)
        l1 = l1 * isB + std::min<mfxU8>(l0, maxLdbL1) * !isB;

        std::copy_n(RPL[0], l1 * !isB, RPL[1]);

        return std::make_tuple(l0, l1);
    }

    static bool CmpRef(
        Defaults::TCmpRef::TExt
        , const Defaults::Param&
        , const FrameBaseInfo &cur
        , const FrameBaseInfo &a
        , const FrameBaseInfo &b)
    {
        return abs(cur.POC - a.POC) < abs(cur.POC - b.POC);
    }

    static const DpbFrame* WeakRef(
        Defaults::TGetWeakRef::TExt
        , const Defaults::Param& par
        , const FrameBaseInfo  &/*cur*/
        , const DpbFrame       *begin
        , const DpbFrame       *end)
    {
        const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par.mvp);

        if (CO3.PRefType == MFX_P_REF_PYRAMID)
        {
            auto CmpLDRefs = [&](const DpbFrame& a, const DpbFrame& b)
            {
                return (a.PyramidLevel > b.PyramidLevel
                    || (a.PyramidLevel == b.PyramidLevel && a.POC < b.POC));
            };

            return std::min_element(begin, end, CmpLDRefs);
        }

        auto POCLess = [](const DpbFrame& a, const DpbFrame& b) { return a.POC < b.POC; };

        return std::min_element(begin, end, POCLess);
    }

    static std::tuple<mfxU8, mfxU8> RPL(
        Defaults::TGetRPL::TExt
        , const Defaults::Param& par
        , const DpbArray &DPB
        , mfxU16 maxL0
        , mfxU16 maxL1
        , const FrameBaseInfo& cur
        , mfxU8(&RPL)[2][MAX_DPB_SIZE])
    {
        const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par.mvp);
        bool bLowDelay = (CO3.PRefType == MFX_P_REF_PYRAMID);
        mfxU8 l0 = 0, l1 = 0;
        const DpbFrame* dpbBegin = &DPB[0];
        std::list<const DpbFrame*> L0(Size(DPB), nullptr);
        std::list<const DpbFrame*> L1(Size(DPB) * (IsB(cur.FrameType) && !cur.isLDB), nullptr);

        std::iota(L0.begin(), L0.end(), dpbBegin);
        std::iota(L1.begin(), L1.end(), dpbBegin);

        L0.remove_if([&](const DpbFrame* x)
        {
            return
                !isValid(*x)
                || x->TemporalID > cur.TemporalID
                || x->POC > cur.POC;
        });

        L1.remove_if([&](const DpbFrame* x)
        {
            return
                !isValid(*x)
                || x->TemporalID > cur.TemporalID
                || x->POC < cur.POC;
        });

        bool bL1 = false;
        auto Closer = [&](const DpbFrame* a, const DpbFrame* b)
        {
            return par.base.CmpRefLX[bL1](par, cur, *a, *b);
        };
        auto CmpRefs = [&](const DpbFrame* a, const DpbFrame* b) // return (a > b)
        {
            bool bEQ = true;
            bool bGT = false;
            bGT |= bEQ && (bLowDelay && (a->PyramidLevel < b->PyramidLevel));
            bEQ &= !bLowDelay || (a->PyramidLevel == b->PyramidLevel);
            bGT |= bEQ && Closer(a, b);
            return bGT;
        };

        if (L0.empty())
        {
            assert(!"L0 is empty - reordering issue?");
            L0 = L1;
        }

        L0.sort(CmpRefs);
        L0.splice(L0.begin(), L0, std::min_element(L0.begin(), L0.end(), Closer));

        bL1 = true;
        L1.sort(CmpRefs);

        l0 = mfxU8(std::min<size_t>(L0.size(), maxL0));
        l1 = mfxU8(std::min<size_t>(L1.size(), maxL1));

        L0.resize(l0);
        L1.resize(l1);

        bool bNoQPOffset = (par.mvp.mfx.RateControlMethod == MFX_RATECONTROL_CQP && IsOff(CO3.EnableQPOffset));
        bool bIsSCC = par.mvp.mfx.CodecProfile == MFX_PROFILE_HEVC_SCC; // use default ref list order for SCC
        bool bUseDefaultOrder = bIsSCC || bNoQPOffset;
        if (bUseDefaultOrder)
        {
            auto POCDescending = [](const DpbFrame* a, const DpbFrame* b) { return a->POC > b->POC; };
            L0.sort(POCDescending);
        }

        auto POCAscending = [](const DpbFrame* a, const DpbFrame* b) { return a->POC < b->POC; };
        L1.sort(POCAscending);

        std::transform(L0.begin(), L0.end(), RPL[0]
            , [&](const DpbFrame* x) { return mfxU8(x - dpbBegin); });

        std::transform(L1.begin(), L1.end(), RPL[1]
            , [&](const DpbFrame* x) { return mfxU8(x - dpbBegin); });

        return std::make_tuple(l0, l1);
    }

    static std::tuple<mfxU8, mfxU8> RPLFromCtrl(
        Defaults::TGetRPLFromCtrl::TExt
        , const Defaults::Param&
        , const DpbArray &DPB
        , mfxU16 maxL0
        , mfxU16 maxL1
        , const FrameBaseInfo& cur
        , const mfxExtAVCRefListCtrl& ctrl
        , mfxU8(&RPL)[2][MAX_DPB_SIZE])
    {
        typedef decltype(*ctrl.RejectedRefList) TRRejected;
        typedef decltype(*ctrl.PreferredRefList) TRPreferred;

        mfxU8 pref[2] = {};
        mfxU8 IdxList[16];
        auto pIdxListEnd = IdxList + Size(IdxList);
        mfxU8 l0 = mfxU8(Size(RPL[0]) - std::count(RPL[0], RPL[0] + Size(RPL[0]), IDX_INVALID));
        mfxU8 l1 = mfxU8(Size(RPL[1]) - std::count(RPL[1], RPL[1] + Size(RPL[1]), IDX_INVALID));

        SetIf(maxL0, ctrl.NumRefIdxL0Active, std::min<mfxU16>(ctrl.NumRefIdxL0Active, maxL0));
        SetIf(maxL1, ctrl.NumRefIdxL1Active, std::min<mfxU16>(ctrl.NumRefIdxL1Active, maxL1));

        std::transform(
            ctrl.RejectedRefList
            , ctrl.RejectedRefList + Size(IdxList)
            , IdxList
            , [&](TRRejected x) { return Legacy::GetDPBIdxByFO(DPB, x.FrameOrder); });

        l0 = mfxU8(std::remove_if(RPL[0], RPL[0] + l0, [&](mfxU8 ref) {
            return std::find(IdxList, pIdxListEnd, ref) != pIdxListEnd; }) - RPL[0]);
        l1 = mfxU8(std::remove_if(RPL[1], RPL[1] + l1, [&](mfxU8 ref) {
            return std::find(IdxList, pIdxListEnd, ref) != pIdxListEnd; }) - RPL[1]);

        std::transform(
            ctrl.PreferredRefList
            , ctrl.PreferredRefList + Size(IdxList)
            , IdxList
            , [&](TRPreferred x) { return Legacy::GetDPBIdxByFO(DPB, x.FrameOrder); });

        pIdxListEnd = std::remove_if(IdxList, pIdxListEnd
            , [&](mfxU8 ref) { return ref >= MAX_DPB_SIZE; });

        mfxU8 tmpRPL[2][MAX_DPB_SIZE + 2] = {};
        mfxU8* pRplEnd[2] = { tmpRPL[0] + l0, tmpRPL[1] + l1 };

        std::fill(std::next(tmpRPL[0], MAX_DPB_SIZE), std::end(tmpRPL[0]), IDX_INVALID);
        std::fill(std::next(tmpRPL[1], MAX_DPB_SIZE), std::end(tmpRPL[1]), IDX_INVALID);

        std::copy(std::begin(RPL[0]), std::end(RPL[0]), std::begin(tmpRPL[0]));
        std::copy(std::begin(RPL[1]), std::end(RPL[1]), std::begin(tmpRPL[1]));

        std::for_each(IdxList, pIdxListEnd
            , [&](mfxU8 ref)
        {
            bool lx = DPB[ref].POC > cur.POC;
            pRplEnd[lx] = std::remove(tmpRPL[lx], pRplEnd[lx], ref);

            Insert(tmpRPL[lx], pref[lx]++, ref);
            ++pRplEnd[lx];
        });

        l0 = std::min<mfxU8>((mfxU8)maxL0, mfxU8(pRplEnd[0] - tmpRPL[0]));
        l1 = std::min<mfxU8>((mfxU8)maxL1, mfxU8(pRplEnd[1] - tmpRPL[1]));

        std::fill(std::next(RPL[0], l0), std::end(RPL[0]), IDX_INVALID);
        std::fill(std::next(RPL[1], l1), std::end(RPL[1]), IDX_INVALID);

        std::copy_n(tmpRPL[0], l0, RPL[0]);
        std::copy_n(tmpRPL[1], l1, RPL[1]);

        bool bDefaultL0 = (l0 == 0);
        RPL[0][l0 += bDefaultL0] *= !bDefaultL0;

        return std::make_tuple(l0, l1);
    }

    static mfxStatus PreReorderInfo(
        Defaults::TGetPreReorderInfo::TExt
        , const Defaults::Param& par
        , FrameBaseInfo& fi
        , const mfxFrameSurface1* pSurfIn
        , const mfxEncodeCtrl*    pCtrl
        , mfxU32 prevIDROrder
        , mfxI32 prevIPOC
        , mfxU32 frameOrder)
    {
        mfxU16 ftype = 0;
        auto SetFrameTypeFromGOP   = [&]() { return          Res2Bool(ftype, par.base.GetFrameType(par, frameOrder, prevIDROrder)); };
        auto SetFrameTypeFromCTRL  = [&]() { return pCtrl && Res2Bool(ftype, pCtrl->FrameType); };
        auto ForceIdr              = [&]() { return          Res2Bool(ftype, mfxU16(MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR)); };
        auto SetFrameOrderFromSurf = [&]() { if (!pSurfIn) return false; frameOrder = pSurfIn->Data.FrameOrder;  return true; };

        bool bFrameInfoValid =
            (par.mvp.mfx.EncodedOrder && SetFrameOrderFromSurf() && SetFrameTypeFromCTRL() )
            || (pCtrl && IsIdr(pCtrl->FrameType) && ForceIdr())
            || SetFrameTypeFromGOP();
        MFX_CHECK(bFrameInfoValid, MFX_ERR_UNDEFINED_BEHAVIOR);

        fi.POC        = !IsIdr(ftype) * (frameOrder - prevIDROrder);
        fi.FrameType  = ftype;
        fi.TemporalID = !IsI(ftype) * par.base.GetTId(par, fi.POC - prevIPOC);

        if (IsP(ftype))
        {
            const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par.mvp);

            fi.isLDB        = IsOn(CO3.GPB);
            fi.PyramidLevel = par.base.GetPLayer(par, fi.POC - prevIPOC);
        }

        return MFX_ERR_NONE;
    }

    static std::tuple<mfxU8, mfxU8> FrameNumRefActive(
        Defaults::TGetFrameNumRefActive::TExt
        , const Defaults::Param& par
        , const FrameBaseInfo& fi)
    {
        mfxU8 nL0 = 0, nL1 = 0;

        if (IsB(fi.FrameType) && !fi.isLDB)
        {
            const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par.mvp);
            const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par.mvp);
            mfxI32 layer = (CO2.BRefType == MFX_B_REF_PYRAMID) * mfx::clamp<mfxI32>(fi.PyramidLevel - 1, 0, 7);

            nL0 = (mfxU8)CO3.NumRefActiveBL0[layer];
            nL1 = (mfxU8)CO3.NumRefActiveBL1[layer];
        }

        if (IsP(fi.FrameType) || fi.isLDB)
        {
            const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par.mvp);
            auto layer = mfx::clamp<mfxI32>(fi.PyramidLevel, 0, 7);

            nL0 = (mfxU8)CO3.NumRefActiveP[layer];
            nL1 = (mfxU8)std::min<mfxU16>(CO3.NumRefActiveP[layer], CO3.NumRefActiveBL1[layer]);
            // on VDENC for LDB frames L1 must be completely identical to L0
            nL1 = IsOn(par.mvp.mfx.LowPower) ? nL0: nL1;
        }

        return std::make_tuple(nL0, nL1);
    }

    static mfxU16 LowPower(
        Defaults::TGetHWDefault<mfxU16>::TExt
        , const mfxVideoParam& par
        , eMFXHWType hw)
    {
        if (!Check<mfxU16, MFX_CODINGOPTION_ON, MFX_CODINGOPTION_OFF>(par.mfx.LowPower))
            return par.mfx.LowPower;

        auto fcc = par.mfx.FrameInfo.FourCC;
        bool bOn =
            ((hw == MFX_HW_CNL
              && par.mfx.TargetUsage >= 6
              && par.mfx.GopRefDist < 2) ||
#if (MFX_VERSION >= 1031)
             (hw == MFX_HW_EHL ||
              hw == MFX_HW_JSL) ||
#endif
             (hw >= MFX_HW_ICL &&
              (fcc == MFX_FOURCC_AYUV
#if (MFX_VERSION >= 1027)
               || fcc == MFX_FOURCC_Y410
#endif
               || fcc == MFX_FOURCC_A2RGB10
                  )));

        return mfxU16(
            bOn * MFX_CODINGOPTION_ON
            + !bOn * MFX_CODINGOPTION_OFF);
    }

    static std::tuple<mfxU16, mfxU16> NumTiles(
        Defaults::TChain<std::tuple<mfxU16, mfxU16>>::TExt
        , const Defaults::Param& par)
    {
        const mfxExtHEVCTiles* pTiles = ExtBuffer::Get(par.mvp);

        mfxU16 nCol = 1;
        mfxU16 nRow = 1;

        if (pTiles)
        {
            nCol = std::max<mfxU16>(nCol, pTiles->NumTileColumns);
            nRow = std::max<mfxU16>(nRow, pTiles->NumTileRows);
        }

        return std::make_tuple(nCol, nRow);
    }

    static mfxU16 TileSlices(
        Defaults::TGetTileSlices::TExt
        , const Defaults::Param& dpar
        , std::vector<SliceInfo>& slices
        , mfxU32 SliceStructure
        , mfxU32 nCol
        , mfxU32 nRow
        , mfxU32 nSlice)
    {
        mfxU32 nLCU                   = nCol * nRow;
        mfxU32 nLcuPerSlice           = 0;
        mfxU32 nSlicePrev             = (mfxU32)slices.size();
        mfxU32 segAddr                = 0;
        mfxU32 nLCUMult               = 1;
        bool   bStrictNumLCUPerSlice  = false;

        if (nSlicePrev)
        {
            segAddr = slices.back().SegmentAddress + slices.back().NumLCU;
        }

        if (SliceStructure == ONESLICE)
        {
            slices.emplace_back(SliceInfo{ segAddr, nLCU });
            return 1;
        }

        if (const mfxExtCodingOption2* pCO2 = ExtBuffer::Get(dpar.mvp))
        {
            bStrictNumLCUPerSlice = !!pCO2->NumMbPerSlice;
            nLcuPerSlice          = pCO2->NumMbPerSlice;
        }

        SetDefault(nLcuPerSlice, nLCU / nSlice);

        //in case of NumMbPerSlice != 0 need to check alignment
        //if it's value is not aligned, warning will be generated in CheckVideoParam() after MakeSlices() call
        bool   bAlignLcuPerSliceByRows = SliceStructure == ROWSLICE && bStrictNumLCUPerSlice;
        mfxU32 nLcuPerSliceAlignment   = std::max<mfxU32>(1, bAlignLcuPerSliceByRows * nCol);

        nLcuPerSlice = CeilDiv<mfxU32>(nLcuPerSlice, nLcuPerSliceAlignment) * nLcuPerSliceAlignment;

        //if not aligned already but need to
        bAlignLcuPerSliceByRows = !bAlignLcuPerSliceByRows && SliceStructure < ARBITRARY_MB_SLICE;

        if (bAlignLcuPerSliceByRows)
        {
            nSlice = std::min<mfxU32>(nSlice, nRow);
            mfxU32 nRowsPerSlice = CeilDiv(nRow, nSlice);
            bool   bAddSlice     = (SliceStructure != POW2ROW) && (nRowsPerSlice * (nSlice - 1)) >= nRow;

            nSlice        += bAddSlice;
            nRowsPerSlice -= bAddSlice;

            if (SliceStructure == POW2ROW)
            {
                mfxU32 nRowLog2     = CeilLog2(nRowsPerSlice);
                mfxU32 nRowsCand[2] = { mfxU32(1 << (nRowLog2 - 1)), mfxU32(1 << nRowLog2) };
                mfxI32 dC0          = nRowsPerSlice - nRowsCand[0];
                mfxI32 dC1          = nRowsCand[1] - nRowsPerSlice;

                nRowsPerSlice = nRowsCand[dC0 > dC1];
                nSlice = CeilDiv(nRow, nRowsPerSlice);
            }

            nLcuPerSlice           = nRowsPerSlice;
            nLCUMult               = nCol;
            nLCU                  /= nCol;
            bStrictNumLCUPerSlice |= ((SliceStructure == POW2ROW) || (SliceStructure == ROWSLICE));
        }

        slices.resize(nSlicePrev + nSlice);

        mfxU32 nLCULeft = nLCU % nSlice;

        bStrictNumLCUPerSlice |= (nSlice < 2 || (nLCULeft == 0));

        SliceInfo zeroSI  = {};
        auto      slBegin = std::next(slices.begin(), nSlicePrev);
        auto      slEnd   = slices.end();

        std::fill(slBegin, slEnd, zeroSI);

        if (!bStrictNumLCUPerSlice)
        {
            bool bLessLCUs = (nLCULeft > (nSlice / 2));

            nLcuPerSlice += bLessLCUs;
            nLCULeft =
                !bLessLCUs * nLCULeft
                + bLessLCUs * (nLcuPerSlice * nSlice - nLCU);

            mfxI32 delta       = (1 - 2 * bLessLCUs) * nLCUMult;
            mfxU32 step        = nSlice / nLCULeft;
            auto   slStepBegin = MakeStepIter(slBegin, step);
            auto   slStepEnd   = MakeStepIter(slEnd, step);
            auto   ModNumLCU   = [&delta](SliceInfo& si) { si.NumLCU += delta; };

            std::for_each(slStepBegin, slStepEnd, ModNumLCU);

            nLCULeft    = (mfxU32)std::max(0, int(nSlice / step - nLCULeft));
            step        = ((nSlice / step) / std::max(1u, nLCULeft)) * step;
            delta      *= -1;
            slStepBegin = MakeStepIter(slBegin, step);
            slStepEnd   = std::next(slStepBegin, nLCULeft);

            std::for_each(slStepBegin, slStepEnd, ModNumLCU);
        }

        std::for_each(
            slBegin
            , slEnd
            , [&](SliceInfo& si)
        {
            si.NumLCU         += nLcuPerSlice * nLCUMult;
            si.SegmentAddress = segAddr;
            segAddr           += si.NumLCU;
        });

        slices.back().NumLCU = slBegin->SegmentAddress + (nLCU * nLCUMult) - slices.back().SegmentAddress;

        return (mfxU16)nSlice;
    }

    static void GetTileUniformSpacingParam(
        std::vector<mfxU32>& colWidth
        , std::vector<mfxU32>& rowHeight
        , std::vector<mfxU32>& TsToRs
        , mfxU32 nCol
        , mfxU32 nRow
        , mfxU32 nTCol
        , mfxU32 nTRow
        , mfxU32 nLCU)
    {
        std::vector<mfxU32> colBd (nTCol + 1, 0);
        std::vector<mfxU32> rowBd (nTRow + 1, 0);

        colWidth.resize(nTCol, 0);
        rowHeight.resize(nTRow, 0);
        TsToRs.resize(nLCU);

        auto pColBd     = colBd.data();
        auto pRowBd     = rowBd.data();
        auto pColWidth  = colWidth.data();
        auto pRowHeight = rowHeight.data();

        mfxI32 i;
        auto NextCW = [nCol, nTCol, &i]() { ++i; return ((i + 1) * nCol) / nTCol - (i * nCol) / nTCol; };
        auto NextRH = [nRow, nTRow, &i]() { ++i; return ((i + 1) * nRow) / nTRow - (i * nRow) / nTRow; };
        auto NextBd  = [&i](mfxU32 wh) { return (i += wh); };

        i = -1;
        std::generate(pColWidth, pColWidth + nTCol, NextCW);
        i = -1;
        std::generate(pRowHeight, pRowHeight + nTRow, NextRH);
        i = 0;
        std::transform(pColWidth, pColWidth + nTCol, pColBd + 1, NextBd);
        i = 0;
        std::transform(pRowHeight, pRowHeight + nTRow, pRowBd + 1, NextBd);

        for (mfxU32 rso = 0; rso < nLCU; ++rso)
        {
            mfxU32 tbX   = rso % nCol;
            mfxU32 tbY   = rso / nCol;
            mfxU32 tso   = 0;
            auto   LTX   = [tbX](mfxU32 x) { return tbX >= x; };
            auto   LTY   = [tbY](mfxU32 y) { return tbY >= y; };
            auto   tileX = std::count_if(pColBd + 1, pColBd + nTCol, LTX);
            auto   tileY = std::count_if(pRowBd + 1, pRowBd + nTRow, LTY);
            
            tso += rowHeight[tileY] * std::accumulate(pColWidth, pColWidth + tileX, 0u);
            tso += nCol             * std::accumulate(pRowHeight, pRowHeight + tileY, 0u);
            tso += (tbY - rowBd[tileY]) * colWidth[tileX] + tbX - colBd[tileX];

            assert(tso < nLCU);

            TsToRs[tso] = rso;
        }
    }

    static mfxU16 Slices(
        Defaults::TGetSlices::TExt
        , const Defaults::Param& defPar
        , std::vector<SliceInfo>& slices)
    {
        const mfxExtCodingOption2* pCO2 = ExtBuffer::Get(defPar.mvp);
        mfxU16 W        = defPar.base.GetCodedPicWidth(defPar);
        mfxU16 H        = defPar.base.GetCodedPicHeight(defPar);
        mfxU16 LCUSize  = defPar.base.GetLCUSize(defPar);
        mfxU32 nCol     = CeilDiv<mfxU32>(W, LCUSize);
        mfxU32 nRow     = CeilDiv<mfxU32>(H, LCUSize);
        auto   tiles    = defPar.base.GetNumTiles(defPar);
        mfxU32 nTCol    = std::get<0>(tiles);
        mfxU32 nTRow    = std::get<1>(tiles);
        mfxU32 nTile    = nTCol * nTRow;
        mfxU32 nLCU     = nCol * nRow;
        mfxU32 nSlice   = std::min<mfxU32>(std::min<mfxU32>(nLCU, MAX_SLICES), std::max<mfxU32>(defPar.mvp.mfx.NumSlice, 1));
        mfxU32 SliceStructure = defPar.caps.SliceStructure;
        mfxU32 minSlice = !defPar.caps.msdk.bSingleSliceMultiTile * nTile;
        // DDI:0.9972: VDEnc supports multiple static slices in case of single tile frame only.
        //             But for multi tiles frame, no single tile can contain multiple static slices.
        // Actually it's no matter for driver whether dynamic or static slices are used
        // It returns an error in the both cases.
        mfxU32 maxSlice = (nTile > 1 && IsOn(defPar.mvp.mfx.LowPower)) * nTile;

        SetDefault(maxSlice, nSlice);
        maxSlice = std::max(maxSlice, minSlice);

        nSlice = mfx::clamp(nSlice, minSlice, maxSlice);

        bool bHardcodeSliceStructureRowSlice =
            defPar.hw >= MFX_HW_ICL
            && IsOn(defPar.mvp.mfx.LowPower)
            && nTile == 1;

        SliceStructure =
            SliceStructure * !bHardcodeSliceStructureRowSlice
            + ROWSLICE * bHardcodeSliceStructureRowSlice;

        slices.resize(0);

        bool bNumMbPerSliceSet = pCO2 && pCO2->NumMbPerSlice != 0;
        if (bNumMbPerSliceSet)
            nSlice = CeilDiv<mfxU32>(nLCU / nTile, pCO2->NumMbPerSlice) * nTile;

        nSlice = std::max<mfxU32>(nSlice * (SliceStructure != 0), 1);
        nSlice = std::max<mfxU32>(nSlice, nTile * (nSlice > 1));

        if (nTile == 1)
        {
            //TileScan = RasterScan, no SegmentAddress conversion required
            return defPar.base.GetTileSlices(defPar, slices, SliceStructure, nCol, nRow, nSlice);
        }

        std::vector<mfxU32> colWidth  (nTCol, 0);
        std::vector<mfxU32> rowHeight (nTRow, 0);
        std::vector<mfxU32> TsToRs    (nLCU);

        //assume uniform spacing
        GetTileUniformSpacingParam(colWidth, rowHeight, TsToRs, nCol, nRow, nTCol, nTRow, nLCU);

        if (nSlice == 1)
        {
            defPar.base.GetTileSlices(defPar, slices, SliceStructure, nCol, nRow, nSlice);
        }
        else
        {
            using TileInfo = struct
            {
                mfxU32 id;
                mfxU32 nCol;
                mfxU32 nRow;
                mfxU32 nLCU;
                mfxU32 nSlice;
            };
            auto TileIdLess = [](TileInfo& l, TileInfo& r)
            {
                return l.id < r.id;
            };
            auto NumSliceCoeff = [](TileInfo const & tile)
            {
                assert(tile.nSlice > 0);
                return (mfxF64(tile.nLCU) / tile.nSlice);
            };
            auto CmpTiles = [&](TileInfo& l, TileInfo& r)
            {
                return NumSliceCoeff(l) > NumSliceCoeff(r);
            };
            auto AddTileSlices = [&](TileInfo& t)
            {
                defPar.base.GetTileSlices(defPar, slices, SliceStructure, t.nCol, t.nRow, t.nSlice);
            };
            std::vector<TileInfo> tile(nTile);
            mfxU32 id           = 0;
            mfxU32 nLcuPerSlice = CeilDiv(nLCU, nSlice);
            mfxU32 nSliceRest   = nSlice;

            std::for_each(std::begin(rowHeight), std::end(rowHeight), [&](mfxU32 h)
            {
                std::for_each(std::begin(colWidth), std::end(colWidth), [&](mfxU32 w)
                {
                    auto& curTile = tile[id];
                    curTile.id     = id;
                    curTile.nCol   = w;
                    curTile.nRow   = h;
                    curTile.nLCU   = curTile.nCol * curTile.nRow;
                    curTile.nSlice = std::max<mfxU32>(1U, curTile.nLCU / nLcuPerSlice);

                    nSliceRest -= curTile.nSlice;
                    ++id;
                });
            });

            while (nSliceRest)
            {
                std::sort(tile.begin(), tile.end(), CmpTiles);

                assert(tile[0].nLCU > tile[0].nSlice);

                tile[0].nSlice++;
                nSliceRest--;
            }

            std::sort(tile.begin(), tile.end(), TileIdLess);

            std::for_each(std::begin(tile), std::end(tile), AddTileSlices);
        }

        std::for_each(std::begin(slices), std::end(slices)
            , [&](SliceInfo& s)
        {
            assert(s.SegmentAddress < nLCU);
            s.SegmentAddress = TsToRs[s.SegmentAddress];
        });

        assert(slices.size() <= MAX_SLICES);

        return (mfxU16)slices.size();
    }

    static mfxStatus VPS(
        Defaults::TGetVPS::TExt
        , const Defaults::Param& dflts
        , Base::VPS& vps)
    {
        auto&                  par       = dflts.mvp;
        const mfxExtHEVCParam& HEVCParam = ExtBuffer::Get(par);
        mfxU16                 NumTL     = dflts.base.GetNumTemporalLayers(dflts);
        PTL&                   general   = vps.general;
        SubLayerOrdering&      slo       = vps.sub_layer[NumTL - 1];

        vps = Base::VPS{};
        vps.video_parameter_set_id                  = 0;
        vps.reserved_three_2bits                    = 3;
        vps.max_layers_minus1                       = 0;
        vps.max_sub_layers_minus1                   = mfxU16(NumTL - 1);
        vps.temporal_id_nesting_flag                = 1;
        vps.reserved_0xffff_16bits                  = 0xFFFF;
        vps.sub_layer_ordering_info_present_flag    = 0;

        vps.timing_info_present_flag        = 1;
        vps.num_units_in_tick               = par.mfx.FrameInfo.FrameRateExtD;
        vps.time_scale                      = par.mfx.FrameInfo.FrameRateExtN;
        vps.poc_proportional_to_timing_flag = 0;
        vps.num_hrd_parameters              = 0;

        general.profile_space                   = 0;
        general.tier_flag                       = !!(par.mfx.CodecLevel & MFX_TIER_HEVC_HIGH);
        general.profile_idc                     = mfxU8(par.mfx.CodecProfile);
        general.profile_compatibility_flags     = 1 << (31 - general.profile_idc);
        general.progressive_source_flag         = 1;
        general.interlaced_source_flag          = 0;
        general.non_packed_constraint_flag      = 0;
        general.frame_only_constraint_flag      = 1;
        general.level_idc                       = mfxU8((par.mfx.CodecLevel & 0xFF) * 3);

        if (par.mfx.CodecProfile == MFX_PROFILE_HEVC_REXT)
        {
            general.rext_constraint_flags_0_31  = (mfxU32)(HEVCParam.GeneralConstraintFlags & 0xffffffff);
            general.rext_constraint_flags_32_42 = (mfxU32)(HEVCParam.GeneralConstraintFlags >> 32);
        }

        auto numReorderFrames = dflts.base.GetNumReorderFrames(dflts);
        slo.max_dec_pic_buffering_minus1 = par.mfx.NumRefFrame;
        slo.max_num_reorder_pics         = std::min<mfxU8>(numReorderFrames, slo.max_dec_pic_buffering_minus1);
        slo.max_latency_increase_plus1   = 0;

        return MFX_ERR_NONE;
    }

    static mfxStatus SPS(
        Defaults::TGetSPS::TExt
        , const Defaults::Param& defPar
        , const Base::VPS& vps
        , Base::SPS& sps)
    {
        const std::map<mfxU32, mfxU32> arIdc =
        {
              {(  0 << 16) +  0,  0}
            , {(  1 << 16) +  1,  1}
            , {( 12 << 16) + 11,  2}
            , {( 10 << 16) + 11,  3}
            , {( 16 << 16) + 11,  4}
            , {( 40 << 16) + 33,  5}
            , {( 24 << 16) + 11,  6}
            , {( 20 << 16) + 11,  7}
            , {( 32 << 16) + 11,  8}
            , {( 80 << 16) + 33,  9}
            , {( 18 << 16) + 11, 10}
            , {( 15 << 16) + 11, 11}
            , {( 64 << 16) + 33, 12}
            , {(160 << 16) + 99, 13}
            , {(  4 << 16) +  3, 14}
            , {(  3 << 16) +  2, 15}
            , {(  2 << 16) +  1, 16}
        };

        auto GetAspectRatioIdc = [&arIdc](mfxU16 w, mfxU16 h)
        {
            mfxU32 key = (w << 16) + h;
            return arIdc.count(key) ? mfxU8(arIdc.at(key)) : mfxU8(255);
        };
        auto& slo = vps.sub_layer[vps.max_sub_layers_minus1];
        const mfxExtHEVCParam&      HEVCParam = ExtBuffer::Get(defPar.mvp);
        const mfxExtCodingOption&   CO        = ExtBuffer::Get(defPar.mvp);
        const mfxExtCodingOption2&  CO2       = ExtBuffer::Get(defPar.mvp);
        const mfxExtCodingOption3&  CO3       = ExtBuffer::Get(defPar.mvp);
        auto  hw  = defPar.hw;
        auto& mfx = defPar.mvp.mfx;

        sps = Base::SPS{};
        ((LayersInfo&)sps) = ((LayersInfo&)vps);
        sps.video_parameter_set_id   = vps.video_parameter_set_id;
        sps.max_sub_layers_minus1    = vps.max_sub_layers_minus1;
        sps.temporal_id_nesting_flag = vps.temporal_id_nesting_flag;

        sps.seq_parameter_set_id              = 0;
        sps.chroma_format_idc                 = CO3.TargetChromaFormatPlus1 - 1;
        sps.separate_colour_plane_flag        = 0;
        sps.pic_width_in_luma_samples         = HEVCParam.PicWidthInLumaSamples;
        sps.pic_height_in_luma_samples        = HEVCParam.PicHeightInLumaSamples;
        sps.conformance_window_flag           = 0;
        sps.bit_depth_luma_minus8             = (mfxU8)std::max<mfxI32>(0, CO3.TargetBitDepthLuma - 8);
        sps.bit_depth_chroma_minus8           = (mfxU8)std::max<mfxI32>(0, CO3.TargetBitDepthChroma - 8);
        sps.log2_max_pic_order_cnt_lsb_minus4 = (mfxU8)mfx::clamp(CeilLog2(mfx.GopRefDist + slo.max_dec_pic_buffering_minus1) - 1, 0u, 12u);

        sps.log2_min_luma_coding_block_size_minus3   = 0;
        sps.log2_diff_max_min_luma_coding_block_size = CeilLog2(HEVCParam.LCUSize) - 3 - sps.log2_min_luma_coding_block_size_minus3;
        sps.log2_min_transform_block_size_minus2     = 0;
        sps.log2_diff_max_min_transform_block_size   = 3;
        sps.max_transform_hierarchy_depth_inter      = 2;
        sps.max_transform_hierarchy_depth_intra      = 2;
        sps.scaling_list_enabled_flag                = 0;

        bool bHwCNLPlus = (hw >= MFX_HW_CNL);
        sps.amp_enabled_flag                    = bHwCNLPlus;
        sps.sample_adaptive_offset_enabled_flag = bHwCNLPlus && !(HEVCParam.SampleAdaptiveOffset & MFX_SAO_DISABLE);
        sps.pcm_enabled_flag                    = 0;
        sps.long_term_ref_pics_present_flag     = 1;
        sps.temporal_mvp_enabled_flag           = 1;
        sps.strong_intra_smoothing_enabled_flag = 0;

        // QpModulation support
        bool  isBPyramid = (CO2.BRefType == MFX_B_REF_PYRAMID);
        sps.low_delay_mode = bHwCNLPlus && (mfx.GopRefDist == 1);
        sps.hierarchical_flag = bHwCNLPlus && isBPyramid && ((mfx.GopRefDist == 4) || (mfx.GopRefDist == 8));

        auto&  fi            = mfx.FrameInfo;
        mfxU16 SubWidthC[4]  = {1,2,2,1};
        mfxU16 SubHeightC[4] = {1,2,1,1};
        mfxU16 cropUnitX     = SubWidthC[sps.chroma_format_idc];
        mfxU16 cropUnitY     = SubHeightC[sps.chroma_format_idc];

        sps.conf_win_left_offset      = (fi.CropX / cropUnitX);
        sps.conf_win_right_offset     = (sps.pic_width_in_luma_samples - fi.CropW - fi.CropX) / cropUnitX;
        sps.conf_win_top_offset       = (fi.CropY / cropUnitY);
        sps.conf_win_bottom_offset    = (sps.pic_height_in_luma_samples - fi.CropH - fi.CropY) / cropUnitY;
        sps.conformance_window_flag   =    sps.conf_win_left_offset
                                        || sps.conf_win_right_offset
                                        || sps.conf_win_top_offset
                                        || sps.conf_win_bottom_offset;

        sps.vui_parameters_present_flag         = 1;
        sps.vui.aspect_ratio_info_present_flag  = 1;
        sps.vui.aspect_ratio_idc                = GetAspectRatioIdc(fi.AspectRatioW, fi.AspectRatioH);
        sps.vui.sar_width                       = fi.AspectRatioW;
        sps.vui.sar_height                      = fi.AspectRatioH;

        const mfxExtVideoSignalInfo& VSI = ExtBuffer::Get(defPar.mvp);
        sps.vui.video_format                    = mfxU8(VSI.VideoFormat);
        sps.vui.video_full_range_flag           = VSI.VideoFullRange;
        sps.vui.colour_description_present_flag = VSI.ColourDescriptionPresent;
        sps.vui.colour_primaries                = mfxU8(VSI.ColourPrimaries);
        sps.vui.transfer_characteristics        = mfxU8(VSI.TransferCharacteristics);
        sps.vui.matrix_coeffs                   = mfxU8(VSI.MatrixCoefficients);
        sps.vui.video_signal_type_present_flag  =
               (VSI.VideoFormat              != 5)
            || (VSI.VideoFullRange           != 0)
            || (VSI.ColourDescriptionPresent != 0);

        sps.vui.timing_info_present_flag      = !!vps.timing_info_present_flag;
        sps.vui.num_units_in_tick             = vps.num_units_in_tick;
        sps.vui.time_scale                    = vps.time_scale;
        sps.vui.field_seq_flag                = 0;
        sps.vui.frame_field_info_present_flag =
            IsOn(CO.PicTimingSEI)
            || sps.vui.field_seq_flag
            || (vps.general.progressive_source_flag
                && vps.general.interlaced_source_flag);

        if (IsOn(CO.VuiNalHrdParameters))
        {
            HRDInfo&                hrd        = sps.vui.hrd;
            HRDInfo::SubLayer&      sl0        = hrd.sl[0];
            HRDInfo::SubLayer::CPB& cpb0       = sl0.cpb[0];
            mfxU32                  hrdBitrate = HEVCEHW::MaxKbps(mfx) * 1000;
            mfxU32                  cpbSize    = HEVCEHW::BufferSizeInKB(mfx) * 8000;

            sps.vui.hrd_parameters_present_flag = 1;
            hrd.nal_hrd_parameters_present_flag = 1;
            hrd.sub_pic_hrd_params_present_flag = 0;

            hrd.bit_rate_scale = mfxU16(mfx::clamp<mfxI32>(CountTrailingZeroes(hrdBitrate) - 6, 0, 16));
            hrd.cpb_size_scale = mfxU16(mfx::clamp<mfxI32>(CountTrailingZeroes(cpbSize) - 4, 2, 16));

            hrd.initial_cpb_removal_delay_length_minus1 = 23;
            hrd.au_cpb_removal_delay_length_minus1      = 23;
            hrd.dpb_output_delay_length_minus1          = 23;

            sl0.fixed_pic_rate_general_flag = 1;
            sl0.low_delay_hrd_flag          = 0;
            sl0.cpb_cnt_minus1              = 0;

            cpb0.bit_rate_value_minus1 = (hrdBitrate >> (6 + hrd.bit_rate_scale)) - 1;
            cpb0.cpb_size_value_minus1 = (cpbSize >> (4 + hrd.cpb_size_scale)) - 1;
            cpb0.cbr_flag              = (mfx.RateControlMethod == MFX_RATECONTROL_CBR);
        }

        return MFX_ERR_NONE;
    }

    static mfxStatus PPS(
        Defaults::TGetPPS::TExt
        , const Defaults::Param& defPar
        , const Base::SPS& sps
        , Base::PPS& pps)
    {
        auto& par = defPar.mvp;
        auto  hw = defPar.hw;
        const mfxExtHEVCParam& HEVCParam = ExtBuffer::Get(par);
        const mfxExtHEVCTiles& HEVCTiles = ExtBuffer::Get(par);
        const mfxExtCodingOption2& CO2 = ExtBuffer::Get(par);
        const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);
        bool bSWBRC = Legacy::IsSWBRC(par, &CO2);
        bool bCQP = (par.mfx.RateControlMethod == MFX_RATECONTROL_CQP);

        mfxU16 maxRefP   = *std::max_element(CO3.NumRefActiveP, CO3.NumRefActiveP + 8);
        mfxU16 maxRefBL0 = *std::max_element(CO3.NumRefActiveBL0, CO3.NumRefActiveBL0 + 8);
        mfxU16 maxRefBL1 = *std::max_element(CO3.NumRefActiveBL1, CO3.NumRefActiveBL1 + 8);

        pps = {};

        pps.seq_parameter_set_id = sps.seq_parameter_set_id;

        pps.pic_parameter_set_id                  = 0;
        pps.dependent_slice_segments_enabled_flag = 0;
        pps.output_flag_present_flag              = 0;
        pps.num_extra_slice_header_bits           = 0;
        pps.sign_data_hiding_enabled_flag         = 0;
        pps.cabac_init_present_flag               = 0;
        pps.num_ref_idx_l0_default_active_minus1  = (par.mfx.GopRefDist <= 2) ? (maxRefP - 1) : (maxRefBL0 - 1);
        pps.num_ref_idx_l1_default_active_minus1  = (par.mfx.GopRefDist <= 2) ? (maxRefP - 1) : (maxRefBL1 - 1);
        pps.init_qp_minus26                       = 0;
        pps.constrained_intra_pred_flag           = 0;
        pps.transform_skip_enabled_flag = (hw >= MFX_HW_CNL) && IsOn(CO3.TransformSkip);

        pps.cu_qp_delta_enabled_flag = !(IsOff(CO3.EnableMBQP) && bCQP) && !bSWBRC;
        pps.cu_qp_delta_enabled_flag |= (IsOn(par.mfx.LowPower) || CO2.MaxSliceSize);

        // according to BSpec only 3 and 0 are supported
        pps.diff_cu_qp_delta_depth                = (HEVCParam.LCUSize == 64) * 3;
        pps.cb_qp_offset                          = (bSWBRC * -1);
        pps.cr_qp_offset                          = (bSWBRC * -1);
        pps.slice_chroma_qp_offsets_present_flag  = (bSWBRC *  1);

        mfxI32 QP =
            (par.mfx.GopPicSize == 1) * par.mfx.QPI
            + (par.mfx.GopPicSize != 1 && par.mfx.GopRefDist == 1) * par.mfx.QPP
            + (par.mfx.GopPicSize != 1 && par.mfx.GopRefDist != 1) * par.mfx.QPB;

        pps.init_qp_minus26 = bCQP * (QP - 26 - (6 * sps.bit_depth_luma_minus8));

        pps.slice_chroma_qp_offsets_present_flag  = 0;
        pps.weighted_pred_flag                    = 0;
        pps.weighted_bipred_flag                  = 0;
        pps.transquant_bypass_enabled_flag        = 0;
        pps.tiles_enabled_flag                    = 0;
        pps.entropy_coding_sync_enabled_flag      = 0;

        if (HEVCTiles.NumTileColumns * HEVCTiles.NumTileRows > 1)
        {
            mfxU16 nCol   = (mfxU16)CeilDiv(HEVCParam.PicWidthInLumaSamples,  HEVCParam.LCUSize);
            mfxU16 nRow   = (mfxU16)CeilDiv(HEVCParam.PicHeightInLumaSamples, HEVCParam.LCUSize);
            mfxU16 nTCol  = std::max<mfxU16>(HEVCTiles.NumTileColumns, 1);
            mfxU16 nTRow  = std::max<mfxU16>(HEVCTiles.NumTileRows, 1);

            pps.tiles_enabled_flag        = 1;
            pps.uniform_spacing_flag      = 1;
            pps.num_tile_columns_minus1   = nTCol - 1;
            pps.num_tile_rows_minus1      = nTRow - 1;

            mfxI32 i = -1;
            std::generate(std::begin(pps.column_width), std::end(pps.column_width)
                , [nCol, nTCol, &i]() { ++i; return mfxU16(((i + 1) * nCol) / nTCol - (i * nCol) / nTCol); });

            i = -1;
            std::generate(std::begin(pps.row_height), std::end(pps.row_height)
                , [nRow, nTRow, &i]() { ++i; return mfxU16(((i + 1) * nRow) / nTRow - (i * nRow) / nTRow); });

            pps.loop_filter_across_tiles_enabled_flag = 1;
        }

        pps.loop_filter_across_slices_enabled_flag = (hw >= MFX_HW_CNL);

        pps.deblocking_filter_control_present_flag  = 1;
        pps.deblocking_filter_disabled_flag         = !!CO2.DisableDeblockingIdc;
        pps.deblocking_filter_override_enabled_flag = 1; // to disable deblocking per frame
#if MFX_VERSION >= MFX_VERSION_NEXT
        pps.beta_offset_div2                        = mfxI8(CO3.DeblockingBetaOffset * 0.5 * !pps.deblocking_filter_disabled_flag);
        pps.tc_offset_div2                          = mfxI8(CO3.DeblockingAlphaTcOffset * 0.5 * !pps.deblocking_filter_disabled_flag);
#endif

        pps.scaling_list_data_present_flag              = 0;
        pps.lists_modification_present_flag             = 1;
        pps.log2_parallel_merge_level_minus2            = 0;
        pps.slice_segment_header_extension_present_flag = 0;

        return MFX_ERR_NONE;
    }

    static mfxU8 SHNUT(
        Defaults::TGetSHNUT::TExt
        , const Defaults::Param&
        , const TaskCommonPar& task
        , bool bRAPIntra)
    {
        const DpbArray& DPB = task.DPB.After;
        const bool isI      = IsI(task.FrameType);
        const bool isRef    = IsRef(task.FrameType);
        const bool isIDR    = IsIdr(task.FrameType);
        auto IsNonCurrLTR = [&](const DpbFrame& ref)
        {
            return (isValid(ref) && ref.isLTR && ref.Rec.Idx != task.Rec.Idx);
        };

        mfxU8 NUT = mfxU8(task.ctrl.MfxNalUnitType);
        bool bExtNUTValid =
            task.ctrl.MfxNalUnitType
            && ((NUT == MFX_HEVC_NALU_TYPE_TRAIL_R && (task.POC > task.PrevRAP && isRef))
                || (NUT == MFX_HEVC_NALU_TYPE_TRAIL_N && (task.POC > task.PrevRAP && !isRef))
                || (NUT == MFX_HEVC_NALU_TYPE_RASL_R && (task.POC < task.PrevRAP && isRef))
                || (NUT == MFX_HEVC_NALU_TYPE_RASL_N && (task.POC < task.PrevRAP && !isRef))
                || (NUT == MFX_HEVC_NALU_TYPE_RADL_R && (task.POC < task.PrevRAP && isRef))
                || (NUT == MFX_HEVC_NALU_TYPE_RADL_N && (task.POC < task.PrevRAP && !isRef))
                || (NUT == MFX_HEVC_NALU_TYPE_CRA_NUT && isI)
                || (NUT == MFX_HEVC_NALU_TYPE_IDR_W_RADL && isIDR)
                || (NUT == MFX_HEVC_NALU_TYPE_IDR_N_LP && isIDR));

        NUT *= bExtNUTValid;
        NUT += mfxU8((!NUT && isIDR) * (IDR_W_RADL + 1));
        NUT += mfxU8((!NUT && isI && bRAPIntra
            && std::end(DPB) != std::find_if(DPB, std::end(DPB), IsNonCurrLTR))
            * (TRAIL_R + 1)); //following frames may refer to prev. GOP
        NUT += mfxU8((!NUT && isI && bRAPIntra) * (CRA_NUT + 1));
        NUT += mfxU8((!NUT && task.TemporalID > 0) * (TSA_N + isRef + 1));
        NUT += mfxU8((!NUT && task.POC > task.PrevRAP) * (TRAIL_N + isRef + 1));
        NUT += mfxU8(!NUT * (RASL_N + isRef + 1));

        return mfxU8(NUT - 1);
    }

    static mfxU32 FrameOrder(
        Defaults::TGetFrameOrder::TExt
        , const Defaults::Param& par
        , const StorageR& s_task
        , mfxU32 prevFrameOrder)
    {
        if (par.mvp.mfx.EncodedOrder)
        {
            auto& task = Task::Common::Get(s_task);
            if (task.pSurfIn)
            {
                return task.pSurfIn->Data.FrameOrder;
            }
        }

        return (prevFrameOrder + 1);
    }

    static void Push(Defaults& df)
    {
#define PUSH_DEFAULT(X) df.Get##X.Push(X);

        PUSH_DEFAULT(CodedPicAlignment);
        PUSH_DEFAULT(CodedPicWidth);
        PUSH_DEFAULT(CodedPicHeight);
        PUSH_DEFAULT(MaxDPB);
        PUSH_DEFAULT(GopPicSize);
        PUSH_DEFAULT(GopRefDist);
        PUSH_DEFAULT(NumBPyramidLayers);
        PUSH_DEFAULT(NumRefBPyramid);
        PUSH_DEFAULT(NumRefPPyramid);
        PUSH_DEFAULT(NumRefNoPyramid);
        PUSH_DEFAULT(NumRefFrames);
        PUSH_DEFAULT(MinRefForBPyramid);
        PUSH_DEFAULT(MinRefForBNoPyramid);
        PUSH_DEFAULT(NumRefActive);
        PUSH_DEFAULT(BRefType);
        PUSH_DEFAULT(PRefType);
        PUSH_DEFAULT(FrameRate);
        PUSH_DEFAULT(MaxBitDepthByFourCC);
        PUSH_DEFAULT(MaxBitDepth);
        PUSH_DEFAULT(MaxChroma);
        PUSH_DEFAULT(MaxChromaByFourCC);
        PUSH_DEFAULT(TargetBitDepthLuma);
        PUSH_DEFAULT(TargetChromaFormat);
        PUSH_DEFAULT(TargetKbps);
        PUSH_DEFAULT(MaxKbps);
        PUSH_DEFAULT(BufferSizeInKB);
        PUSH_DEFAULT(MaxNumRef);
        PUSH_DEFAULT(RateControlMethod);
        PUSH_DEFAULT(MinQPMFX);
        PUSH_DEFAULT(MaxQPMFX);
        PUSH_DEFAULT(QPMFX);
        PUSH_DEFAULT(QPOffset);
        PUSH_DEFAULT(Profile);
        PUSH_DEFAULT(GUID);
        PUSH_DEFAULT(MBBRC);
        PUSH_DEFAULT(AsyncDepth);
        PUSH_DEFAULT(NumSlices);
        PUSH_DEFAULT(LCUSize);
        PUSH_DEFAULT(HRDConformanceON);
        PUSH_DEFAULT(PicTimingSEI);
        PUSH_DEFAULT(FrameType);
        PUSH_DEFAULT(PPyrInterval);
        PUSH_DEFAULT(PLayer);
        PUSH_DEFAULT(NumTemporalLayers);
        PUSH_DEFAULT(TId);
        PUSH_DEFAULT(HighestTId);
        PUSH_DEFAULT(RPLFromExt);
        PUSH_DEFAULT(RPL);
        PUSH_DEFAULT(RPLMod);
        PUSH_DEFAULT(RPLFromCtrl);
        PUSH_DEFAULT(PreReorderInfo);
        PUSH_DEFAULT(LowPower);
        PUSH_DEFAULT(NumReorderFrames);
        PUSH_DEFAULT(NonStdReordering);
        PUSH_DEFAULT(NumTiles);
        PUSH_DEFAULT(TileSlices);
        PUSH_DEFAULT(Slices);
        PUSH_DEFAULT(VPS);
        PUSH_DEFAULT(SPS);
        PUSH_DEFAULT(PPS);
        PUSH_DEFAULT(FrameNumRefActive);
        PUSH_DEFAULT(SHNUT);
        PUSH_DEFAULT(WeakRef);
        PUSH_DEFAULT(FrameOrder);

#undef PUSH_DEFAULT

        df.CmpRefLX[0].Push(CmpRef);
        df.CmpRefLX[1].Push(CmpRef);
    }
};

class PreCheck
{
public:
    static mfxStatus CodecId(
        Defaults::TPreCheck::TExt
        , const mfxVideoParam& in)
    {
        MFX_CHECK(in.mfx.CodecId == MFX_CODEC_HEVC, MFX_ERR_UNSUPPORTED);
        return MFX_ERR_NONE;
    }

    static mfxStatus ChromaFormat(
        Defaults::TPreCheck::TExt
        , const mfxVideoParam& in)
    {
        bool bUnsupported = Check<mfxU16
            , MFX_CHROMAFORMAT_YUV420
            , MFX_CHROMAFORMAT_YUV422
            , MFX_CHROMAFORMAT_YUV444>
            (in.mfx.FrameInfo.ChromaFormat);

        MFX_CHECK(!bUnsupported, MFX_ERR_UNSUPPORTED);
        return MFX_ERR_NONE;
    }

    static void Push(Defaults& df)
    {
#define PUSH_DEFAULT(X) df.PreCheck##X.Push(X);

        PUSH_DEFAULT(CodecId);
        PUSH_DEFAULT(ChromaFormat);

#undef PUSH_DEFAULT
    }
};

class CheckAndFix
{
public:
    static mfxStatus LCUSize(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& dpar
        , mfxVideoParam& par)
    {
        mfxExtHEVCParam* pHEVC = ExtBuffer::Get(par);
        MFX_CHECK(pHEVC, MFX_ERR_NONE);

        auto& lcuSize = pHEVC->LCUSize;
        MFX_CHECK(lcuSize != 0, MFX_ERR_NONE);// zero is allowed

        /*
            LCUSizeSupported - Supported LCU sizes, bit fields
            0b001 : 16x16
            0b010 : 32x32
            0b100 : 64x64
        */
        assert(dpar.caps.LCUSizeSupported > 0);

        bool bValid =
            !Check<mfxU16, 16, 32, 64>(lcuSize)
            && ((lcuSize >> 4) & dpar.caps.LCUSizeSupported);

        lcuSize *= bValid;
        MFX_CHECK(bValid, MFX_ERR_UNSUPPORTED);

        return MFX_ERR_NONE;
    }

    static mfxStatus LowDelayBRC(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& dpar
        , mfxVideoParam& par)
    {
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

        MFX_CHECK(pCO3, MFX_ERR_NONE);

        mfxU32 changed = 0;
        bool bAllowed =
            (dpar.hw >= MFX_HW_ICL)
            && !Check<mfxU16
            , MFX_RATECONTROL_VBR
            , MFX_RATECONTROL_QVBR
            , MFX_RATECONTROL_VCM>
            (par.mfx.RateControlMethod);

        changed += CheckOrZero<mfxU16>(
            pCO3->LowDelayBRC
            , mfxU16(MFX_CODINGOPTION_UNKNOWN)
            , mfxU16(MFX_CODINGOPTION_OFF)
            , mfxU16(MFX_CODINGOPTION_ON * bAllowed));

        bool bOn = IsOn(pCO3->LowDelayBRC);

        changed += bOn && CheckOrZero<mfxU16, 0>(pCO3->WinBRCMaxAvgKbps);
        changed += bOn && CheckOrZero<mfxU16, 0>(pCO3->WinBRCSize);
        changed += bOn && SetIf(par.mfx.GopRefDist, par.mfx.GopRefDist > 1, 1);

        MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        return MFX_ERR_NONE;
    }

    static mfxStatus Level(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& /*dpar*/
        , mfxVideoParam& par)
    {
        bool bInalid = CheckOrZero<mfxU16
            , MFX_LEVEL_HEVC_1
            , MFX_LEVEL_HEVC_2
            , MFX_LEVEL_HEVC_21
            , MFX_LEVEL_HEVC_3
            , MFX_LEVEL_HEVC_31
            , MFX_LEVEL_HEVC_4
            , MFX_LEVEL_HEVC_41
            , MFX_LEVEL_HEVC_5
            , MFX_LEVEL_HEVC_51
            , MFX_LEVEL_HEVC_52
            , MFX_LEVEL_HEVC_6
            , MFX_LEVEL_HEVC_61
            , MFX_LEVEL_HEVC_62
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_1
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_2
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_21
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_3
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_31
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_4
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_41
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_5
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_51
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_52
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_6
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_61
            , MFX_TIER_HEVC_HIGH | MFX_LEVEL_HEVC_62
            , 0>
            (par.mfx.CodecLevel);

        MFX_CHECK(!bInalid, MFX_ERR_UNSUPPORTED);
        return MFX_ERR_NONE;
    }

    static mfxStatus SurfSize(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& dpar
        , mfxVideoParam& par)
    {
        auto      pCO2     = (const mfxExtCodingOption2*)ExtBuffer::Get(par);
        auto&     W        = par.mfx.FrameInfo.Width;
        auto&     H        = par.mfx.FrameInfo.Height;
        auto&     CRPX     = par.mfx.FrameInfo.CropX;
        auto&     CRPY     = par.mfx.FrameInfo.CropY;
        auto      CRPX_c   = CRPX;
        auto      CRPY_c   = CRPY;
        auto&     CRPW     = par.mfx.FrameInfo.CropW;
        auto&     CRPH     = par.mfx.FrameInfo.CropH;
        auto      AW       = mfx::align2_value(W, HW_SURF_ALIGN_W);
        auto      AH       = mfx::align2_value(H, HW_SURF_ALIGN_H);
        mfxU32    changed  = 0;
        mfxStatus UnsSts   = MFX_ERR_NONE;
        bool      bVDEnc64 = IsOn(par.mfx.LowPower) && (64 == dpar.base.GetLCUSize(dpar));
        mfxU16    MaxW     = mfxU16(dpar.caps.MaxPicWidth);
        mfxU16    MaxH     = mfxU16(dpar.caps.MaxPicHeight);
        mfxU16    MinW     = mfxU16(128 * (bVDEnc64 && !dpar.caps.SliceByteSizeCtrl));
        mfxU16    MinH     = mfxU16(128 * (bVDEnc64));

        MinW = std::max(MinW, mfxU16(192 * (bVDEnc64 && pCO2 && pCO2->MaxSliceSize)));

        MFX_CHECK(W, MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(H, MFX_ERR_INVALID_VIDEO_PARAM);

        changed += (W != AW) + (H != AH);
        W = AW;
        H = AH;

        MFX_CHECK_NO_RET(!CheckRangeOrSetDefault<mfxU16>(W, MinW, MaxW, 0), UnsSts, MFX_ERR_UNSUPPORTED);
        MFX_CHECK_NO_RET(!CheckRangeOrSetDefault<mfxU16>(H, MinH, MaxH, 0), UnsSts, MFX_ERR_UNSUPPORTED);
        MFX_CHECK_NO_RET(!CheckMaxOrZero        <mfxU16>(CRPX, W),          UnsSts, MFX_ERR_UNSUPPORTED);
        MFX_CHECK_NO_RET(!CheckMaxOrZero        <mfxU16>(CRPY, H),          UnsSts, MFX_ERR_UNSUPPORTED);
        MFX_CHECK_NO_RET(!CheckMaxOrZero        <mfxU16>(CRPW, W - CRPX_c), UnsSts, MFX_ERR_UNSUPPORTED);
        MFX_CHECK_NO_RET(!CheckMaxOrZero        <mfxU16>(CRPH, H - CRPY_c), UnsSts, MFX_ERR_UNSUPPORTED);

        MFX_CHECK_STS(UnsSts);

        MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        return MFX_ERR_NONE;
    }

    static mfxStatus Profile(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& defPar
        , mfxVideoParam& par)
    {
        mfxU32 changed = 0;
        mfxExtHEVCParam* pHEVC = ExtBuffer::Get(par);

        changed += (pHEVC && defPar.hw < MFX_HW_ICL)
            && CheckOrZero<mfxU64, 0>(pHEVC->GeneralConstraintFlags);

        mfxU16 ChromaFormat = defPar.base.GetTargetChromaFormat(defPar) - 1;
        mfxU16 BitDepth     = defPar.base.GetTargetBitDepthLuma(defPar);
        mfxU64 REXTConstr   = 0;

        if (pHEVC)
            REXTConstr = pHEVC->GeneralConstraintFlags;

        bool bValid =
            !par.mfx.CodecProfile
            || (par.mfx.CodecProfile == MFX_PROFILE_HEVC_REXT
                && !((defPar.hw < MFX_HW_ICL)
                    || ((REXTConstr & MFX_HEVC_CONSTR_REXT_MAX_8BIT) && BitDepth > 8)
                    || ((REXTConstr & MFX_HEVC_CONSTR_REXT_MAX_10BIT) && BitDepth > 10)
                    || ((REXTConstr & MFX_HEVC_CONSTR_REXT_MAX_12BIT) && BitDepth > 12)
                    || ((REXTConstr & MFX_HEVC_CONSTR_REXT_MAX_420CHROMA) && ChromaFormat > MFX_CHROMAFORMAT_YUV420)
                    || ((REXTConstr & MFX_HEVC_CONSTR_REXT_MAX_422CHROMA) && ChromaFormat > MFX_CHROMAFORMAT_YUV422)))
            || (par.mfx.CodecProfile == MFX_PROFILE_HEVC_MAIN10 && !(BitDepth != 10 && BitDepth != 0))
            || (par.mfx.CodecProfile == MFX_PROFILE_HEVC_MAINSP)
            || (par.mfx.CodecProfile == MFX_PROFILE_HEVC_MAIN && !(BitDepth != 8 && BitDepth != 0));

        changed += (par.mfx.CodecProfile == MFX_PROFILE_HEVC_MAINSP)
            && CheckOrZero<mfxU16>(par.mfx.GopPicSize, 0, 1);

        par.mfx.CodecProfile *= bValid;

        MFX_CHECK(bValid, MFX_ERR_UNSUPPORTED);
        MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        return MFX_ERR_NONE;
    }

    static const std::map<mfxU32, std::array<mfxU32, 3>> FourCCPar;

    static mfxStatus FourCC(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& dpar
        , mfxVideoParam& par)
    {
        const mfxU16 BdMap[] = {8, 10, 12, 16};
        auto& FourCC    = par.mfx.FrameInfo.FourCC;
        auto  it        = FourCCPar.find(FourCC);
        bool  bInvalid  = (it == FourCCPar.end());

        bInvalid = bInvalid || (it->second[2] > mfxU32(dpar.hw));
        bInvalid = bInvalid || (it->second[1] > BdMap[dpar.caps.MaxEncodedBitDepth & 3]);

        FourCC *= !bInvalid;

        MFX_CHECK(!bInvalid, MFX_ERR_UNSUPPORTED);

        return MFX_ERR_NONE;
    }

    static mfxStatus InputFormatByFourCC(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& /*dpar*/
        , mfxVideoParam& par)
    {
        mfxU32 invalid = 0;

        auto itFourCCPar = FourCCPar.find(par.mfx.FrameInfo.FourCC);
        invalid += (itFourCCPar == FourCCPar.end() && Res2Bool(par.mfx.FrameInfo.FourCC, mfxU32(MFX_FOURCC_NV12)));

        itFourCCPar = FourCCPar.find(par.mfx.FrameInfo.FourCC);
        assert(itFourCCPar != FourCCPar.end());

        invalid += CheckOrZero(par.mfx.FrameInfo.ChromaFormat,   mfxU16(itFourCCPar->second[0]));
        invalid += CheckOrZero(par.mfx.FrameInfo.BitDepthLuma,   mfxU16(itFourCCPar->second[1]), 0);
        invalid += CheckOrZero(par.mfx.FrameInfo.BitDepthChroma, mfxU16(itFourCCPar->second[1]), 0);

        MFX_CHECK(!invalid, MFX_ERR_UNSUPPORTED);
        return MFX_ERR_NONE;
    }

    static mfxStatus TargetChromaFormat(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& dpar
        , mfxVideoParam& par)
    {
        mfxU32 invalid = 0;
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

        MFX_CHECK(pCO3, MFX_ERR_NONE);

        // 422 target is not supported on VDENC
        invalid +=
            IsOn(par.mfx.LowPower)
            && pCO3->TargetChromaFormatPlus1 == (1 + MFX_CHROMAFORMAT_YUV422);

        // 444 target is not POR on VME
        invalid +=
            !IsOn(par.mfx.LowPower)
            && pCO3->TargetChromaFormatPlus1 == (1 + MFX_CHROMAFORMAT_YUV444);

        static const mfxU16 cfTbl[] =
        {
            0
            , 1 + MFX_CHROMAFORMAT_YUV420
            , 1 + MFX_CHROMAFORMAT_YUV422
            , 1 + MFX_CHROMAFORMAT_YUV444
        };

        mfxU16 maxChroma = dpar.base.GetMaxChroma(dpar) + 1;
        maxChroma *= (maxChroma <= Size(cfTbl));
        SetDefault(maxChroma, mfxU16(MFX_CHROMAFORMAT_YUV420 + 1));

        invalid += !std::count(cfTbl, cfTbl + maxChroma, pCO3->TargetChromaFormatPlus1);
        invalid += (pCO3->TargetChromaFormatPlus1 == (1 + MFX_CHROMAFORMAT_YUV444) && !dpar.caps.YUV444ReconSupport);
        invalid += (pCO3->TargetChromaFormatPlus1 == (1 + MFX_CHROMAFORMAT_YUV422) && !dpar.caps.YUV422ReconSupport);

        pCO3->TargetChromaFormatPlus1 *= !invalid;

        MFX_CHECK(!invalid, MFX_ERR_UNSUPPORTED);
        return MFX_ERR_NONE;
    }

    static mfxStatus TargetBitDepth(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& dpar
        , mfxVideoParam& par)
    {
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

        MFX_CHECK(pCO3, MFX_ERR_NONE);

        mfxU32 invalid = 0;
        auto CheckBD   = [&](bool bMax10)
        {
            invalid += bMax10 && CheckOrZero<mfxU16, 10, 8, 0>(pCO3->TargetBitDepthLuma);
            invalid += bMax10 && CheckOrZero<mfxU16, 10, 8, 0>(pCO3->TargetBitDepthChroma);
            invalid += !bMax10 && CheckOrZero<mfxU16, 8, 0>(pCO3->TargetBitDepthLuma);
            invalid += !bMax10 && CheckOrZero<mfxU16, 8, 0>(pCO3->TargetBitDepthChroma);
        };

        CheckBD(dpar.base.GetMaxBitDepth(dpar) == 10);
        CheckBD(pCO3->TargetBitDepthLuma == 10);

        MFX_CHECK(!invalid, MFX_ERR_UNSUPPORTED);

        auto& fi = par.mfx.FrameInfo;
        mfxU32 changed = 0;
        changed += fi.BitDepthLuma && pCO3->TargetBitDepthLuma && !IsOn(par.mfx.LowPower) &&
            SetIf(pCO3->TargetBitDepthLuma, pCO3->TargetBitDepthLuma != fi.BitDepthLuma, 0);
        changed += fi.BitDepthChroma && pCO3->TargetBitDepthChroma && !IsOn(par.mfx.LowPower) &&
            SetIf(pCO3->TargetBitDepthChroma,  pCO3->TargetBitDepthChroma != fi.BitDepthChroma, 0);

        MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);

        return MFX_ERR_NONE;
    }

    static mfxStatus FourCCByTargetFormat(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& dpar
        , mfxVideoParam& par)
    {
        auto tbdl = dpar.base.GetTargetBitDepthLuma(dpar);
        auto tcf = dpar.base.GetTargetChromaFormat(dpar);

        static const std::map<mfxU16, std::set<mfxU32>> Compatible[2] =
        {
            //8
            {
                {
                    mfxU16(1 + MFX_CHROMAFORMAT_YUV444)
                    , {MFX_FOURCC_AYUV, MFX_FOURCC_Y410, MFX_FOURCC_RGB4, MFX_FOURCC_A2RGB10}
                }
                ,{
                    mfxU16(1 + MFX_CHROMAFORMAT_YUV422)
                    , {MFX_FOURCC_YUY2, MFX_FOURCC_Y210, MFX_FOURCC_P210
                    , MFX_FOURCC_AYUV, MFX_FOURCC_Y410
                    , MFX_FOURCC_RGB4, MFX_FOURCC_A2RGB10}
                }
                ,{
                    mfxU16(1 + MFX_CHROMAFORMAT_YUV420)
                    , {MFX_FOURCC_NV12, MFX_FOURCC_P010, MFX_FOURCC_RGB4
                    , MFX_FOURCC_YUY2, MFX_FOURCC_P210, MFX_FOURCC_Y210
                    , MFX_FOURCC_AYUV, MFX_FOURCC_Y410, MFX_FOURCC_A2RGB10}
                }
            },
            //10
            {
                {
                    mfxU16(1 + MFX_CHROMAFORMAT_YUV444)
                    , {MFX_FOURCC_Y410, MFX_FOURCC_A2RGB10}
                }
                ,{
                    mfxU16(1 + MFX_CHROMAFORMAT_YUV422)
                    , {MFX_FOURCC_P210, MFX_FOURCC_Y210, MFX_FOURCC_Y410, MFX_FOURCC_A2RGB10}
                }
                ,{
                    mfxU16(1 + MFX_CHROMAFORMAT_YUV420)
                    , {MFX_FOURCC_P010, MFX_FOURCC_P210, MFX_FOURCC_Y210, MFX_FOURCC_Y410, MFX_FOURCC_A2RGB10}
                }
            },
        };

        bool bUndefinedTargetFormat =
            (tbdl != 8 && tbdl != 10)
            || !Compatible[tbdl == 10].count(tcf)
            || !Compatible[tbdl == 10].at(tcf).count(par.mfx.FrameInfo.FourCC);

        assert(!bUndefinedTargetFormat);

        par.mfx.FrameInfo.FourCC *= !bUndefinedTargetFormat;

        MFX_CHECK(!bUndefinedTargetFormat, MFX_ERR_UNSUPPORTED);
        return MFX_ERR_NONE;
    }

    static mfxStatus WinBRC(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& dpar
        , mfxVideoParam& par)
    {
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);
        MFX_CHECK(pCO3 && (pCO3->WinBRCSize || pCO3->WinBRCMaxAvgKbps), MFX_ERR_NONE);

        mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);
        bool   bExtBRC      = pCO2 && IsOn(pCO2->ExtBRC);
        bool   bVBR         = (par.mfx.RateControlMethod == MFX_RATECONTROL_VBR || par.mfx.RateControlMethod == MFX_RATECONTROL_QVBR);
        bool   bUnsupported = bVBR && pCO3->WinBRCMaxAvgKbps && (pCO3->WinBRCMaxAvgKbps < TargetKbps(par.mfx));
        mfxU32 changed      = !bVBR || bUnsupported;

        pCO3->WinBRCSize       *= !changed;
        pCO3->WinBRCMaxAvgKbps *= !changed;

        MFX_CHECK(!bUnsupported, MFX_ERR_UNSUPPORTED);
        MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(bVBR && !bExtBRC, MFX_ERR_NONE);

        auto   fr      = dpar.base.GetFrameRate(dpar);
        mfxU16 fps     = mfxU16(CeilDiv(std::get<0>(fr), std::get<1>(fr)));
        auto   maxKbps = dpar.base.GetMaxKbps(dpar);

        changed += pCO3->WinBRCSize && SetIf(pCO3->WinBRCSize, pCO3->WinBRCSize != fps, fps);
        changed += SetIf(pCO3->WinBRCMaxAvgKbps, pCO3->WinBRCMaxAvgKbps != maxKbps, maxKbps);

        MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        return MFX_ERR_NONE;
    }

    static mfxStatus SAO(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& defPar
        , mfxVideoParam& par)
    {
        mfxExtHEVCParam* pHEVC = ExtBuffer::Get(par);
        MFX_CHECK(pHEVC, MFX_ERR_NONE);

        mfxU32 changed = 0;
        mfxExtCodingOption2* pCO2 = ExtBuffer::Get(par);
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

        /* On Gen10 VDEnc, this flag should be set to 0 when SliceSizeControl flag is on.
        On Gen10/CNL both VDEnc and VME, this flag should be set to 0 for 10-bit encoding.
        On CNL+, this flag should be set to 0 for max LCU size is 16x16 */
        bool bSAOSupported =
            !(     defPar.hw < MFX_HW_CNL
                || (par.mfx.TargetUsage == MFX_TARGETUSAGE_BEST_SPEED && defPar.hw == MFX_HW_CNL)
                || (pCO3 && pCO3->WeightedPred == MFX_WEIGHTED_PRED_EXPLICIT)
                || (pCO3 && pCO3->WeightedBiPred == MFX_WEIGHTED_PRED_EXPLICIT)
                || defPar.base.GetLCUSize(defPar) == 16
                || (defPar.hw == MFX_HW_CNL
                    && (   (defPar.base.GetTargetBitDepthLuma(defPar) == 10)
                        || (IsOn(par.mfx.LowPower) && pCO2 && pCO2->MaxSliceSize)
                       )
                   )
            );

        //On Gen10 VDEnc SAO for only Luma/Chroma in CQP mode isn't supported by driver until real customer usage
        //For TU 1 and TU 4 SAO isn't supported due to HuC restrictions, for TU 7 SAO isn't supported at all
        bSAOSupported &= !(
            defPar.hw == MFX_HW_CNL
            && par.mfx.RateControlMethod == MFX_RATECONTROL_CQP
            && IsOn(par.mfx.LowPower));

        changed += CheckOrZero<mfxU16>
            (pHEVC->SampleAdaptiveOffset
            , mfxU16(MFX_SAO_UNKNOWN)
            , mfxU16(MFX_SAO_DISABLE)
            , mfxU16(bSAOSupported * MFX_SAO_ENABLE_LUMA)
            , mfxU16(bSAOSupported * MFX_SAO_ENABLE_CHROMA)
            , mfxU16(bSAOSupported * (MFX_SAO_ENABLE_LUMA | MFX_SAO_ENABLE_CHROMA)));

        MFX_CHECK(!changed, MFX_ERR_UNSUPPORTED);
        return MFX_ERR_NONE;
    }

    static mfxStatus NumRefActive(
        Defaults::TCheckAndFix::TExt
        , const Defaults::Param& defPar
        , mfxVideoParam& par)
    {
        mfxU32 changed = 0;
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

        MFX_CHECK(pCO3, MFX_ERR_NONE);

        mfxU16 maxDPB = par.mfx.NumRefFrame + 1;
        SetIf(maxDPB, !par.mfx.NumRefFrame, defPar.base.GetMaxDPB, defPar);

        mfxU16 maxP   = std::min<mfxU16>(defPar.caps.MaxNum_Reference0, maxDPB - 1);
        mfxU16 maxBL0 = std::min<mfxU16>(defPar.caps.MaxNum_Reference0, maxDPB - 1);
        mfxU16 maxBL1 = std::min<mfxU16>(defPar.caps.MaxNum_Reference1, maxDPB - 1);

        if (defPar.hw >= MFX_HW_CNL)
        {
            auto maxRefByTu = defPar.base.GetMaxNumRef(defPar);
            maxP   = std::min<mfxU16>(maxP,   std::get<P>(maxRefByTu));
            maxBL0 = std::min<mfxU16>(maxBL0, std::get<BL0>(maxRefByTu));
            maxBL1 = std::min<mfxU16>(maxBL1, std::get<BL1>(maxRefByTu));
        }

        for (mfxU16 i = 0; i < 8; i++)
        {
            changed += CheckMaxOrClip(pCO3->NumRefActiveP  [i], maxP);
            changed += CheckMaxOrClip(pCO3->NumRefActiveBL0[i], maxBL0);
            changed += CheckMaxOrClip(pCO3->NumRefActiveBL1[i], maxBL1);
        }

        MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        return MFX_ERR_NONE;
    }

    static void Push(Defaults& df)
    {
#define PUSH_DEFAULT(X) df.Check##X.Push(X);

        PUSH_DEFAULT(LCUSize);
        PUSH_DEFAULT(LowDelayBRC);
        PUSH_DEFAULT(Level);
        PUSH_DEFAULT(SurfSize);
        PUSH_DEFAULT(Profile);
        PUSH_DEFAULT(FourCC);
        PUSH_DEFAULT(InputFormatByFourCC);
        PUSH_DEFAULT(TargetChromaFormat);
        PUSH_DEFAULT(TargetBitDepth);
        PUSH_DEFAULT(FourCCByTargetFormat);
        PUSH_DEFAULT(WinBRC);
        PUSH_DEFAULT(SAO);
        PUSH_DEFAULT(NumRefActive);

#undef PUSH_DEFAULT
    }

};

const std::map<mfxU32, std::array<mfxU32, 3>> CheckAndFix::FourCCPar =
{
    {mfxU32(MFX_FOURCC_AYUV),       {mfxU32(MFX_CHROMAFORMAT_YUV444), 8,  mfxU32(MFX_HW_ICL) }}
    , {mfxU32(MFX_FOURCC_RGB4),     {mfxU32(MFX_CHROMAFORMAT_YUV444), 8,  mfxU32(MFX_HW_SCL) }}
    , {mfxU32(MFX_FOURCC_A2RGB10),  {mfxU32(MFX_CHROMAFORMAT_YUV444), 10, mfxU32(MFX_HW_ICL) }}
    , {mfxU32(MFX_FOURCC_Y410),     {mfxU32(MFX_CHROMAFORMAT_YUV444), 10, mfxU32(MFX_HW_ICL) }}
    , {mfxU32(MFX_FOURCC_P210),     {mfxU32(MFX_CHROMAFORMAT_YUV422), 10, mfxU32(MFX_HW_ICL) }}
    , {mfxU32(MFX_FOURCC_Y210),     {mfxU32(MFX_CHROMAFORMAT_YUV422), 10, mfxU32(MFX_HW_ICL) }}
    , {mfxU32(MFX_FOURCC_YUY2),     {mfxU32(MFX_CHROMAFORMAT_YUV422), 8,  mfxU32(MFX_HW_ICL) }}
    , {mfxU32(MFX_FOURCC_P010),     {mfxU32(MFX_CHROMAFORMAT_YUV420), 10, mfxU32(MFX_HW_KBL) }}
    , {mfxU32(MFX_FOURCC_NV12),     {mfxU32(MFX_CHROMAFORMAT_YUV420), 8,  mfxU32(MFX_HW_SCL) }}
};

void Legacy::PushDefaults(Defaults& df)
{
    GetDefault::Push(df);
    PreCheck::Push(df);
    CheckAndFix::Push(df);
}


#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
