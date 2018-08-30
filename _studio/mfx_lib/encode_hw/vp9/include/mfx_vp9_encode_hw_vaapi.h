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


#include "mfx_vp9_encode_hw_ddi.h"

namespace MfxHwVP9Encode
{
#if defined (MFX_VA_LINUX)
#include <va/va.h>
#include <va/va_enc_vp9.h>

#define MFX_DESTROY_VABUFFER(vaBufferId, vaDisplay)    \
do {                                               \
    if (vaBufferId != VA_INVALID_ID)               \
    {                                              \
        vaDestroyBuffer(vaDisplay, vaBufferId);    \
        vaBufferId = VA_INVALID_ID;                \
    }                                              \
} while (0)

    enum {
        MFX_FOURCC_VP9_NV12    = MFX_MAKEFOURCC('V','P','8','N'),
        MFX_FOURCC_VP9_SEGMAP  = MFX_MAKEFOURCC('V','P','8','S'),
    };

    typedef struct
    {
        VASurfaceID surface;
        mfxU32 number;
        mfxU32 idxBs;

    } ExtVASurface;

    /* Convert MediaSDK into DDI */

    void FillSpsBuffer(mfxVideoParam const & par,
        VAEncSequenceParameterBufferVP9 & sps);

    mfxStatus FillPpsBuffer(Task const & task,
        mfxVideoParam const & par,
        VAEncPictureParameterBufferVP9 & pps,
        std::vector<ExtVASurface> const & reconQueue,
        BitOffsets const &offsets);

    class VAAPIEncoder : public DriverEncoder
    {
    public:
        VAAPIEncoder();

        virtual
        ~VAAPIEncoder();

        virtual
        mfxStatus CreateAuxilliaryDevice(
            mfxCoreInterface* core,
            GUID       guid,
            mfxU32     width,
            mfxU32     height);

        virtual
        mfxStatus CreateAccelerationService(
            VP9MfxVideoParam const & par);

        virtual
        mfxStatus Reset(
            VP9MfxVideoParam const & par);

        // empty  for Lin
        virtual
        mfxStatus Register(
            mfxMemId memId,
            D3DDDIFORMAT type);

        // 2 -> 1
        virtual
        mfxStatus Register(
            mfxFrameAllocResponse& response,
            D3DDDIFORMAT type);

        // (mfxExecuteBuffers& data)
        virtual
        mfxStatus Execute(
            Task const &task,
            mfxHDLPair pair);

        // recomendation from HW
        virtual
        mfxStatus QueryCompBufferInfo(
            D3DDDIFORMAT type,
            mfxFrameAllocRequest& request,
            mfxU32 frameWidth,
            mfxU32 frameHeight);

        virtual
        mfxStatus QueryEncodeCaps(
            ENCODE_CAPS_VP9& caps);

        virtual
        mfxStatus QueryPlatform(
            mfxPlatform& platform);

        virtual
        mfxStatus QueryStatus(
            Task & task);

        virtual
            mfxU32 GetReconSurfFourCC();

        virtual
        mfxStatus Destroy();

    private:
        VAAPIEncoder(const VAAPIEncoder&); // no implementation
        VAAPIEncoder& operator=(const VAAPIEncoder&); // no implementation

        mfxCoreInterface*  m_pmfxCore;
        VP9MfxVideoParam    m_video;

        // encoder specific. can be encapsulated by auxDevice class
        VADisplay    m_vaDisplay;
        VAContextID  m_vaContextEncode;
        VAConfigID   m_vaConfig;

        // encode params (extended structures)
        VAEncSequenceParameterBufferVP9             m_sps;
        VAEncPictureParameterBufferVP9              m_pps;
        VAEncMiscParameterTypeVP9PerSegmantParam    m_segPar;
        VAEncMiscParameterRateControl               m_vaBrcPar;
        VAEncMiscParameterFrameRate                 m_vaFrameRate;

        VP9SeqLevelParam                            m_seqParam;

        // encode buffer to send vaRender()
        VABufferID m_spsBufferId;
        VABufferID m_ppsBufferId;
        VABufferID m_segMapBufferId;
        VABufferID m_segParBufferId;
        VABufferID m_frameRateBufferId;
        VABufferID m_rateCtrlBufferId;
        VABufferID m_hrdBufferId;
        VABufferID m_qualityLevelBufferId;
        VABufferID m_packedHeaderParameterBufferId;
        VABufferID m_packedHeaderDataBufferId;

        std::vector<ExtVASurface> m_feedbackCache;
        std::vector<ExtVASurface> m_reconQueue;
        std::vector<ExtVASurface> m_segMapQueue;
        std::vector<ExtVASurface> m_bsQueue;

        std::vector<mfxU8> m_frameHeaderBuf;

        static const mfxU32 MAX_CONFIG_BUFFERS_COUNT = 10; // sps, pps, bitstream, uncomp header, segment map, per-segment parameters, frame rate, rate ctrl, hrd, quality level

        mfxU32 m_width;
        mfxU32 m_height;
        bool m_isBrcResetRequired;

        ENCODE_CAPS_VP9 m_caps;
        mfxPlatform m_platform;

        UMC::Mutex                      m_guard;
    };
#endif // (MFX_VA_LINUX)
} // MfxHwVP9Encode

