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

#ifndef __MFX_H264_ENCODE_VAAPI__H
#define __MFX_H264_ENCODE_VAAPI__H

#include "mfx_common.h"

#if defined (MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined (MFX_VA_LINUX)

#include "umc_mutex.h"

#include <va/va.h>
#include <va/va_enc_h264.h>
#include "vaapi_ext_interface.h"

#if defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
//#include <va/vendor/va_intel_fei.h>
//#include <va/vendor/va_intel_statistics.h>
#endif

#include "mfx_h264_encode_interface.h"
#include "mfx_h264_encode_hw_utils.h"

uint32_t ConvertRateControlMFX2VAAPI(mfxU8 rateControl);

VAProfile ConvertProfileTypeMFX2VAAPI(mfxU32 type);

mfxStatus SetHRD(
    MfxHwH264Encode::MfxVideoParam const & par,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & hrdBuf_id);

mfxStatus SetQualityParams(
    MfxHwH264Encode::MfxVideoParam const & par,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & qualityParams_id,
    MfxHwH264Encode::DdiTask const * pTask = nullptr);

mfxStatus SetQualityLevel(
    MfxHwH264Encode::MfxVideoParam const & par,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & privateParams_id,
    mfxEncodeCtrl const * pCtrl = 0);

void FillConstPartOfPps(
    MfxHwH264Encode::MfxVideoParam const & par,
    VAEncPictureParameterBufferH264 & pps);

mfxStatus SetFrameRate(
    MfxHwH264Encode::MfxVideoParam const & par,
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    VABufferID & frameRateBuf_id);

#if defined (MFX_ENABLE_H264_ROUNDING_OFFSET)
mfxStatus SetRoundingOffset(
    VADisplay    vaDisplay,
    VAContextID  vaContextEncode,
    mfxExtAVCRoundingOffset const & roundingOffset,
    VABufferID & roundingOffsetBuf_id);
#endif

namespace MfxHwH264Encode
{
    // map feedbackNumber <-> VASurface
    struct ExtVASurface
    {
        VASurfaceID surface = VA_INVALID_SURFACE;
        mfxU32 number       = 0;
        mfxU32 idxBs        = 0;
        mfxU32 size         = 0; // valid only if Surface ID == VA_INVALID_SURFACE (skipped frames)
#if defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC) || defined(MFX_ENABLE_H264_VIDEO_FEI_ENCPAK)
        VASurfaceID mv      = VA_INVALID_ID;
        VASurfaceID mbstat  = VA_INVALID_ID;
        VASurfaceID mbcode  = VA_INVALID_ID;
#endif
    };

    void UpdatePPS(
        DdiTask const & task,
        mfxU32          fieldId,
        VAEncPictureParameterBufferH264 & pps,
        std::vector<ExtVASurface> const & reconQueue);

    void UpdateSlice(
        MFX_ENCODE_CAPS const &                     hwCaps,
        DdiTask const &                             task,
        mfxU32                                      fieldId,
        VAEncSequenceParameterBufferH264 const     & sps,
        VAEncPictureParameterBufferH264 const      & pps,
        std::vector<VAEncSliceParameterBufferH264> & slice,
        MfxVideoParam const                        & par,
        std::vector<ExtVASurface> const & reconQueue);

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
            mfxU32     width,
            mfxU32     height,
            bool       isTemporal = false) override;

        virtual
        mfxStatus CreateAccelerationService(
            MfxVideoParam const & par) override;

        virtual
        mfxStatus Reset(
            MfxVideoParam const & par) override;

        // 2 -> 1
        virtual
        mfxStatus Register(
            mfxFrameAllocResponse& response,
            D3DDDIFORMAT type) override;

        // (mfxExecuteBuffers& data)
        virtual
        mfxStatus Execute(
            mfxHDLPair      pair,
            DdiTask const & task,
            mfxU32          fieldId,
            PreAllocatedVector const & sei) override;

        // recomendation from HW
        virtual
        mfxStatus QueryCompBufferInfo(
            D3DDDIFORMAT type,
            mfxFrameAllocRequest& request) override;

        virtual
        mfxStatus QueryEncodeCaps(
            MFX_ENCODE_CAPS& caps) override;

        virtual
        mfxStatus QueryMbPerSec(
            mfxVideoParam const & par,
            mfxU32              (&mbPerSec)[16]) override;

        virtual
        mfxStatus QueryStatus(
            DdiTask & task,
            mfxU32    fieldId) override;

        virtual
        mfxStatus QueryStatusFEI(
            DdiTask const & task,
            mfxU32  feiFieldId,
            ExtVASurface const & curFeedback,
            mfxU32 codedStatus);

        virtual
        mfxStatus Destroy() override;

        void ForceCodingFunction (mfxU16 /*codingFunction*/) override
        {
            // no need in it on Linux
        }

        virtual
        mfxStatus QueryHWGUID(
            VideoCORE * core,
            GUID        guid,
            bool        isTemporal) override;

        VAAPIEncoder(const VAAPIEncoder&) = delete;
        VAAPIEncoder& operator=(const VAAPIEncoder&) = delete;

    protected:
        void FillSps( MfxVideoParam const & par, VAEncSequenceParameterBufferH264 & sps);

        VideoCORE*    m_core;
        MfxVideoParam m_videoParam;

        // encoder specific. can be encapsulated by auxDevice class
        VADisplay    m_vaDisplay;
        VAContextID  m_vaContextEncode;
        VAConfigID   m_vaConfig;

        // encode params (extended structures)
        VAEncSequenceParameterBufferH264 m_sps;
        VAEncPictureParameterBufferH264  m_pps;
        VAContextParameterUpdateBuffer   m_priorityBuffer;
        std::vector<VAEncSliceParameterBufferH264> m_slice;

        // encode buffer to send vaRender()
        VABufferID m_spsBufferId;
        VABufferID m_hrdBufferId;
        VABufferID m_rateParamBufferId;         // VAEncMiscParameterRateControl
        VABufferID m_frameRateId;               // VAEncMiscParameterFrameRate
        VABufferID m_qualityLevelId;            // VAEncMiscParameterBufferQualityLevel
        VABufferID m_maxFrameSizeId;            // VAEncMiscParameterFrameRate
        VABufferID m_multiPassFrameSizeId;      // VAEncMiscParameterBufferMultiPassFrameSize
        VABufferID m_quantizationId;            // VAEncMiscParameterQuantization
        VABufferID m_rirId;                     // VAEncMiscParameterRIR
        VABufferID m_qualityParamsId;           // VAEncMiscParameterEncQuality
        VABufferID m_miscParameterSkipBufferId; // VAEncMiscParameterSkipFrame
        VABufferID m_maxSliceSizeId;            // VAEncMiscParameterMaxSliceSize
