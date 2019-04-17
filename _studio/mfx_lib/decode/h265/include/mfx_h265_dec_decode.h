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

#include "mfx_common.h"
#if defined (MFX_ENABLE_H265_VIDEO_DECODE)

#include "umc_defs.h"

#ifndef _MFX_H265_DEC_DECODE_H_
#define _MFX_H265_DEC_DECODE_H_

#include "mfx_common_int.h"
#include "umc_video_decoder.h"
#include "mfx_umc_alloc_wrapper.h"

#include "umc_mutex.h"
#include <queue>
#include <list>
#include <memory>

#include "mfx_task.h"
#include "mfxpcp.h"

namespace UMC
{
    class VideoData;
}

namespace UMC_HEVC_DECODER
{
    class MFXTaskSupplier_H265;
    class VATaskSupplier;
    class H265DecoderFrame;
}

typedef UMC_HEVC_DECODER::VATaskSupplier  MFX_AVC_Decoder_H265;

class VideoDECODE;
// HEVC decoder interface class
class VideoDECODEH265 : public VideoDECODE
{
public:
    // MediaSDK DECODE_Query API function
    static mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out);
    // MediaSDK DECODE_QueryIOSurf API function
    static mfxStatus QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request);
    // Decode bitstream header and exctract parameters from it
    static mfxStatus DecodeHeader(VideoCORE *core, mfxBitstream *bs, mfxVideoParam *par);

    VideoDECODEH265(VideoCORE *core, mfxStatus * sts);
    virtual ~VideoDECODEH265(void);

    // Initialize decoder instance
    mfxStatus Init(mfxVideoParam *par);
    // Reset decoder with new parameters
    virtual mfxStatus Reset(mfxVideoParam *par);
    // Free decoder resources
    virtual mfxStatus Close(void);
    // Returns decoder threading mode
    virtual mfxTaskThreadingPolicy GetThreadingPolicy(void);

    // MediaSDK DECODE_GetVideoParam API function
    virtual mfxStatus GetVideoParam(mfxVideoParam *par);
    // MediaSDK DECODE_GetDecodeStat API function
    virtual mfxStatus GetDecodeStat(mfxDecodeStat *stat);
    // Initialize threads callbacks
    virtual mfxStatus DecodeFrameCheck(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, MFX_ENTRY_POINT *pEntryPoint);
    // Wait until a frame is ready to be output and set necessary surface flags
    virtual mfxStatus DecodeFrame(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 *surface_out);
    // Returns closed caption data
    virtual mfxStatus GetUserData(mfxU8 *ud, mfxU32 *sz, mfxU64 *ts);
    // Returns stored SEI messages
    virtual mfxStatus GetPayload(mfxU64 *ts, mfxPayload *payload);
    // MediaSDK DECODE_SetSkipMode API function
    virtual mfxStatus SetSkipMode(mfxSkipMode mode);

    // Decoder instance threads entry point. Do async tasks here
    mfxStatus RunThread(void * params, mfxU32 threadNumber);

protected:
    // Actually calculate needed frames number
    static mfxStatus QueryIOSurfInternal(eMFXPlatform platform, eMFXHWType type, mfxVideoParam *par, mfxFrameAllocRequest *request);

    // Check if new parameters are compatible with new parameters
    bool IsSameVideoParam(mfxVideoParam * newPar, mfxVideoParam * oldPar, eMFXHWType type);

    // Fill up frame parameters before returning it to application
    void FillOutputSurface(mfxFrameSurface1 **surface_out, mfxFrameSurface1 *surface_work, UMC_HEVC_DECODER::H265DecoderFrame * pFrame);
    // Find a next frame ready to be output from decoder
    UMC_HEVC_DECODER::H265DecoderFrame * GetFrameToDisplay_H265(bool force);

    // Wait until a frame is ready to be output and set necessary surface flags
    mfxStatus DecodeFrame(mfxFrameSurface1 *surface_out, UMC_HEVC_DECODER::H265DecoderFrame * pFrame = 0);

    // Check if there is enough data to start decoding in async mode
    mfxStatus DecodeFrameCheck(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out);

    // Fill up resolution information if new header arrived
    void FillVideoParam(mfxVideoParamWrapper *par, bool full);

    // Fill up frame allocator request data
    mfxStatus UpdateAllocRequest(mfxVideoParam *par,
                                mfxFrameAllocRequest *request,
                                mfxExtOpaqueSurfaceAlloc * &pOpaqAlloc,
                                bool &mapping);

    // Get original Surface corresponding to OpaqueSurface
    mfxFrameSurface1 * GetOriginalSurface(mfxFrameSurface1 *surface);

    std::unique_ptr<UMC_HEVC_DECODER::MFXTaskSupplier_H265>  m_pH265VideoDecoder;
    mfx_UMC_MemAllocator                                     m_MemoryAllocator;

    std::unique_ptr<mfx_UMC_FrameAllocator>                  m_FrameAllocator;

    mfxVideoParamWrapper m_vInitPar;
    mfxVideoParamWrapper m_vFirstPar;
    mfxVideoParamWrapper m_vPar;
    ExtendedBuffer m_extBuffers;

    VideoCORE * m_core;

    bool    m_isInit;
    bool    m_isOpaq;
    bool    m_globalTask;

    mfxU16  m_frameOrder;

    mfxFrameAllocResponse m_response;
    mfxFrameAllocResponse m_response_alien;
    mfxDecodeStat m_stat;
    eMFXPlatform m_platform;

    UMC::Mutex m_mGuard;
    UMC::Mutex m_mGuardRunThread;
    bool m_useDelayedDisplay;

    UMC::VideoAccelerator * m_va;
    enum
    {
        NUMBER_OF_ADDITIONAL_FRAMES = 10
    };

    bool m_isFirstRun;
};

#endif // _MFX_H265_DEC_DECODE_H_
#endif
