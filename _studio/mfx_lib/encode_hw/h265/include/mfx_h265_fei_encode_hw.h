// Copyright (c) 2017-2018 Intel Corporation
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

#pragma once

#include "mfx_common.h"
#if defined(MFX_ENABLE_HEVC_VIDEO_FEI_ENCODE)

#include "mfx_h265_fei_encode_vaapi.h"
#include "mfx_h265_encode_hw.h"

namespace MfxHwH265FeiEncode
{

class H265FeiEncode_HW : public MfxHwH265Encode::MFXVideoENCODEH265_HW
{
public:
    H265FeiEncode_HW(VideoCORE *core, mfxStatus *status)
        :MFXVideoENCODEH265_HW(core, status)
    {}

    virtual ~H265FeiEncode_HW()
    {
        Close();
    }

    virtual MfxHwH265Encode::DriverEncoder* CreateHWh265Encoder(VideoCORE* /*core*/, MfxHwH265Encode::ENCODER_TYPE type = MfxHwH265Encode::ENCODER_DEFAULT) override
    {
        type;

        return new VAAPIh265FeiEncoder;
    }

    virtual mfxStatus Reset(mfxVideoParam *par) override
    {
        // waiting for submitted in driver tasks
        // This Sync is required to guarantee correct encoding of Async tasks in case of dynamic CTU QP change
        mfxStatus sts = WaitingForAsyncTasks(true);
        MFX_CHECK_STS(sts);

        // Call base Reset()
        return MfxHwH265Encode::MFXVideoENCODEH265_HW::Reset(par);
    }

    virtual mfxStatus ExtraParametersCheck(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs) override;

    virtual mfxStatus ExtraCheckVideoParam(MfxHwH265Encode::MfxVideoParam & par, ENCODE_CAPS_HEVC const & caps, bool bInit = false) override
    {
        // HEVC FEI Encoder uses own controls to switch on LCU QP buffer
        if (MfxHwH265Encode::IsOn(m_vpar.m_ext.CO3.EnableMBQP))
        {
            m_vpar.m_ext.CO3.EnableMBQP = MFX_CODINGOPTION_OFF;

            return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        }

        return MFX_ERR_NONE;
    }

    virtual void ExtraTaskPreparation(MfxHwH265Encode::Task& task) override
    {
        mfxExtFeiHevcEncFrameCtrl* EncFrameCtrl = reinterpret_cast<mfxExtFeiHevcEncFrameCtrl*>(GetBufById(task.m_ctrl, MFX_EXTBUFF_HEVCFEI_ENC_CTRL));

        mfxExtFeiHevcRepackCtrl* repackctrl = reinterpret_cast<mfxExtFeiHevcRepackCtrl*>(GetBufById(task.m_ctrl, MFX_EXTBUFF_HEVCFEI_REPACK_CTRL));

        if (m_vpar.m_pps.cu_qp_delta_enabled_flag)
        {
            if (!repackctrl && !EncFrameCtrl->PerCuQp)
            {
                // repackctrl or PerCuQp is disabled, so insert PPS and turn the flag off.
                SoftReset(task);
            }
        }
        else
        {
            if (repackctrl || EncFrameCtrl->PerCuQp)
            {
                // repackctrl or PerCuQp is enabled, so insert PPS and turn the flag on.
                SoftReset(task);
            }
        }
    }

    void SoftReset(MfxHwH265Encode::Task& task)
    {
        m_vpar.m_pps.cu_qp_delta_enabled_flag = 1 - m_vpar.m_pps.cu_qp_delta_enabled_flag;

        // If state of CTU QP buffer changes, PPS header update required
        task.m_insertHeaders |= MfxHwH265Encode::INSERT_PPS;

        (dynamic_cast<VAAPIh265FeiEncoder*> (m_ddi.get()))->SoftReset(m_vpar);
    }
};

} //MfxHwH265FeiEncode

#endif
