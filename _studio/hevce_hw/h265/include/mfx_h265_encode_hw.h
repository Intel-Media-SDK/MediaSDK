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
#include "mfxvideo++int.h"
#include <mfx_task.h>

namespace MfxHwH265Encode
{

static const mfxPluginUID  MFX_PLUGINID_HEVCE_HW = {{0x6f, 0xad, 0xc7, 0x91, 0xa0, 0xc2, 0xeb, 0x47, 0x9a, 0xb6, 0xdc, 0xd5, 0xea, 0x9d, 0xa3, 0x47}};

class MFXVideoENCODEH265_HW : public VideoENCODE
{
public:
    MFXVideoENCODEH265_HW(mfxCoreInterface *core, mfxStatus *status)
        : m_caps()
        , m_lastTask()
        , m_prevBPEO(0)
        , m_NumberOfSlicesForOpt(0)
        , m_bInit(false)
        , m_runtimeErr(MFX_ERR_NONE)
        , m_brc(NULL)
    {
        ZeroParams();
        if (core)
            m_core = *core;

        if (status)
            *status = MFX_ERR_NONE;
    }
    virtual ~MFXVideoENCODEH265_HW()
    {
        Close();
    }

    static mfxStatus QueryIOSurf(mfxCoreInterface *core, mfxVideoParam *par, mfxFrameAllocRequest *request);

    static mfxStatus Query(mfxCoreInterface *core, mfxVideoParam *in, mfxVideoParam *out);

    virtual mfxStatus Init(mfxVideoParam *par);

    virtual mfxStatus Reset(mfxVideoParam *par);

    virtual mfxStatus Close(void);

    virtual mfxStatus GetVideoParam(mfxVideoParam *par);

    virtual mfxStatus GetFrameParam(mfxFrameParam * /*par*/)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    virtual mfxStatus GetEncodeStat(mfxEncodeStat * /*stat*/)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    virtual mfxStatus EncodeFrameCheck(mfxEncodeCtrl * /*ctrl*/, mfxFrameSurface1 * /*surface*/, mfxBitstream * /*bs*/, mfxFrameSurface1 ** /*reordered_surface*/, mfxEncodeInternalParams * /*pInternalParams*/)
    {
        return MFX_ERR_NONE;
    }

    virtual mfxStatus EncodeFrameCheck(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxFrameSurface1 ** /*reordered_surface*/, mfxEncodeInternalParams * /*pInternalParams*/, MFX_ENTRY_POINT *pEntryPoint)
    {
        mfxStatus mfxRes;
        mfxThreadTask userParam;

        mfxRes = EncodeFrameSubmit(ctrl, surface, bs, &userParam);
        if (mfxRes >= MFX_ERR_NONE || mfxRes == MFX_ERR_MORE_DATA_SUBMIT_TASK)
        {
            pEntryPoint->pState = this;
            pEntryPoint->pRoutine = Execute;
            pEntryPoint->pCompleteProc = FreeResources;
            pEntryPoint->requiredNumThreads = 1;
            pEntryPoint->pParam = userParam;
        }

        return mfxRes;
    }

    virtual mfxStatus EncodeFrame(mfxEncodeCtrl * /*ctrl*/, mfxEncodeInternalParams * /*pInternalParams*/, mfxFrameSurface1 * /*surface*/, mfxBitstream * /*bs*/)
    {
        return MFX_ERR_NONE;
    }

    virtual mfxStatus CancelFrame(mfxEncodeCtrl * /*ctrl*/, mfxEncodeInternalParams * /*pInternalParams*/, mfxFrameSurface1 * /*surface*/, mfxBitstream * /*bs*/)
    {
        return MFX_ERR_NONE;
    }

    virtual mfxStatus EncodeFrameSubmit(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxThreadTask *task);

    static mfxStatus Execute(void *pState, void *task, mfxU32 uid_p, mfxU32 uid_a)
    {
        if (pState)
            return ((MFXVideoENCODEH265_HW*)pState)->Execute(task, uid_p, uid_a);
        else
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    static mfxStatus FreeResources(void *pState, void *task, mfxStatus sts)
    {
        if (pState)
            return ((MFXVideoENCODEH265_HW*)pState)->FreeResources(task, sts);
        else
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    virtual mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts);

    mfxStatus WaitingForAsyncTasks(bool bResetTasks);

    void ZeroParams()
    {
        m_frameOrder = 0;
        m_lastIDR = 0;
        m_baseLayerOrder = 0;
        m_numBuffered = 0;
    }
protected:
    mfxStatus InitImpl(mfxVideoParam *par);
    void      FreeResources();

    virtual DriverEncoder* CreateHWh265Encoder(MFXCoreInterface* core, ENCODER_TYPE type = ENCODER_DEFAULT)
    {
        return CreatePlatformH265Encoder(core, type);
    }

    mfxStatus CheckVideoParam(MfxVideoParam & par, ENCODE_CAPS_HEVC const & caps, bool bInit = false);
    mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a);

