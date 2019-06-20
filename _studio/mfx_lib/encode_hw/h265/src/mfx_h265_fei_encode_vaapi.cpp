// Copyright (c) 2017-2019 Intel Corporation
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

#include <unordered_map>

#include "mfx_common.h"
#if defined(MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE)

#include "mfx_h265_fei_encode_vaapi.h"

using namespace MfxHwH265Encode;

namespace MfxHwH265FeiEncode
{

static const std::unordered_map<GUID, VAParameters, GUIDhash> GUID2VAFEIParam = {
    { DXVA2_Intel_Encode_HEVC_Main,                   VAParameters(VAProfileHEVCMain,       VAEntrypointFEI)},
    { DXVA2_Intel_Encode_HEVC_Main10,                 VAParameters(VAProfileHEVCMain10,     VAEntrypointFEI)},
#if VA_CHECK_VERSION(1,2,0)
    { DXVA2_Intel_Encode_HEVC_Main422,                VAParameters(VAProfileHEVCMain422_10, VAEntrypointFEI)},
    { DXVA2_Intel_Encode_HEVC_Main422_10,             VAParameters(VAProfileHEVCMain422_10, VAEntrypointFEI)},
    { DXVA2_Intel_Encode_HEVC_Main444,                VAParameters(VAProfileHEVCMain444,    VAEntrypointFEI)},
    { DXVA2_Intel_Encode_HEVC_Main444_10,             VAParameters(VAProfileHEVCMain444_10, VAEntrypointFEI)},
#endif
};


    VAAPIh265FeiEncoder::VAAPIh265FeiEncoder()
        : VAAPIEncoder()
    {}

    VAAPIh265FeiEncoder::~VAAPIh265FeiEncoder()
    {}

    mfxStatus VAAPIh265FeiEncoder::ConfigureExtraVAattribs(std::vector<VAConfigAttrib> & attrib)
    {
        attrib.reserve(attrib.size() + 1);

        VAConfigAttrib attr = {};

        attr.type = VAConfigAttribFEIFunctionType;
        attrib.push_back(attr);

        return MFX_ERR_NONE;
    }

    mfxStatus VAAPIh265FeiEncoder::CheckExtraVAattribs(std::vector<VAConfigAttrib> & attrib)
    {
        mfxU32 i = 0;
        for (; i < attrib.size(); ++i)
        {
            if (attrib[i].type == VAConfigAttribFEIFunctionType)
                break;
        }
        MFX_CHECK(i < attrib.size(), MFX_ERR_DEVICE_FAILED);

        MFX_CHECK(attrib[i].value & VA_FEI_FUNCTION_ENC_PAK, MFX_ERR_DEVICE_FAILED);

        attrib[i].value = VA_FEI_FUNCTION_ENC_PAK;

        return MFX_ERR_NONE;
    }

    MfxHwH265Encode::VAParameters VAAPIh265FeiEncoder::GetVaParams(const GUID & guid)
    {
        auto it = GUID2VAFEIParam.find(guid);
        if (it != std::end(GUID2VAFEIParam))
            return it->second;
        else
            return {VAProfileNone, static_cast<VAEntrypoint>(0)};
    }

    mfxStatus VAAPIh265FeiEncoder::PreSubmitExtraStage(Task const & task)
    {
        {
            VAStatus vaSts;
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FrameCtrl");

            VABufferID &vaFeiFrameControlId = VABufferNew(VABID_FEI_FRM_CTRL, 0);
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaCreateBuffer");
                vaSts = vaCreateBuffer(m_vaDisplay,
                    m_vaContextEncode,
                    VAEncMiscParameterBufferType,
                    sizeof(VAEncMiscParameterBuffer) + sizeof (VAEncMiscParameterFEIFrameControlHEVC),
                    1,
                    NULL,
                    &vaFeiFrameControlId);
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }

            VAEncMiscParameterBuffer *miscParam;
            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                vaSts = vaMapBuffer(m_vaDisplay,
                    vaFeiFrameControlId,
                    (void **)&miscParam);
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }

            miscParam->type = (VAEncMiscParameterType)VAEncMiscParameterTypeFEIFrameControl;
            VAEncMiscParameterFEIFrameControlHEVC* vaFeiFrameControl = (VAEncMiscParameterFEIFrameControlHEVC*)miscParam->data;
            memset(vaFeiFrameControl, 0, sizeof(VAEncMiscParameterFEIFrameControlHEVC));

            mfxExtFeiHevcEncFrameCtrl* EncFrameCtrl = reinterpret_cast<mfxExtFeiHevcEncFrameCtrl*>(GetBufById(task.m_ctrl, MFX_EXTBUFF_HEVCFEI_ENC_CTRL));
            MFX_CHECK(EncFrameCtrl, MFX_ERR_UNDEFINED_BEHAVIOR);

