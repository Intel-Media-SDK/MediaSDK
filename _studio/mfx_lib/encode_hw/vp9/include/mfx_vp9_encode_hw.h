// Copyright (c) 2018 Intel Corporation
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

#include <stdio.h>
#include <memory>
#include <queue>
#include "mfxvideo.h"
#include "mfxvideo++int.h"
#include "mfx_utils.h"
#include <umc_mutex.h>
#include <mfx_task.h>
#include "mfx_vp9_encode_hw_utils.h"
#include "mfx_vp9_encode_hw_ddi.h"

namespace MfxHwVP9Encode
{
class MFXVideoENCODEVP9_HW : public VideoENCODE
{
public:
    MFXVideoENCODEVP9_HW(VideoCORE *core, mfxStatus *status)
        : m_bStartIVFSequence(false)
        , m_maxBsSize(0)
        , m_pCore(core)
        , m_initialized(false)
        , m_frameArrivalOrder(0)
        , m_drainState(false)
        , m_resetBrc(false)
        , m_taskIdForDriver(0)
        , m_numBufferedFrames(0)
        , m_initWidth(0)
        , m_initHeight(0)
        , m_frameOrderInGop(0)
        , m_frameOrderInRefStructure(0)
    {
        m_prevSegment.Header.BufferId = MFX_EXTBUFF_VP9_SEGMENTATION;
        m_prevSegment.Header.BufferSz = sizeof(mfxExtVP9Segmentation);
        ZeroExtBuffer(m_prevSegment);

        Zero(m_prevFrameParam);

        if (status)
            *status = MFX_ERR_NONE;
    }
    virtual ~MFXVideoENCODEVP9_HW()
    {
        Close();
    }

    static mfxStatus QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request);

    static mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out);

    virtual mfxStatus Init(mfxVideoParam *par);

    virtual mfxStatus Reset(mfxVideoParam *par);

    virtual mfxStatus Close();

    virtual mfxTaskThreadingPolicy GetThreadingPolicy(void) { return MFX_TASK_THREADING_INTRA; }

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

    static mfxStatus Execute(void *pState, void *task, mfxU32 uid_p, mfxU32 uid_a)
    {
        if (pState)
            return ((MFXVideoENCODEVP9_HW*)pState)->Execute(task, uid_p, uid_a);
        else
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    virtual mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a);

    static mfxStatus FreeResources(void *pState, void *task, mfxStatus sts)
    {
        if (pState)
            return ((MFXVideoENCODEVP9_HW*)pState)->FreeResources(task, sts);
        else
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    virtual mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts);

    virtual mfxStatus EncodeFrameSubmit(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxThreadTask *task);
    virtual mfxStatus ConfigTask(Task &task);
    virtual mfxStatus RemoveObsoleteParameters();
    virtual mfxStatus UpdateBitstream(Task & task);

    bool isInitialized()
    {
        return m_initialized;
    }

protected:
    VP9MfxVideoParam              m_video;
    std::list<VP9MfxVideoParam>   m_videoForParamChange; // encoder keeps several versions of encoding parameters
                                                         // to allow dynamic parameter change w/o drain of all buffered tasks.
                                                         // Tasks submitted before the change reference to previous parameters version.
    std::unique_ptr <DriverEncoder> m_ddi;
    UMC::Mutex m_taskMutex;
    bool       m_bStartIVFSequence;
    mfxU64     m_maxBsSize;

    std::vector<Task> m_tasks;
    std::list<Task> m_free;
    std::list<Task> m_accepted;
    std::list<Task> m_submitted;
    std::queue<mfxBitstream*> m_outs;

    ExternalFrames  m_rawFrames;
    InternalFrames  m_opaqFrames;
    InternalFrames  m_rawLocalFrames;
    InternalFrames  m_reconFrames;
    InternalFrames  m_outBitstreams;
    InternalFrames  m_segmentMaps;

    std::vector<sFrameEx*> m_dpb;

    VideoCORE    *m_pCore;

    bool m_initialized;

    mfxU32 m_frameArrivalOrder;

    bool m_drainState;
    bool m_resetBrc;

    mfxU32 m_taskIdForDriver; // should be unique among all the frames submitted to driver

    mfxI32 m_numBufferedFrames;

    mfxU16 m_initWidth;
    mfxU16 m_initHeight;

    mfxU32 m_frameOrderInGop;
    mfxU32 m_frameOrderInRefStructure; // reset of ref structure happens at key-frames and after dynamic scaling

    mfxExtVP9Segmentation m_prevSegment;
    VP9FrameLevelParam m_prevFrameParam;
};
} // MfxHwVP9Encode
