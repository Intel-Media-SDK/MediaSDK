// Copyright (c) 2017 Intel Corporation
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

static const mfxPluginUID  MFX_PLUGINID_HEVC_FEI_ENCODE = {{0x54, 0x18, 0xa7, 0x06, 0x66, 0xf9, 0x4d, 0x5c, 0xb4, 0xf7, 0xb1, 0xca, 0xee, 0x86, 0x33, 0x9b}};

class H265FeiEncodePlugin : public MfxHwH265Encode::Plugin
{
public:
    static MFXEncoderPlugin* Create()
    {
        return new H265FeiEncodePlugin(false);
    }

    static mfxStatus CreateByDispatcher(mfxPluginUID guid, mfxPlugin* mfxPlg)
    {
        if (memcmp(&guid, &MFX_PLUGINID_HEVC_FEI_ENCODE, sizeof(mfxPluginUID)))
        {
            return MFX_ERR_NOT_FOUND;
        }

        H265FeiEncodePlugin* tmp_pplg = 0;

        try
        {
            tmp_pplg = new H265FeiEncodePlugin(false);
        }
        catch(std::bad_alloc&)
        {
            return MFX_ERR_MEMORY_ALLOC;
        }
        catch(...)
        {
            delete tmp_pplg;
            return MFX_ERR_UNKNOWN;
        }

        *mfxPlg = tmp_pplg->m_adapter;
        tmp_pplg->m_createdByDispatcher = true;

        return MFX_ERR_NONE;
    }

    virtual mfxStatus GetPluginParam(mfxPluginParam *par);

    virtual mfxU32 GetPluginType()
    {
        return MFX_PLUGINTYPE_VIDEO_ENCODE;
    }

    virtual void Release()
    {
        delete this;
    }

    virtual mfxStatus Close();

    virtual mfxStatus Reset(mfxVideoParam *par)
    {
        // waiting for submitted in driver tasks
        // This Sync is required to guarantee correct encoding of Async tasks in case of dynamic CTU QP change
        MFX_CHECK_STS(WaitingForAsyncTasks(true));

        // Call actual Reset()
        return MfxHwH265Encode::Plugin::Reset(par);
    }


protected:
    explicit H265FeiEncodePlugin(bool CreateByDispatcher);
    virtual ~H265FeiEncodePlugin();

    virtual MfxHwH265Encode::DriverEncoder* CreateHWh265Encoder(MFXCoreInterface* core, MfxHwH265Encode::ENCODER_TYPE type = MfxHwH265Encode::ENCODER_DEFAULT);

    virtual mfxStatus ExtraParametersCheck(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs);

    virtual mfxStatus ExtraCheckVideoParam(MfxHwH265Encode::MfxVideoParam & par, ENCODE_CAPS_HEVC const & caps, bool bInit = false)
    {
        // HEVC FEI Encoder uses own controls to switch on LCU QP buffer
        if (MfxHwH265Encode::IsOn(m_vpar.m_ext.CO3.EnableMBQP))
        {
            m_vpar.m_ext.CO3.EnableMBQP = MFX_CODINGOPTION_OFF;

            return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        }

        return MFX_ERR_NONE;
    }

    virtual void ExtraTaskPreparation(MfxHwH265Encode::Task& task)
    {
        mfxExtFeiHevcEncFrameCtrl* EncFrameCtrl = reinterpret_cast<mfxExtFeiHevcEncFrameCtrl*>(GetBufById(task.m_ctrl, MFX_EXTBUFF_HEVCFEI_ENC_CTRL));

        if (EncFrameCtrl && (!!EncFrameCtrl->PerCuQp != !!m_vpar.m_pps.cu_qp_delta_enabled_flag))
        {
            SoftReset(task);
        }

        return;
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
