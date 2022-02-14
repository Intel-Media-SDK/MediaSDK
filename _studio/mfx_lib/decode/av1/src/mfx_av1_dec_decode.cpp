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

#if defined(MFX_ENABLE_AV1_VIDEO_DECODE)

#include "mfx_session.h"
#include "mfx_av1_dec_decode.h"

#include "mfx_task.h"

#include "mfx_common_int.h"
#include "mfx_common_decode_int.h"
#include "mfx_vpx_dec_common.h"

#include "mfx_umc_alloc_wrapper.h"

#include "umc_av1_dec_defs.h"
#include "umc_av1_frame.h"
#include "umc_av1_utils.h"

#include "libmfx_core_hw.h"

#include "umc_av1_decoder_va.h"

#include <algorithm>

#include "vm_sys_info.h"

#if defined(MFX_VA)
#include "umc_h265_va_supplier.h"
#if defined(MFX_ENABLE_CPLIB) || !defined(MFX_PROTECTED_FEATURE_DISABLE)
#include "umc_va_linux_protected.h"
#endif
#endif

inline
mfxU32 CalculateAsyncDepth(eMFXPlatform platform, mfxVideoParam *par)
{
    mfxU32 asyncDepth = par ? par->AsyncDepth : 0;
    if (!asyncDepth)
    {
        asyncDepth = (platform == MFX_PLATFORM_SOFTWARE) ? vm_sys_info_get_cpu_num() : MFX_AUTO_ASYNC_DEPTH_VALUE;
    }

    return asyncDepth;
}

namespace MFX_VPX_Utility
{
    inline
    mfxU16 MatchProfile(mfxU32)
    {
        return MFX_PROFILE_AV1_MAIN;
    }

#if defined (MFX_VA_WIN)
    const GUID DXVA_Intel_ModeAV1_VLD =
    { 0xca44afc5, 0xe1d0, 0x42e6, { 0x91, 0x54, 0xb1, 0x27, 0x18, 0x6d, 0x4d, 0x40 } };

    const GUID DXVA_Intel_ModeAV1_VLD_420_10b =
    { 0xf9a16190, 0x3fb4, 0x4dc5, { 0x98, 0x46, 0xc8, 0x75, 0x1f, 0x83, 0xd6, 0xd7 } };
#endif

    inline
    bool CheckGUID(VideoCORE* core, eMFXHWType type, mfxVideoParam const* par)
    {
        mfxVideoParam vp = *par;
        mfxU16 profile = vp.mfx.CodecProfile & 0xFF;
        if (profile == MFX_PROFILE_UNKNOWN)
        {
            profile = MatchProfile(vp.mfx.FrameInfo.FourCC);;
            vp.mfx.CodecProfile = profile;
        }

#if defined (MFX_VA_LINUX)
        if (core->IsGuidSupported(DXVA_Intel_ModeAV1_VLD, &vp) != MFX_ERR_NONE)
            return false;

        switch (profile)
        {
            case MFX_PROFILE_AV1_MAIN:
                return true;
            default:
                return false;
        }
#else
        (void)core;
        return false;
#endif
    }

    eMFXPlatform GetPlatform(VideoCORE* core, mfxVideoParam const* par)
    {
        VM_ASSERT(core);
        VM_ASSERT(par);

        if (!par)
            return MFX_PLATFORM_SOFTWARE;

        eMFXPlatform platform = core->GetPlatformType();
        return
            platform != MFX_PLATFORM_SOFTWARE && !CheckGUID(core, core->GetHWType(), par) ?
            MFX_PLATFORM_SOFTWARE : platform;
    }
}

static void SetFrameType(const UMC_AV1_DECODER::AV1DecoderFrame& frame, mfxFrameSurface1 &surface_out)
{
    auto extFrameInfo = reinterpret_cast<mfxExtDecodedFrameInfo *>(GetExtendedBuffer(surface_out.Data.ExtParam, surface_out.Data.NumExtParam, MFX_EXTBUFF_DECODED_FRAME_INFO));
    if (extFrameInfo == nullptr)
        return;

    const UMC_AV1_DECODER::FrameHeader& fh = frame.GetFrameHeader();
    switch (fh.frame_type)
    {
    case UMC_AV1_DECODER::KEY_FRAME:
        extFrameInfo->FrameType = MFX_FRAMETYPE_I;
        break;
    case UMC_AV1_DECODER::INTER_FRAME:
    case UMC_AV1_DECODER::INTRA_ONLY_FRAME:
    case UMC_AV1_DECODER::SWITCH_FRAME:
        extFrameInfo->FrameType = MFX_FRAMETYPE_P;
        break;
    default:
        extFrameInfo->FrameType = MFX_FRAMETYPE_UNKNOWN;
    }
}

VideoDECODEAV1::VideoDECODEAV1(VideoCORE* core, mfxStatus* sts)
    : m_core(core)
    , m_platform(MFX_PLATFORM_SOFTWARE)
    , m_opaque(false)
    , m_first_run(true)
    , m_request()
    , m_response()
    , m_is_init(false)
    , m_in_framerate(0)
{
    if (sts)
    {
        *sts = MFX_ERR_NONE;
    }
}

VideoDECODEAV1::~VideoDECODEAV1()
{
    if (m_is_init)
    {
        Close();
    }
}

