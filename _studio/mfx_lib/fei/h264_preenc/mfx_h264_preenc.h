// Copyright (c) 2018-2019 Intel Corporation
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

#ifndef _MFX_H264_PREENC_H_
#define _MFX_H264_PREENC_H_

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)

#include "mfx_enc_ext.h"
#include "mfx_h264_encode_hw_utils.h"
#include "mfxfei.h"

#include <memory>
#include <list>
#include <vector>

bool bEnc_PREENC(mfxVideoParam *par);

class VideoENC_PREENC:  public VideoENC_Ext
{
public:

     VideoENC_PREENC(VideoCORE *core, mfxStatus *sts);

    // Destructor
    virtual ~VideoENC_PREENC(void);

    virtual mfxStatus Init(mfxVideoParam *par);
    virtual mfxStatus Reset(mfxVideoParam *par);
    virtual mfxStatus Close(void);
    virtual mfxTaskThreadingPolicy GetThreadingPolicy(void) {return MFX_TASK_THREADING_INTRA;}

    mfxStatus Submit(mfxEncodeInternalParams * iParams);

    mfxStatus QueryStatus(MfxHwH264Encode::DdiTask& task);

    static mfxStatus Query(VideoCORE*, mfxVideoParam *in, mfxVideoParam *out);
    static mfxStatus QueryIOSurf(VideoCORE*, mfxVideoParam *par, mfxFrameAllocRequest *request);

    virtual mfxStatus GetVideoParam(mfxVideoParam *par);

    virtual mfxStatus RunFrameVmeENCCheck(mfxENCInput  *in,
                                          mfxENCOutput *out,
                                          MFX_ENTRY_POINT pEntryPoints[],
                                          mfxU32 &numEntryPoints);

    virtual mfxStatus RunFrameVmeENC(mfxENCInput *in, mfxENCOutput *out);

private:

    bool                                          m_bInit;
    VideoCORE*                                    m_core;

    std::list <mfxFrameSurface1*>                 m_SurfacesForOutput;

    std::unique_ptr<MfxHwH264Encode::DriverEncoder> m_ddi;
    MFX_ENCODE_CAPS                               m_caps;

    MfxHwH264Encode::MfxVideoParam                m_video;
    MfxHwH264Encode::PreAllocatedVector           m_sei;

    MfxHwH264Encode::MfxFrameAllocResponse        m_raw;
    MfxHwH264Encode::MfxFrameAllocResponse        m_opaqHren;

    std::list<MfxHwH264Encode::DdiTask>           m_free;
    std::list<MfxHwH264Encode::DdiTask>           m_incoming;
    std::list<MfxHwH264Encode::DdiTask>           m_encoding;
    UMC::Mutex                                    m_listMutex;

    mfxU32                                        m_inputFrameType;
    eMFXHWType                                    m_currentPlatform;
    eMFXVAType                                    m_currentVaType;
    bool                                          m_bSingleFieldMode;
    mfxU32                                        m_firstFieldDone;
};

#endif
#endif

