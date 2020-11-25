// Copyright (c) 2018-2020 Intel Corporation
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
            VideoCORE* core,
            GUID       guid,
            VP9MfxVideoParam const & par) override;

        virtual
        mfxStatus CreateAccelerationService(
            VP9MfxVideoParam const & par) override;

        virtual
        mfxStatus Reset(
            VP9MfxVideoParam const & par) override;

        // 2 -> 1
        virtual
        mfxStatus Register(
            mfxFrameAllocResponse& response,
            D3DDDIFORMAT type) override;

        // (mfxExecuteBuffers& data)
        virtual
        mfxStatus Execute(
            Task const &task,
            mfxHDLPair pair) override;

        // recommendation from HW
        virtual
        mfxStatus QueryCompBufferInfo(
            D3DDDIFORMAT type,
            mfxFrameAllocRequest& request,
            mfxU32 frameWidth,
            mfxU32 frameHeight) override;

        virtual
        mfxStatus QueryEncodeCaps(
            ENCODE_CAPS_VP9& caps) override;

        virtual
        mfxStatus QueryPlatform(
            eMFXHWType& platform);

        virtual
        mfxStatus QueryStatus(
            Task & task) override;

        virtual
            mfxU32 GetReconSurfFourCC() override;

        virtual
        mfxStatus Destroy() override;

        VAAPIEncoder(const VAAPIEncoder&) = delete;
        VAAPIEncoder& operator=(const VAAPIEncoder&) = delete;

    private:
        VideoCORE*  m_pmfxCore;
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
        VAContextParameterUpdateBuffer              m_priorityBuffer;

        VP9SeqLevelParam                            m_seqParam;

        // encode buffer to send vaRender()
        VABufferID m_spsBufferId;
        VABufferID m_ppsBufferId;
        VABufferID m_segMapBufferId;
        VABufferID m_segParBufferId;
        VABufferID m_hrdBufferId;
        VABufferID m_qualityLevelBufferId;
        VABufferID m_packedHeaderParameterBufferId;
        VABufferID m_packedHeaderDataBufferId;
        VABufferID m_priorityBufferId;

        // max number of temp layers is 8, but now supported only 4
        VABufferID m_tempLayersBufferId;
        bool       m_tempLayersParamsReset;
        std::vector<VABufferID> m_frameRateBufferIds; // individual buffer for every temporal layer
        std::vector<VABufferID> m_rateCtrlBufferIds;  // individual buffer for every temporal layer

        std::vector<ExtVASurface> m_feedbackCache;
        std::vector<ExtVASurface> m_reconQueue;
        std::vector<ExtVASurface> m_segMapQueue;
        std::vector<ExtVASurface> m_bsQueue;

        std::vector<mfxU8> m_frameHeaderBuf;

        static const mfxU32 MAX_CONFIG_BUFFERS_COUNT = 27; // sps, pps, bitstream, uncomp header, segment map, per-segment parameters, temp layers, frame rate(up to 8), rate ctrl(up to 8), hrd, quality level

        mfxU32 m_width;
        mfxU32 m_height;
        bool m_isBrcResetRequired;

        ENCODE_CAPS_VP9 m_caps;
        eMFXHWType m_platform;
        mfxU32 m_MaxContextPriority;

        UMC::Mutex                      m_guard;
    };
#endif // (MFX_VA_LINUX)
} // MfxHwVP9Encode

