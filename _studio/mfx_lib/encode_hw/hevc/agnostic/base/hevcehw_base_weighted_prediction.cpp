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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)

#include "hevcehw_base_weighted_prediction.h"
#include <numeric>

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

void WeightPred::SetSupported(ParamSupport& blocks)
{
    blocks.m_ebCopySupported[MFX_EXTBUFF_CODING_OPTION3].emplace_back(
        [](const mfxExtBuffer* pSrc, mfxExtBuffer* pDst) -> void
    {
        auto& buf_src = *(const mfxExtCodingOption3*)pSrc;
        auto& buf_dst = *(mfxExtCodingOption3*)pDst;

        MFX_COPY_FIELD(WeightedPred);
        MFX_COPY_FIELD(WeightedBiPred);
        MFX_COPY_FIELD(FadeDetection);
    });
}

void WeightPred::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckAndFix
        , [](const mfxVideoParam& /*in*/, mfxVideoParam& par, StorageW& global) -> mfxStatus
    {
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);
        MFX_CHECK(pCO3, MFX_ERR_NONE);

        auto&  caps         = Glob::EncodeCaps::Get(global);
        auto   hw           = Glob::VideoCore::Get(global).GetHWType();
        bool   bFDSupported = !caps.NoWeightedPred && (hw >= MFX_HW_ICL);
        mfxU32 changed      = 0;

        changed += CheckOrZero<mfxU16>(
            pCO3->WeightedPred
            , mfxU16(MFX_WEIGHTED_PRED_UNKNOWN)
            , mfxU16(MFX_WEIGHTED_PRED_DEFAULT)
            , mfxU16(!caps.NoWeightedPred * MFX_WEIGHTED_PRED_EXPLICIT));

        changed += CheckOrZero<mfxU16>(
            pCO3->WeightedBiPred
            , mfxU16(MFX_WEIGHTED_PRED_UNKNOWN)
            , mfxU16(MFX_WEIGHTED_PRED_DEFAULT)
            , mfxU16(!caps.NoWeightedPred * MFX_WEIGHTED_PRED_EXPLICIT));

        changed += CheckOrZero<mfxU16>(
            pCO3->FadeDetection
            , mfxU16(MFX_CODINGOPTION_UNKNOWN)
            , mfxU16(MFX_CODINGOPTION_OFF)
            , mfxU16(bFDSupported * MFX_CODINGOPTION_ON));

        MFX_CHECK(!changed, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);

        return MFX_ERR_NONE;
    });
}

void WeightPred::SetDefaults(const FeatureBlocks& /*blocks*/, TPushSD Push)
{
    Push(BLK_SetDefaults
        , [](mfxVideoParam& par, StorageW& /*strg*/, StorageRW&)
    {
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(par);
        MFX_CHECK(pCO3, MFX_ERR_NONE);

        SetDefault(pCO3->FadeDetection, MFX_CODINGOPTION_OFF);
        SetDefault(pCO3->WeightedPred
            , IsOn(pCO3->FadeDetection) * MFX_WEIGHTED_PRED_EXPLICIT
            + IsOff(pCO3->FadeDetection) * MFX_WEIGHTED_PRED_DEFAULT);
        SetDefault(pCO3->WeightedBiPred, pCO3->WeightedPred);

        return MFX_ERR_NONE;
    });
}

void WeightPred::InitInternal(const FeatureBlocks& /*blocks*/, TPushII Push)
{
    Push(BLK_SetPPS
        , [](StorageRW& strg, StorageRW&) -> mfxStatus
    {
        auto& par = Glob::VideoParam::Get(strg);
        const mfxExtCodingOption3& CO3 = ExtBuffer::Get(par);

        PPS& pps = Glob::PPS::Get(strg);
        pps.weighted_pred_flag   = (CO3.WeightedPred   == MFX_WEIGHTED_PRED_EXPLICIT);
        pps.weighted_bipred_flag = (CO3.WeightedBiPred == MFX_WEIGHTED_PRED_EXPLICIT) || (IsOn(CO3.GPB) && pps.weighted_pred_flag);

        MFX_CHECK(strg.Contains(Glob::RealState::Key), MFX_ERR_NONE);

        const PPS& oldPPS = Glob::PPS::Get(Glob::RealState::Get(strg));
        Glob::ResetHint::Get(strg).Flags |=
            RF_PPS_CHANGED
            * (    (pps.weighted_pred_flag != oldPPS.weighted_pred_flag)
                || (pps.weighted_bipred_flag != oldPPS.weighted_bipred_flag));

        return MFX_ERR_NONE;
    });
}

