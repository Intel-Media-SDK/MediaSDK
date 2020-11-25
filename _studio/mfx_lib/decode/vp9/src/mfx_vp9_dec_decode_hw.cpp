// Copyright (c) 2017-2020 Intel Corporation
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
#include "mfxvideo++int.h"

#if defined(MFX_ENABLE_VP9_VIDEO_DECODE_HW)

#include "mfx_session.h"
#include "mfx_vp9_dec_decode.h"
#include "mfx_vp9_dec_decode_hw.h"

#include "umc_vp9_utils.h"
#include "umc_vp9_bitstream.h"
#include "umc_vp9_va_packer.h"

#include "mfx_common_decode_int.h"
#include "mfx_vpx_dec_common.h"
#include <libmfx_core_vaapi.h>


using namespace UMC_VP9_DECODER;

static bool IsSameVideoParam(mfxVideoParam *newPar, mfxVideoParam *oldPar);

inline
bool CheckHardwareSupport(VideoCORE *p_core, mfxVideoParam *p_video_param)
{
    MFX_CHECK(p_core, false);
    MFX_CHECK(p_video_param, false);

    GUID guid;

    mfxU32 profile = p_video_param->mfx.CodecProfile;

    if (!profile)
    {
        profile = UMC_VP9_DECODER::GetMinProfile(p_video_param->mfx.FrameInfo.BitDepthLuma, p_video_param->mfx.FrameInfo.ChromaFormat);
    }

    switch(profile)
    {
    case MFX_PROFILE_VP9_0:
    {
        guid = DXVA_Intel_ModeVP9_Profile0_VLD;
        break;
    }
    case MFX_PROFILE_VP9_1:
    {
        guid = DXVA_Intel_ModeVP9_Profile1_YUV444_VLD;
        break;
    }
    case MFX_PROFILE_VP9_2:
    {
        guid = DXVA_Intel_ModeVP9_Profile2_10bit_VLD;
        break;
    }
    case MFX_PROFILE_VP9_3:
    {
        guid = DXVA_Intel_ModeVP9_Profile3_YUV444_10bit_VLD;
        break;
    }
    default: return false;
    }

    mfxStatus mfxSts = p_core->IsGuidSupported(guid, p_video_param);
    MFX_CHECK(mfxSts == MFX_ERR_NONE, false);

    return true;
}

static void SetFrameType(const VP9DecoderFrame &frameInfo, mfxFrameSurface1 &surface_out)
{
    auto extFrameInfo = reinterpret_cast<mfxExtDecodedFrameInfo *>(GetExtendedBuffer(surface_out.Data.ExtParam, surface_out.Data.NumExtParam, MFX_EXTBUFF_DECODED_FRAME_INFO));
    if (extFrameInfo == nullptr)
        return;

    switch (frameInfo.frameType)
    {
        case KEY_FRAME:
            extFrameInfo->FrameType = MFX_FRAMETYPE_I;
            break;
        case INTER_FRAME:
            extFrameInfo->FrameType = MFX_FRAMETYPE_P;
            break;
        default:
            extFrameInfo->FrameType = MFX_FRAMETYPE_UNKNOWN;
    }
}

mfxStatus VideoDECODEVP9_HW::UpdateRefFrames()
{
    if (m_frameInfo.show_existing_frame)
        return MFX_ERR_NONE;

    for (mfxI32 ref_index = 0; ref_index < NUM_REF_FRAMES; ++ref_index)
    {
        mfxU8 mask = m_frameInfo.refreshFrameFlags & (1 << ref_index);
        if (mask != 0 // we should update the reference according to the bitstream data

            // The next condition is for decoder robustness.
            // After the first keyframe is decoded this frame occupies all ref slots
            // its mask is 011111111 and the condition "minus one" never hits.
            // But if the first frame is Inter-frame encoded in the intra-only mode
            // then we can receive "minus one" ref_frame_map item.
            // The decoder never should touch these references.
            // But if the stream is broken then decode may touch not initialized reference
            // So to prevent crash let's put something as the reference frame.
            || (m_frameInfo.ref_frame_map[ref_index] == (UMC::FrameMemID)-1))
        {
            MFX_CHECK((m_FrameAllocator->IncreaseReference(m_frameInfo.currFrame) == UMC::UMC_OK), MFX_ERR_UNKNOWN);

            if (m_frameInfo.ref_frame_map[ref_index] >= 0)
                MFX_CHECK((m_FrameAllocator->DecreaseReference(m_frameInfo.ref_frame_map[ref_index]) == UMC::UMC_OK), MFX_ERR_UNKNOWN);
            m_frameInfo.ref_frame_map[ref_index] = m_frameInfo.currFrame;

            m_frameInfo.sizesOfRefFrame[ref_index].width = m_frameInfo.width;
            m_frameInfo.sizesOfRefFrame[ref_index].height = m_frameInfo.height;
        }
    }
    return MFX_ERR_NONE;
}
mfxStatus VideoDECODEVP9_HW::CleanRefList()
{
    for (mfxI32 ref_index = 0; ref_index < NUM_REF_FRAMES; ++ref_index)
    {
        if (m_frameInfo.ref_frame_map[ref_index] >= 0)
            MFX_CHECK((m_FrameAllocator->DecreaseReference(m_frameInfo.ref_frame_map[ref_index]) == UMC::UMC_OK), MFX_ERR_UNKNOWN);

        m_frameInfo.ref_frame_map[ref_index] = -1;
    }
    return MFX_ERR_NONE;
}


class FrameStorage
{
private:
    std::vector<UMC_VP9_DECODER::VP9DecoderFrame> m_submittedFrames;
    mfx_UMC_FrameAllocator *m_frameAllocator;

    void LockResources(const UMC_VP9_DECODER::VP9DecoderFrame & frame) const
    {
        // lock current frame for decode
        VP9_CHECK_AND_THROW((m_frameAllocator->IncreaseReference(frame.currFrame) == UMC::UMC_OK), MFX_ERR_UNKNOWN);
        // lock frame for copy
        if (frame.show_existing_frame)
        {
            VP9_CHECK_AND_THROW((m_frameAllocator->IncreaseReference(frame.ref_frame_map[frame.frame_to_show]) == UMC::UMC_OK), MFX_ERR_UNKNOWN);
        }
        // lock all references
        else
        {
            for (mfxI32 ref_index = 0; ref_index < NUM_REF_FRAMES; ++ref_index)
            {
                const UMC::FrameMemID mid = frame.ref_frame_map[ref_index];
                if (mid >= 0)
                {
                     VP9_CHECK_AND_THROW((m_frameAllocator->IncreaseReference(mid) == UMC::UMC_OK), MFX_ERR_UNKNOWN);
                }
            }
        }
    }

    void UnLockResources(const UMC_VP9_DECODER::VP9DecoderFrame & frame) const
    {
        // decoded frame should be unlocked in async routine

        // unlock frame for copy
        if (frame.show_existing_frame)
        {
            VP9_CHECK_AND_THROW((m_frameAllocator->DecreaseReference(frame.ref_frame_map[frame.frame_to_show]) == UMC::UMC_OK), MFX_ERR_UNKNOWN);
        }
        // unlock all references
        else
        {
            for (mfxI32 ref_index = 0; ref_index < NUM_REF_FRAMES; ++ref_index)
            {
                const UMC::FrameMemID mid = frame.ref_frame_map[ref_index];
                if (mid >= 0)
                {
                    VP9_CHECK_AND_THROW((m_frameAllocator->DecreaseReference(mid) == UMC::UMC_OK), MFX_ERR_UNKNOWN);
                }
            }
        }
    }
public:
    FrameStorage(mfx_UMC_FrameAllocator *frameAllocator):
        m_submittedFrames(),
        m_frameAllocator(frameAllocator)
    {
    }