            vaFeiFrameControl->function                           = VA_FEI_FUNCTION_ENC_PAK;
            vaFeiFrameControl->search_path                        = EncFrameCtrl->SearchPath;
            vaFeiFrameControl->len_sp                             = EncFrameCtrl->LenSP;
            vaFeiFrameControl->ref_width                          = EncFrameCtrl->RefWidth;
            vaFeiFrameControl->ref_height                         = EncFrameCtrl->RefHeight;
            vaFeiFrameControl->search_window                      = EncFrameCtrl->SearchWindow;
            vaFeiFrameControl->num_mv_predictors_l0               = EncFrameCtrl->MVPredictor ? EncFrameCtrl->NumMvPredictors[0] : 0;
            vaFeiFrameControl->num_mv_predictors_l1               = EncFrameCtrl->MVPredictor ? EncFrameCtrl->NumMvPredictors[1] : 0;
            vaFeiFrameControl->multi_pred_l0                      = EncFrameCtrl->MultiPred[0];
            vaFeiFrameControl->multi_pred_l1                      = EncFrameCtrl->MultiPred[1];
            vaFeiFrameControl->sub_pel_mode                       = EncFrameCtrl->SubPelMode;
            vaFeiFrameControl->adaptive_search                    = EncFrameCtrl->AdaptiveSearch;
            vaFeiFrameControl->mv_predictor_input                 = EncFrameCtrl->MVPredictor;
            vaFeiFrameControl->per_block_qp                       = EncFrameCtrl->PerCuQp;
            vaFeiFrameControl->per_ctb_input                      = EncFrameCtrl->PerCtuInput;
            vaFeiFrameControl->force_lcu_split                    = EncFrameCtrl->ForceCtuSplit;
            vaFeiFrameControl->num_concurrent_enc_frame_partition = EncFrameCtrl->NumFramePartitions;
            vaFeiFrameControl->fast_intra_mode                    = EncFrameCtrl->FastIntraMode;

            // Input buffers
            mfxExtFeiHevcEncMVPredictors* mvp = reinterpret_cast<mfxExtFeiHevcEncMVPredictors*>(GetBufById(task.m_ctrl, MFX_EXTBUFF_HEVCFEI_ENC_MV_PRED));
            vaFeiFrameControl->mv_predictor = (EncFrameCtrl->MVPredictor && mvp) ? mvp->VaBufferID : VA_INVALID_ID;

            mfxExtFeiHevcEncQP* qp = reinterpret_cast<mfxExtFeiHevcEncQP*>(GetBufById(task.m_ctrl, MFX_EXTBUFF_HEVCFEI_ENC_QP));
            vaFeiFrameControl->qp = (EncFrameCtrl->PerCuQp && qp) ? qp->VaBufferID : VA_INVALID_ID;

            mfxExtFeiHevcEncCtuCtrl* ctuctrl = reinterpret_cast<mfxExtFeiHevcEncCtuCtrl*>(GetBufById(task.m_ctrl, MFX_EXTBUFF_HEVCFEI_ENC_CTU_CTRL));
            vaFeiFrameControl->ctb_ctrl = (EncFrameCtrl->PerCtuInput && ctuctrl) ? ctuctrl->VaBufferID : VA_INVALID_ID;

            mfxExtFeiHevcRepackCtrl* repackctrl = reinterpret_cast<mfxExtFeiHevcRepackCtrl*>(GetBufById(task.m_ctrl, MFX_EXTBUFF_HEVCFEI_REPACK_CTRL));
            if (NULL != repackctrl)
            {
                vaFeiFrameControl->max_frame_size = repackctrl->MaxFrameSize;
                vaFeiFrameControl->num_passes = repackctrl->NumPasses;
                vaFeiFrameControl->delta_qp = repackctrl->DeltaQP;
            }

            // Output buffers
#if MFX_VERSION >= MFX_VERSION_NEXT
            mfxExtFeiHevcPakCtuRecordV0* ctucmd = reinterpret_cast<mfxExtFeiHevcPakCtuRecordV0*>(GetBufById(task.m_bs, MFX_EXTBUFF_HEVCFEI_PAK_CTU_REC));
            vaFeiFrameControl->ctb_cmd = ctucmd ? ctucmd->VaBufferID : VA_INVALID_ID;

            mfxExtFeiHevcPakCuRecordV0* curec = reinterpret_cast<mfxExtFeiHevcPakCuRecordV0*>(GetBufById(task.m_bs, MFX_EXTBUFF_HEVCFEI_PAK_CU_REC));
            vaFeiFrameControl->cu_record = curec ? curec->VaBufferID : VA_INVALID_ID;

            mfxExtFeiHevcDistortion* distortion = reinterpret_cast<mfxExtFeiHevcDistortion*>(GetBufById(task.m_bs, MFX_EXTBUFF_HEVCFEI_ENC_DIST));
            vaFeiFrameControl->distortion = distortion ? distortion->VaBufferID : VA_INVALID_ID;
#else
            vaFeiFrameControl->ctb_cmd    = VA_INVALID_ID;
            vaFeiFrameControl->cu_record  = VA_INVALID_ID;
            vaFeiFrameControl->distortion = VA_INVALID_ID;
#endif

            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
                vaSts = vaUnmapBuffer(m_vaDisplay, vaFeiFrameControlId);
                MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);
            }
        }

        return MFX_ERR_NONE;
    }

    mfxStatus VAAPIh265FeiEncoder::PostQueryExtraStage(Task const & task, mfxU32 codedStatus)
    {
        mfxExtFeiHevcRepackStat *repackStat = reinterpret_cast<mfxExtFeiHevcRepackStat *>
            (GetBufById(task.m_ctrl, MFX_EXTBUFF_HEVCFEI_REPACK_STAT));

        if (repackStat)
        {
            repackStat->NumPasses = 0;

            mfxExtFeiHevcRepackCtrl* repackctrl = reinterpret_cast<mfxExtFeiHevcRepackCtrl*>
                (GetBufById(task.m_ctrl, MFX_EXTBUFF_HEVCFEI_REPACK_CTRL));

            if (NULL != repackctrl)
            {
                mfxU8 QP = codedStatus & VA_CODED_BUF_STATUS_PICTURE_AVE_QP_MASK;
                mfxU8 QPOption = task.m_qpY;
                mfxU32 numPasses = 0;

                while ((QPOption != QP) && (numPasses < repackctrl->NumPasses))
                    QPOption += repackctrl->DeltaQP[numPasses++];

                if (QPOption == QP)
                    repackStat->NumPasses = numPasses+1;
            }
        }

        return MFX_ERR_NONE;
    }
}

#endif
