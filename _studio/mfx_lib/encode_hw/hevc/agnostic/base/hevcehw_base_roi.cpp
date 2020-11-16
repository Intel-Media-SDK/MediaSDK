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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_ENABLE_HEVCE_ROI)

#include "hevcehw_base_roi.h"
#include "hevcehw_base_legacy.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

static bool ROIViaMBQP(const ENCODE_CAPS_HEVC caps, const mfxVideoParam par)
{
    return (!caps.MaxNumOfROI || !caps.ROIDeltaQPSupport) && Legacy::IsSWBRC(par, ExtBuffer::Get(par));
}

static mfxStatus CheckAndFixRect(
    ROI::RectData& rect
    , mfxVideoParam const & par
    , ENCODE_CAPS_HEVC const & caps)
{
    mfxU32 changed = 0;

    changed += CheckMaxOrClip(rect.Left, par.mfx.FrameInfo.Width);
    changed += CheckMaxOrClip(rect.Right, par.mfx.FrameInfo.Width);
    changed += CheckMaxOrClip(rect.Top, par.mfx.FrameInfo.Height);
    changed += CheckMaxOrClip(rect.Bottom, par.mfx.FrameInfo.Height);

    mfxU32 blockSize = 1 << (caps.BlockSize + 3);
    changed += AlignDown(rect.Left, blockSize);
    changed += AlignDown(rect.Top, blockSize);
    changed += AlignUp(rect.Right, blockSize);
    changed += AlignUp(rect.Bottom, blockSize);

    if(rect.Left == 0 && rect.Top == 0 && rect.Right == 0 && rect.Bottom == 0)
    {
        rect.Right  = blockSize;//expand zero rectangle to min supported size
        rect.Bottom = blockSize;
        changed++;
    }

    MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    return MFX_ERR_NONE;
}

mfxStatus ROI::CheckAndFixROI(
    ENCODE_CAPS_HEVC const & caps
    , mfxVideoParam const & par
    , mfxExtEncoderROI& roi)
{
    mfxStatus sts = MFX_ERR_NONE, rsts = MFX_ERR_NONE;
    mfxU32 changed = 0, invalid = 0;

    bool bSWBRC = Legacy::IsSWBRC(par, ExtBuffer::Get(par));
    bool bROIViaMBQP = ROIViaMBQP(caps, par);
    bool bForcedQPMode = par.mfx.RateControlMethod == MFX_RATECONTROL_CQP || bSWBRC;

    changed += CheckOrZero<mfxU16>(roi.ROIMode, MFX_ROI_MODE_PRIORITY, MFX_ROI_MODE_QP_DELTA);

    if (!bForcedQPMode)
    {
        invalid += !(roi.ROIMode != MFX_ROI_MODE_QP_DELTA || caps.ROIDeltaQPSupport);
        roi.ROIMode *= !invalid;
    }

    invalid += !(par.mfx.RateControlMethod == MFX_RATECONTROL_CBR
        || par.mfx.RateControlMethod == MFX_RATECONTROL_VBR
        || par.mfx.RateControlMethod == MFX_RATECONTROL_CQP
        || par.mfx.RateControlMethod == MFX_RATECONTROL_QVBR);

    invalid += !(caps.MaxNumOfROI || bROIViaMBQP);
    invalid += !(par.mfx.RateControlMethod != MFX_RATECONTROL_CQP || caps.ROIDeltaQPSupport);

    mfxU16 maxNumOfRoi = !invalid * std::max<mfxU16>(
        mfxU16(bROIViaMBQP * (caps.MbQpDataSupport * ROI::MAX_NUM_ROI))
        , mfxU16(!bROIViaMBQP * caps.MaxNumOfROI));

    changed += CheckMaxOrClip(roi.NumROI, maxNumOfRoi);

    mfxI16 minVal = std::min<mfxI16>(-51 * (roi.ROIMode == MFX_ROI_MODE_QP_DELTA), -3);
    mfxI16 maxVal = std::max<mfxI16>(51 * (roi.ROIMode == MFX_ROI_MODE_QP_DELTA), 3);

    std::for_each(roi.ROI, roi.ROI + roi.NumROI
        , [&](ROI::RectData& rect)
    {
        rsts = GetWorstSts(rsts, CheckAndFixRect(rect, par, caps));
        invalid += CheckMinOrZero(rect.Priority, minVal);
        invalid += CheckMaxOrZero(rect.Priority, maxVal);

        if (roi.ROIMode == MFX_ROI_MODE_PRIORITY && !caps.ROIBRCPriorityLevelSupport)
        {
            // Priority mode is not supported. Convert it to delta QP mode
            roi.ROI->DeltaQP = (mfxI16)(-1 * roi.ROI->Priority);
        }
    });

    auto IsInvalidRect = [](const ROI::RectData& rect)
    {
        return ((rect.Left >= rect.Right) || (rect.Top >= rect.Bottom));
    };
    mfxU16 numValidROI = mfxU16(std::remove_if(roi.ROI, roi.ROI + roi.NumROI, IsInvalidRect) - roi.ROI);
    changed += CheckMaxOrClip(roi.NumROI, numValidROI);

    if (roi.NumROI && roi.ROIMode == MFX_ROI_MODE_PRIORITY && !caps.ROIBRCPriorityLevelSupport)
    {
        // Priority mode is not supported. Convert it to delta QP mode
        roi.ROIMode = MFX_ROI_MODE_QP_DELTA;
        changed++;
    }

    sts = GetWorstSts(
        mfxStatus(MFX_WRN_INCOMPATIBLE_VIDEO_PARAM * !!changed)
        , mfxStatus(MFX_ERR_UNSUPPORTED * !!invalid));

    return GetWorstSts(sts, rsts);
}