    ~FrameStorage()
    {
        // unlock all locked resources
        try
        {
            for (auto & it : m_submittedFrames)
                UnLockResources(it);
        }
        catch (vp9_exception const& e)
        {
            m_submittedFrames.shrink_to_fit();
            m_submittedFrames.clear();
            VM_ASSERT(0);
        }
        m_submittedFrames.shrink_to_fit();
        m_submittedFrames.clear();
    }

    void Add(UMC_VP9_DECODER::VP9DecoderFrame & frame)
    {
        VP9_CHECK_AND_THROW((frame.currFrame >= 0), MFX_ERR_UNKNOWN);

        // lock refereces for current frame
        LockResources(frame);

        // avoid double submit
        auto find_it = std::find_if(m_submittedFrames.begin(), m_submittedFrames.end(),
            [frame](const UMC_VP9_DECODER::VP9DecoderFrame & item){ return item.currFrame == frame.currFrame; });
        VP9_CHECK_AND_THROW((find_it == m_submittedFrames.end()), MFX_ERR_UNKNOWN);

        m_submittedFrames.push_back(frame);
    }

    void DecodeFrame(UMC::FrameMemID frameId)
    {
        auto find_it = std::find_if(m_submittedFrames.begin(), m_submittedFrames.end(),
            [frameId](const UMC_VP9_DECODER::VP9DecoderFrame & item){ return item.currFrame == frameId; });

        if (find_it != m_submittedFrames.end())
        {
            find_it->isDecoded = true;
        }
    }

    void CompleteFrames()
    {
        for( auto it = m_submittedFrames.begin(); it != m_submittedFrames.end(); )
        {
            if (it->isDecoded)
            {
                UnLockResources(*it);
                it = m_submittedFrames.erase(it);
            }
            else
                it = std::next(it);
        }
    }

    bool IsAllFramesCompleted() const
    {
        return m_submittedFrames.empty();
    }
};

VideoDECODEVP9_HW::VideoDECODEVP9_HW(VideoCORE *p_core, mfxStatus *sts)
    : m_isInit(false),
      m_is_opaque_memory(false),
      m_core(p_core),
      m_platform(MFX_PLATFORM_HARDWARE),
      m_num_output_frames(0),
      m_in_framerate(0),
      m_frameOrder(0),
      m_statusReportFeedbackNumber(0),
      m_mGuard(),
      m_adaptiveMode(false),
      m_index(0),
      m_FrameAllocator(),
      m_Packer(),
      m_request(),
      m_response(),
      m_OpaqAlloc(),
      m_stat(),
      m_va(nullptr),
      m_completedList(),
      m_firstSizes(),
      m_bs(),
      m_baseQIndex(0)
{
    memset(&m_sizesOfRefFrame, 0, sizeof(m_sizesOfRefFrame));
    memset(&m_frameInfo.ref_frame_map, VP9_INVALID_REF_FRAME, sizeof(m_frameInfo.ref_frame_map)); // TODO: move to another place
    ResetFrameInfo();

    if (sts)
    {
        *sts = MFX_ERR_NONE;
    }
}

VideoDECODEVP9_HW::~VideoDECODEVP9_HW(void)
{
    Close();
}


static bool CheckVP9BitDepthRestriction(mfxFrameInfo *info)
{
    return ((info->BitDepthLuma == FourCcBitDepth(info->FourCC)) &&
     (info->BitDepthLuma == info->BitDepthChroma));
}

