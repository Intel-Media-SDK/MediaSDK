// Copyright (c) 2017-2021 Intel Corporation
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

#ifndef __MFX_MPEG2_ENCODE_VAAPI__H
#define __MFX_MPEG2_ENCODE_VAAPI__H

#include "mfx_common.h"

#if defined (MFX_VA_LINUX)
#if defined(MFX_ENABLE_MPEG2_VIDEO_ENCODE)

#include <vector>
#include <assert.h>

#include "mfx_h264_encode_struct_vaapi.h"
#include <va/va.h>
#include <va/va_enc_mpeg2.h>
#include "vaapi_ext_interface.h"

#include "mfx_ext_buffers.h"

#include "mfx_mpeg2_enc_common_hw.h"
#include "mfx_mpeg2_encode_interface.h"
#include "umc_mutex.h"


namespace MfxHwMpeg2Encode
{

    class VAAPIEncoder : public DriverEncoder
    {
    public:
        enum { MAX_SLICES = 128 };

        explicit VAAPIEncoder(VideoCORE* core);

        virtual
        ~VAAPIEncoder();

        virtual
        void QueryEncodeCaps(ENCODE_CAPS & caps) override;

        virtual
        mfxStatus Init(ExecuteBuffers* pExecuteBuffers, mfxU32 numRefFrames, mfxU32 funcId) override;

        virtual
        mfxStatus CreateContext(ExecuteBuffers* pExecuteBuffers, mfxU32 numRefFrames, mfxU32 funcId) override;

        virtual
        mfxStatus Execute(ExecuteBuffers* pExecuteBuffers, mfxU8* pUserData = 0, mfxU32 userDataLen = 0) override;

        virtual
        mfxStatus Close() override;

        virtual
        bool      IsFullEncode() const override { return true; }

        virtual
        mfxStatus RegisterRefFrames(const mfxFrameAllocResponse* pResponse) override;

        virtual
        mfxStatus FillMBBufferPointer(ExecuteBuffers* pExecuteBuffers) override;

        virtual
        mfxStatus FillBSBuffer(mfxU32 nFeedback,mfxU32 nBitstream, mfxBitstream* pBitstream, Encryption *pEncrypt) override;

        virtual
        mfxStatus SetFrames (ExecuteBuffers* pExecuteBuffers) override;

        virtual
        mfxStatus CreateAuxilliaryDevice(mfxU16 codecProfile) override;

        VAAPIEncoder(const VAAPIEncoder&) = delete;
        VAAPIEncoder& operator=(const VAAPIEncoder&) = delete;

    private:
        struct VAEncQpBufferMPEG2 {
            mfxU32 qp_y;
        };

        mfxStatus QueryCompBufferInfo(D3DDDIFORMAT type, mfxFrameAllocRequest* pRequest, ExecuteBuffers* pExecuteBuffers);
        mfxStatus CreateCompBuffers  (ExecuteBuffers* pExecuteBuffers, mfxU32 numRefFrames);
        mfxStatus CreateBSBuffer      (mfxU32 numRefFrames, ExecuteBuffers* pExecuteBuffers);

        mfxStatus GetBuffersInfo();
        mfxStatus QueryMbDataLayout();
        mfxStatus Init(ENCODE_FUNC func, ExecuteBuffers* pExecuteBuffers);
        mfxStatus FillSlices(ExecuteBuffers* pExecuteBuffers);
        mfxStatus FillMiscParameterBuffer(ExecuteBuffers* pExecuteBuffers);
        mfxStatus FillQualityLevelBuffer(ExecuteBuffers* pExecuteBuffers);
        mfxStatus FillUserDataBuffer(mfxU8 *pUserData, mfxU32 userDataLen);
        mfxStatus FillVideoSignalInfoBuffer(ExecuteBuffers* pExecuteBuffers);
        mfxStatus FillMBQPBuffer(ExecuteBuffers* pExecuteBuffers, mfxU8* mbqp, mfxU32 numMB);
        mfxStatus FillSkipFrameBuffer(mfxU8 skipFlag);

        mfxStatus Execute(ExecuteBuffers* pExecuteBuffers, mfxU32 func, mfxU8* pUserData, mfxU32 userDataLen);
        mfxStatus Register (const mfxFrameAllocResponse* pResponse, D3DDDIFORMAT type);
        mfxI32    GetRecFrameIndex (mfxMemId memID);
        mfxI32    GetRawFrameIndex (mfxMemId memIDe, bool bAddFrames);
        mfxStatus FillPriorityBuffer(mfxPriority&);


        VideoCORE*                          m_core;

        // encoder specific. can be encapsulated by auxDevice class
        VADisplay                           m_vaDisplay;
        VAContextID                         m_vaContextEncode;
        VAConfigID                          m_vaConfig;
        VAEncSequenceParameterBufferMPEG2   m_vaSpsBuf;
        VABufferID                          m_spsBufferId;
        VAEncPictureParameterBufferMPEG2    m_vaPpsBuf;
        VABufferID                          m_ppsBufferId;
        VABufferID                          m_qmBufferId;
        VAEncSliceParameterBufferMPEG2      m_sliceParam[MAX_SLICES];
        VABufferID                          m_sliceParamBufferId[MAX_SLICES];  /* Slice level parameter, multi slices */
        int                                 m_numSliceGroups;
        mfxU32                              m_codedbufISize;
        mfxU32                              m_codedbufPBSize;

        VAEncMiscParameterBuffer           *m_pMiscParamsFps;
        VAEncMiscParameterBuffer           *m_pMiscParamsBrc;
        VAEncMiscParameterBuffer           *m_pMiscParamsQuality;
        VAEncMiscParameterBuffer           *m_pMiscParamsSeqInfo;
        VAEncMiscParameterBuffer           *m_pMiscParamsSkipFrame;

        VABufferID                          m_miscParamFpsId;
        VABufferID                          m_miscParamBrcId;
        VABufferID                          m_miscParamQualityId;
        VABufferID                          m_miscParamSeqInfoId;
        VABufferID                          m_miscParamSkipFrameId;
        VABufferID                          m_packedUserDataParamsId;
        VABufferID                          m_packedUserDataId;
        VABufferID                          m_mbqpBufferId;
        VABufferID                          m_miscQualityParamId;
        std::vector<VAEncQpBufferMPEG2>     m_mbqpDataBuffer;
        VABufferID                          m_priorityBufferId;
        VAContextParameterUpdateBuffer      m_priorityBuffer;

        mfxU32                              m_MaxContextPriority;

        mfxU16                              m_initFrameWidth;
        mfxU16                              m_initFrameHeight;

        ENCODE_MBDATA_LAYOUT m_layout;

        mfxFeedback                         m_feedback;
        std::vector<ExtVASurface>           m_bsQueue;
        std::vector<ExtVASurface>           m_reconQueue;


        mfxFrameAllocResponse               m_allocResponseMB;
        mfxFrameAllocResponse               m_allocResponseBS;
        mfxRecFrames                        m_recFrames;
        mfxRawFrames                        m_rawFrames;

        UMC::Mutex                          m_guard;
        ENCODE_CAPS                         m_caps;
    }; // class VAAPIEncoder


}; // namespace

#endif // defined(MFX_ENABLE_MPEG2_VIDEO_ENCODE) || defined(MFX_ENABLE_MPEG2_VIDEO_ENC)
#endif // defined (MFX_VA_LINUX)
#endif // __MFX_MPEG2_ENCODE_VAAPI__H
/* EOF */
