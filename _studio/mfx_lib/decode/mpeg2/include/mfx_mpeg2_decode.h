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

#include <memory>
#include <mutex>
#include "mfx_common.h"
#include "mfx_common_int.h"

#include "umc_defs.h"

#if defined(MFX_ENABLE_MPEG2_VIDEO_DECODE)

class mfx_UMC_FrameAllocator;

namespace UMC_MPEG2_DECODER
{
    class MPEG2Decoder;
    class MPEG2DecoderFrame;
}

class VideoDECODEMPEG2
    : public VideoDECODE
{
    // scheduler task info
    struct TaskInfo
    {
        mfxFrameSurface1 *surface_work;
        mfxFrameSurface1 *surface_out;
    };

public:

    VideoDECODEMPEG2(VideoCORE*, mfxStatus*);
    virtual ~VideoDECODEMPEG2();

    // Check correctness of input parameters
    static mfxStatus Query(VideoCORE*, mfxVideoParam* in, mfxVideoParam* out);
    // Return a number of surfaces
    static mfxStatus QueryIOSurf(VideoCORE*, mfxVideoParam*, mfxFrameAllocRequest*);
    // Decode header and initialize video parameters
    static mfxStatus DecodeHeader(VideoCORE* core, mfxBitstream* bs, mfxVideoParam* par);
    // Initialize decoder
    mfxStatus Init(mfxVideoParam*) override;
    // Reset decoder
    mfxStatus Reset(mfxVideoParam*) override;
    // Close decoder
    mfxStatus Close() override;
    // Return current video parameters
    mfxStatus GetVideoParam(mfxVideoParam*) override;
    // Return decoder statistic
    mfxStatus GetDecodeStat(mfxDecodeStat*) override;
    // MediaSDK DECODE_SetSkipMode API function
    mfxStatus SetSkipMode(mfxSkipMode) override;
    // Return stream payload
    mfxStatus GetPayload( mfxU64 *ts, mfxPayload *payload) override;
    // Decode frame
    mfxStatus DecodeFrameCheck(mfxBitstream*, mfxFrameSurface1*, mfxFrameSurface1**, MFX_ENTRY_POINT*) override;
    // Return scheduler threading policy
    mfxTaskThreadingPolicy GetThreadingPolicy() override;

private:
    // Internal implementation of API QueryIOSurf function
    static mfxStatus QueryIOSurfInternal(eMFXPlatform, mfxVideoParam*, mfxFrameAllocRequest*);
    // Fill up frame allocator request data
    mfxStatus UpdateAllocRequest(mfxVideoParam *par,
                                mfxFrameAllocRequest *request,
                                mfxExtOpaqueSurfaceAlloc * &pOpaqAlloc,
                                bool &mapping);
    // Decoder threads entry point
    static mfxStatus DecodeRoutine(void* state, void* param, mfxU32, mfxU32);
    // Threads complete proc callback
    static mfxStatus CompleteProc(void*, void* param, mfxStatus);
    // Initialize threads callbacks
    mfxStatus SubmitFrame(mfxBitstream* bs, mfxFrameSurface1* surface_work, mfxFrameSurface1** surface_out, mfxThreadTask* task);
    // Check if there is enough data to start decoding in async mode
    mfxStatus SubmitFrame(mfxBitstream* bs, mfxFrameSurface1* surface_work, mfxFrameSurface1** surface_out);
    // Decoder instance threads entry point. Do async tasks here
    mfxStatus QueryFrame(mfxThreadTask);
    // Wait until a frame is ready to be output and set necessary surface flags
    mfxStatus DecodeFrame(mfxFrameSurface1 *surface_out, UMC_MPEG2_DECODER::MPEG2DecoderFrame* frame) const;
    // Find a next frame ready to be output from decoder
    UMC_MPEG2_DECODER::MPEG2DecoderFrame* GetFrameToDisplay();
    // Fill mfxFrameSurface1 meta information
    mfxStatus FillOutputSurface(mfxFrameSurface1* surface_work, mfxFrameSurface1** surf_out, UMC_MPEG2_DECODER::MPEG2DecoderFrame* frame) const;
    // Get real surface from opaque surface
    mfxFrameSurface1* GetOriginalSurface(mfxFrameSurface1* surface) const
    { return m_opaque ? m_core->GetNativeSurface(surface) : surface; }
    // Fill up resolution information if new header arrived
    void FillVideoParam(mfxVideoParamWrapper*, bool);
    // Check if new parameters are compatible with new parameters
    bool IsSameVideoParam(mfxVideoParam * newPar, mfxVideoParam * oldPar, eMFXHWType type) const;

private:

    VideoCORE*                                       m_core;
    eMFXPlatform                                     m_platform;

    std::mutex                                       m_guard;
    std::unique_ptr<mfx_UMC_FrameAllocator>          m_allocator;
    std::unique_ptr<UMC_MPEG2_DECODER::MPEG2Decoder> m_decoder;

    bool                                             m_opaque;
    bool                                             m_first_run;

    mfxVideoParamWrapper                             m_init_video_par;
    mfxVideoParamWrapper                             m_first_video_par;
    mfxVideoParamWrapper                             m_video_par;

    mfxFrameAllocResponse                            m_response;
    mfxFrameAllocResponse                            m_response_alien;
    mfxDecodeStat                                    m_stat;
};

#endif // MFX_ENABLE_MPEG2_VIDEO_DECODE
