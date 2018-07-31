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
#include "mfx_h265_encode_hw_brc.h"
#include "mfxvideo++int.h"
#include <mfx_task.h>

namespace MfxHwH265Encode
{

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
        mfxThreadTask userParam;

        mfxStatus mfxRes = EncodeFrameSubmit(ctrl, surface, bs, &userParam);
        if (mfxRes >= MFX_ERR_NONE || mfxRes == MFX_ERR_MORE_DATA_SUBMIT_TASK)
        {
            pEntryPoint->pState             = this;
            pEntryPoint->pRoutine           = Execute;
            pEntryPoint->pCompleteProc      = FreeResources;
            pEntryPoint->requiredNumThreads = 1;
            pEntryPoint->pParam             = userParam;
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

    bool IsInitialized()
    {
        return m_bInit;
    }

    void ZeroParams()
    {
        m_frameOrder     = 0;
        m_lastIDR        = 0;
        m_baseLayerOrder = 0;
        m_numBuffered    = 0;
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

    virtual mfxStatus ExtraCheckVideoParam(MfxVideoParam & /*par*/, ENCODE_CAPS_HEVC const & /*caps*/, bool /*bInit = false*/)
    {
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

} //MfxHwH265Encode

#endif
