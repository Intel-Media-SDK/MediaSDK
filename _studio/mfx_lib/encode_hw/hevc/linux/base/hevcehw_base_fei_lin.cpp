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

#if defined(MFX_ENABLE_H265_VIDEO_ENCODE) && defined (MFX_VA_LINUX)
#include "mfxfeihevc.h"
#include "hevcehw_base_fei_lin.h"
#include "hevcehw_base_va_lin.h"
#include "hevcehw_base_va_packer_lin.h"

using namespace HEVCEHW;
using namespace HEVCEHW::Base;
using namespace HEVCEHW::Linux;
using namespace HEVCEHW::Linux::Base;

void FEI::Query1NoCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_SetGuidMap
        , [](const mfxVideoParam&, mfxVideoParam& /*out*/, StorageRW& strg) -> mfxStatus
    {
        auto& gmap = Glob::GuidToVa::GetOrConstruct(strg);
        gmap[DXVA2_Intel_Encode_HEVC_Main]       = VAGUID{ VAProfileHEVCMain, VAEntrypointFEI };
        gmap[DXVA2_Intel_Encode_HEVC_Main10]     = VAGUID{ VAProfileHEVCMain10, VAEntrypointFEI };
#if VA_CHECK_VERSION(1,2,0)
        gmap[DXVA2_Intel_Encode_HEVC_Main422]    = VAGUID{ VAProfileHEVCMain422_10, VAEntrypointFEI };
        gmap[DXVA2_Intel_Encode_HEVC_Main422_10] = VAGUID{ VAProfileHEVCMain422_10, VAEntrypointFEI };
        gmap[DXVA2_Intel_Encode_HEVC_Main444]    = VAGUID{ VAProfileHEVCMain444,    VAEntrypointFEI };
        gmap[DXVA2_Intel_Encode_HEVC_Main444_10] = VAGUID{ VAProfileHEVCMain444_10, VAEntrypointFEI };
#endif
        return MFX_ERR_NONE;
    });

    Push(BLK_SetVaCallChain
        , [](const mfxVideoParam&, mfxVideoParam& /*out*/, StorageRW& strg) -> mfxStatus
    {
        auto& ddiExec = Glob::DDI_Execute::GetOrConstruct(strg);

        ddiExec.Push([&](Glob::DDI_Execute::TRef::TExt prev, const DDIExecParam& ep)
        {
            MFX_CHECK(ep.Function == DDI_VA::VAFID_GetConfigAttributes, prev(ep));

            using TArgs = TupleArgs<decltype(vaGetConfigAttributes)>::type;
            ThrowAssert(!ep.In.pData || ep.In.Size != sizeof(TArgs), "Invalid arguments for VAFID_GetConfigAttributes");

            TArgs& args         = *(TArgs*)ep.In.pData;
            auto&  pAttr        = std::get<3>(args);
            auto&  nAttr        = std::get<4>(args);
            auto   pAttrInitial = pAttr;
            auto   nAttrInitial = nAttr;

            std::vector<VAConfigAttrib> vAttr(pAttr, pAttr + nAttr);
            vAttr.emplace_back(VAConfigAttrib{ VAConfigAttribFEIFunctionType, 0 });

            pAttr = vAttr.data();
            ++nAttr;

            auto sts = prev(ep);
            MFX_CHECK_STS(sts);
            MFX_CHECK(vAttr.back().value & VA_FEI_FUNCTION_ENC_PAK, MFX_ERR_DEVICE_FAILED);

            std::copy_n(pAttr, nAttrInitial, pAttrInitial);

            return sts;
        });

        return MFX_ERR_NONE;
    });
}

void FEI::Query1WithCaps(const FeatureBlocks& /*blocks*/, TPushQ1 Push)
{
    Push(BLK_CheckAndFix
        , [](const mfxVideoParam&, mfxVideoParam& out, StorageW& /*strg*/) -> mfxStatus
    {
        mfxExtCodingOption3* pCO3 = ExtBuffer::Get(out);
        MFX_CHECK(pCO3, MFX_ERR_NONE);

        // HEVC FEI Encoder uses own controls to switch on LCU QP buffer
        MFX_CHECK(!CheckOrZero(pCO3->EnableMBQP, mfxU16(MFX_CODINGOPTION_OFF), mfxU16(MFX_CODINGOPTION_UNKNOWN))
            , MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);

        return MFX_ERR_NONE;
    });
}