    virtual mfxStatus ExtraCheckVideoParam(MfxVideoParam & /*par*/, ENCODE_CAPS_HEVC const & /*caps*/, bool bInit = false)
    {
        bInit;
        return MFX_ERR_NONE;
    }

    virtual mfxStatus ExtraParametersCheck(mfxEncodeCtrl * /*ctrl*/, mfxFrameSurface1 * /*surface*/, mfxBitstream * /*bs*/)
    {
        return MFX_ERR_NONE;
    }

    virtual void ExtraTaskPreparation(Task& /*task*/)
    {
        return;
    }

    mfxStatus PrepareTask(Task& task);
    mfxStatus FreeTask(Task& task);
    mfxStatus WaitForQueringTask(Task& task);

    std::unique_ptr<DriverEncoder>  m_ddi;
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

    virtual mfxStatus PluginInit(mfxCoreInterface *core)
    {
        MFX_TRACE_INIT();
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "Plugin::PluginInit");

        MFX_CHECK_NULL_PTR1(core);

        m_core = *core;

        return MFX_ERR_NONE;
    }
    virtual mfxStatus PluginClose()
    {
        {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "Plugin::PluginClose");
            if (m_createdByDispatcher)
                Release();
        }

        MFX_TRACE_CLOSE();

        return MFX_ERR_NONE;
    }
    virtual mfxStatus GetPluginParam(mfxPluginParam *par)
    {
        MFX_CHECK_NULL_PTR1(par);

        par->PluginUID          = MFX_PLUGINID_HEVCE_HW;
        par->PluginVersion      = 1;
        par->ThreadPolicy       = MFX_THREADPOLICY_SERIAL;
        par->MaxThreadNum       = 1;
        par->APIVersion.Major   = MFX_VERSION_MAJOR;
        par->APIVersion.Minor   = MFX_VERSION_MINOR;
        par->Type               = MFX_PLUGINTYPE_VIDEO_ENCODE;
        par->CodecId            = MFX_CODEC_HEVC;

        return MFX_ERR_NONE;
    }
    virtual mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a)
    {
        if (m_pImpl.get())
            return MFXVideoENCODEH265_HW::Execute((reinterpret_cast<void*>(m_pImpl.get())), task, uid_p, uid_a);
        else
            return MFX_ERR_NOT_INITIALIZED;
    }
    virtual mfxStatus FreeResources(mfxThreadTask /*task*/, mfxStatus /*sts*/)
    {
        return MFX_ERR_NONE;
    }

    virtual mfxStatus Init(mfxVideoParam *par)
    {
        mfxStatus sts;
        m_pImpl.reset(new MFXVideoENCODEH265_HW(&m_core, &sts));
        MFX_CHECK_STS(sts);
        return m_pImpl->Init(par);
    }
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest * /*out*/)
    {
        return MFXVideoENCODEH265_HW::QueryIOSurf(&m_core, par, in);
    }
    virtual mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out)
    {
        return MFXVideoENCODEH265_HW::Query(&m_core, in, out);
    }
    virtual mfxStatus Reset(mfxVideoParam *par)
    {
        if (m_pImpl.get())
            return m_pImpl->Reset(par);
        else
            return MFX_ERR_NOT_INITIALIZED;
    }
    virtual mfxStatus GetVideoParam(mfxVideoParam *par)
    {
        if (m_pImpl.get())
            return m_pImpl->GetVideoParam(par);
        else
            return MFX_ERR_NOT_INITIALIZED;
    }

    virtual mfxStatus EncodeFrameSubmit(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxThreadTask *task)
    {
        if (m_pImpl.get())
            return m_pImpl->EncodeFrameSubmit(ctrl, surface, bs, task);
        else
            return MFX_ERR_NOT_INITIALIZED;
    }

    virtual mfxU32 GetPluginType()
    {
        return MFX_PLUGINTYPE_VIDEO_ENCODE;
    }

    virtual void Release()
    {
        delete this;
    }

    virtual mfxStatus Close()
    {
        if (m_pImpl.get())
            return m_pImpl->Close();
        else
            return MFX_ERR_NOT_INITIALIZED;
    }

    virtual mfxStatus SetAuxParams(void*, int)
    {
        return MFX_ERR_UNSUPPORTED;
    }

protected:
    explicit Plugin(bool CreateByDispatcher)
        : m_createdByDispatcher(CreateByDispatcher)
        , m_adapter(this)
        , m_pImpl(nullptr)
    {}
    virtual ~Plugin()
    {}

    bool m_createdByDispatcher;
    MFXPluginAdapter<MFXEncoderPlugin> m_adapter;
    mfxCoreInterface m_core;
    std::unique_ptr<MFXVideoENCODEH265_HW> m_pImpl;

};

} //MfxHwH265Encode

#endif