mfxStatus VideoDECODEVP9_HW::Init(mfxVideoParam *par)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    MFX_CHECK(!m_isInit, MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxStatus sts   = MFX_ERR_NONE;
    UMC::Status umcSts   = UMC::UMC_OK;
    eMFXHWType type = m_core->GetHWType();

    m_platform = m_core->GetPlatformType();

    MFX_CHECK(MFX_ERR_NONE <= CheckVideoParamDecoders(par, m_core->IsExternalFrameAllocator(), type), MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(CheckHardwareSupport(m_core, par), MFX_ERR_UNSUPPORTED);

    MFX_CHECK(MFX_VPX_Utility::CheckVideoParam(par, MFX_CODEC_VP9, m_platform, type), MFX_ERR_INVALID_VIDEO_PARAM);

    m_FrameAllocator.reset(new mfx_UMC_FrameAllocator_D3D());

    m_vInitPar = *par;

    MFX_CHECK(InitBitDepthFields(&m_vInitPar.mfx.FrameInfo), MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(CheckVP9BitDepthRestriction(&m_vInitPar.mfx.FrameInfo), MFX_ERR_INVALID_VIDEO_PARAM);

    if ((m_vInitPar.mfx.FrameInfo.AspectRatioH == 0) && (m_vInitPar.mfx.FrameInfo.AspectRatioW == 0))
    {
        m_vInitPar.mfx.FrameInfo.AspectRatioH = 1;
        m_vInitPar.mfx.FrameInfo.AspectRatioW = 1;
    }

    m_vPar = m_vInitPar;

    m_vInitPar.IOPattern = (m_vInitPar.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY)
        | (m_vInitPar.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
        | (m_vInitPar.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY);

    if (0 == m_vInitPar.mfx.FrameInfo.FrameRateExtN || 0 == m_vInitPar.mfx.FrameInfo.FrameRateExtD)
    {
        m_vInitPar.mfx.FrameInfo.FrameRateExtD = 1000;
        m_vInitPar.mfx.FrameInfo.FrameRateExtN = 30000;
    }

    m_in_framerate = (mfxF64) m_vInitPar.mfx.FrameInfo.FrameRateExtD / m_vInitPar.mfx.FrameInfo.FrameRateExtN;

    // allocate memory
    memset(&m_request, 0, sizeof(m_request));
    memset(&m_response, 0, sizeof(m_response));
    memset(&m_firstSizes, 0, sizeof(m_firstSizes));
    ResetFrameInfo();

    sts = MFX_VPX_Utility::QueryIOSurfInternal(&m_vInitPar, &m_request);
    MFX_CHECK_STS(sts);

    if (m_vInitPar.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc *pOpaqAlloc = (mfxExtOpaqueSurfaceAlloc*)
            GetExtendedBuffer(
                par->ExtParam, par->NumExtParam,
                MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

        MFX_CHECK(pOpaqAlloc && m_request.NumFrameMin <= pOpaqAlloc->Out.NumSurface, MFX_ERR_INVALID_VIDEO_PARAM);

        m_is_opaque_memory = true;

        m_request.Type = MFX_MEMTYPE_FROM_DECODE;
        m_request.Type |= MFX_MEMTYPE_OPAQUE_FRAME;
        m_request.Type |= (pOpaqAlloc->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY) ?
            MFX_MEMTYPE_SYSTEM_MEMORY : MFX_MEMTYPE_DXVA2_DECODER_TARGET;

        m_request.NumFrameMin = pOpaqAlloc->Out.NumSurface;
        m_request.NumFrameSuggested = m_request.NumFrameMin;

        sts = m_core->AllocFrames(&m_request, &m_response,
            pOpaqAlloc->Out.Surfaces, pOpaqAlloc->Out.NumSurface);
    }
    else
    {
        m_request.AllocId = par->AllocId;
        sts = m_core->AllocFrames(&m_request, &m_response, false);
    }

    MFX_CHECK_STS(sts);

    sts = m_core->CreateVA(&m_vInitPar, &m_request, &m_response, m_FrameAllocator.get());
    MFX_CHECK_STS(sts);

    m_core->GetVA((mfxHDL*)&m_va, MFX_MEMTYPE_FROM_DECODE);

    bool isUseExternalFrames = (par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) || m_is_opaque_memory;
    bool reallocFrames = (par->mfx.EnableReallocRequest == MFX_CODINGOPTION_ON);
    m_adaptiveMode =
#ifndef MFX_VA_LINUX
            isUseExternalFrames &&
#endif
            reallocFrames;

    if (isUseExternalFrames && !reallocFrames)
    {
        m_FrameAllocator->SetExternalFramesResponse(&m_response);
    }

    umcSts = m_FrameAllocator->InitMfx(0, m_core, par, &m_request, &m_response, isUseExternalFrames, false);
    MFX_CHECK(UMC::UMC_OK == umcSts, MFX_ERR_MEMORY_ALLOC);

    m_frameOrder = 0;
    m_statusReportFeedbackNumber = 0;
    m_isInit = true;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEVP9_HW::Reset(mfxVideoParam *par)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    MFX_CHECK(m_isInit, MFX_ERR_NOT_INITIALIZED);

    MFX_CHECK_NULL_PTR1(par);

    eMFXHWType type = m_core->GetHWType();

    MFX_CHECK(MFX_ERR_NONE <= CheckVideoParamDecoders(par, m_core->IsExternalFrameAllocator(), type), MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(MFX_VPX_Utility::CheckVideoParam(par, MFX_CODEC_VP9, m_core->GetPlatformType(), type), MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(CheckHardwareSupport(m_core, par), MFX_ERR_UNSUPPORTED);

    MFX_CHECK(IsSameVideoParam(par, &m_vInitPar), MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    mfxExtOpaqueSurfaceAlloc *pOpaqAlloc = (mfxExtOpaqueSurfaceAlloc*)
        GetExtendedBuffer(
            par->ExtParam, par->NumExtParam,
            MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

    if (pOpaqAlloc)
    {
        MFX_CHECK(m_is_opaque_memory, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        MFX_CHECK(m_request.NumFrameMin == pOpaqAlloc->Out.NumSurface, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }

    // need to sw acceleration
    MFX_CHECK(m_platform == m_core->GetPlatformType(), MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    ResetFrameInfo();
    MFX_CHECK(m_FrameAllocator->Reset() == UMC::UMC_OK, MFX_ERR_MEMORY_ALLOC);

    m_frameOrder = 0;
    m_statusReportFeedbackNumber = 0;
    memset(&m_stat, 0, sizeof(m_stat));
    memset(&m_firstSizes, 0, sizeof(m_firstSizes));

    m_vPar = *par;

    if (0 == m_vPar.mfx.FrameInfo.FrameRateExtN || 0 == m_vPar.mfx.FrameInfo.FrameRateExtD)
    {
        m_vPar.mfx.FrameInfo.FrameRateExtD = m_vInitPar.mfx.FrameInfo.FrameRateExtD;
        m_vPar.mfx.FrameInfo.FrameRateExtN = m_vInitPar.mfx.FrameInfo.FrameRateExtN;
    }

    m_in_framerate = (mfxF64) m_vPar.mfx.FrameInfo.FrameRateExtD / m_vPar.mfx.FrameInfo.FrameRateExtN;
    m_index = 0;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEVP9_HW::Close()
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    MFX_CHECK(m_isInit, MFX_ERR_NOT_INITIALIZED);

    ResetFrameInfo();
    m_FrameAllocator->Close();

    if (0 < m_response.NumFrameActual)
    {
        m_core->FreeFrames(&m_response);
    }

    m_isInit = false;
    m_adaptiveMode = false;
    m_is_opaque_memory = false;

    m_frameOrder = (mfxU16)MFX_FRAMEORDER_UNKNOWN;
    m_statusReportFeedbackNumber = 0;

    m_va = NULL;

    memset(&m_request, 0, sizeof(m_request));
    memset(&m_response, 0, sizeof(m_response));
    memset(&m_OpaqAlloc, 0, sizeof(m_OpaqAlloc));
    memset(&m_stat, 0, sizeof(m_stat));

    return MFX_ERR_NONE;
}

void VideoDECODEVP9_HW::ResetFrameInfo()
{
    CleanRefList();
    m_framesStorage.reset(new FrameStorage(m_FrameAllocator.get()));

    memset(&m_frameInfo, 0, sizeof(m_frameInfo));
    m_frameInfo.currFrame = -1;
    m_frameInfo.frameCountInBS = 0;
    m_frameInfo.currFrameInBS = 0;
    memset(&m_frameInfo.ref_frame_map, VP9_INVALID_REF_FRAME, sizeof(m_frameInfo.ref_frame_map)); // TODO: move to another place
}

mfxStatus VideoDECODEVP9_HW::DecodeHeader(VideoCORE* core, mfxBitstream* bs, mfxVideoParam* par)
{
    MFX_CHECK_NULL_PTR1(par);

    mfxStatus sts = MFX_VP9_Utility::DecodeHeader(core, bs, par);
    MFX_CHECK_STS(sts);

    return sts;
}

static bool IsSameVideoParam(mfxVideoParam *newPar, mfxVideoParam *oldPar)
{
    if (newPar->IOPattern != oldPar->IOPattern)
    {
        return false;
    }

    if (newPar->mfx.FrameInfo.BitDepthLuma != oldPar->mfx.FrameInfo.BitDepthLuma ||
        newPar->mfx.FrameInfo.BitDepthChroma != oldPar->mfx.FrameInfo.BitDepthChroma)
    {
        return false;
    }

    if (newPar->Protected != oldPar->Protected)
    {
        return false;
    }

    int32_t asyncDepth = std::min<int32_t>(newPar->AsyncDepth, MFX_MAX_ASYNC_DEPTH_VALUE);
    if (asyncDepth != oldPar->AsyncDepth)
    {
        return false;
    }

    if (newPar->mfx.FrameInfo.Height > oldPar->mfx.FrameInfo.Height)
    {
        return false;
    }

    if (newPar->mfx.FrameInfo.Width > oldPar->mfx.FrameInfo.Width)
    {
        return false;
    }

    if (newPar->mfx.FrameInfo.ChromaFormat != oldPar->mfx.FrameInfo.ChromaFormat)
    {
        return false;
    }

    if (newPar->mfx.NumThread > oldPar->mfx.NumThread && oldPar->mfx.NumThread > 0) //  need more surfaces for efficient decoding
    {
        return false;
    }

    return true;
}

mfxTaskThreadingPolicy VideoDECODEVP9_HW::GetThreadingPolicy()
{
    return MFX_TASK_THREADING_SHARED;
}

mfxStatus VideoDECODEVP9_HW::Query(VideoCORE *p_core, mfxVideoParam *p_in, mfxVideoParam *p_out)
{
    MFX_CHECK_NULL_PTR1(p_out);

    mfxVideoParam localIn{};
    if (p_in == p_out)
    {
        MFX_CHECK_NULL_PTR1(p_in);
        MFX_INTERNAL_CPY(&localIn, p_in, sizeof(mfxVideoParam));
        p_in = &localIn;
    }

    eMFXHWType type = p_core->GetHWType();
    mfxVideoParam *p_check_hw_par = p_in;

    mfxVideoParam check_hw_par{};
    if (p_in == NULL)
    {
        check_hw_par.mfx.CodecId = MFX_CODEC_VP9;
        p_check_hw_par = &check_hw_par;
    }

    MFX_CHECK(CheckHardwareSupport(p_core, p_check_hw_par), MFX_ERR_UNSUPPORTED);

    mfxStatus status = MFX_VPX_Utility::Query(p_core, p_in, p_out, MFX_CODEC_VP9, type);

    if (p_in == NULL)
    {
        p_out->mfx.EnableReallocRequest = 1;
    }
    else
    {
        // Disable DRC realloc surface, unless the user explicitly enable it.
        p_out->mfx.EnableReallocRequest = MFX_CODINGOPTION_OFF;

        if (p_in->mfx.EnableReallocRequest == MFX_CODINGOPTION_ON)
        {
            p_out->mfx.EnableReallocRequest = MFX_CODINGOPTION_ON;
        }
    }

    return status;
}

mfxStatus VideoDECODEVP9_HW::QueryIOSurf(VideoCORE *p_core, mfxVideoParam *p_video_param, mfxFrameAllocRequest *p_request)
{

    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK_NULL_PTR3(p_core, p_video_param, p_request);

    const mfxVideoParam p_params = *p_video_param;

    MFX_CHECK(
        p_params.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY ||
        p_params.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY ||
        p_params.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY,
        MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(!(
        p_params.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY &&
        p_params.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY),
        MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(!(
        p_params.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY &&
        p_params.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY),
        MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(!(
        p_params.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY &&
        p_params.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY),
        MFX_ERR_INVALID_VIDEO_PARAM);

    if (p_params.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
    {
        p_request->Info = p_params.mfx.FrameInfo;
        p_request->NumFrameMin = 1;
        p_request->NumFrameSuggested = p_request->NumFrameMin + (p_params.AsyncDepth ? p_params.AsyncDepth : MFX_AUTO_ASYNC_DEPTH_VALUE);
        p_request->Type = MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_FROM_DECODE;
    }
    else
    {
        sts = MFX_VPX_Utility::QueryIOSurfInternal(p_video_param, p_request);
    }

    if (p_params.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        p_request->Type |= MFX_MEMTYPE_OPAQUE_FRAME;
    }
    else
    {
        p_request->Type |= MFX_MEMTYPE_EXTERNAL_FRAME;
    }

    MFX_CHECK(CheckHardwareSupport(p_core, p_video_param), MFX_ERR_UNSUPPORTED);

    MFX_CHECK_STS(sts);
    return sts;
}

mfxStatus VideoDECODEVP9_HW::GetVideoParam(mfxVideoParam *par)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    MFX_CHECK(m_isInit, MFX_ERR_NOT_INITIALIZED);

    MFX_CHECK_NULL_PTR1(par);

    par->mfx = m_vPar.mfx;

    par->Protected = m_vInitPar.Protected;
    par->IOPattern = m_vInitPar.IOPattern;
    par->AsyncDepth = m_vInitPar.AsyncDepth;

    par->mfx.FrameInfo.FrameRateExtD = m_vInitPar.mfx.FrameInfo.FrameRateExtD;
    par->mfx.FrameInfo.FrameRateExtN = m_vInitPar.mfx.FrameInfo.FrameRateExtN;

    par->mfx.FrameInfo.AspectRatioH = m_vInitPar.mfx.FrameInfo.AspectRatioH;
    par->mfx.FrameInfo.AspectRatioW = m_vInitPar.mfx.FrameInfo.AspectRatioW;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEVP9_HW::GetDecodeStat(mfxDecodeStat *pStat)
{
    MFX_CHECK(m_isInit, MFX_ERR_NOT_INITIALIZED);

    MFX_CHECK_NULL_PTR1(pStat);

    m_stat.NumSkippedFrame = 0;
    m_stat.NumCachedFrame = 0;

    *pStat = m_stat;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEVP9_HW::GetOutputSurface(mfxFrameSurface1 **surface_out, mfxFrameSurface1 *surface_work, UMC::FrameMemID index)
{
    mfxFrameSurface1 *pNativeSurface = m_FrameAllocator->GetSurface(index, surface_work, &m_vInitPar);

    MFX_CHECK(pNativeSurface, MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxFrameSurface1 *pOpaqueSurface = m_core->GetOpaqSurface(pNativeSurface->Data.MemId);
    *surface_out = pOpaqueSurface ? pOpaqueSurface : pNativeSurface;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEVP9_HW::GetUserData(mfxU8 *pUserData, mfxU32 *pSize, mfxU64 *pTimeStamp)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    MFX_CHECK(m_isInit, MFX_ERR_NOT_INITIALIZED);

    MFX_CHECK_NULL_PTR3(pUserData, pSize, pTimeStamp);

    return MFX_ERR_UNSUPPORTED;
}

mfxStatus VideoDECODEVP9_HW::GetPayload(mfxU64 *pTimeStamp, mfxPayload *pPayload)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    MFX_CHECK(m_isInit, MFX_ERR_NOT_INITIALIZED);

    MFX_CHECK_NULL_PTR3(pTimeStamp, pPayload, pPayload->Data);

    return MFX_ERR_UNSUPPORTED;
}

mfxStatus VideoDECODEVP9_HW::SetSkipMode(mfxSkipMode /*mode*/)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    MFX_CHECK(m_isInit, MFX_ERR_NOT_INITIALIZED);

    return MFX_ERR_NONE;
}

void VideoDECODEVP9_HW::CalculateTimeSteps(mfxFrameSurface1 *p_surface)
{
    p_surface->Data.TimeStamp = GetMfxTimeStamp(m_num_output_frames * m_in_framerate);
    p_surface->Data.FrameOrder = m_num_output_frames;
    m_num_output_frames += 1;

    p_surface->Info.FrameRateExtD = m_vInitPar.mfx.FrameInfo.FrameRateExtD;
    p_surface->Info.FrameRateExtN = m_vInitPar.mfx.FrameInfo.FrameRateExtN;

    p_surface->Info.CropX = m_vInitPar.mfx.FrameInfo.CropX;
    p_surface->Info.CropY = m_vInitPar.mfx.FrameInfo.CropY;
    p_surface->Info.PicStruct = m_vInitPar.mfx.FrameInfo.PicStruct;

    p_surface->Info.AspectRatioH = 1;
    p_surface->Info.AspectRatioW = 1;
}

enum
{
    NUMBER_OF_STATUS = 32,
};

mfxStatus MFX_CDECL VP9DECODERoutine(void *p_state, void * /* pp_param */, mfxU32 /* thread_number */, mfxU32)
{
    VideoDECODEVP9_HW::VP9DECODERoutineData& data = *(VideoDECODEVP9_HW::VP9DECODERoutineData*)p_state;
    VideoDECODEVP9_HW& decoder = *data.decoder;

    MFX_CHECK(data.surface_work, MFX_ERR_UNDEFINED_BEHAVIOR);

    if (data.copyFromFrame != UMC::FRAME_MID_INVALID)
    {
        UMC::AutomaticUMCMutex guard(decoder.m_mGuard);

        mfxFrameSurface1 surfaceDst = *data.surface_work;
        surfaceDst.Info.Width = (surfaceDst.Info.CropW + 15) & ~0x0f;
        surfaceDst.Info.Height = (surfaceDst.Info.CropH + 15) & ~0x0f;

        mfxFrameSurface1 *surfaceSrc = decoder.m_FrameAllocator->GetSurfaceByIndex(data.copyFromFrame);
        MFX_CHECK(surfaceSrc, MFX_ERR_UNDEFINED_BEHAVIOR);

        bool systemMemory = (decoder.m_vInitPar.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) != 0;
        bool opaqMemory = (decoder.m_vInitPar.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) != 0;
        mfxU16 srcMemType = MFX_MEMTYPE_DXVA2_DECODER_TARGET;
        srcMemType |= (systemMemory || opaqMemory) ? MFX_MEMTYPE_INTERNAL_FRAME : MFX_MEMTYPE_EXTERNAL_FRAME;
        mfxU16 dstMemType = opaqMemory ? (mfxU16) MFX_MEMTYPE_INTERNAL_FRAME : (mfxU16) MFX_MEMTYPE_EXTERNAL_FRAME;
        dstMemType |= systemMemory ? MFX_MEMTYPE_SYSTEM_MEMORY : MFX_MEMTYPE_DXVA2_DECODER_TARGET;

        mfxStatus sts = decoder.m_core->DoFastCopyWrapper(&surfaceDst, dstMemType, surfaceSrc, srcMemType);
        MFX_CHECK_STS(sts);

        decoder.m_FrameAllocator->SetSfcPostProcessingFlag(true);
        sts = decoder.m_FrameAllocator->PrepareToOutput(data.surface_work, data.copyFromFrame, 0, false);
        MFX_CHECK_STS(sts);

        if (data.currFrameId != -1)
            decoder.m_FrameAllocator->DecreaseReference(data.currFrameId);
        decoder.m_framesStorage->DecodeFrame(data.currFrameId);

        decoder.m_FrameAllocator->SetSfcPostProcessingFlag(false);
        return MFX_ERR_NONE;
    }

#ifdef MFX_VA_LINUX

    UMC::Status status = decoder.m_va->SyncTask(data.currFrameId);
    if (status != UMC::UMC_OK)
    {
        mfxStatus CriticalErrorStatus = (status == UMC::UMC_ERR_GPU_HANG) ? MFX_ERR_GPU_HANG : MFX_ERR_DEVICE_FAILED;
        decoder.SetCriticalErrorOccured(CriticalErrorStatus);
        return CriticalErrorStatus;
    }

#endif

    if (decoder.m_vInitPar.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
    {
        if (data.showFrame)
        {
            mfxStatus sts = decoder.m_FrameAllocator->PrepareToOutput(data.surface_work, data.currFrameId, 0, false);
            MFX_CHECK_STS(sts);
        }
        else
        {
            mfxStatus sts = decoder.m_core->DecreaseReference(&data.surface_work->Data);
            MFX_CHECK_STS(sts);
        }
    }

    UMC::AutomaticUMCMutex guard(decoder.m_mGuard);

    if (data.currFrameId != -1)
        decoder.m_FrameAllocator->DecreaseReference(data.currFrameId);
    decoder.m_framesStorage->DecodeFrame(data.currFrameId);

    return MFX_TASK_DONE;
}

mfxStatus VP9CompleteProc(void *p_state, void * /* pp_param */, mfxStatus)
{
    delete (VideoDECODEVP9_HW::VP9DECODERoutineData*)p_state;
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEVP9_HW::PrepareInternalSurface(UMC::FrameMemID &mid, mfxFrameInfo &frameInfo)
{
    UMC::VideoDataInfo videoInfo;

    UMC::ColorFormat const cf = GetUMCColorFormat_VP9(&m_frameInfo);

    MFX_CHECK(UMC::UMC_OK == videoInfo.Init(m_vPar.mfx.FrameInfo.Width, m_vPar.mfx.FrameInfo.Height, cf, m_frameInfo.bit_depth), MFX_ERR_MEMORY_ALLOC);

#ifndef MFX_VA_LINUX
    UMC::Status umc_sts = m_FrameAllocator->Alloc(&mid, &videoInfo, 0);
#else
    UMC::Status umc_sts = m_FrameAllocator->Alloc(&mid, &videoInfo, mfx_UMC_ReallocAllowed);
    if (UMC::UMC_ERR_NOT_ENOUGH_BUFFER == umc_sts && m_adaptiveMode)
    {
        mfxFrameSurface1 *surf = m_FrameAllocator->GetSurfaceByIndex(mid);
        MFX_CHECK(surf, MFX_ERR_INVALID_HANDLE);
        surf->Info.Width = frameInfo.Width;
        surf->Info.Height = frameInfo.Height;
        VAAPIVideoCORE *vaapi_core = reinterpret_cast<VAAPIVideoCORE *>(m_core);
        return vaapi_core->ReallocFrame(surf);
    }
    else
#endif
    MFX_CHECK(UMC::UMC_OK == umc_sts, MFX_ERR_MEMORY_ALLOC);

    return MFX_ERR_NONE;
}


static mfxStatus CheckFrameInfo(mfxFrameInfo const &currInfo, mfxFrameInfo &info)
{
    MFX_SAFE_CALL(CheckFrameInfoCommon(&info, MFX_CODEC_VP9));

    switch (info.FourCC)
    {
        case MFX_FOURCC_NV12:
        case MFX_FOURCC_AYUV:
#if (MFX_VERSION >= 1027)
        case MFX_FOURCC_Y410:
#endif
            break;
#if (MFX_VERSION >= 1031)
        case MFX_FOURCC_P016:
        case MFX_FOURCC_Y416:
#endif
        case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1027)
        case MFX_FOURCC_Y210:
#endif
            MFX_CHECK(info.Shift == 1, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
            break;
        default:
            MFX_CHECK_STS(MFX_ERR_INVALID_VIDEO_PARAM);
    }

    switch(info.ChromaFormat)
    {
        case MFX_CHROMAFORMAT_YUV420:
        case MFX_CHROMAFORMAT_YUV444:
            break;
        default:
            MFX_CHECK_STS(MFX_ERR_INVALID_VIDEO_PARAM);
    }

    MFX_CHECK(currInfo.FourCC == info.FourCC, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    return MFX_ERR_NONE;
}


mfxStatus VideoDECODEVP9_HW::DecodeFrameCheck(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, MFX_ENTRY_POINT * p_entry_point)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    mfxStatus sts = MFX_ERR_NONE;

    MFX_CHECK(m_isInit, MFX_ERR_NOT_INITIALIZED);

    try
    { m_framesStorage->CompleteFrames(); }
    catch (vp9_exception const& e)
    {
        UMC::Status status = e.GetStatus();
        MFX_CHECK(status == UMC::UMC_OK, MFX_ERR_UNKNOWN);
    }

    if (NeedToReturnCriticalStatus(bs))
        return ReturningCriticalStatus();

    if (bs && !bs->DataLength)
        return MFX_ERR_MORE_DATA;

    MFX_CHECK_NULL_PTR1(surface_out);

    *surface_out = nullptr;

    if (bs == nullptr)
    {
        sts = CleanRefList();
        MFX_CHECK_STS(sts);
        return m_framesStorage->IsAllFramesCompleted() ? MFX_ERR_MORE_DATA : MFX_WRN_DEVICE_BUSY;
    }

    MFX_CHECK_NULL_PTR1(surface_work);

    if (m_is_opaque_memory)
    {
        // sanity check: opaque surfaces are zero'd out
        if (surface_work->Data.MemId || surface_work->Data.Y
            || surface_work->Data.UV || surface_work->Data.R
            || surface_work->Data.A)
        {
            MFX_RETURN(MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        // work with the native (original) surface
        surface_work = GetOriginalSurface(surface_work);
        MFX_CHECK(surface_work, MFX_ERR_UNDEFINED_BEHAVIOR);
    }

    sts = CheckFrameInfo(m_vPar.mfx.FrameInfo, surface_work->Info);
    MFX_CHECK_STS(sts);

    sts = CheckFrameData(surface_work);
    MFX_CHECK_STS(sts);

    eMFXHWType type = m_core->GetHWType();
    if (!MFX_VPX_Utility::CheckFrameInfo(surface_work->Info, MFX_CODEC_VP9, m_platform, type))
    {
        MFX_CHECK_STS(MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
    }

    sts = CheckBitstream(bs);
    MFX_CHECK_STS(sts);

    MFX_CHECK(!surface_work->Data.Locked, MFX_ERR_MORE_SURFACE);

    m_index++;
    m_frameInfo.showFrame = 0;

    VP9DecoderFrame frameInfo = m_frameInfo;
    sts = DecodeFrameHeader(bs, frameInfo);
    MFX_CHECK_STS(sts);

    MFX_VP9_Utility::FillVideoParam(m_core->GetPlatformType(), frameInfo, m_vPar);

    // check bit depth change
    MFX_CHECK((m_vPar.mfx.FrameInfo.BitDepthLuma == surface_work->Info.BitDepthLuma &&
                m_vPar.mfx.FrameInfo.BitDepthChroma == surface_work->Info.BitDepthChroma),
                MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    // check resize
    if (m_vPar.mfx.FrameInfo.Width > surface_work->Info.Width ||
        m_vPar.mfx.FrameInfo.Height > surface_work->Info.Height)
    {
        MFX_CHECK(m_adaptiveMode, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
        return MFX_ERR_REALLOC_SURFACE;
    }

    // possible system memory case. when external surface is enough, but internal is not.
    if (!m_adaptiveMode)
    {
        MFX_CHECK( 
            m_vPar.mfx.FrameInfo.Width <= m_vInitPar.mfx.FrameInfo.Width && 
            m_vPar.mfx.FrameInfo.Height <= m_vInitPar.mfx.FrameInfo.Height,
            MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

// this check is not needed if re-allocation of internal surface function is implemented 
// (currently only in VAAPI linux)
#ifndef MFX_VA_LINUX
        MFX_CHECK(
            KEY_FRAME != frameInfo.frameType ||
            1 != m_index ||
            (m_vPar.mfx.FrameInfo.Width == m_vInitPar.mfx.FrameInfo.Width && 
            m_vPar.mfx.FrameInfo.Height == m_vInitPar.mfx.FrameInfo.Height),
            MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
#endif
    }

    //update frame info
    m_frameInfo = frameInfo;
    //gpu session priority
    m_va->m_ContextPriority = m_core->GetSession()->m_priority;

    sts = m_FrameAllocator->SetCurrentMFXSurface(surface_work, m_is_opaque_memory);
    MFX_CHECK_STS(sts);

    if (m_FrameAllocator->FindFreeSurface() == -1)
        return MFX_WRN_DEVICE_BUSY;

    sts = PrepareInternalSurface(m_frameInfo.currFrame, surface_work->Info);
    MFX_CHECK_STS(sts);

    if (!m_frameInfo.frameCountInBS)
    {
        bs->DataOffset += bs->DataLength;
        bs->DataLength = 0;
    }

    UMC::FrameMemID repeateFrame = UMC::FRAME_MID_INVALID;

    if (!m_frameInfo.show_existing_frame)
    {
        UMC::Status umc_sts = m_va->BeginFrame(m_frameInfo.currFrame, 0);
        MFX_CHECK(UMC::UMC_OK == umc_sts, MFX_ERR_DEVICE_FAILED);

        sts = PackHeaders(&m_bs, m_frameInfo);
        MFX_CHECK_STS(sts);

        umc_sts = m_va->Execute();
        MFX_CHECK(UMC::UMC_OK == umc_sts, MFX_ERR_DEVICE_FAILED);

        umc_sts = m_va->EndFrame();
        MFX_CHECK(UMC::UMC_OK == umc_sts, MFX_ERR_DEVICE_FAILED);
    }
    else
    {
        repeateFrame = m_frameInfo.ref_frame_map[m_frameInfo.frame_to_show];
        ++m_statusReportFeedbackNumber;
    }

    p_entry_point->pRoutine = &VP9DECODERoutine;
    p_entry_point->pCompleteProc = &VP9CompleteProc;

    VP9DECODERoutineData* routineData = new VP9DECODERoutineData;
    routineData->decoder = this;
    routineData->currFrameId = m_frameInfo.currFrame;
    routineData->copyFromFrame = repeateFrame;
    routineData->surface_work = surface_work;
    routineData->index        = m_index;
    routineData->showFrame = m_frameInfo.showFrame;

    p_entry_point->pState = routineData;
    p_entry_point->requiredNumThreads = 1;

    try
    { m_framesStorage->Add(m_frameInfo); }
    catch (vp9_exception const& e)
    {
        UMC::Status status = e.GetStatus();
        MFX_CHECK(status == UMC::UMC_OK, MFX_ERR_UNKNOWN);
    }

    // update list of references for next frame
    sts = UpdateRefFrames();
    MFX_CHECK_STS(sts);

    sts = GetOutputSurface(surface_out, surface_work, m_frameInfo.currFrame);
    MFX_CHECK_STS(sts);

    if (m_frameInfo.showFrame)
    {
        SetFrameType(m_frameInfo, **surface_out);

        (*surface_out)->Data.TimeStamp = bs->TimeStamp != static_cast<mfxU64>(MFX_TIMESTAMP_UNKNOWN) ? bs->TimeStamp : GetMfxTimeStamp(m_frameOrder * m_in_framerate);
        (*surface_out)->Data.Corrupted = 0;
        (*surface_out)->Data.FrameOrder = m_frameOrder;
        (*surface_out)->Info.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        (*surface_out)->Info.CropW = (mfxU16)m_frameInfo.width;
        (*surface_out)->Info.CropH = (mfxU16)m_frameInfo.height;
        (*surface_out)->Info.AspectRatioW = m_vPar.mfx.FrameInfo.AspectRatioW;
        (*surface_out)->Info.AspectRatioH = m_vPar.mfx.FrameInfo.AspectRatioH;

        m_frameOrder++;
        return MFX_ERR_NONE;
    }
    else
    {
        surface_out = 0;
        return (mfxStatus)MFX_ERR_MORE_DATA_SUBMIT_TASK;
    }
}

mfxStatus VideoDECODEVP9_HW::DecodeSuperFrame(mfxBitstream *in, VP9DecoderFrame & info)
{
    mfxU32 frameSizes[8] = { 0 };
    mfxU32 frameCount = 0;

    m_bs = *in;

    MfxVP9Decode::ParseSuperFrameIndex(in->Data + in->DataOffset, in->DataLength, frameSizes, &frameCount);

    if (frameCount > 1)
    {
        if (info.frameCountInBS == 0) // first call we meet super frame
        {
            info.frameCountInBS = frameCount;
            info.currFrameInBS = 0;
        }

        // it is not what we met before
        MFX_CHECK(info.frameCountInBS == frameCount, MFX_ERR_UNDEFINED_BEHAVIOR);

        m_bs.DataLength = frameSizes[info.currFrameInBS];
        m_bs.DataOffset = in->DataOffset;
        for (mfxU32 i = 0; i < info.currFrameInBS; i++)
            m_bs.DataOffset += frameSizes[i];

        info.currFrameInBS++;
        if (info.currFrameInBS < info.frameCountInBS)
            return MFX_ERR_NONE;
    }

    info.currFrameInBS = 0;
    info.frameCountInBS = 0;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEVP9_HW::DecodeFrameHeader(mfxBitstream *in, VP9DecoderFrame & info)
{
    MFX_CHECK_NULL_PTR2(in, in->Data);

    try
    {
        mfxStatus sts = DecodeSuperFrame(in, info);
        MFX_CHECK_STS(sts);
        in = &m_bs;

        VP9Bitstream bsReader(in->Data + in->DataOffset, in->DataOffset + in->DataLength);

        if (VP9_FRAME_MARKER != bsReader.GetBits(2))
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        info.profile = bsReader.GetBit();
        info.profile |= bsReader.GetBit() << 1;
        if (info.profile > 2)
            info.profile += bsReader.GetBit();

        MFX_CHECK(info.profile < 4, MFX_ERR_UNDEFINED_BEHAVIOR);

        info.show_existing_frame = bsReader.GetBit();
        if (info.show_existing_frame)
        {
            info.frame_to_show = bsReader.GetBits(3);
            info.width = info.sizesOfRefFrame[info.frame_to_show].width;
            info.height = info.sizesOfRefFrame[info.frame_to_show].height;
            info.showFrame = 1;
            return MFX_ERR_NONE;
        }

        info.frameType = (VP9_FRAME_TYPE) bsReader.GetBit();
        info.showFrame = bsReader.GetBit();
        info.errorResilientMode = bsReader.GetBit();

        if (KEY_FRAME == info.frameType)
        {
            MFX_CHECK(CheckSyncCode(&bsReader), MFX_ERR_UNDEFINED_BEHAVIOR);

            try
            { GetBitDepthAndColorSpace(&bsReader, &info); }
            catch (vp9_exception const& e)
            {
                UMC::Status status = e.GetStatus();
                MFX_CHECK(status == UMC::UMC_OK, MFX_ERR_UNDEFINED_BEHAVIOR);
            }

            info.refreshFrameFlags = (1 << NUM_REF_FRAMES) - 1;

            for (mfxU8 i = 0; i < REFS_PER_FRAME; ++i)
            {
                info.activeRefIdx[i] = 0;
            }

            GetFrameSize(&bsReader, &info);
        }
        else
        {
            info.intraOnly = info.showFrame ? 0 : bsReader.GetBit();
            info.resetFrameContext = info.errorResilientMode ? 0 : bsReader.GetBits(2);

            if (info.intraOnly)
            {
                MFX_CHECK(CheckSyncCode(&bsReader), MFX_ERR_UNDEFINED_BEHAVIOR);

                try
                {
                    GetBitDepthAndColorSpace(&bsReader, &info);

                    info.refreshFrameFlags = (mfxU8)bsReader.GetBits(NUM_REF_FRAMES);
                    GetFrameSize(&bsReader, &info);
                }
                catch (vp9_exception const& e)
                {
                    UMC::Status status = e.GetStatus();
                    MFX_CHECK(status == UMC::UMC_OK, MFX_ERR_UNDEFINED_BEHAVIOR);
                }

            }
            else
            {
                info.refreshFrameFlags = (mfxU8)bsReader.GetBits(NUM_REF_FRAMES);

                for (mfxU8 i = 0; i < REFS_PER_FRAME; ++i)
                {
                    const mfxI32 ref = bsReader.GetBits(REF_FRAMES_LOG2);
                    info.activeRefIdx[i] = ref;
                    info.refFrameSignBias[LAST_FRAME + i] = bsReader.GetBit();
                }

                GetFrameSizeWithRefs(&bsReader, &info);

                info.allowHighPrecisionMv = bsReader.GetBit();

                static const INTERP_FILTER literal2Filter[] =
                    { EIGHTTAP_SMOOTH, EIGHTTAP, EIGHTTAP_SHARP, BILINEAR };
                info.interpFilter = bsReader.GetBit() ? SWITCHABLE : literal2Filter[bsReader.GetBits(2)];
            }
        }

        if (!info.errorResilientMode)
        {
            info.refreshFrameContext = bsReader.GetBit();
            info.frameParallelDecodingMode = bsReader.GetBit();
        }
        else
        {
            info.refreshFrameContext = 0;
            info.frameParallelDecodingMode = 1;
        }

        // This flag will be overridden by the call to vp9_setup_past_independence
        // below, forcing the use of context 0 for those frame types.
        info.frameContextIdx = bsReader.GetBits(FRAME_CONTEXTS_LOG2);

        if (info.frameType == KEY_FRAME || info.intraOnly || info.errorResilientMode)
        {
           SetupPastIndependence(info);
        }

        SetupLoopFilter(&bsReader, &info.lf);

        //setup_quantization()
        {
            info.baseQIndex = bsReader.GetBits(QINDEX_BITS);
            mfxI32 old_y_dc_delta_q = info.y_dc_delta_q;
            mfxI32 old_uv_dc_delta_q = info.uv_dc_delta_q;
            mfxI32 old_uv_ac_delta_q = info.uv_ac_delta_q;

            if (bsReader.GetBit())
            {
                info.y_dc_delta_q = bsReader.GetBits(4);
                info.y_dc_delta_q = bsReader.GetBit() ? -info.y_dc_delta_q : info.y_dc_delta_q;
            }
            else
                info.y_dc_delta_q = 0;

            if (bsReader.GetBit())
            {
                info.uv_dc_delta_q = bsReader.GetBits(4);
                info.uv_dc_delta_q = bsReader.GetBit() ? -info.uv_dc_delta_q : info.uv_dc_delta_q;
            }
            else
                info.uv_dc_delta_q = 0;

            if (bsReader.GetBit())
            {
                info.uv_ac_delta_q = bsReader.GetBits(4);
                info.uv_ac_delta_q = bsReader.GetBit() ? -info.uv_ac_delta_q : info.uv_ac_delta_q;
            }
            else
                info.uv_ac_delta_q = 0;

            if (old_y_dc_delta_q  != info.y_dc_delta_q  ||
                old_uv_dc_delta_q != info.uv_dc_delta_q ||
                old_uv_ac_delta_q != info.uv_ac_delta_q ||
                0 == m_frameOrder)
            {
                InitDequantizer(&info);
            }

            info.lossless = (0 == info.baseQIndex &&
                             0 == info.y_dc_delta_q &&
                             0 == info.uv_dc_delta_q &&
                             0 == info.uv_ac_delta_q);
        }

        // setup_segmentation()
        {
            info.segmentation.updateMap = 0;
            info.segmentation.updateData = 0;

            info.segmentation.enabled = (mfxU8)bsReader.GetBit();
            if (info.segmentation.enabled)
            {
                // Segmentation map update
                info.segmentation.updateMap = (mfxU8)bsReader.GetBit();
                if (info.segmentation.updateMap)
                {
                    for (mfxU8 i = 0; i < VP9_NUM_OF_SEGMENT_TREE_PROBS; ++i)
                        info.segmentation.treeProbs[i] = (mfxU8) (bsReader.GetBit() ? bsReader.GetBits(8) : VP9_MAX_PROB);

                    info.segmentation.temporalUpdate = (mfxU8)bsReader.GetBit();
                    if (info.segmentation.temporalUpdate)
                    {
                        for (mfxU8 i = 0; i < VP9_NUM_OF_PREDICTION_PROBS; ++i)
                            info.segmentation.predProbs[i] = (mfxU8) (bsReader.GetBit() ? bsReader.GetBits(8) : VP9_MAX_PROB);
                    }
                    else
                    {
                        for (mfxU8 i = 0; i < VP9_NUM_OF_PREDICTION_PROBS; ++i)
                            info.segmentation.predProbs[i] = VP9_MAX_PROB;
                    }
                }

                info.segmentation.updateData = (mfxU8)bsReader.GetBit();
                if (info.segmentation.updateData)
                {
                    info.segmentation.absDelta = (mfxU8)bsReader.GetBit();

                    ClearAllSegFeatures(info.segmentation);

                    mfxI32 data = 0;
                    mfxU32 nBits = 0;
                    for (mfxU8 i = 0; i < VP9_MAX_NUM_OF_SEGMENTS; ++i)
                    {
                        for (mfxU8 j = 0; j < SEG_LVL_MAX; ++j)
                        {
                            data = 0;
                            if (bsReader.GetBit()) // feature_enabled
                            {
                                EnableSegFeature(info.segmentation, i, (SEG_LVL_FEATURES) j);

                                nBits = GetUnsignedBits(SEG_FEATURE_DATA_MAX[j]);
                                data = bsReader.GetBits(nBits);
                                data = data > SEG_FEATURE_DATA_MAX[j] ? SEG_FEATURE_DATA_MAX[j] : data;

                                if (IsSegFeatureSigned( (SEG_LVL_FEATURES) j))
                                    data = bsReader.GetBit() ? -data : data;
                            }

                            SetSegData(info.segmentation, i, (SEG_LVL_FEATURES) j, data);
                        }
                    }
                }
            }
        }

        // setup_tile_info()
        {
            const mfxI32 alignedWidth = AlignPowerOfTwo(info.width, MI_SIZE_LOG2);
            int minLog2TileColumns, maxLog2TileColumns, maxOnes;
            mfxU32 miCols = alignedWidth >> MI_SIZE_LOG2;
            GetTileNBits(miCols, minLog2TileColumns, maxLog2TileColumns);

            maxOnes = maxLog2TileColumns - minLog2TileColumns;
            info.log2TileColumns = minLog2TileColumns;
            while (maxOnes-- && bsReader.GetBit())
                info.log2TileColumns++;

            info.log2TileRows = bsReader.GetBit();
            if (info.log2TileRows)
                info.log2TileRows += bsReader.GetBit();
        }

        info.firstPartitionSize = bsReader.GetBits(16);
        MFX_CHECK(0 != info.firstPartitionSize, MFX_ERR_UNSUPPORTED);

        info.frameHeaderLength = uint32_t(bsReader.BitsDecoded() / 8 + (bsReader.BitsDecoded() % 8 > 0));
        info.frameDataSize = in->DataLength;

        // vp9_loop_filter_frame_init()
        if (info.lf.filterLevel)
        {
            const mfxI32 scale = 1 << (info.lf.filterLevel >> 5);

            LoopFilterInfo & lf_info = info.lf_info;

            for (mfxU8 segmentId = 0; segmentId < VP9_MAX_NUM_OF_SEGMENTS; ++segmentId)
            {
                mfxI32 segmentFilterLevel = info.lf.filterLevel;
                if (IsSegFeatureActive(info.segmentation, segmentId, SEG_LVL_ALT_LF))
                {
                    const mfxI32 data = GetSegData(info.segmentation, segmentId, SEG_LVL_ALT_LF);
                    segmentFilterLevel = clamp(info.segmentation.absDelta == SEGMENT_ABSDATA ? data : info.lf.filterLevel + data,
                                               0,
                                               MAX_LOOP_FILTER);
                }

                if (!info.lf.modeRefDeltaEnabled)
                {
                    memset(lf_info.level[segmentId], segmentFilterLevel, sizeof(lf_info.level[segmentId]) );
                }
                else
                {
                    const mfxI32 intra_lvl = segmentFilterLevel + info.lf.refDeltas[INTRA_FRAME] * scale;
                    lf_info.level[segmentId][INTRA_FRAME][0] = (mfxU8) clamp(intra_lvl, 0, MAX_LOOP_FILTER);

                    for (mfxU8 ref = LAST_FRAME; ref < MAX_REF_FRAMES; ++ref)
                    {
                        for (mfxU8 mode = 0; mode < MAX_MODE_LF_DELTAS; ++mode)
                        {
                            const mfxI32 inter_lvl = segmentFilterLevel + info.lf.refDeltas[ref] * scale
                                                            + info.lf.modeDeltas[mode] * scale;
                            lf_info.level[segmentId][ref][mode] = (mfxU8) clamp(inter_lvl, 0, MAX_LOOP_FILTER);
                        }
                    }
                }
            }
        }
    }
    catch(const vp9_exception & ex)
    {
        mfxStatus sts = ConvertStatusUmc2Mfx(ex.GetStatus());
        MFX_CHECK_STS(sts);
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEVP9_HW::PackHeaders(mfxBitstream *bs, VP9DecoderFrame const & info)
{
    MFX_CHECK_NULL_PTR2(bs, bs->Data);

    if (!m_Packer)
    {
        m_Packer.reset(Packer::CreatePacker(m_va));
        MFX_CHECK(m_Packer, MFX_ERR_UNDEFINED_BEHAVIOR);
    }

    VP9Bitstream vp9bs(bs->Data + bs->DataOffset, bs->DataLength);

    try
    {
        m_Packer->BeginFrame();
        VP9DecoderFrame packerInfo = info;
        m_Packer->PackAU(&vp9bs, &packerInfo);
        m_Packer->EndFrame();
    }
    catch (vp9_exception const&)
    {
        MFX_RETURN(MFX_ERR_UNDEFINED_BEHAVIOR);
    }

    return MFX_ERR_NONE;
}

mfxFrameSurface1 * VideoDECODEVP9_HW::GetOriginalSurface(mfxFrameSurface1 *p_surface)
{
    if (m_is_opaque_memory)
    {
        return m_core->GetNativeSurface(p_surface);
    }

    return p_surface;
}

#endif