mfxStatus VideoDECODEAV1::Init(mfxVideoParam* par)
{
    MFX_CHECK_NULL_PTR1(par);

    std::lock_guard<std::mutex> guard(m_guard);

    MFX_CHECK(!m_decoder, MFX_ERR_UNDEFINED_BEHAVIOR);

    m_platform = MFX_VPX_Utility::GetPlatform(m_core, par);
    eMFXHWType type = MFX_HW_UNKNOWN;
    if (m_platform == MFX_PLATFORM_HARDWARE)
    {
        type = m_core->GetHWType();
    }

    MFX_CHECK(CheckVideoParamDecoders(par, m_core->IsExternalFrameAllocator(), type) >= MFX_ERR_NONE, MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(MFX_VPX_Utility::CheckVideoParam(par, MFX_CODEC_AV1, m_platform), MFX_ERR_INVALID_VIDEO_PARAM);

    m_first_par = (mfxVideoParamWrapper)(*par);

    MFX_CHECK(m_platform != MFX_PLATFORM_SOFTWARE, MFX_ERR_UNSUPPORTED);
#if !defined (MFX_VA)
    MFX_RETURN(MFX_ERR_UNSUPPORTED);

#else
    m_decoder.reset(new UMC_AV1_DECODER::AV1DecoderVA());
    m_allocator.reset(new mfx_UMC_FrameAllocator_D3D());
#endif

    m_request = {};
    m_response = {};

    mfxStatus sts = MFX_VPX_Utility::QueryIOSurfInternal(par, &m_request);
    MFX_CHECK_STS(sts);

    m_init_par = (mfxVideoParamWrapper)(*par);

    if (0 == m_init_par.mfx.FrameInfo.FrameRateExtN || 0 == m_init_par.mfx.FrameInfo.FrameRateExtD)
    {
        m_init_par.mfx.FrameInfo.FrameRateExtD = 1;
        m_init_par.mfx.FrameInfo.FrameRateExtN = 30;
    }

    m_first_par = m_init_par;
    m_in_framerate = (mfxF64) m_first_par.mfx.FrameInfo.FrameRateExtD / m_first_par.mfx.FrameInfo.FrameRateExtN;
    m_decoder->SetInFrameRate(m_in_framerate);

    //mfxFrameAllocResponse response{};
    bool internal = ((m_platform == MFX_PLATFORM_SOFTWARE) ?
        (par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) : (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY));

    if (!(par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
    {
        if (!internal)
            m_request.AllocId = par->AllocId;

        sts = m_core->AllocFrames(&m_request, &m_response, internal);
    }
    else
    {
        auto opaq =
            reinterpret_cast<mfxExtOpaqueSurfaceAlloc*>(GetExtendedBuffer(par->ExtParam, par->NumExtParam,MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION));

        MFX_CHECK(opaq && m_request.NumFrameMin <= opaq->Out.NumSurface, MFX_ERR_INVALID_VIDEO_PARAM);

        m_opaque = true;

        m_request.Type = MFX_MEMTYPE_FROM_DECODE | MFX_MEMTYPE_OPAQUE_FRAME;
        m_request.Type |= (opaq->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY) ?
            MFX_MEMTYPE_SYSTEM_MEMORY : MFX_MEMTYPE_DXVA2_DECODER_TARGET;

        m_request.NumFrameMin       = opaq->Out.NumSurface;
        m_request.NumFrameSuggested = m_request.NumFrameMin;

        sts = m_core->AllocFrames(&m_request, &m_response,
            opaq->Out.Surfaces, opaq->Out.NumSurface);
    }

    MFX_CHECK_STS(sts);
    if (!internal || m_opaque)
    {
        m_allocator->SetExternalFramesResponse(&m_response);
    }

    UMC::Status umcSts = m_allocator->InitMfx(0, m_core, par, &m_request, &m_response, !internal, m_platform == MFX_PLATFORM_SOFTWARE);
    MFX_CHECK(umcSts == UMC::UMC_OK, MFX_ERR_MEMORY_ALLOC);

    UMC_AV1_DECODER::AV1DecoderParams vp{};
    vp.allocator = m_allocator.get();
    vp.async_depth = par->AsyncDepth;
    vp.film_grain = par->mfx.FilmGrain ? 1 : 0; // 0 - film grain is forced off, 1 - film grain is controlled by apply_grain syntax parameter
    if (!vp.async_depth)
        vp.async_depth = MFX_AUTO_ASYNC_DEPTH_VALUE;
    vp.io_pattern = par->IOPattern;

#if defined (MFX_VA)
    sts = m_core->CreateVA(par, &m_request, &m_response, m_allocator.get());
    MFX_CHECK_STS(sts);

    m_core->GetVA((mfxHDL*)&vp.pVideoAccelerator, MFX_MEMTYPE_FROM_DECODE);
#endif

    ConvertMFXParamsToUMC(par, &vp);

    umcSts = m_decoder->Init(&vp);
    MFX_CHECK(umcSts == UMC::UMC_OK, MFX_ERR_NOT_INITIALIZED);

    m_first_run = true;
    m_is_init = true;

    return MFX_ERR_NONE;
}

// Check if new parameters are compatible with old parameters
bool VideoDECODEAV1::IsNeedChangeVideoParam(mfxVideoParam * newPar, mfxVideoParam * oldPar, eMFXHWType /*type*/) const
{
    if ((newPar->IOPattern & (MFX_IOPATTERN_OUT_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY)) !=
        (oldPar->IOPattern & (MFX_IOPATTERN_OUT_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY)))
    {
        return false;
    }

    if (CalculateAsyncDepth(m_platform, newPar) != CalculateAsyncDepth(m_platform, oldPar))
    {
        return false;
    }

    mfxFrameAllocRequest requestOld{};
    mfxFrameAllocRequest requestNew{};

    mfxStatus mfxSts = MFX_VPX_Utility::QueryIOSurfInternal(oldPar, &requestOld);

    if (mfxSts != MFX_ERR_NONE)
        return false;

    mfxSts = MFX_VPX_Utility::QueryIOSurfInternal(newPar, &requestNew);

    if (mfxSts != MFX_ERR_NONE)
        return false;

    if (newPar->mfx.FrameInfo.Height > oldPar->mfx.FrameInfo.Height)
    {
        return false;
    }

    if (newPar->mfx.FrameInfo.Width > oldPar->mfx.FrameInfo.Width)
    {
        return false;
    }

    if (m_response.NumFrameActual)
    {
        if (requestNew.NumFrameMin > m_response.NumFrameActual)
            return false;
    }
    else
    {
        if (requestNew.NumFrameMin > requestOld.NumFrameMin || requestNew.Type != requestOld.Type)
            return false;
    }

    if (newPar->mfx.FrameInfo.FourCC != oldPar->mfx.FrameInfo.FourCC)
    {
        return false;
    }

    if (newPar->mfx.FrameInfo.ChromaFormat != oldPar->mfx.FrameInfo.ChromaFormat)
    {
        return false;
    }

    if (oldPar->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc * opaqueNew = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(newPar->ExtParam, newPar->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        mfxExtOpaqueSurfaceAlloc * opaqueOld = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(oldPar->ExtParam, oldPar->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

        if (!opaqueNew || !opaqueOld)
            return false;
            
        if (opaqueNew->In.Type != opaqueOld->In.Type)
            return false;

        if (opaqueNew->In.NumSurface != opaqueOld->In.NumSurface)
            return false;

        for (uint32_t i = 0; i < opaqueNew->In.NumSurface; i++)
        {
            if (opaqueNew->In.Surfaces[i] != opaqueOld->In.Surfaces[i])
                return false;
        }

        if (opaqueNew->Out.Type != opaqueOld->Out.Type)
            return false;

        if (opaqueNew->Out.NumSurface != opaqueOld->Out.NumSurface)
            return false;

        for (uint32_t i = 0; i < opaqueNew->Out.NumSurface; i++)
        {
            if (opaqueNew->Out.Surfaces[i] != opaqueOld->Out.Surfaces[i])
                return false;
        }
    }

    return true;
}


mfxStatus VideoDECODEAV1::Reset(mfxVideoParam* par)
{
    std::lock_guard<std::mutex> guard(m_guard);

    MFX_CHECK_NULL_PTR1(par);

    MFX_CHECK(m_is_init, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK(m_decoder, MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK(m_core, MFX_ERR_UNDEFINED_BEHAVIOR);

    eMFXHWType type = m_core->GetHWType();
    eMFXPlatform platform = MFX_VPX_Utility::GetPlatform(m_core, par);

    MFX_CHECK(CheckVideoParamDecoders(par, m_core->IsExternalFrameAllocator(), type) >= MFX_ERR_NONE, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(MFX_VPX_Utility::CheckVideoParam(par, MFX_CODEC_AV1, m_platform), MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(IsNeedChangeVideoParam(par, &m_init_par, type), MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    MFX_CHECK(m_platform == platform, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    MFX_CHECK(m_allocator->Reset() == UMC::UMC_OK, MFX_ERR_MEMORY_ALLOC);

    m_first_par = *par;

    if (0 == m_first_par.mfx.FrameInfo.FrameRateExtN || 0 == m_first_par.mfx.FrameInfo.FrameRateExtD)
    {
        m_first_par.mfx.FrameInfo.FrameRateExtD = 1;
        m_first_par.mfx.FrameInfo.FrameRateExtN = 30;
    }

    m_in_framerate = (mfxF64) m_first_par.mfx.FrameInfo.FrameRateExtD / m_first_par.mfx.FrameInfo.FrameRateExtN;
    m_decoder->SetInFrameRate(m_in_framerate);

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEAV1::Close()
{
    std::lock_guard<std::mutex> guard(m_guard);

    MFX_CHECK(m_is_init, MFX_ERR_NOT_INITIALIZED);

    m_allocator->Close();

    if (0 < m_response.NumFrameActual)
    {
        m_core->FreeFrames(&m_response);
    }

    m_request = {};
    m_response = {};
    m_is_init = false;

    return MFX_ERR_NONE;
}

mfxTaskThreadingPolicy VideoDECODEAV1::GetThreadingPolicy()
{
    return MFX_TASK_THREADING_SHARED;
}

inline
mfxStatus CheckLevel(mfxVideoParam* in, mfxVideoParam* out)
{
    MFX_CHECK_NULL_PTR1(out);

    mfxStatus sts = MFX_ERR_NONE;

    if (in)
    {
        switch(in->mfx.CodecLevel)
        {
        case MFX_LEVEL_AV1_2:
        case MFX_LEVEL_AV1_21:
        case MFX_LEVEL_AV1_22:
        case MFX_LEVEL_AV1_23:
        case MFX_LEVEL_AV1_3:
        case MFX_LEVEL_AV1_31:
        case MFX_LEVEL_AV1_32:
        case MFX_LEVEL_AV1_33:
        case MFX_LEVEL_AV1_4:
        case MFX_LEVEL_AV1_41:
        case MFX_LEVEL_AV1_42:
        case MFX_LEVEL_AV1_43:
        case MFX_LEVEL_AV1_5:
        case MFX_LEVEL_AV1_51:
        case MFX_LEVEL_AV1_52:
        case MFX_LEVEL_AV1_53:
        case MFX_LEVEL_AV1_6:
        case MFX_LEVEL_AV1_61:
        case MFX_LEVEL_AV1_62:
        case MFX_LEVEL_AV1_63:
            out->mfx.CodecLevel = in->mfx.CodecLevel;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }
    }
    else
        out->mfx.CodecLevel = MFX_LEVEL_AV1_2;

    return sts;
}

mfxStatus VideoDECODEAV1::Query(VideoCORE* core, mfxVideoParam* in, mfxVideoParam* out)
{
    MFX_CHECK(core, MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK_NULL_PTR1(out);

    if (in)
    {
        MFX_CHECK((in->mfx.FrameInfo.PicStruct & (MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_BFF | MFX_PICSTRUCT_FIELD_SINGLE)) == 0, MFX_ERR_UNSUPPORTED);
        MFX_CHECK(in->Protected == 0, MFX_ERR_UNSUPPORTED);
        MFX_CHECK(in->Protected == 0, MFX_ERR_UNSUPPORTED);
        MFX_CHECK(in->mfx.ExtendedPicStruct == 0, MFX_ERR_UNSUPPORTED);
    }

    mfxStatus sts = MFX_VPX_Utility::Query(core, in, out, MFX_CODEC_AV1, core->GetHWType());
    MFX_CHECK_STS(sts);

    eMFXPlatform platform = MFX_VPX_Utility::GetPlatform(core, out);
    if (platform != core->GetPlatformType())
    {
#ifdef MFX_VA
        sts = MFX_ERR_UNSUPPORTED;
#endif
    }

    sts = CheckLevel(in, out);
    MFX_CHECK_STS(sts);

    if (in)
        out->mfx.FilmGrain = in->mfx.FilmGrain;
    else
        out->mfx.FilmGrain = 1;

    MFX_CHECK_STS(sts);
    return sts;
}

mfxStatus VideoDECODEAV1::QueryIOSurf(VideoCORE* core, mfxVideoParam* par, mfxFrameAllocRequest* request)
{
    MFX_CHECK(core, MFX_ERR_UNDEFINED_BEHAVIOR)
    MFX_CHECK_NULL_PTR2(par, request);

    MFX_CHECK(
        par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY  ||
        par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY ||
        par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY,
        MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(!(
        par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY &&
        par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY),
        MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(!(
        par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY &&
        par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY),
        MFX_ERR_INVALID_VIDEO_PARAM);

    MFX_CHECK(!(
        par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY &&
        par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY),
        MFX_ERR_INVALID_VIDEO_PARAM);

    mfxStatus sts = MFX_ERR_NONE;
    if (!(par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
        sts = MFX_VPX_Utility::QueryIOSurfInternal(par, request);
    else
    {
        request->Info = par->mfx.FrameInfo;
        request->NumFrameMin = 1;
        request->NumFrameSuggested = request->NumFrameMin + (par->AsyncDepth ? par->AsyncDepth : MFX_AUTO_ASYNC_DEPTH_VALUE);
        request->Type = MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_FROM_DECODE;
    }

    sts = UpdateCscOutputFormat(par, request);
    MFX_CHECK_STS(sts);

    request->Type |= par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY ?
        MFX_MEMTYPE_OPAQUE_FRAME : MFX_MEMTYPE_EXTERNAL_FRAME;

    return sts;
}

inline
UMC::Status FillParam(VideoCORE *core, mfxVideoParam *par)
{
    if (MFX_VPX_Utility::GetPlatform(core, par) != MFX_PLATFORM_SOFTWARE)
    {
        if (par->mfx.FrameInfo.FourCC == MFX_FOURCC_P010
#if (MFX_VERSION >= 1027)
            || par->mfx.FrameInfo.FourCC == MFX_FOURCC_Y210
#endif
#if (MFX_VERSION >= 1330)
            || par->mfx.FrameInfo.FourCC == MFX_FOURCC_P016
            || par->mfx.FrameInfo.FourCC == MFX_FOURCC_Y216
            || par->mfx.FrameInfo.FourCC == MFX_FOURCC_Y416
#endif
            )

            par->mfx.FrameInfo.Shift = 1;
    }

    return UMC::UMC_OK;
}

mfxStatus VideoDECODEAV1::DecodeHeader(VideoCORE* core, mfxBitstream* bs, mfxVideoParam* par)
{
    MFX_CHECK(core, MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK_NULL_PTR3(bs, bs->Data, par);

    mfxStatus sts = MFX_ERR_NONE;

    try
    {
        MFXMediaDataAdapter in(bs);

        UMC_AV1_DECODER::AV1DecoderParams vp;
        sts = ConvertStatusUmc2Mfx(UMC_AV1_DECODER::AV1Decoder::DecodeHeader(&in, vp));
        if (sts == MFX_ERR_MORE_DATA || sts == MFX_ERR_MORE_SURFACE)
            return sts;
        MFX_CHECK_STS(sts);

        sts = FillVideoParam(core, &vp, par);
        if (sts == MFX_ERR_MORE_DATA || sts == MFX_ERR_MORE_SURFACE)
            return sts;
        MFX_CHECK_STS(sts);
    }
    catch (UMC_AV1_DECODER::av1_exception const &ex)
    {
        sts = ConvertStatusUmc2Mfx(ex.GetStatus());
    }


    return sts;
}

mfxStatus VideoDECODEAV1::GetVideoParam(mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR1(par);
    MFX_CHECK(m_decoder, MFX_ERR_NOT_INITIALIZED);

    std::lock_guard<std::mutex> guard(m_guard);

    mfxStatus sts = MFX_ERR_NONE;
    UMC_AV1_DECODER::AV1DecoderParams vp;

    UMC::Status umcRes = m_decoder->GetInfo(&vp);
    MFX_CHECK(umcRes == UMC::UMC_OK, MFX_ERR_UNKNOWN);

    // fills par->mfx structure
    sts = FillVideoParam(m_core, &vp, par);
    MFX_CHECK_STS(sts);

    par->AsyncDepth = static_cast<mfxU16>(vp.async_depth);
    par->IOPattern = static_cast<mfxU16>(vp.io_pattern  & (MFX_IOPATTERN_OUT_VIDEO_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY));

    par->mfx.FrameInfo.FrameRateExtN = m_init_par.mfx.FrameInfo.FrameRateExtN;
    par->mfx.FrameInfo.FrameRateExtD = m_init_par.mfx.FrameInfo.FrameRateExtD;

    if (!par->mfx.FrameInfo.AspectRatioH && !par->mfx.FrameInfo.AspectRatioW)
    {
        if (m_init_par.mfx.FrameInfo.AspectRatioH || m_init_par.mfx.FrameInfo.AspectRatioW)
        {
            par->mfx.FrameInfo.AspectRatioH = m_init_par.mfx.FrameInfo.AspectRatioH;
            par->mfx.FrameInfo.AspectRatioW = m_init_par.mfx.FrameInfo.AspectRatioW;
        }
        else
        {
            par->mfx.FrameInfo.AspectRatioH = 1;
            par->mfx.FrameInfo.AspectRatioW = 1;
        }
    }

    switch (par->mfx.FrameInfo.FourCC)
    {
    case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
#endif
#if (MFX_VERSION >= 1330)
    case MFX_FOURCC_P016:
    case MFX_FOURCC_Y216:
    case MFX_FOURCC_Y416:
#endif
        par->mfx.FrameInfo.Shift = 1;
    default:
        break;
    }

    return MFX_ERR_NONE;
}


mfxStatus VideoDECODEAV1::GetDecodeStat(mfxDecodeStat* stat)
{
    MFX_CHECK_NULL_PTR1(stat);

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEAV1::DecodeFrameCheck(mfxBitstream* bs, mfxFrameSurface1* surface_work, mfxFrameSurface1** surface_out, MFX_ENTRY_POINT* entry_point)
{
    MFX_CHECK_NULL_PTR1(entry_point);

    std::lock_guard<std::mutex> guard(m_guard);

    MFX_CHECK(m_core, MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK(m_decoder, MFX_ERR_NOT_INITIALIZED);

    //gpu session priority
    UMC_AV1_DECODER::AV1DecoderParams* av1DecoderParams = m_decoder->GetAv1DecoderParams();
    if (av1DecoderParams->pVideoAccelerator != nullptr)
        av1DecoderParams->pVideoAccelerator->m_ContextPriority = m_core->GetSession()->m_priority;

    mfxStatus sts = SubmitFrame(bs, surface_work, surface_out);

    if (sts == MFX_ERR_MORE_DATA || sts == MFX_ERR_MORE_SURFACE)
        return sts;

    MFX_CHECK_STS(sts);

    std::unique_ptr<TaskInfo> info(new TaskInfo);

    info->surface_work = GetOriginalSurface(surface_work);
    if (*surface_out)
        info->surface_out = GetOriginalSurface(*surface_out);

    mfxThreadTask task =
        reinterpret_cast<mfxThreadTask>(info.release());

    entry_point->pRoutine = &DecodeRoutine;
    entry_point->pCompleteProc = &CompleteProc;
    entry_point->pState = this;
    entry_point->requiredNumThreads = 1;
    entry_point->pParam = task;

    return sts;
}

mfxStatus VideoDECODEAV1::SetSkipMode(mfxSkipMode /*mode*/)
{
    MFX_RETURN(MFX_ERR_UNSUPPORTED);
}

mfxStatus VideoDECODEAV1::GetPayload(mfxU64* /*time_stamp*/, mfxPayload* /*pPayload*/)
{
    MFX_RETURN(MFX_ERR_UNSUPPORTED);
}

mfxStatus VideoDECODEAV1::DecodeFrame(mfxFrameSurface1 *surface_out, AV1DecoderFrame* frame)
{
    MFX_CHECK(surface_out, MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK(frame, MFX_ERR_UNDEFINED_BEHAVIOR);

    surface_out->Data.Corrupted = 0;
    int32_t const error = frame->GetError();

    if (error & UMC::ERROR_FRAME_DEVICE_FAILURE)
    {
        surface_out->Data.Corrupted |= MFX_CORRUPTION_MAJOR;
        MFX_CHECK(error != UMC::UMC_ERR_GPU_HANG, MFX_ERR_GPU_HANG);
        MFX_RETURN(MFX_ERR_DEVICE_FAILED);
    }
    else
    {
        if (error & UMC::ERROR_FRAME_MINOR)
            surface_out->Data.Corrupted |= MFX_CORRUPTION_MINOR;

        if (error & UMC::ERROR_FRAME_MAJOR)
            surface_out->Data.Corrupted |= MFX_CORRUPTION_MAJOR;

        if (error & UMC::ERROR_FRAME_REFERENCE_FRAME)
            surface_out->Data.Corrupted |= MFX_CORRUPTION_REFERENCE_FRAME;

        if (error & UMC::ERROR_FRAME_DPB)
            surface_out->Data.Corrupted |= MFX_CORRUPTION_REFERENCE_LIST;

        if (error & UMC::ERROR_FRAME_RECOVERY)
            surface_out->Data.Corrupted |= MFX_CORRUPTION_MAJOR;

        if (error & UMC::ERROR_FRAME_TOP_FIELD_ABSENT)
            surface_out->Data.Corrupted |= MFX_CORRUPTION_ABSENT_TOP_FIELD;

        if (error & UMC::ERROR_FRAME_BOTTOM_FIELD_ABSENT)
            surface_out->Data.Corrupted |= MFX_CORRUPTION_ABSENT_BOTTOM_FIELD;
    }

    UMC::FrameMemID id = frame->GetFrameData()->GetFrameMID();
    mfxStatus sts = m_allocator->PrepareToOutput(surface_out, id, &m_video_par, m_opaque);
#ifdef MFX_VA
    frame->Displayed(true);
#else
    frame->Reset();
#endif

    return sts;
}

mfxStatus VideoDECODEAV1::QueryFrame(mfxThreadTask task)
{
    MFX_CHECK_NULL_PTR1(task);

    MFX_CHECK(m_core, MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK(m_decoder, MFX_ERR_NOT_INITIALIZED);

    auto info =
        reinterpret_cast<TaskInfo*>(task);

    mfxFrameSurface1* surface_out = info->surface_out;
    MFX_CHECK(surface_out, MFX_ERR_UNDEFINED_BEHAVIOR);

    UMC::FrameMemID id = m_allocator->FindSurface(surface_out, m_opaque);
    UMC_AV1_DECODER::AV1DecoderFrame* frame =
        m_decoder->FindFrameByMemID(id);

    MFX_CHECK(frame, MFX_ERR_UNDEFINED_BEHAVIOR);

    MFX_CHECK(frame->DecodingStarted(), MFX_ERR_UNDEFINED_BEHAVIOR);

    if (!frame->DecodingCompleted())
    {
        m_decoder->QueryFrames();
    }

    MFX_CHECK(frame->DecodingCompleted(), MFX_TASK_WORKING);

    mfxStatus sts = DecodeFrame(surface_out, frame);
    MFX_CHECK_STS(sts);

    return MFX_TASK_DONE;
}

static mfxStatus CheckFrameInfo(mfxFrameInfo &info)
{
    MFX_SAFE_CALL(CheckFrameInfoCommon(&info, MFX_CODEC_AV1));

    switch (info.FourCC)
    {
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_AYUV:
    case MFX_FOURCC_YUY2:
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y410:
#endif
        break;
#if (MFX_VERSION >= 1330)
    case MFX_FOURCC_Y216:
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

    switch (info.ChromaFormat)
    {
    case MFX_CHROMAFORMAT_YUV420:
    case MFX_CHROMAFORMAT_YUV422:
    case MFX_CHROMAFORMAT_YUV444:
        break;
    default:
        MFX_CHECK_STS(MFX_ERR_INVALID_VIDEO_PARAM);
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEAV1::SubmitFrame(mfxBitstream* bs, mfxFrameSurface1* surface_work, mfxFrameSurface1** surface_out)
{
    MFX_CHECK_NULL_PTR2(surface_work, surface_out);

    bool workSfsIsClean =
        !(surface_work->Data.MemId || surface_work->Data.Y
            || surface_work->Data.UV || surface_work->Data.R
            || surface_work->Data.A);

    if (m_opaque)
    {
        MFX_CHECK(workSfsIsClean, MFX_ERR_UNDEFINED_BEHAVIOR);
        // work with the native (original) surface
        surface_work = GetOriginalSurface(surface_work);
        MFX_CHECK(surface_work, MFX_ERR_UNDEFINED_BEHAVIOR);
    }
    else
    {
        MFX_CHECK(!workSfsIsClean, MFX_ERR_LOCK_MEMORY);
    }

    mfxStatus sts = CheckFrameInfo(surface_work->Info);
    MFX_CHECK_STS(sts);

    sts = CheckFrameData(surface_work);
    MFX_CHECK_STS(sts);

    if (!bs)
        return MFX_ERR_MORE_DATA;

    sts = CheckBitstream(bs);
    MFX_CHECK_STS(sts);

    if (surface_work->Data.Locked)
        return MFX_ERR_MORE_SURFACE;

    sts = m_allocator->SetCurrentMFXSurface(surface_work, m_opaque);
    MFX_CHECK_STS(sts);

    try
    {
        MFXMediaDataAdapter src(bs);

        for (;;)
        {
            UMC::Status umcRes = m_allocator->FindFreeSurface() != -1 ?
                m_decoder->GetFrame(bs ? &src : 0, nullptr) : UMC::UMC_ERR_NEED_FORCE_OUTPUT;

            UMC::Status umcFrameRes = umcRes;

             if (umcRes == UMC::UMC_NTF_NEW_RESOLUTION ||
                 umcRes == UMC::UMC_WRN_REPOSITION_INPROGRESS ||
                 umcRes == UMC::UMC_ERR_UNSUPPORTED)
            {
                 UMC_AV1_DECODER::AV1DecoderParams vp;
                 umcRes = m_decoder->GetInfo(&vp);
                 FillVideoParam(m_core, &vp, &m_video_par);
                 m_video_par.AsyncDepth = static_cast<mfxU16>(vp.async_depth);
            }

            if (umcFrameRes == UMC::UMC_NTF_NEW_RESOLUTION)
            {
                MFX_RETURN(MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);
            }

            if (umcRes == UMC::UMC_ERR_INVALID_STREAM)
            {
                umcRes = UMC::UMC_OK;
            }

            switch (umcRes)
            {
                case UMC::UMC_OK:
                    if (m_allocator->FindFreeSurface() == -1)
                    {
                        sts = MFX_ERR_MORE_SURFACE;
                        umcFrameRes = UMC::UMC_ERR_NOT_ENOUGH_BUFFER;
                    }
                    break;

                case UMC::UMC_NTF_NEW_RESOLUTION:
                    MFX_RETURN(MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

#if defined(MFX_VA)
                case UMC::UMC_ERR_DEVICE_FAILED:
                    sts = MFX_ERR_DEVICE_FAILED;
                    break;

                case UMC::UMC_ERR_GPU_HANG:
                    sts = MFX_ERR_GPU_HANG;
                    break;
#endif

                case UMC::UMC_ERR_NOT_ENOUGH_BUFFER:
                case UMC::UMC_WRN_INFO_NOT_READY:
                case UMC::UMC_ERR_NEED_FORCE_OUTPUT:
                    sts = umcRes == UMC::UMC_ERR_NOT_ENOUGH_BUFFER ? (mfxStatus)MFX_ERR_MORE_DATA_SUBMIT_TASK: MFX_WRN_DEVICE_BUSY;
                    break;

                case UMC::UMC_ERR_NOT_ENOUGH_DATA:
                case UMC::UMC_ERR_SYNC:
                    sts = MFX_ERR_MORE_DATA;
                    break;

                default:
                    if (sts < 0)
                        sts = MFX_ERR_UNDEFINED_BEHAVIOR;
                    break;
            }


            src.Save(bs);

            if (sts == MFX_ERR_INCOMPATIBLE_VIDEO_PARAM)
            {
                MFX_CHECK_STS(sts);
            }

            if (sts == MFX_ERR_DEVICE_FAILED || sts == MFX_ERR_GPU_HANG || sts == MFX_ERR_UNDEFINED_BEHAVIOR)
            {
                MFX_CHECK_STS(sts);
            }

            UMC_AV1_DECODER::AV1DecoderFrame* frame = m_decoder->GetCurrFrame();
            if (frame && bs->TimeStamp != static_cast<mfxU64>(MFX_TIMESTAMP_UNKNOWN))
                frame->SetFrameTime(GetUmcTimeStamp(bs->TimeStamp));

            frame = GetFrameToDisplay();
            if (frame)
            {
                sts = FillOutputSurface(surface_out, surface_work, frame);
                return MFX_ERR_NONE;
            }

            if (umcFrameRes != UMC::UMC_OK)
                break;

        } // for (;;)
    }
    catch(UMC_AV1_DECODER::av1_exception const &ex)
    {
        sts = ConvertUMCStatusToMfx(ex.GetStatus());
    }
    catch (std::bad_alloc const&)
    {
        sts = MFX_ERR_MEMORY_ALLOC;
    }
    catch(...)
    {
        sts = MFX_ERR_UNKNOWN;
    }

    if (sts == MFX_ERR_MORE_DATA)
        return sts;

    MFX_CHECK_STS(sts);

    return sts;
}

mfxStatus VideoDECODEAV1::DecodeRoutine(void* state, void* param, mfxU32, mfxU32)
{
    auto decoder = reinterpret_cast<VideoDECODEAV1*>(state);
    MFX_CHECK(decoder, MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxThreadTask task =
        reinterpret_cast<TaskInfo*>(param);

    MFX_CHECK(task, MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxStatus sts = decoder->QueryFrame(task);
    MFX_CHECK_STS(sts);
    return sts;
}

mfxStatus VideoDECODEAV1::CompleteProc(void*, void* param, mfxStatus)
{
    auto info =
        reinterpret_cast<TaskInfo*>(param);
    delete info;

    return MFX_ERR_NONE;
}

inline
mfxU16 color_format2bit_depth(UMC::ColorFormat format)
{
    switch (format)
    {
        case UMC::NV12:
        case UMC::YUY2:
        case UMC::AYUV: return 8;

        case UMC::P010:
        case UMC::Y210:
        case UMC::Y410:  return 10;

        case UMC::P016:
        case UMC::Y216:
        case UMC::Y416:  return 12;

        default:         return 0;
    }
}

inline
mfxU16 color_format2chroma_format(UMC::ColorFormat format)
{
    switch (format)
    {
        case UMC::NV12:
        case UMC::P010:
        case UMC::P016: return MFX_CHROMAFORMAT_YUV420;

        case UMC::YUY2:
        case UMC::Y210:
        case UMC::Y216: return MFX_CHROMAFORMAT_YUV422;

        case UMC::AYUV:
        case UMC::Y410:
        case UMC::Y416: return MFX_CHROMAFORMAT_YUV444;

        default:        return MFX_CHROMAFORMAT_YUV420;
    }
}

inline
mfxU16 av1_native_profile_to_mfx_profile(mfxU16 native)
{
    switch (native)
    {
    case 0: return MFX_PROFILE_AV1_MAIN;
    case 1: return MFX_PROFILE_AV1_HIGH;
    case 2: return MFX_PROFILE_AV1_PRO;
    default: return 0;
    }
}

mfxStatus VideoDECODEAV1::FillVideoParam(VideoCORE* core, UMC_AV1_DECODER::AV1DecoderParams const* vp, mfxVideoParam* par)
{
    VM_ASSERT(vp);
    VM_ASSERT(par);

    mfxVideoParam p{};
    ConvertUMCParamsToMFX(vp, &p);

    p.mfx.FrameInfo.BitDepthLuma =
    p.mfx.FrameInfo.BitDepthChroma =
        color_format2bit_depth(vp->info.color_format);

    p.mfx.FrameInfo.ChromaFormat =
        color_format2chroma_format(vp->info.color_format);

    par->mfx.FrameInfo            = p.mfx.FrameInfo;
    par->mfx.CodecProfile         = av1_native_profile_to_mfx_profile(p.mfx.CodecProfile);
    par->mfx.CodecLevel           = p.mfx.CodecLevel;
    par->mfx.DecodedOrder         = p.mfx.DecodedOrder;
    par->mfx.MaxDecFrameBuffering = p.mfx.MaxDecFrameBuffering;

    par->mfx.FilmGrain = static_cast<mfxU16>(vp->film_grain);

    if (MFX_VPX_Utility::GetPlatform(core, par) != MFX_PLATFORM_SOFTWARE)
    {
        if (par->mfx.FrameInfo.FourCC == MFX_FOURCC_P010
#if (MFX_VERSION >= 1027)
            || par->mfx.FrameInfo.FourCC == MFX_FOURCC_Y210
#endif
#if (MFX_VERSION >= 1330)
            || par->mfx.FrameInfo.FourCC == MFX_FOURCC_P016
            || par->mfx.FrameInfo.FourCC == MFX_FOURCC_Y216
            || par->mfx.FrameInfo.FourCC == MFX_FOURCC_Y416
#endif
            )

            par->mfx.FrameInfo.Shift = 1;
    }

    return MFX_ERR_NONE;
}

UMC_AV1_DECODER::AV1DecoderFrame* VideoDECODEAV1::GetFrameToDisplay()
{
    VM_ASSERT(m_decoder);

    UMC_AV1_DECODER::AV1DecoderFrame* frame
        = m_decoder->GetFrameToDisplay();

    if (!frame)
        return nullptr;

    frame->Outputted(true);
    frame->ShowAsExisting(false);

    return frame;
}

inline void CopyFilmGrainParam(mfxExtAV1FilmGrainParam &extBuf, UMC_AV1_DECODER::FilmGrainParams const& par)
{
    extBuf.FilmGrainFlags = 0;

    if (par.apply_grain)
        extBuf.FilmGrainFlags |= MFX_FILM_GRAIN_APPLY;

    extBuf.GrainSeed = (mfxU16)par.grain_seed;

    if (par.update_grain)
        extBuf.FilmGrainFlags |= MFX_FILM_GRAIN_UPDATE;

    extBuf.RefIdx = (mfxU8)par.film_grain_params_ref_idx;

    extBuf.NumYPoints = (mfxU8)par.num_y_points;
    for (int i = 0; i < UMC_AV1_DECODER::MAX_POINTS_IN_SCALING_FUNCTION_LUMA; i++)
    {
        extBuf.PointY[i].Value = (mfxU8)par.point_y_value[i];
        extBuf.PointY[i].Scaling = (mfxU8)par.point_y_scaling[i];
    }

    if (par.chroma_scaling_from_luma)
        extBuf.FilmGrainFlags |= MFX_FILM_GRAIN_CHROMA_SCALING_FROM_LUMA;

    extBuf.NumCbPoints = (mfxU8)par.num_cb_points;
    extBuf.NumCrPoints = (mfxU8)par.num_cr_points;
    for (int i = 0; i < UMC_AV1_DECODER::MAX_POINTS_IN_SCALING_FUNCTION_CHROMA; i++)
    {
        extBuf.PointCb[i].Value = (mfxU8)par.point_cb_value[i];
        extBuf.PointCb[i].Scaling = (mfxU8)par.point_cb_scaling[i];
        extBuf.PointCr[i].Value = (mfxU8)par.point_cr_value[i];
        extBuf.PointCr[i].Scaling = (mfxU8)par.point_cr_scaling[i];
    }

    extBuf.GrainScalingMinus8 = (mfxU8)par.grain_scaling - 8;
    extBuf.ArCoeffLag = (mfxU8)par.ar_coeff_lag;

    for (int i = 0; i < UMC_AV1_DECODER::MAX_AUTOREG_COEFFS_LUMA; i++)
        extBuf.ArCoeffsYPlus128[i] = (mfxU8)(par.ar_coeffs_y[i] + 128);

    for (int i = 0; i < UMC_AV1_DECODER::MAX_AUTOREG_COEFFS_CHROMA; i++)
    {
        extBuf.ArCoeffsCbPlus128[i] = (mfxU8)(par.ar_coeffs_cb[i] + 128);
        extBuf.ArCoeffsCrPlus128[i] = (mfxU8)(par.ar_coeffs_cr[i] + 128);
    }

    extBuf.ArCoeffShiftMinus6 = (mfxU8)par.ar_coeff_shift - 6;
    extBuf.GrainScaleShift = (mfxU8)par.grain_scale_shift;
    extBuf.CbMult = (mfxU8)par.cb_mult;
    extBuf.CbLumaMult = (mfxU8)par.cb_luma_mult;
    extBuf.CbOffset = (mfxU16)par.cb_offset;
    extBuf.CrMult = (mfxU8)par.cr_mult;
    extBuf.CrLumaMult = (mfxU8)par.cr_luma_mult;
    extBuf.CrOffset = (mfxU16)par.cr_offset;

    if (par.overlap_flag)
        extBuf.FilmGrainFlags |= MFX_FILM_GRAIN_OVERLAP;

    if (par.clip_to_restricted_range)
        extBuf.FilmGrainFlags |= MFX_FILM_GRAIN_CLIP_TO_RESTRICTED_RANGE;
}

mfxStatus VideoDECODEAV1::FillOutputSurface(mfxFrameSurface1** surf_out, mfxFrameSurface1* surface_work, UMC_AV1_DECODER::AV1DecoderFrame* pFrame)
{
    MFX_CHECK_NULL_PTR3(surface_work, pFrame, surf_out);

    UMC::FrameData const* fd = pFrame->GetFrameData();
    MFX_CHECK(fd, MFX_ERR_DEVICE_FAILED);

    mfxVideoParam vp;
    *surf_out = m_allocator->GetSurface(fd->GetFrameMID(), surface_work, &vp);
    if (m_opaque)
       *surf_out = m_core->GetOpaqSurface((*surf_out)->Data.MemId);

    mfxFrameSurface1* surface_out = *surf_out;
    MFX_CHECK(surface_out, MFX_ERR_INVALID_HANDLE);

    SetFrameType(*pFrame, *surface_out);

    surface_out->Info.FrameId.TemporalId = 0;

    UMC::VideoDataInfo const* vi = fd->GetInfo();
    MFX_CHECK(vi, MFX_ERR_DEVICE_FAILED);

    surface_out->Data.TimeStamp = GetMfxTimeStamp(pFrame->FrameTime());
    surface_out->Data.FrameOrder = pFrame->FrameOrder();
    surface_out->Data.Corrupted = 0;
    surface_out->Info.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    surface_out->Info.CropW = static_cast<mfxU16>(pFrame->GetUpscaledWidth());
    surface_out->Info.CropH = static_cast<mfxU16>(pFrame->GetFrameHeight());
    surface_out->Info.AspectRatioW = 1;
    surface_out->Info.AspectRatioH = 1;

    bool isShouldUpdate = !(m_first_par.mfx.FrameInfo.FrameRateExtD || m_first_par.mfx.FrameInfo.FrameRateExtN);

    surface_out->Info.FrameRateExtD = isShouldUpdate ? m_init_par.mfx.FrameInfo.FrameRateExtD : m_first_par.mfx.FrameInfo.FrameRateExtD;
    surface_out->Info.FrameRateExtN = isShouldUpdate ? m_init_par.mfx.FrameInfo.FrameRateExtN : m_first_par.mfx.FrameInfo.FrameRateExtN;

    mfxExtAV1FilmGrainParam* extFilmGrain = (mfxExtAV1FilmGrainParam*)GetExtendedBuffer(surface_out->Data.ExtParam, surface_out->Data.NumExtParam, MFX_EXTBUFF_AV1_FILM_GRAIN_PARAM);
    if (extFilmGrain)
    {
        UMC_AV1_DECODER::FrameHeader const& fh = pFrame->GetFrameHeader();
        CopyFilmGrainParam(*extFilmGrain, fh.film_grain_params);
    }

    return MFX_ERR_NONE;
}

#endif
