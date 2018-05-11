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
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include <memory>
#include <list>
#include "mfx_h265_encode_hw_ddi.h"
#include "mfx_h265_encode_hw_utils.h"
#include "umc_mutex.h"
#include "mfxplugin++.h"
#include "mfx_h265_encode_hw_brc.h"

namespace MfxHwH265Encode
{

static const mfxPluginUID  MFX_PLUGINID_HEVCE_HW = {{0x6f, 0xad, 0xc7, 0x91, 0xa0, 0xc2, 0xeb, 0x47, 0x9a, 0xb6, 0xdc, 0xd5, 0xea, 0x9d, 0xa3, 0x47}};

class Plugin : public MFXEncoderPlugin
{
public:
    static MFXEncoderPlugin* Create()
    {
        return new Plugin(false);
    }

    static mfxStatus CreateByDispatcher(mfxPluginUID guid, mfxPlugin* mfxPlg)
    {
        if (memcmp(& guid , &MFX_PLUGINID_HEVCE_HW, sizeof(mfxPluginUID)))
        {
            return MFX_ERR_NOT_FOUND;
        }

        Plugin* tmp_pplg = 0;

        try
        {
            tmp_pplg = new Plugin(false);
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

    virtual mfxStatus PluginInit(mfxCoreInterface *core);
    virtual mfxStatus PluginClose();
    virtual mfxStatus GetPluginParam(mfxPluginParam *par);
    virtual mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a);
    virtual mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts);

    virtual mfxStatus Init(mfxVideoParam *par);
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    virtual mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out);
    virtual mfxStatus Reset(mfxVideoParam *par);
    virtual mfxStatus GetVideoParam(mfxVideoParam *par);

    virtual mfxStatus EncodeFrameSubmit(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxThreadTask *task);

    virtual mfxU32 GetPluginType()
    {
        return MFX_PLUGINTYPE_VIDEO_ENCODE;
    }

    virtual void Release()
    {
        delete this;
    }

    virtual mfxStatus Close();

    virtual mfxStatus SetAuxParams(void*, int)
    {
        return MFX_ERR_UNSUPPORTED;
    }
    void ZeroParams()
    {
       m_frameOrder = 0;
       m_lastIDR = 0;
       m_baseLayerOrder = 0;
       m_numBuffered = 0;
    }

protected:
    explicit Plugin(bool CreateByDispatcher);
    virtual ~Plugin();

    mfxStatus InitImpl(mfxVideoParam *par);
    void      FreeResources();
    mfxStatus WaitingForAsyncTasks(bool bResetTasks);

    virtual DriverEncoder* CreateHWh265Encoder(MFXCoreInterface* core, ENCODER_TYPE type = ENCODER_DEFAULT)
    {
        return CreatePlatformH265Encoder(core, type);
    }

    mfxStatus CheckVideoParam(MfxVideoParam & par, ENCODE_CAPS_HEVC const & caps, bool bInit = false);

    virtual mfxStatus ExtraCheckVideoParam(MfxVideoParam & par, ENCODE_CAPS_HEVC const & caps, bool bInit = false)
    {
        par;
        caps;
        bInit;

        return MFX_ERR_NONE;
    }

    virtual mfxStatus ExtraParametersCheck(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs)
    {
        ctrl;
        surface;
        bs;

        return MFX_ERR_NONE;
    }

    virtual void ExtraTaskPreparation(Task& task)
    {
        task;

        return;
    }

    bool m_createdByDispatcher;
    MFXPluginAdapter<MFXEncoderPlugin> m_adapter;

    mfxStatus PrepareTask( Task& task);
    mfxStatus FreeTask(Task& task);
    mfxStatus WaitForQueringTask(Task& task);

    std::unique_ptr<DriverEncoder>    m_ddi;
    MFXCoreInterface                m_core;

    MfxVideoParam                   m_vpar;
    ENCODE_CAPS_HEVC                m_caps;

    MfxFrameAllocResponse           m_raw;
    MfxFrameAllocResponse           m_rawSkip;
    MfxFrameAllocResponse           m_rec;
    MfxFrameAllocResponse           m_bs;
    MfxFrameAllocResponse           m_CuQp; // for DDI only (not used in VA)

    TaskManager                     m_task;
    Task                            m_lastTask;
    HRD                             m_hrd;
    mfxU32                          m_frameOrder;
    mfxU32                          m_lastIDR;
    mfxU32                          m_prevBPEO;
    mfxU32                          m_baseLayerOrder;
    mfxU32                          m_numBuffered;
    mfxU16                          m_NumberOfSlicesForOpt;
    bool                            m_bInit;
    mfxStatus                       m_runtimeErr;
    BrcIface*                       m_brc;

};

} //MfxHwH265Encode

#endif
