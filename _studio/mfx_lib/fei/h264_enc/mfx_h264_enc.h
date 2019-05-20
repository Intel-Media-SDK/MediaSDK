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

#ifndef _MFX_H264_FEI_ENC_H_
#define _MFX_H264_FEI_ENC_H_

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_H264_VIDEO_FEI_ENC)

#include "fei_common.h"
#include "mfx_enc_ext.h"

bool bEnc_ENC(mfxVideoParam *par);

class VideoENC_ENC :  public VideoENC_Ext
{
public:

     VideoENC_ENC(VideoCORE *core, mfxStatus *sts);

    // Destructor
    virtual ~VideoENC_ENC(void);

    virtual mfxStatus Init(mfxVideoParam *par) ;
    virtual mfxStatus Reset(mfxVideoParam *par);
    virtual mfxStatus Close(void);
    virtual mfxTaskThreadingPolicy GetThreadingPolicy(void) {return MFX_TASK_THREADING_INTRA;}

    mfxStatus Submit(mfxEncodeInternalParams * iParams);

    mfxStatus QueryStatus(MfxHwH264Encode::DdiTask& task);

    static
    mfxStatus QueryIOSurf(VideoCORE*, mfxVideoParam *par, mfxFrameAllocRequest *request);

    virtual mfxStatus GetVideoParam(mfxVideoParam *par);

    virtual mfxStatus RunFrameVmeENCCheck(mfxENCInput *in,
                                    mfxENCOutput *out,
                                    MFX_ENTRY_POINT pEntryPoints[],
                                    mfxU32 &numEntryPoints);

    virtual mfxStatus RunFrameVmeENC(mfxENCInput *in, mfxENCOutput *out);

protected:
    mfxStatus ProcessAndCheckNewParameters(MfxHwH264Encode::MfxVideoParam & newPar,
                                           bool & isIdrRequired,
                                           mfxVideoParam const * newParIn = 0);

private:

    bool                                          m_bInit;
    VideoCORE*                                    m_core;

    std::unique_ptr<MfxHwH264Encode::DriverEncoder> m_ddi;
    std::vector<mfxU32>                           m_recFrameOrder; // !!! HACK !!!
    MFX_ENCODE_CAPS                               m_caps;

    MfxHwH264Encode::MfxVideoParam                m_video;
    MfxHwH264Encode::MfxVideoParam                m_videoInit; // m_video may change by Reset, m_videoInit doesn't change
    MfxHwH264Encode::PreAllocatedVector           m_sei;

    MfxHwH264Encode::MfxFrameAllocResponse        m_rec;

    std::list<MfxHwH264Encode::DdiTask>           m_free;
    std::list<MfxHwH264Encode::DdiTask>           m_incoming;
    std::list<MfxHwH264Encode::DdiTask>           m_encoding;
    MfxHwH264Encode::DdiTask                      m_prevTask;
    UMC::Mutex                                    m_listMutex;

    mfxU32                                        m_inputFrameType;
    eMFXHWType                                    m_currentPlatform;
    eMFXVAType                                    m_currentVaType;
    bool                                          m_bSingleFieldMode;
    mfxU32                                        m_firstFieldDone;

#if MFX_VERSION < 1023
    std::map<mfxU32, mfxU32>                      m_frameOrder_frameNum;
#endif // MSDK_API < 1023
};

#endif
#endif

