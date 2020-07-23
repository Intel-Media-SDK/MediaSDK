// Copyright (c) 2014-2020 Intel Corporation
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
#include "mfx_common_int.h"

#include "umc_defs.h"
#include <mutex>

#ifndef _MFX_AV1_DEC_DECODE_H_
#define _MFX_AV1_DEC_DECODE_H_

#if defined(MFX_ENABLE_AV1_VIDEO_DECODE)

class mfx_UMC_FrameAllocator;

namespace UMC
{
    class VideoDecoderParams;
}

namespace UMC_AV1_DECODER
{
    class AV1Decoder;
    class AV1DecoderFrame;
    class AV1DecoderParams;
}

using UMC_AV1_DECODER::AV1DecoderFrame;

class VideoDECODEAV1
    : public VideoDECODE
{
    struct TaskInfo
    {
        mfxFrameSurface1 *surface_work;
        mfxFrameSurface1 *surface_out;
    };

public:

    VideoDECODEAV1(VideoCORE*, mfxStatus*);
    virtual ~VideoDECODEAV1();

    static mfxStatus Query(VideoCORE*, mfxVideoParam* in, mfxVideoParam* out);
    static mfxStatus QueryIOSurf(VideoCORE*, mfxVideoParam*, mfxFrameAllocRequest*);
    static mfxStatus DecodeHeader(VideoCORE*, mfxBitstream*, mfxVideoParam*);

    virtual mfxStatus Init(mfxVideoParam*) override;
    virtual mfxStatus Reset(mfxVideoParam*) override;
    virtual mfxStatus Close() override;
    virtual mfxTaskThreadingPolicy GetThreadingPolicy() override;

    virtual mfxStatus GetVideoParam(mfxVideoParam*) override;
    virtual mfxStatus GetDecodeStat(mfxDecodeStat*) override;
    virtual mfxStatus DecodeFrameCheck(mfxBitstream*, mfxFrameSurface1* surface_work, mfxFrameSurface1** surface_out, MFX_ENTRY_POINT*) override;
    virtual mfxStatus SetSkipMode(mfxSkipMode) override;
    virtual mfxStatus GetPayload(mfxU64* time_stamp, mfxPayload*) override;

    mfxStatus QueryFrame(mfxThreadTask);

private:
    static mfxStatus FillVideoParam(VideoCORE*, UMC_AV1_DECODER::AV1DecoderParams const*, mfxVideoParam*);
    static mfxStatus DecodeRoutine(void* state, void* param, mfxU32, mfxU32);
    static mfxStatus CompleteProc(void*, void* param, mfxStatus);

    mfxStatus SubmitFrame(mfxBitstream* bs, mfxFrameSurface1* surface_work, mfxFrameSurface1** surface_out);

    AV1DecoderFrame* GetFrameToDisplay();
    mfxStatus FillOutputSurface(mfxFrameSurface1** surface_out, mfxFrameSurface1* surface_work, AV1DecoderFrame*);

    mfxFrameSurface1* GetOriginalSurface(mfxFrameSurface1* surface)
    {
        VM_ASSERT(m_core);

        return m_opaque ?
            m_core->GetNativeSurface(surface) : surface;
    }

    mfxStatus DecodeFrame(mfxFrameSurface1 *surface_out, AV1DecoderFrame* pFrame);
    bool IsNeedChangeVideoParam(mfxVideoParam * newPar, mfxVideoParam * oldPar, eMFXHWType type) const;

private:

    VideoCORE*                                   m_core;
    eMFXPlatform                                 m_platform;

    std::mutex                                   m_guard;
    std::unique_ptr<mfx_UMC_FrameAllocator>      m_allocator;
    std::unique_ptr<UMC_AV1_DECODER::AV1Decoder> m_decoder;

    bool                                         m_opaque;
    bool                                         m_first_run;

    mfxVideoParamWrapper                         m_video_par;
    mfxVideoParamWrapper                         m_init_par;
    mfxVideoParamWrapper                         m_first_par;

    mfxFrameAllocRequest                         m_request;
    mfxFrameAllocResponse                        m_response;

    bool                                         m_is_init;
    mfxF64                                       m_in_framerate;
};

#endif // MFX_ENABLE_AV1_VIDEO_DECODE

#endif // _MFX_AV1_DEC_DECODE_H_