#if defined (MFX_ENABLE_H264_ROUNDING_OFFSET)
        VABufferID m_roundingOffsetId;          // VAEncMiscParameterCustomRoundingControl
#endif
        VABufferID m_roiBufferId;
        VABufferID m_ppsBufferId;
        VABufferID m_mbqpBufferId;
        VABufferID m_mbNoSkipBufferId;
        std::vector<VABufferID> m_sliceBufferId;

        VABufferID m_packedAudHeaderBufferId;
        VABufferID m_packedAudBufferId;
        VABufferID m_packedSpsHeaderBufferId;
        VABufferID m_packedSpsBufferId;
        VABufferID m_packedPpsHeaderBufferId;
        VABufferID m_packedPpsBufferId;
        VABufferID m_packedSeiHeaderBufferId;
        VABufferID m_packedSeiBufferId;
        VABufferID m_packedSkippedSliceHeaderBufferId;
        VABufferID m_packedSkippedSliceBufferId;
        VABufferID m_priorityBufferId;
        std::vector<VABufferID> m_packedSliceHeaderBufferId;
        std::vector<VABufferID> m_packedSliceBufferId;
        std::vector<VABufferID> m_packedSvcPrefixHeaderBufferId;
        std::vector<VABufferID> m_packedSvcPrefixBufferId;
        std::vector<VABufferID> m_vaFeiMBStatId;
        std::vector<VABufferID> m_vaFeiMVOutId;
        std::vector<VABufferID> m_vaFeiMCODEOutId;

        // The following 3 members are used in pair with 3 above,
        // to indicate the size of allocated buf.
        std::vector<mfxU32> m_vaFeiMBStatBufSize;
        std::vector<mfxU32> m_vaFeiMVOutBufSize;
        std::vector<mfxU32> m_vaFeiMCODEOutBufSize;

        std::vector<ExtVASurface> m_feedbackCache;
        std::vector<ExtVASurface> m_bsQueue;
        std::vector<ExtVASurface> m_reconQueue;

        mfxU32 m_width;
        mfxU32 m_height;
        mfxU32 m_userMaxFrameSize;  // current MaxFrameSize from user.
        mfxU32 m_mbbrc;
        MFX_ENCODE_CAPS m_caps;
/*
 * Current RollingIntraRefresh state, as it came through the task state and passing to DDI in PPS
 * for Windows we keep it here to send update by VAMapBuffer as happened.
 */
        IntraRefreshState m_RIRState;

        mfxU32            m_curTrellisQuantization;   // mapping in accordance with libva
        mfxU32            m_newTrellisQuantization;   // will be sent through config.

        std::vector<VAEncROI> m_arrayVAEncROI;

        static const mfxU32 MAX_CONFIG_BUFFERS_COUNT = 30 + 5; //added FEI buffers

        UMC::Mutex m_guard;
        HeaderPacker m_headerPacker;

        // SkipFlag
        enum {
            NO_SKIP,
            NORMAL_MODE,
        };

        mfxU8  m_numSkipFrames;
        mfxU32 m_sizeSkipFrames;
        mfxU32 m_skipMode;
        bool m_isENCPAK;

        bool                           m_isBrcResetRequired;
        VAEncMiscParameterRateControl  m_vaBrcPar;
        VAEncMiscParameterFrameRate    m_vaFrameRate;

        std::vector<VAEncQPBufferH264> m_mbqp_buffer;

        std::vector<mfxU8>             m_mb_noskip_buffer;