void FEI::Reset(const FeatureBlocks& /*blocks*/, TPushR Push)
{
    Push(BLK_CancelTasks
        , [](
            const mfxVideoParam& /*par*/
            , StorageRW& global
            , StorageRW& /*local*/) -> mfxStatus
    {
        Glob::ResetHint::Get(global).Flags |= RF_CANCEL_TASKS;
        return MFX_ERR_NONE;
    });
}

void FEI::FrameSubmit(const FeatureBlocks& /*blocks*/, TPushFS Push)
{
    Push(BLK_FrameCheck
        , [this](
            const mfxEncodeCtrl* pCtrl
            , const mfxFrameSurface1* pSurf
            , mfxBitstream& /*bs*/
            , StorageW& global
            , StorageRW& /*local*/) -> mfxStatus
    {

        MFX_CHECK(pSurf, MFX_ERR_NONE);// In case of frames draining in display order
        MFX_CHECK(pCtrl, MFX_ERR_INVALID_VIDEO_PARAM);

        eMFXHWType platform = Glob::VideoCore::Get(global).GetHWType();

        bool isSKL = platform == MFX_HW_SCL
            , isICLplus = platform >= MFX_HW_ICL;

        // mfxExtFeiHevcEncFrameCtrl is a mandatory buffer
        mfxExtFeiHevcEncFrameCtrl* EncFrameCtrl =
            reinterpret_cast<mfxExtFeiHevcEncFrameCtrl*>(ExtBuffer::Get(*pCtrl, MFX_EXTBUFF_HEVCFEI_ENC_CTRL));
        MFX_CHECK(EncFrameCtrl, MFX_ERR_INVALID_VIDEO_PARAM);

        // Check HW limitations for mfxExtFeiHevcEncFrameCtrl parameters
        MFX_CHECK(EncFrameCtrl->NumMvPredictors[0] <= 4, MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(EncFrameCtrl->NumMvPredictors[1] <= 4, MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(EncFrameCtrl->MultiPred[0] <= (isICLplus + 1), MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(EncFrameCtrl->MultiPred[1] <= (isICLplus + 1), MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(EncFrameCtrl->SubPelMode <= 3 && EncFrameCtrl->SubPelMode != 2, MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(EncFrameCtrl->MVPredictor == 0
            || EncFrameCtrl->MVPredictor == 1
            || EncFrameCtrl->MVPredictor == 2
            || (EncFrameCtrl->MVPredictor == 3 && isICLplus)
            || EncFrameCtrl->MVPredictor == 7, MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(EncFrameCtrl->ForceCtuSplit <= mfxU16(isSKL), MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(EncFrameCtrl->FastIntraMode <= mfxU16(isSKL), MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(!Check<mfxU16>(EncFrameCtrl->NumFramePartitions, (mfxU16)1, (mfxU16)2, (mfxU16)4, (mfxU16)8, (mfxU16)16), MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(EncFrameCtrl->AdaptiveSearch <= 1, MFX_ERR_INVALID_VIDEO_PARAM);

        if (EncFrameCtrl->SearchWindow)
        {
            MFX_CHECK(EncFrameCtrl->LenSP == 0
                && EncFrameCtrl->SearchPath == 0
                && EncFrameCtrl->RefWidth == 0
                && EncFrameCtrl->RefHeight == 0, MFX_ERR_INVALID_VIDEO_PARAM);

            MFX_CHECK(EncFrameCtrl->SearchWindow <= 5, MFX_ERR_INVALID_VIDEO_PARAM);
        }
        else
        {
            MFX_CHECK(EncFrameCtrl->SearchPath <= 2, MFX_ERR_INVALID_VIDEO_PARAM);
            MFX_CHECK(EncFrameCtrl->LenSP >= 1 && EncFrameCtrl->LenSP <= 63, MFX_ERR_INVALID_VIDEO_PARAM);
            MFX_CHECK(!EncFrameCtrl->AdaptiveSearch || EncFrameCtrl->LenSP >= 2, MFX_ERR_INVALID_VIDEO_PARAM);

            if (isSKL)
            {
                Defaults::Param ccpar(
                        Glob::VideoParam::Get(global)
                        , Glob::EncodeCaps::Get(global)
                        , platform
                        , Glob::Defaults::Get(global));
                FrameBaseInfo fi;

                auto sts = ccpar.base.GetPreReorderInfo(ccpar, fi, pSurf, pCtrl, m_lastIDR, m_prevIPoc, m_frameOrder);
                MFX_CHECK_STS(sts);

                SetIf(m_frameOrder, !!ccpar.mvp.mfx.EncodedOrder, pSurf->Data.FrameOrder);
                SetIf(m_lastIDR, IsIdr(fi.FrameType), m_frameOrder);
                SetIf(m_prevIPoc, IsI(fi.FrameType), fi.POC);

                ++m_frameOrder;

                MFX_CHECK(EncFrameCtrl->RefWidth % 4 == 0
                    && EncFrameCtrl->RefHeight % 4 == 0
                    && EncFrameCtrl->RefWidth  <= (32 * (2 - IsB(fi.FrameType)))
                    && EncFrameCtrl->RefHeight <= (32 * (2 - IsB(fi.FrameType)))
                    && EncFrameCtrl->RefWidth  >= 20
                    && EncFrameCtrl->RefHeight >= 20
                    && EncFrameCtrl->RefWidth * EncFrameCtrl->RefHeight <= 2048,
                    // For B frames actual limit is RefWidth*RefHeight <= 1024.
                    // Is is already guranteed by limit of 32 pxls for RefWidth and RefHeight in case of B frame
                    MFX_ERR_INVALID_VIDEO_PARAM);
            }
            else
            {
                // ICL+
                MFX_CHECK((EncFrameCtrl->RefWidth == 64 && EncFrameCtrl->RefHeight == 64)
                    || (EncFrameCtrl->RefWidth == 48 && EncFrameCtrl->RefHeight == 40),
                    MFX_ERR_INVALID_VIDEO_PARAM);
            }
        }

        // Check if requested buffers are provided
        MFX_CHECK(
            !EncFrameCtrl->MVPredictor
            || ExtBuffer::Get(*pCtrl, MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED)
            , MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(!EncFrameCtrl->PerCuQp
            || ExtBuffer::Get(*pCtrl, MFX_EXTBUFF_HEVCFEI_ENC_QP)
            , MFX_ERR_INVALID_VIDEO_PARAM);

        MFX_CHECK(!EncFrameCtrl->PerCtuInput
            || ExtBuffer::Get(*pCtrl, MFX_EXTBUFF_HEVCFEI_ENC_CTU_CTRL)
            , MFX_ERR_INVALID_VIDEO_PARAM);

        // Check for mfxExtFeiHevcRepackCtrl
        mfxExtFeiHevcRepackCtrl* repackctrl =
            reinterpret_cast<mfxExtFeiHevcRepackCtrl*>(ExtBuffer::Get(*pCtrl, MFX_EXTBUFF_HEVCFEI_REPACK_CTRL));

        MFX_CHECK(!repackctrl || !EncFrameCtrl->PerCuQp, MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(!repackctrl || repackctrl->NumPasses <= 8, MFX_ERR_INVALID_VIDEO_PARAM);

        return MFX_ERR_NONE;
    });
}

void FEI::PostReorderTask(const FeatureBlocks& /*blocks*/, TPushPostRT Push)
{
    Push(BLK_UpdatePPS
        , [](StorageW& global, StorageW& s_task) -> mfxStatus
    {
        auto& task = Task::Common::Get(s_task);
        auto& pps  = Glob::PPS::Get(global);

        mfxExtFeiHevcEncFrameCtrl* EncFrameCtrl =
            reinterpret_cast<mfxExtFeiHevcEncFrameCtrl*>(ExtBuffer::Get(task.ctrl, MFX_EXTBUFF_HEVCFEI_ENC_CTRL));
        mfxExtFeiHevcRepackCtrl* repackctrl =
            reinterpret_cast<mfxExtFeiHevcRepackCtrl*>(ExtBuffer::Get(task.ctrl, MFX_EXTBUFF_HEVCFEI_REPACK_CTRL));

        MFX_CHECK(EncFrameCtrl, MFX_ERR_INVALID_VIDEO_PARAM);

        bool bRepackPPS = false;

        // repackctrl or PerCuQp is disabled, so insert PPS and turn the flag off.
        bRepackPPS |= pps.cu_qp_delta_enabled_flag && !repackctrl && !EncFrameCtrl->PerCuQp;

        // repackctrl or PerCuQp is enabled, so insert PPS and turn the flag on.
        bRepackPPS |= !pps.cu_qp_delta_enabled_flag && (repackctrl || EncFrameCtrl->PerCuQp);

        MFX_CHECK(bRepackPPS, MFX_ERR_NONE);

        pps.cu_qp_delta_enabled_flag = 1 - pps.cu_qp_delta_enabled_flag;

        // If state of CTU QP buffer changes, PPS header update required
        task.RepackHeaders |= INSERT_PPS;
        task.InsertHeaders |= INSERT_PPS;

        return MFX_ERR_NONE;
    });
}

template<class TEB, class TBase>
inline mfxU32 GetVaBufferID(TBase* pBase, mfxU32 ID, bool bSearch = true)
{
    if (!pBase || !bSearch)
        return VA_INVALID_ID;

    auto pEB = reinterpret_cast<TEB*>(ExtBuffer::Get(*pBase, ID));
    if (!pEB)
        return VA_INVALID_ID;

    return pEB->VaBufferID;
}

void FEI::InitAlloc(const FeatureBlocks& /*blocks*/, TPushIA Push)
{
    Push(BLK_SetFeedbackCallChain
        , [](StorageRW& strg, StorageRW& /*local*/) -> mfxStatus
    {
        auto& cc = VAPacker::CC::Get(strg);

        cc.AddPerPicMiscData[VAEncMiscParameterTypeFEIFrameControl].Push([](
            VAPacker::CallChains::TAddMiscData::TExt
            , const StorageR& /*global*/
            , const StorageR& s_task
            , std::list<std::vector<mfxU8>>& data)
        {
            auto& task = Task::Common::Get(s_task);

            mfxExtFeiHevcEncFrameCtrl* EncFrameCtrl =
                reinterpret_cast<mfxExtFeiHevcEncFrameCtrl*>(ExtBuffer::Get(task.ctrl, MFX_EXTBUFF_HEVCFEI_ENC_CTRL));
            ThrowIf(!EncFrameCtrl, MFX_ERR_UNDEFINED_BEHAVIOR);

            auto& efctrl      = *EncFrameCtrl;
            auto& vaFrameCtrl = AddVaMisc<VAEncMiscParameterFEIFrameControlHEVC>(VAEncMiscParameterTypeFEIFrameControl, data);

            vaFrameCtrl.function                           = VA_FEI_FUNCTION_ENC_PAK;
            vaFrameCtrl.search_path                        = efctrl.SearchPath;
            vaFrameCtrl.len_sp                             = efctrl.LenSP;
            vaFrameCtrl.ref_width                          = efctrl.RefWidth;
            vaFrameCtrl.ref_height                         = efctrl.RefHeight;
            vaFrameCtrl.search_window                      = efctrl.SearchWindow;
            vaFrameCtrl.num_mv_predictors_l0               = mfxU16(!!efctrl.MVPredictor * efctrl.NumMvPredictors[0]);
            vaFrameCtrl.num_mv_predictors_l1               = mfxU16(!!efctrl.MVPredictor * efctrl.NumMvPredictors[1]);
            vaFrameCtrl.multi_pred_l0                      = efctrl.MultiPred[0];
            vaFrameCtrl.multi_pred_l1                      = efctrl.MultiPred[1];
            vaFrameCtrl.sub_pel_mode                       = efctrl.SubPelMode;
            vaFrameCtrl.adaptive_search                    = efctrl.AdaptiveSearch;
            vaFrameCtrl.mv_predictor_input                 = efctrl.MVPredictor;
            vaFrameCtrl.per_block_qp                       = efctrl.PerCuQp;
            vaFrameCtrl.per_ctb_input                      = efctrl.PerCtuInput;
            vaFrameCtrl.force_lcu_split                    = efctrl.ForceCtuSplit;
            vaFrameCtrl.num_concurrent_enc_frame_partition = efctrl.NumFramePartitions;
            vaFrameCtrl.fast_intra_mode                    = efctrl.FastIntraMode;

            // Input buffers
            vaFrameCtrl.mv_predictor =
                GetVaBufferID<mfxExtFeiHevcEncMVPredictors>(&task.ctrl, MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED, !!efctrl.MVPredictor);
            vaFrameCtrl.qp =
                GetVaBufferID<mfxExtFeiHevcEncQP>(&task.ctrl, MFX_EXTBUFF_HEVCFEI_ENC_QP, !!efctrl.PerCuQp);
            vaFrameCtrl.ctb_ctrl =
                GetVaBufferID<mfxExtFeiHevcEncCtuCtrl>(&task.ctrl, MFX_EXTBUFF_HEVCFEI_ENC_QP, !!efctrl.PerCtuInput);

            auto repackctrl = reinterpret_cast<mfxExtFeiHevcRepackCtrl*>(ExtBuffer::Get(task.ctrl, MFX_EXTBUFF_HEVCFEI_REPACK_CTRL));
            if (repackctrl)
            {
                vaFrameCtrl.max_frame_size  = repackctrl->MaxFrameSize;
                vaFrameCtrl.num_passes      = repackctrl->NumPasses;
                vaFrameCtrl.delta_qp        = repackctrl->DeltaQP;
            }

            // Output buffers
#if MFX_VERSION >= MFX_VERSION_NEXT
            vaFrameCtrl.ctb_cmd    = GetVaBufferID<mfxExtFeiHevcPakCtuRecordV0>(task.pBsOut, MFX_EXTBUFF_HEVCFEI_PAK_CTU_REC);
            vaFrameCtrl.cu_record  = GetVaBufferID<mfxExtFeiHevcPakCuRecordV0>(task.pBsOut, MFX_EXTBUFF_HEVCFEI_PAK_CU_REC);
            vaFrameCtrl.distortion = GetVaBufferID<mfxExtFeiHevcDistortion>(task.pBsOut, MFX_EXTBUFF_HEVCFEI_ENC_DIST);
#else
            vaFrameCtrl.ctb_cmd    = VA_INVALID_ID;
            vaFrameCtrl.cu_record  = VA_INVALID_ID;
            vaFrameCtrl.distortion = VA_INVALID_ID;
#endif

            return true;
        });

        cc.ReadFeedback.Push([](
            VAPacker::CallChains::TReadFeedback::TExt prev
            , const StorageR& global
            , StorageW& s_task
            , const VACodedBufferSegment& fb)
        {
            auto& task = Task::Common::Get(s_task);

            mfxExtFeiHevcRepackStat *repackStat = reinterpret_cast<mfxExtFeiHevcRepackStat *>
                (ExtBuffer::Get(task.ctrl, MFX_EXTBUFF_HEVCFEI_REPACK_STAT));

            if (repackStat)
            {
                repackStat->NumPasses = 0;

                mfxExtFeiHevcRepackCtrl* repackctrl = reinterpret_cast<mfxExtFeiHevcRepackCtrl*>
                    (ExtBuffer::Get(task.ctrl, MFX_EXTBUFF_HEVCFEI_REPACK_CTRL));

                if (repackctrl)
                {
                    mfxU8 QP         = fb.status & VA_CODED_BUF_STATUS_PICTURE_AVE_QP_MASK;
                    mfxU8 QPOption   = task.QpY;
                    mfxU32 numPasses = 0;

                    while ((QPOption != QP) && (numPasses < repackctrl->NumPasses))
                        QPOption += repackctrl->DeltaQP[numPasses++];

                    if (QPOption == QP)
                        repackStat->NumPasses = numPasses + 1;
                }
            }

            return prev(global, s_task, fb);
        });

        return MFX_ERR_NONE;
    });
}


#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