void ROI::SetSupported(ParamSupport& blocks)
{
    blocks.m_ebCopySupported[MFX_EXTBUFF_ENCODER_ROI].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        auto& src = *(const mfxExtEncoderROI*)pSrc;
        auto& dst = *(mfxExtEncoderROI*)pDst;

        dst.NumROI = src.NumROI;
        dst.ROIMode = src.ROIMode;

        std::transform(src.ROI, src.ROI + Size(src.ROI), dst.ROI
            , [](const RectData& s)
        {
            RectData d = {};

            d.Left     = s.Left;
            d.Top      = s.Top;
            d.Right    = s.Right;
            d.Bottom   = s.Bottom;
            d.DeltaQP  = s.DeltaQP;

            return d;
        });
    });
}

void ROI::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckAndFix
        , [this](const mfxVideoParam& /*in*/, mfxVideoParam& par, StorageW& global) -> mfxStatus
    {
        mfxExtEncoderROI* pROI = ExtBuffer::Get(par);
        MFX_CHECK(pROI && pROI->NumROI, MFX_ERR_NONE);

        auto& caps = Glob::EncodeCaps::Get(global);
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);
        bool changed = false;

        auto sts = CheckAndFixROI(caps, par, *pROI);
        MFX_CHECK(sts >= MFX_ERR_NONE && pCO3, sts);

        changed |= CheckOrZero<mfxU16>(pCO3->EnableMBQP
            , MFX_CODINGOPTION_UNKNOWN
            , MFX_CODINGOPTION_ON
            , !ROIViaMBQP(caps, par) * MFX_CODINGOPTION_OFF);

        return GetWorstSts(sts, mfxStatus(changed * MFX_WRN_INCOMPATIBLE_VIDEO_PARAM));
    });
}

void ROI::SetDefaults(const FeatureBlocks& /*blocks*/, TPushSD Push)
{
    Push(BLK_SetDefaults
        , [this](mfxVideoParam& par, StorageW& strg, StorageRW&)
    {
        auto& defaults = Glob::Defaults::Get(strg);
        auto& bSet = defaults.SetForFeature[GetID()];
        if (!bSet)
        {
            defaults.GetPPS.Push([](
                Defaults::TGetPPS::TExt prev
                , const Defaults::Param& defPar
                , const Base::SPS& sps
                , Base::PPS& pps)
            {
                auto sts = prev(defPar, sps, pps);

                pps.cu_qp_delta_enabled_flag |= !!((const mfxExtEncoderROI&)ExtBuffer::Get(defPar.mvp)).NumROI;

                return sts;
            });

            bSet = true;
        }

        mfxExtEncoderROI* pROI = ExtBuffer::Get(par);
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);

        MFX_CHECK(pCO3 && pROI && pROI->NumROI, MFX_ERR_NONE);

        auto& caps = Glob::EncodeCaps::Get(strg);

        SetDefault(pCO3->EnableMBQP, mfxU16(ROIViaMBQP(caps, par) * MFX_CODINGOPTION_ON));

        return MFX_ERR_NONE;
    });
}

void ROI::InitInternal(const FeatureBlocks& /*blocks*/, TPushII Push)
{
    Push(BLK_SetPPS
        , [this](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        m_bViaCuQp = ROIViaMBQP(Glob::EncodeCaps::Get(strg), Glob::VideoParam::Get(strg));
        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
