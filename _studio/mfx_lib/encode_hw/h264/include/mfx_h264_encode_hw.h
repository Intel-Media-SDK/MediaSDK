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

#include "mfx_common.h"
#ifdef MFX_ENABLE_H264_VIDEO_ENCODE_HW

#include "mfx_h264_enc_common_hw.h"
#include "mfx_h264_encode_hw_utils.h"

#ifndef _MFX_H264_ENCODE_HW_H_
#define _MFX_H264_ENCODE_HW_H_

class MFXHWVideoENCODEH264 : public VideoENCODE
{
public:
    static mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out, void * state = 0);
    static mfxStatus QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request);

    MFXHWVideoENCODEH264(VideoCORE *core, mfxStatus *sts)
        : m_core(core)
    {
        if (sts)
        {
            *sts = MFX_ERR_NONE;
        }
    }

    virtual mfxStatus Init(mfxVideoParam *par);

    virtual mfxStatus Close()
    {
        m_impl.reset();
        return MFX_ERR_NONE;
    }

    virtual mfxTaskThreadingPolicy GetThreadingPolicy()
    {
        return MFX_TASK_THREADING_INTRA;
    }

    virtual mfxStatus Reset(mfxVideoParam *par)
    {
        return m_impl.get()
            ? m_impl->Reset(par)
            : MFX_ERR_NOT_INITIALIZED;
    }

    virtual mfxStatus GetVideoParam(mfxVideoParam *par)
    {
        return m_impl.get()
            ? m_impl->GetVideoParam(par)
            : MFX_ERR_NOT_INITIALIZED;
    }

    virtual mfxStatus GetFrameParam(mfxFrameParam *par)
    {
        return m_impl.get()
            ? m_impl->GetFrameParam(par)
            : MFX_ERR_NOT_INITIALIZED;
    }

    virtual mfxStatus GetEncodeStat(mfxEncodeStat *stat)
    {
        return m_impl.get()
            ? m_impl->GetEncodeStat(stat)
            : MFX_ERR_NOT_INITIALIZED;
    }

    virtual mfxStatus EncodeFrameCheck(
        mfxEncodeCtrl *ctrl,
        mfxFrameSurface1 *surface,
        mfxBitstream *bs,
        mfxFrameSurface1 **reordered_surface,
        mfxEncodeInternalParams *internalParams)
    {
        return m_impl.get()
            ? m_impl->EncodeFrameCheck(ctrl, surface, bs, reordered_surface, internalParams)
            : MFX_ERR_NOT_INITIALIZED;
    }

    virtual mfxStatus EncodeFrameCheck(
        mfxEncodeCtrl *ctrl,
        mfxFrameSurface1 *surface,
        mfxBitstream *bs,
        mfxFrameSurface1 **reordered_surface,
        mfxEncodeInternalParams *pInternalParams,
        MFX_ENTRY_POINT pEntryPoints[],
        mfxU32 &numEntryPoints)
    {
        return m_impl.get()
            ? m_impl->EncodeFrameCheck(ctrl, surface, bs, reordered_surface, pInternalParams, pEntryPoints, numEntryPoints)
            : MFX_ERR_NOT_INITIALIZED;
    }

    virtual mfxStatus EncodeFrame(
        mfxEncodeCtrl *ctrl,
        mfxEncodeInternalParams *internalParams,
        mfxFrameSurface1 *surface,
        mfxBitstream *bs)
    {
        return m_impl.get()
            ? m_impl->EncodeFrame(ctrl, internalParams, surface, bs)
            : MFX_ERR_NOT_INITIALIZED;
    }

    virtual mfxStatus CancelFrame(
        mfxEncodeCtrl* ctrl,
        mfxEncodeInternalParams* internalParams,
        mfxFrameSurface1* surface,
        mfxBitstream* bs)
    {
        return m_impl.get()
            ? m_impl->CancelFrame(ctrl, internalParams, surface, bs)
            : MFX_ERR_NOT_INITIALIZED;
    }

protected:
    VideoCORE*                 m_core;
    std::unique_ptr<VideoENCODE> m_impl;
};


#endif // _MFX_H264_ENCODE_HW_H_
#endif // MFX_ENABLE_H264_VIDEO_ENCODE_HW