#ifdef MFX_ENABLE_MFE
        MFEVAAPIEncoder*               m_mfe;
#endif
        mfxU32                         m_MaxContextPriority;
    };

    //extend encoder to FEI interface
#if defined(MFX_ENABLE_H264_VIDEO_FEI_PREENC)
    //typedef std::pair<VASurfaceID, VABufferID> SurfaceAndBufferPair;

    class VAAPIFEIPREENCEncoder : public VAAPIEncoder
    {
    public:
        VAAPIFEIPREENCEncoder();

        virtual
        ~VAAPIFEIPREENCEncoder();

        virtual mfxStatus CreateAccelerationService(MfxVideoParam const & par);
        virtual mfxStatus Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type);
        virtual mfxStatus Execute(mfxHDLPair pair, DdiTask const & task,
                mfxU32 fieldId, PreAllocatedVector const & sei);
        virtual mfxStatus QueryStatus(DdiTask & task, mfxU32 fieldId);
        virtual mfxStatus Destroy();

    protected:
        //helper functions
        mfxStatus CreatePREENCAccelerationService(MfxVideoParam const & par);

        mfxI32 m_codingFunction;

        VABufferID m_statParamsId;
        std::vector<VABufferID> m_statMVId;
        std::vector<VABufferID> m_statOutId;

        std::vector<ExtVASurface> m_statFeedbackCache;
        std::vector<ExtVASurface> m_inputQueue;
        //std::vector <SurfaceAndBufferPair> m_statPairs;
    };
#endif

#if defined(MFX_ENABLE_H264_VIDEO_FEI_ENC)
    class VAAPIFEIENCEncoder : public VAAPIEncoder
    {
    public:
        VAAPIFEIENCEncoder();

        virtual
        ~VAAPIFEIENCEncoder();

        virtual mfxStatus CreateAccelerationService(MfxVideoParam const & par);
        virtual mfxStatus Reset(MfxVideoParam const & par);
        virtual mfxStatus Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type);
        virtual mfxStatus Execute(mfxHDLPair pair, DdiTask const & task,
                mfxU32 fieldId, PreAllocatedVector const & sei);
        virtual mfxStatus QueryStatus(DdiTask & task, mfxU32 fieldId);
        virtual mfxStatus Destroy();

    protected:
        //helper functions
        mfxStatus CreateENCAccelerationService(MfxVideoParam const & par);

        mfxI32 m_codingFunction;

        VABufferID m_statParamsId;
        VABufferID m_statMVId;
        VABufferID m_statOutId;

        std::vector<ExtVASurface> m_statFeedbackCache;
        std::vector<ExtVASurface> m_inputQueue;

        VABufferID m_codedBufferId;
    };
#endif

#if defined(MFX_ENABLE_H264_VIDEO_FEI_PAK) && defined(MFX_ENABLE_H264_VIDEO_FEI_ENC)
    class VAAPIFEIPAKEncoder : public VAAPIEncoder
    {
    public:
        VAAPIFEIPAKEncoder();

        virtual
        ~VAAPIFEIPAKEncoder();

        virtual mfxStatus CreateAccelerationService(MfxVideoParam const & par);
        virtual mfxStatus Reset(MfxVideoParam const & par);
        virtual mfxStatus Register(mfxFrameAllocResponse& response, D3DDDIFORMAT type);
        virtual mfxStatus Execute(mfxHDLPair pair, DdiTask const & task,
                mfxU32 fieldId, PreAllocatedVector const & sei);
        virtual mfxStatus QueryStatus(DdiTask & task, mfxU32 fieldId);
        virtual mfxStatus Destroy();

    protected:
        //helper functions
        mfxStatus CreatePAKAccelerationService(MfxVideoParam const & par);

        mfxI32 m_codingFunction;

        VABufferID m_statParamsId;
        VABufferID m_statMVId;
        VABufferID m_statOutId;

        std::vector<ExtVASurface> m_statFeedbackCache;
        std::vector<ExtVASurface> m_inputQueue;

        VABufferID m_codedBufferId[2];
    };
#endif


}; // namespace

#endif // MFX_ENABLE_H264_VIDEO_ENCODE && (MFX_VA_LINUX)
#endif // __MFX_H264_ENCODE_VAAPI__H
/* EOF */