void WeightPred::PostReorderTask(const FeatureBlocks& /*blocks*/, TPushPostRT Push)
{
    Push(BLK_SetSSH
        , [](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& pps = Glob::PPS::Get(global);
        auto& ssh = Task::SSH::Get(s_task);

        bool bNeedPWT =
            (pps.weighted_pred_flag && ssh.type == 1)
            || (pps.weighted_bipred_flag && ssh.type == 0);

        MFX_CHECK(bNeedPWT, MFX_ERR_NONE);

        const mfxU16 Y = 0, Cb = 1, Cr = 2, W = 0, O = 1;
        auto& task             = Task::Common::Get(s_task);
        auto& caps             = Glob::EncodeCaps::Get(global);
        auto  pExtPWT          = (mfxExtPredWeightTable*)ExtBuffer::Get(task.ctrl);
        auto  SetDefaultWeight = [&](mfxI16 (&pwt)[3][2])
        {
            pwt[Y][W]  = (1 << ssh.luma_log2_weight_denom);
            pwt[Y][O]  = 0;
            pwt[Cb][W] = (1 << ssh.chroma_log2_weight_denom);
            pwt[Cb][O] = 0;
            pwt[Cr][W] = (1 << ssh.chroma_log2_weight_denom);
            pwt[Cr][O] = 0;
        };
        auto  CopyLxPWT        = [&](mfxU16 l, mfxU16 sz)
        {
            auto CopyY = [&](mfxU16 i)
            {
                ssh.pwt[l][i][Y][W] = pExtPWT->Weights[l][i][Y][W];
                ssh.pwt[l][i][Y][O] = pExtPWT->Weights[l][i][Y][O];
            };
            auto CopyC = [&](mfxU16 i)
            {
                ssh.pwt[l][i][Cb][W] = pExtPWT->Weights[l][i][Cb][W];
                ssh.pwt[l][i][Cb][O] = pExtPWT->Weights[l][i][Cb][O];
                ssh.pwt[l][i][Cr][W] = pExtPWT->Weights[l][i][Cr][W];
                ssh.pwt[l][i][Cr][O] = pExtPWT->Weights[l][i][Cr][O];
            };

            std::list<mfxI16> idxY(sz * caps.LumaWeightedPred, 0);
            std::list<mfxI16> idxC(sz * caps.ChromaWeightedPred, 0);

            std::iota(idxY.begin(), idxY.end(), mfxI16(0));
            std::iota(idxC.begin(), idxC.end(), mfxI16(0));

            idxY.remove_if([&](mfxI16 i) { return !pExtPWT->LumaWeightFlag[l][i]; });
            idxC.remove_if([&](mfxI16 i) { return !pExtPWT->ChromaWeightFlag[l][i]; });

            for_each(idxY.begin(), idxY.end(), CopyY);
            for_each(idxC.begin(), idxC.end(), CopyC);
        };

        ssh.luma_log2_weight_denom   = 6;
        ssh.chroma_log2_weight_denom = ssh.luma_log2_weight_denom;

        if (pExtPWT)
        {
            ssh.luma_log2_weight_denom   = pExtPWT->LumaLog2WeightDenom;
            ssh.chroma_log2_weight_denom = pExtPWT->ChromaLog2WeightDenom;
        }

        std::for_each(std::begin(ssh.pwt[0]), std::end(ssh.pwt[0]), SetDefaultWeight);
        std::for_each(std::begin(ssh.pwt[1]), std::end(ssh.pwt[1]), SetDefaultWeight);

        MFX_CHECK(pExtPWT, MFX_ERR_NONE);

        CopyLxPWT(0, std::min<mfxU16>(ssh.num_ref_idx_l0_active_minus1 + 1, caps.MaxNum_WeightedPredL0));
        CopyLxPWT(1, std::min<mfxU16>(ssh.num_ref_idx_l1_active_minus1 + 1, caps.MaxNum_WeightedPredL1));

        bool bForceSameWeights =
            task.isLDB
            && std::equal(
                std::begin(task.RefPicList[0])
                , std::end(task.RefPicList[0])
                , std::begin(task.RefPicList[1]));

        MFX_CHECK(bForceSameWeights, MFX_ERR_NONE);

        std::copy_n((mfxU8*)ssh.pwt[0], sizeof(ssh.pwt[0]), (mfxU8*)ssh.pwt[1]);


        return MFX_ERR_NONE;
    });
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
