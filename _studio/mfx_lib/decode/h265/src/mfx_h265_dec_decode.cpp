// Copyright (c) 2017-2018 Intel Corporation
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

#include "mfx_h265_dec_decode.h"

#if defined (MFX_ENABLE_H265_VIDEO_DECODE)

#include "mfx_common.h"
#include "mfx_common_decode_int.h"

#include "umc_h265_mfx_supplier.h"
#include "umc_h265_mfx_utils.h"
#include "umc_h265_frame_list.h"

#include "vm_sys_info.h"

#include "umc_h265_va_supplier.h"

inline
mfxU32 CalculateAsyncDepth(eMFXPlatform platform, mfxVideoParam *par)
{
    mfxU32 asyncDepth = par->AsyncDepth;
    if (!asyncDepth)
    {
        asyncDepth = (platform == MFX_PLATFORM_SOFTWARE) ? vm_sys_info_get_cpu_num() : MFX_AUTO_ASYNC_DEPTH_VALUE;
    }

    return asyncDepth;
}

inline
mfxU32 CalculateNumThread(mfxVideoParam *par, eMFXPlatform platform)
{
    mfxU32 numThread = (MFX_PLATFORM_SOFTWARE == platform) ? vm_sys_info_get_cpu_num() : 1;
    if (!par->AsyncDepth)
        return numThread;

    return MFX_MIN(par->AsyncDepth, numThread);
}

inline
bool IsNeedToUseHWBuffering(eMFXHWType /*type*/)
{
    return false;
}

inline
mfxU16 UMC2MFX_PicStruct(int dps, bool extended)
{
    switch (dps)
    {
        case UMC_HEVC_DECODER::DPS_FRAME_H265:
            return MFX_PICSTRUCT_PROGRESSIVE;

        case UMC_HEVC_DECODER::DPS_TOP_H265:
            return MFX_PICSTRUCT_FIELD_TOP;
        case UMC_HEVC_DECODER::DPS_BOTTOM_H265:
            return MFX_PICSTRUCT_FIELD_BOTTOM;

        case UMC_HEVC_DECODER::DPS_TOP_BOTTOM_H265:
            return MFX_PICSTRUCT_PROGRESSIVE | (extended ? MFX_PICSTRUCT_FIELD_TFF: 0);
        case UMC_HEVC_DECODER::DPS_BOTTOM_TOP_H265:
            return MFX_PICSTRUCT_PROGRESSIVE | (extended ? MFX_PICSTRUCT_FIELD_BFF: 0);

        case UMC_HEVC_DECODER::DPS_TOP_BOTTOM_TOP_H265:
            return MFX_PICSTRUCT_PROGRESSIVE | (extended ? MFX_PICSTRUCT_FIELD_REPEATED | MFX_PICSTRUCT_FIELD_TFF: 0);
        case UMC_HEVC_DECODER::DPS_BOTTOM_TOP_BOTTOM_H265:
            return MFX_PICSTRUCT_PROGRESSIVE | (extended ? MFX_PICSTRUCT_FIELD_REPEATED | MFX_PICSTRUCT_FIELD_BFF: 0);

        case UMC_HEVC_DECODER::DPS_FRAME_DOUBLING_H265:
            return MFX_PICSTRUCT_PROGRESSIVE | (extended ? MFX_PICSTRUCT_FRAME_DOUBLING : 0);
        case UMC_HEVC_DECODER::DPS_FRAME_TRIPLING_H265:
            return MFX_PICSTRUCT_PROGRESSIVE | (extended ? MFX_PICSTRUCT_FRAME_TRIPLING : 0);

        case DPS_TOP_BOTTOM_PREV_H265:
            return MFX_PICSTRUCT_FIELD_TOP | (extended ? MFX_PICSTRUCT_FIELD_PAIRED_PREV : 0);
        case DPS_BOTTOM_TOP_PREV_H265:
            return MFX_PICSTRUCT_FIELD_BOTTOM | (extended ? MFX_PICSTRUCT_FIELD_PAIRED_PREV : 0);

        case DPS_TOP_BOTTOM_NEXT_H265:
            return MFX_PICSTRUCT_FIELD_TOP | (extended ? MFX_PICSTRUCT_FIELD_PAIRED_NEXT : 0);
        case DPS_BOTTOM_TOP_NEXT_H265:
            return MFX_PICSTRUCT_FIELD_BOTTOM | (extended ? MFX_PICSTRUCT_FIELD_PAIRED_NEXT : 0);

        default:
            return MFX_PICSTRUCT_UNKNOWN;
    }
}

struct ThreadTaskInfo
{
    ThreadTaskInfo()
        : surface_work(0)
        , surface_out(0)
        , taskID(0)
        , pFrame(0)
    {
    }

    mfxFrameSurface1 *surface_work;
    mfxFrameSurface1 *surface_out;
    mfxU32            taskID; // for task ordering
    H265DecoderFrame *pFrame;
};

enum
{
    ENABLE_DELAYED_DISPLAY_MODE = 1
};

VideoDECODEH265::VideoDECODEH265(VideoCORE *core, mfxStatus * sts)
    : VideoDECODE()
    , m_core(core)
    , m_isInit(false)
    , m_isOpaq(false)
    , m_globalTask(false)
    , m_frameOrder((mfxU16)MFX_FRAMEORDER_UNKNOWN)
    , m_response()
    , m_response_alien()
    , m_platform(MFX_PLATFORM_SOFTWARE)
    , m_useDelayedDisplay(false)
    , m_va(0)
    , m_isFirstRun(true)
{
    memset(&m_stat, 0, sizeof(m_stat));
    memset(&m_response, 0, sizeof(m_response));
    memset(&m_response_alien, 0, sizeof(m_response_alien));

    if (sts)
    {
        *sts = MFX_ERR_NONE;
    }
}

VideoDECODEH265::~VideoDECODEH265(void)
{
    Close();
}

// Initialize decoder instance
mfxStatus VideoDECODEH265::Init(mfxVideoParam *par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEH265::Init");
    UMC::AutomaticUMCMutex guard(m_mGuard);

    if (m_isInit)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    m_globalTask = false;

    MFX_CHECK_NULL_PTR1(par);


    m_platform = MFX_Utility::GetPlatform_H265(m_core, par);

    eMFXHWType type = MFX_HW_UNKNOWN;
    if (m_platform == MFX_PLATFORM_HARDWARE)
    {
        type = m_core->GetHWType();
    }

    if (CheckVideoParamDecoders(par, m_core->IsExternalFrameAllocator(), type) < MFX_ERR_NONE)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (!MFX_Utility::CheckVideoParam_H265(par, type))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    m_vInitPar = *par;
    m_vFirstPar = *par;
    m_vFirstPar.mfx.NumThread = 0;

    bool isNeedChangeVideoParamWarning = IsNeedChangeVideoParam(&m_vFirstPar);

    m_vPar = m_vFirstPar;
    m_vPar.CreateExtendedBuffer(MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
    m_vPar.CreateExtendedBuffer(MFX_EXTBUFF_CODING_OPTION_SPSPPS);
    m_vPar.CreateExtendedBuffer(MFX_EXTBUFF_HEVC_PARAM);

    mfxU32 asyncDepth = CalculateAsyncDepth(m_platform, par);
    m_vPar.mfx.NumThread = (mfxU16)CalculateNumThread(par, m_platform);

    if (MFX_PLATFORM_SOFTWARE == m_platform)
    {
        return MFX_ERR_UNSUPPORTED;
    }
    else
    {
        m_useDelayedDisplay = ENABLE_DELAYED_DISPLAY_MODE != 0 && IsNeedToUseHWBuffering(m_core->GetHWType()) && (asyncDepth != 1);

        bool useBigSurfacePoolWA = MFX_Utility::IsBugSurfacePoolApplicable(type, par);

            m_pH265VideoDecoder.reset(useBigSurfacePoolWA ? new VATaskSupplierBigSurfacePool<VATaskSupplier>() : new VATaskSupplier()); // HW
        m_FrameAllocator.reset(new mfx_UMC_FrameAllocator_D3D());
    }

    int32_t useInternal = (MFX_PLATFORM_SOFTWARE == m_platform) ?
        (m_vPar.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) : (m_vPar.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY);

    if (m_vPar.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc *pOpaqAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (!pOpaqAlloc)
            return MFX_ERR_INVALID_VIDEO_PARAM;

        useInternal = (m_platform == MFX_PLATFORM_SOFTWARE) ? !(pOpaqAlloc->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY) : (pOpaqAlloc->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY);
    }

    // allocate memory
    mfxFrameAllocRequest request;
    mfxFrameAllocRequest request_internal;
    memset(&request, 0, sizeof(request));
    memset(&m_response, 0, sizeof(m_response));
    memset(&m_response_alien, 0, sizeof(m_response_alien));
    m_isOpaq = false;

    mfxStatus mfxSts = QueryIOSurfInternal(m_platform, type, &m_vPar, &request);
    if (mfxSts != MFX_ERR_NONE)
        return mfxSts;

    if (useInternal)
        request.Type |= MFX_MEMTYPE_INTERNAL_FRAME
        ;
    else
        request.Type |= MFX_MEMTYPE_EXTERNAL_FRAME;

    request_internal = request;

    // allocates external surfaces:
    bool mapOpaq = true;
    mfxExtOpaqueSurfaceAlloc *pOpqAlloc = 0;
    mfxSts = UpdateAllocRequest(par, &request, pOpqAlloc, mapOpaq);
    if (mfxSts < MFX_ERR_NONE)
        return mfxSts;

    if (m_isOpaq && !m_core->IsCompatibleForOpaq())
        return MFX_ERR_UNDEFINED_BEHAVIOR;


    if (mapOpaq)
    {
        mfxSts = m_core->AllocFrames(&request,
                                      &m_response,
                                      pOpqAlloc->Out.Surfaces,
                                      pOpqAlloc->Out.NumSurface);
    }
    else
    {
        if (m_platform != MFX_PLATFORM_SOFTWARE && !useInternal)
        {
            request.AllocId = par->AllocId;
            mfxSts = m_core->AllocFrames(&request, &m_response, false);
        }
    }

    if (mfxSts < MFX_ERR_NONE)
        return mfxSts;

    // allocates internal surfaces:
    if (useInternal)
    {
        m_response_alien = m_response;
        m_FrameAllocator->SetExternalFramesResponse(&m_response_alien);
        request = request_internal;

        if (m_platform != MFX_PLATFORM_SOFTWARE)
        {
            if (   par->mfx.FrameInfo.FourCC == MFX_FOURCC_P010
                )

                request.Info.Shift = 1;
        }

        mfxSts = m_core->AllocFrames(&request_internal, &m_response, true);
        if (mfxSts < MFX_ERR_NONE)
            return mfxSts;
    }
    else
    {
        m_FrameAllocator->SetExternalFramesResponse(&m_response);
    }

    if (m_platform != MFX_PLATFORM_SOFTWARE)
    {
        mfxSts = m_core->CreateVA(&m_vFirstPar, &request, &m_response, m_FrameAllocator.get());
        if (mfxSts < MFX_ERR_NONE)
            return mfxSts;
    }

    UMC::Status umcSts = m_FrameAllocator->InitMfx(0, m_core, &m_vFirstPar, &request, &m_response, !useInternal, m_platform == MFX_PLATFORM_SOFTWARE);
    if (umcSts != UMC::UMC_OK)
        return MFX_ERR_MEMORY_ALLOC;

    umcSts = m_MemoryAllocator.InitMem(0, m_core);
    if (umcSts != UMC::UMC_OK)
        return MFX_ERR_MEMORY_ALLOC;

    m_pH265VideoDecoder->SetFrameAllocator(m_FrameAllocator.get());

    UMC::VideoDecoderParams umcVideoParams;
    ConvertMFXParamsToUMC(&m_vFirstPar, &umcVideoParams);
    umcVideoParams.numThreads = m_vPar.mfx.NumThread;
    umcVideoParams.info.bitrate = MFX_MAX(asyncDepth - umcVideoParams.numThreads, 0); // buffered frames

    if (MFX_PLATFORM_SOFTWARE != m_platform)
    {
        m_core->GetVA((mfxHDL*)&m_va, MFX_MEMTYPE_FROM_DECODE);
        umcVideoParams.pVideoAccelerator = m_va;
        static_cast<VATaskSupplier*>(m_pH265VideoDecoder.get())->SetVideoHardwareAccelerator(m_va);

    }

    umcVideoParams.lpMemoryAllocator = &m_MemoryAllocator;

    umcSts = m_pH265VideoDecoder->Init(&umcVideoParams);
    if (umcSts != UMC::UMC_OK)
    {
        return ConvertUMCStatusToMfx(umcSts);
    }

    m_isInit = true;

    m_frameOrder = (mfxU16)MFX_FRAMEORDER_UNKNOWN;
    m_isFirstRun = true;

    if (MFX_PLATFORM_SOFTWARE != m_platform && m_useDelayedDisplay)
    {
        static_cast<VATaskSupplier*>(m_pH265VideoDecoder.get())->SetBufferedFramesNumber(NUMBER_OF_ADDITIONAL_FRAMES);
    }

    m_pH265VideoDecoder->SetVideoParams(&m_vFirstPar);

    if (m_platform != m_core->GetPlatformType())
    {
        VM_ASSERT(m_platform == MFX_PLATFORM_SOFTWARE);
            return MFX_ERR_UNSUPPORTED;
    }

    if (isNeedChangeVideoParamWarning)
    {
        return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;
}

// Reset decoder with new parameters
mfxStatus VideoDECODEH265::Reset(mfxVideoParam *par)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    m_globalTask = false;

    MFX_CHECK_NULL_PTR1(par);

    eMFXHWType type = MFX_HW_UNKNOWN;
    if (m_platform == MFX_PLATFORM_HARDWARE)
    {
        type = m_core->GetHWType();
    }

    eMFXPlatform platform = MFX_Utility::GetPlatform_H265(m_core, par);

    if (CheckVideoParamDecoders(par, m_core->IsExternalFrameAllocator(), type) < MFX_ERR_NONE)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (!MFX_Utility::CheckVideoParam_H265(par, type))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (!IsSameVideoParam(par, &m_vInitPar, type))
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    if (m_platform != platform)
    {
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
    }

    m_pH265VideoDecoder->Reset();

    if (m_FrameAllocator->Reset() != UMC::UMC_OK)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    m_frameOrder = (mfxU16)MFX_FRAMEORDER_UNKNOWN;
    m_isFirstRun = true;

    memset(&m_stat, 0, sizeof(m_stat));
    m_vFirstPar = *par;

    bool isNeedChangeVideoParamWarning = IsNeedChangeVideoParam(&m_vFirstPar);
    m_vPar = m_vFirstPar;
    m_vPar.CreateExtendedBuffer(MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
    m_vPar.CreateExtendedBuffer(MFX_EXTBUFF_CODING_OPTION_SPSPPS);
    m_vPar.CreateExtendedBuffer(MFX_EXTBUFF_HEVC_PARAM);

    m_vPar.mfx.NumThread = (mfxU16)CalculateNumThread(par, m_platform);


    m_pH265VideoDecoder->SetVideoParams(&m_vFirstPar);

    if (m_platform != m_core->GetPlatformType())
    {
        VM_ASSERT(m_platform == MFX_PLATFORM_SOFTWARE);
            return MFX_ERR_UNSUPPORTED;
    }

    if (isNeedChangeVideoParamWarning)
    {
        return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;
}

// Free decoder resources
mfxStatus VideoDECODEH265::Close(void)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEH265::Close");
    UMC::AutomaticUMCMutex guard(m_mGuard);

    if (!m_isInit || !m_pH265VideoDecoder.get())
        return MFX_ERR_NOT_INITIALIZED;

    m_pH265VideoDecoder->Close();
    m_FrameAllocator->Close();

    if (m_response.NumFrameActual)
        m_core->FreeFrames(&m_response);

    if (m_response_alien.NumFrameActual)
        m_core->FreeFrames(&m_response_alien);

    m_isOpaq = false;
    m_isInit = false;
    m_isFirstRun = true;
    m_frameOrder = (mfxU16)MFX_FRAMEORDER_UNKNOWN;
    m_va = 0;
    memset(&m_stat, 0, sizeof(m_stat));

    return MFX_ERR_NONE;
}

// Returns decoder threading mode
mfxTaskThreadingPolicy VideoDECODEH265::GetThreadingPolicy(void)
{
    return MFX_TASK_THREADING_SHARED;
}

// MediaSDK DECODE_Query API function
mfxStatus VideoDECODEH265::Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out)
{
    MFX_CHECK_NULL_PTR1(out);

    eMFXPlatform platform = MFX_Utility::GetPlatform_H265(core, in);

    eMFXHWType type = MFX_HW_UNKNOWN;
    if (platform == MFX_PLATFORM_HARDWARE)
    {
        type = core->GetHWType();
    }

    return MFX_Utility::Query_H265(core, in, out, type);
}

// MediaSDK DECODE_GetVideoParam API function
mfxStatus VideoDECODEH265::GetVideoParam(mfxVideoParam *par)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR1(par);

    FillVideoParam(&m_vPar, true);

    par->mfx = m_vPar.mfx;

    par->Protected = m_vPar.Protected;
    par->IOPattern = m_vPar.IOPattern;
    par->AsyncDepth = m_vPar.AsyncDepth;


    mfxExtVideoSignalInfo * videoSignal = (mfxExtVideoSignalInfo *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
    if (videoSignal)
    {
        mfxExtVideoSignalInfo * videoSignalInternal = m_vPar.GetExtendedBuffer<mfxExtVideoSignalInfo>(MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
        *videoSignal = *videoSignalInternal;
    }

    mfxExtHEVCParam * hevcParam = (mfxExtHEVCParam *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_HEVC_PARAM);
    if (hevcParam)
    {
        mfxExtHEVCParam * hevcParamInternal = m_vPar.GetExtendedBuffer<mfxExtHEVCParam>(MFX_EXTBUFF_HEVC_PARAM);
        *hevcParam = *hevcParamInternal;
    }

    // sps/pps headers
    mfxExtCodingOptionSPSPPS * spsPps = (mfxExtCodingOptionSPSPPS *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_CODING_OPTION_SPSPPS);
    if (spsPps)
    {
        mfxExtCodingOptionSPSPPS * spsPpsInternal = m_vPar.GetExtendedBuffer<mfxExtCodingOptionSPSPPS>(MFX_EXTBUFF_CODING_OPTION_SPSPPS);

        spsPps->SPSId = spsPpsInternal->SPSId;
        spsPps->PPSId = spsPpsInternal->PPSId;

        if (spsPps->SPSBufSize < spsPpsInternal->SPSBufSize ||
            spsPps->PPSBufSize < spsPpsInternal->PPSBufSize)
            return MFX_ERR_NOT_ENOUGH_BUFFER;

        spsPps->SPSBufSize = spsPpsInternal->SPSBufSize;
        spsPps->PPSBufSize = spsPpsInternal->PPSBufSize;

        memcpy_s(spsPps->SPSBuffer, spsPps->SPSBufSize, spsPpsInternal->SPSBuffer, spsPps->SPSBufSize);
        memcpy_s(spsPps->PPSBuffer, spsPps->PPSBufSize, spsPpsInternal->PPSBuffer, spsPps->PPSBufSize);
    }

    par->mfx.FrameInfo.FrameRateExtN = m_vFirstPar.mfx.FrameInfo.FrameRateExtN;
    par->mfx.FrameInfo.FrameRateExtD = m_vFirstPar.mfx.FrameInfo.FrameRateExtD;

    if (!par->mfx.FrameInfo.FrameRateExtD && !par->mfx.FrameInfo.FrameRateExtN)
    {
        par->mfx.FrameInfo.FrameRateExtD = m_vPar.mfx.FrameInfo.FrameRateExtD;
        par->mfx.FrameInfo.FrameRateExtN = m_vPar.mfx.FrameInfo.FrameRateExtN;

        if (!par->mfx.FrameInfo.FrameRateExtD && !par->mfx.FrameInfo.FrameRateExtN)
        {
            par->mfx.FrameInfo.FrameRateExtN = 30;
            par->mfx.FrameInfo.FrameRateExtD = 1;
        }
    }

    par->mfx.FrameInfo.AspectRatioW = m_vFirstPar.mfx.FrameInfo.AspectRatioW;
    par->mfx.FrameInfo.AspectRatioH = m_vFirstPar.mfx.FrameInfo.AspectRatioH;

    if (!par->mfx.FrameInfo.AspectRatioH && !par->mfx.FrameInfo.AspectRatioW)
    {
        par->mfx.FrameInfo.AspectRatioH = m_vPar.mfx.FrameInfo.AspectRatioH;
        par->mfx.FrameInfo.AspectRatioW = m_vPar.mfx.FrameInfo.AspectRatioW;

        if (!par->mfx.FrameInfo.AspectRatioH && !par->mfx.FrameInfo.AspectRatioW)
        {
            par->mfx.FrameInfo.AspectRatioH = 1;
            par->mfx.FrameInfo.AspectRatioW = 1;
        }
    }

    return MFX_ERR_NONE;
}

// Decode bitstream header and exctract parameters from it
mfxStatus VideoDECODEH265::DecodeHeader(VideoCORE *core, mfxBitstream *bs, mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR2(bs, par);

    mfxStatus sts = CheckBitstream(bs);
    if (sts != MFX_ERR_NONE)
        return sts;

    MFXMediaDataAdapter in(bs);

    mfx_UMC_MemAllocator  tempAllocator;
    tempAllocator.InitMem(0, core);

    UMC::VideoDecoderParams avcInfo;
    avcInfo.m_pData = &in;

    MFX_AVC_Decoder_H265 decoder;

    decoder.SetMemoryAllocator(&tempAllocator);
    UMC::Status umcRes = MFX_Utility::DecodeHeader(&decoder, &avcInfo, bs, par);

    if (umcRes == UMC::UMC_ERR_NOT_ENOUGH_DATA)
        return MFX_ERR_MORE_DATA;
    else if (umcRes != UMC::UMC_OK)
        return ConvertUMCStatusToMfx(umcRes);

    umcRes = decoder.FillVideoParam(par, false);
    if (umcRes != UMC::UMC_OK)
        return ConvertUMCStatusToMfx(umcRes);

    if (MFX_Utility::GetPlatform_H265(core, par) != MFX_PLATFORM_SOFTWARE)
    {
        if (   par->mfx.FrameInfo.FourCC == MFX_FOURCC_P010
            )

            par->mfx.FrameInfo.Shift = 1;
    }

    // sps/pps headers
    mfxExtCodingOptionSPSPPS * spsPps = (mfxExtCodingOptionSPSPPS *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_CODING_OPTION_SPSPPS);
    if (spsPps)
    {
        RawHeader_H265 *sps = decoder.GetSPS();
        RawHeader_H265 *pps = decoder.GetPPS();

        if (sps->GetSize())
        {
            if (spsPps->SPSBufSize < sps->GetSize())
                return MFX_ERR_NOT_ENOUGH_BUFFER;

            spsPps->SPSBufSize = (mfxU16)sps->GetSize();
            memcpy_s(spsPps->SPSBuffer, spsPps->SPSBufSize, sps->GetPointer(), spsPps->SPSBufSize);
        }
        else
        {
            spsPps->SPSBufSize = 0;
        }

        if (pps->GetSize())
        {
            if (spsPps->PPSBufSize < pps->GetSize())
                return MFX_ERR_NOT_ENOUGH_BUFFER;

            spsPps->PPSBufSize = (mfxU16)pps->GetSize();
            memcpy_s(spsPps->PPSBuffer, spsPps->PPSBufSize, pps->GetPointer(), spsPps->PPSBufSize);
        }
        else
        {
            spsPps->PPSBufSize = 0;
        }
    }

    return MFX_ERR_NONE;
}

// MediaSDK DECODE_QueryIOSurf API function
mfxStatus VideoDECODEH265::QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEH265::QueryIOSurf");
    MFX_CHECK_NULL_PTR2(par, request);

    eMFXPlatform platform = MFX_Utility::GetPlatform_H265(core, par);

    eMFXHWType type = MFX_HW_UNKNOWN;
    if (platform == MFX_PLATFORM_HARDWARE)
    {
        type = core->GetHWType();
    }

    mfxVideoParam params;
    params = *par;
    bool isNeedChangeVideoParamWarning = IsNeedChangeVideoParam(&params);

    if (!(par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && !(par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) && !(par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if ((par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if ((par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) && (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if ((par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) && (par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    int32_t isInternalManaging = (MFX_PLATFORM_SOFTWARE == platform) ?
        (params.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) : (params.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY);

    mfxStatus sts = QueryIOSurfInternal(platform, type, &params, request);
    if (sts != MFX_ERR_NONE)
        return sts;

    if (isInternalManaging)
    {
        request->NumFrameSuggested = request->NumFrameMin = (mfxU16)CalculateAsyncDepth(platform, par);

        if (MFX_PLATFORM_SOFTWARE == platform)
            request->Type = MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_FROM_DECODE;
        else
            request->Type = MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_FROM_DECODE;
    }

    if (par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        request->Type |= MFX_MEMTYPE_OPAQUE_FRAME;
    }
    else
    {
        request->Type |= MFX_MEMTYPE_EXTERNAL_FRAME;
    }

    if (platform != core->GetPlatformType())
    {
        VM_ASSERT(platform == MFX_PLATFORM_SOFTWARE);
            return MFX_ERR_UNSUPPORTED;
    }

    if (isNeedChangeVideoParamWarning)
    {
        return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;
}

// Actually calculate needed frames number
mfxStatus VideoDECODEH265::QueryIOSurfInternal(eMFXPlatform platform, eMFXHWType type, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoDECODEH265::QueryIOSurfInternal");
    request->Info = par->mfx.FrameInfo;

    mfxU32 asyncDepth = CalculateAsyncDepth(platform, par);
    bool useDelayedDisplay = (ENABLE_DELAYED_DISPLAY_MODE != 0) && IsNeedToUseHWBuffering(type) && (asyncDepth != 1);

    mfxExtHEVCParam * hevcParam = (mfxExtHEVCParam *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_HEVC_PARAM);

    if (hevcParam && (!hevcParam->PicWidthInLumaSamples || !hevcParam->PicHeightInLumaSamples)) //  not initialized
        hevcParam = 0;

    mfxI32 dpbSize = 0;

    uint32_t level_idc = par->mfx.CodecLevel;
    if (hevcParam)
        dpbSize = CalculateDPBSize(par->mfx.CodecProfile, level_idc, hevcParam->PicWidthInLumaSamples, hevcParam->PicHeightInLumaSamples, 0);
    else
        dpbSize = CalculateDPBSize(par->mfx.CodecProfile, level_idc, par->mfx.FrameInfo.Width, par->mfx.FrameInfo.Height, 0) + 1; //1 extra for avoid aligned size issue

    if (par->mfx.MaxDecFrameBuffering && par->mfx.MaxDecFrameBuffering < dpbSize)
        dpbSize = par->mfx.MaxDecFrameBuffering;

    mfxU32 numMin = dpbSize + 1 + asyncDepth;
    if (platform != MFX_PLATFORM_SOFTWARE && useDelayedDisplay) // equals if (m_useDelayedDisplay)
        numMin += NUMBER_OF_ADDITIONAL_FRAMES;
    request->NumFrameMin = (mfxU16)numMin;

    request->NumFrameSuggested = request->NumFrameMin;

    if (MFX_PLATFORM_SOFTWARE == platform)
    {
        request->Type = MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_FROM_DECODE;
    }
    else
    {
        request->Type = MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_FROM_DECODE;
    }

    return MFX_ERR_NONE;
}

// MediaSDK DECODE_GetDecodeStat API function
mfxStatus VideoDECODEH265::GetDecodeStat(mfxDecodeStat *stat)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR1(stat);

    m_stat.NumSkippedFrame = m_pH265VideoDecoder->GetSkipInfo().numberOfSkippedFrames;
    m_stat.NumCachedFrame = 0;

    H265DecoderFrame *pFrame = m_pH265VideoDecoder->GetDPBList()->head();
    for (; pFrame; pFrame = pFrame->future())
    {
        if (!pFrame->wasOutputted() && !isAlmostDisposable(pFrame))
            m_stat.NumCachedFrame++;
    }

    *stat = m_stat;
    return MFX_ERR_NONE;
}

// Decoder threads entry point
static mfxStatus __CDECL HEVCDECODERoutine(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 )
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "HEVCDECODERoutine");
    VideoDECODEH265 *decoder = (VideoDECODEH265 *)pState;

    mfxStatus sts = decoder->RunThread(pParam, threadNumber);
    return sts;
}

// Threads complete proc callback
static mfxStatus HEVCCompleteProc(void *, void *pParam, mfxStatus )
{
    delete (ThreadTaskInfo *)pParam;
    return MFX_ERR_NONE;
}

// Decoder instance threads entry point. Do async tasks here
mfxStatus VideoDECODEH265::RunThread(void * params, mfxU32 threadNumber)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEH265::RunThread");
    ThreadTaskInfo * info = (ThreadTaskInfo *)params;

    mfxStatus sts = MFX_TASK_WORKING;

    if (!info->surface_work)
    {
        return MFX_TASK_DONE;
    }

    if (!info->surface_out)
    {
        for (int32_t i = 0; i < 2 && sts == MFX_TASK_WORKING; i++)
        {
            sts = m_pH265VideoDecoder->RunThread(threadNumber);
        }

        UMC::AutomaticUMCMutex guard(m_mGuardRunThread);

        if (sts == MFX_TASK_BUSY && !m_pH265VideoDecoder->GetTaskBroker()->IsEnoughForStartDecoding(true))
            m_globalTask = false;

        return
            m_globalTask ? sts : MFX_TASK_DONE;
    }

    bool isDecoded;
    {
        UMC::AutomaticUMCMutex guard(m_mGuardRunThread);

        if (!info->surface_work)
            return MFX_TASK_DONE;

        isDecoded = m_pH265VideoDecoder->CheckDecoding(true, info->pFrame);
    }

    if (!isDecoded)
    {
        sts = m_pH265VideoDecoder->RunThread(threadNumber);
    }

    {
        UMC::AutomaticUMCMutex guard(m_mGuardRunThread);
        if (!info->surface_work)
            return MFX_TASK_DONE;

        isDecoded = m_pH265VideoDecoder->CheckDecoding(true, info->pFrame);
        if (isDecoded)
        {
            info->surface_work = 0;
        }
    }

    if (isDecoded)
    {
        if (!info->pFrame->wasDisplayed() && info->surface_out)
        {
            mfxStatus status = DecodeFrame(info->surface_out, info->pFrame);

            if (status != MFX_ERR_NONE && status != MFX_ERR_NOT_FOUND)
                return status;
        }

        return MFX_TASK_DONE;
    }

    return sts;
}

// Initialize threads callbacks
mfxStatus VideoDECODEH265::DecodeFrameCheck(mfxBitstream *bs,
                                              mfxFrameSurface1 *surface_work,
                                              mfxFrameSurface1 **surface_out,
                                              MFX_ENTRY_POINT *pEntryPoint)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    mfxStatus mfxSts = DecodeFrameCheck(bs, surface_work, surface_out);

    if (MFX_ERR_NONE == mfxSts || (mfxStatus)MFX_ERR_MORE_DATA_SUBMIT_TASK == mfxSts) // It can be useful to run threads right after first frame receive
    {
        H265DecoderFrame *frame = 0;
        if (*surface_out)
        {
            mfxI32 index = m_FrameAllocator->FindSurface(GetOriginalSurface(*surface_out), m_isOpaq);
            frame = m_pH265VideoDecoder->FindSurface((UMC::FrameMemID)index);
        }
        else
        {
            UMC::AutomaticUMCMutex mGuard(m_mGuardRunThread);

            H265DecoderFrame *pFrame = m_pH265VideoDecoder->GetDPBList()->head();
            for (; pFrame; pFrame = pFrame->future())
            {
                if (!pFrame->m_pic_output && !pFrame->IsDecoded())
                {
                    frame = pFrame;
                    break;
                }
            }

            if (!frame)
            {
                if (m_pH265VideoDecoder->GetTaskBroker()->IsEnoughForStartDecoding(true) && !m_globalTask)
                    m_globalTask = true;
                else
                    return MFX_WRN_DEVICE_BUSY;
            }
        }

        ThreadTaskInfo * info = new ThreadTaskInfo();

        info->surface_work = GetOriginalSurface(surface_work);

        if (*surface_out)
            info->surface_out = GetOriginalSurface(*surface_out);

        info->pFrame = frame;
        pEntryPoint->pRoutine = &HEVCDECODERoutine;
        pEntryPoint->pCompleteProc = &HEVCCompleteProc;
        pEntryPoint->pState = this;
        pEntryPoint->requiredNumThreads = m_vPar.mfx.NumThread;
        pEntryPoint->pParam = info;

        return mfxSts;
    }

    return mfxSts;
}

// Check if there is enough data to start decoding in async mode
mfxStatus VideoDECODEH265::DecodeFrameCheck(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "VideoDECODEH265::DecodeFrameCheck");
    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR2(surface_work, surface_out);

    mfxStatus sts = MFX_ERR_NONE;
        sts = bs ? CheckBitstream(bs) : MFX_ERR_NONE;

    if (sts != MFX_ERR_NONE)
        return sts;
    UMC::Status umcRes = UMC::UMC_OK;

    *surface_out = 0;

    if (m_isOpaq)
    {
        sts = CheckFrameInfoCodecs(&surface_work->Info, MFX_CODEC_HEVC, m_platform != MFX_PLATFORM_SOFTWARE);
        if (sts != MFX_ERR_NONE)
            return MFX_ERR_UNSUPPORTED;

        if (surface_work->Data.MemId || surface_work->Data.Y || surface_work->Data.R || surface_work->Data.A || surface_work->Data.UV) // opaq surface
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        surface_work = GetOriginalSurface(surface_work);
        if (!surface_work)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    sts = CheckFrameInfoCodecs(&surface_work->Info, MFX_CODEC_HEVC, m_platform != MFX_PLATFORM_SOFTWARE);
    if (sts != MFX_ERR_NONE)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    sts = CheckFrameData(surface_work);
    if (sts != MFX_ERR_NONE)
        return sts;

    sts = m_FrameAllocator->SetCurrentMFXSurface(surface_work, m_isOpaq);
    if (sts != MFX_ERR_NONE)
        return sts;

#ifdef MFX_MAX_DECODE_FRAMES
    if (m_stat.NumFrame >= MFX_MAX_DECODE_FRAMES)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
#endif

    sts = MFX_ERR_UNDEFINED_BEHAVIOR;


    try
    {
        bool force = false;

        UMC::Status umcFrameRes = UMC::UMC_OK;
        UMC::Status umcAddSourceRes = UMC::UMC_OK;

        MFXMediaDataAdapter src(bs);

        for (;;)
        {
            if (m_FrameAllocator->FindFreeSurface() == -1)
            {
                umcRes = UMC::UMC_ERR_NEED_FORCE_OUTPUT;
            }
            else
            {
                    umcRes = m_pH265VideoDecoder->AddSource(bs ? &src : 0);
            }

            umcAddSourceRes = umcFrameRes = umcRes;

            if (umcRes == UMC::UMC_NTF_NEW_RESOLUTION || umcRes == UMC::UMC_WRN_REPOSITION_INPROGRESS || umcRes == UMC::UMC_ERR_UNSUPPORTED)
            {
                FillVideoParam(&m_vPar, true);
            }

            if (umcRes == UMC::UMC_WRN_REPOSITION_INPROGRESS)
            {
                if (!m_isFirstRun)
                {
                    sts = MFX_WRN_VIDEO_PARAM_CHANGED;
                }
                else
                {
                    umcAddSourceRes = umcFrameRes = umcRes = UMC::UMC_OK;
                    m_isFirstRun = false;
                }
            }

            if (umcRes == UMC::UMC_ERR_INVALID_STREAM)
            {
                umcAddSourceRes = umcFrameRes = umcRes = UMC::UMC_OK;

            }

            if (umcRes == UMC::UMC_NTF_NEW_RESOLUTION)
            {
                sts = MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
            }

            if (umcRes == UMC::UMC_OK && m_FrameAllocator->FindFreeSurface() == -1)
            {
                sts = MFX_ERR_MORE_SURFACE;
                umcFrameRes = UMC::UMC_ERR_NOT_ENOUGH_BUFFER;
            }

            if (umcRes == UMC::UMC_ERR_NOT_ENOUGH_BUFFER || umcRes == UMC::UMC_WRN_INFO_NOT_READY || umcRes == UMC::UMC_ERR_NEED_FORCE_OUTPUT)
            {
                force = (umcRes == UMC::UMC_ERR_NEED_FORCE_OUTPUT);
                sts = umcRes == UMC::UMC_ERR_NOT_ENOUGH_BUFFER ? (mfxStatus)MFX_ERR_MORE_DATA_SUBMIT_TASK: MFX_WRN_DEVICE_BUSY;
            }

            if (umcRes == UMC::UMC_ERR_NOT_ENOUGH_DATA || umcRes == UMC::UMC_ERR_SYNC)
            {
                if (!bs || bs->DataFlag == MFX_BITSTREAM_EOS)
                    force = true;
                sts = MFX_ERR_MORE_DATA;
            }

#if defined (MFX_VA_LINUX)
            if (umcRes == UMC::UMC_ERR_DEVICE_FAILED)
            {
                sts = MFX_ERR_DEVICE_FAILED;
            }
            if (umcRes == UMC::UMC_ERR_GPU_HANG)
            {
                sts = MFX_ERR_GPU_HANG;
            }
#endif

            {
                src.Save(bs);
            }

            if (sts == MFX_ERR_INCOMPATIBLE_VIDEO_PARAM)
                return sts;

            //return these errors immediatelly unless we have [input == 0]
            if (sts == MFX_ERR_DEVICE_FAILED || sts == MFX_ERR_GPU_HANG)
            {
                if (!bs || bs->DataFlag == MFX_BITSTREAM_EOS)
                    force = true;
                else
                    return sts;
            }
            umcRes = m_pH265VideoDecoder->RunDecoding();

            if (m_vInitPar.mfx.DecodedOrder)
                force = true;

            H265DecoderFrame *pFrame = GetFrameToDisplay_H265(force);

            // return frame to display
            if (pFrame)
            {
                FillOutputSurface(surface_out, surface_work, pFrame);

                m_frameOrder = (mfxU16)pFrame->m_frameOrder;
                (*surface_out)->Data.FrameOrder = m_frameOrder;
                return MFX_ERR_NONE;
            }

            *surface_out = 0;

            if (umcFrameRes != UMC::UMC_OK)
                break;
        } // for (;;)
    }
    catch(const h265_exception & ex)
    {
        FillVideoParam(&m_vPar, false);

        if (ex.GetStatus() == UMC::UMC_ERR_ALLOC)
        {
            // check incompatibility of video params
            if (m_vInitPar.mfx.FrameInfo.Width != m_vPar.mfx.FrameInfo.Width ||
                m_vInitPar.mfx.FrameInfo.Height != m_vPar.mfx.FrameInfo.Height)
            {
                return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
            }
        }

        return ConvertUMCStatusToMfx(ex.GetStatus());
    }
    catch(const std::bad_alloc &)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }
    catch(...)
    {
        return MFX_ERR_UNKNOWN;
    }

    return sts;
}

// Fill up resolution information if new header arrived
void VideoDECODEH265::FillVideoParam(mfxVideoParamWrapper *par, bool full)
{
    if (!m_pH265VideoDecoder.get())
        return;

    m_pH265VideoDecoder->FillVideoParam(par, full);

    if (MFX_Utility::GetPlatform_H265(m_core, par) != MFX_PLATFORM_SOFTWARE)
    {
        if (par->mfx.FrameInfo.FourCC == MFX_FOURCC_P010)
            par->mfx.FrameInfo.Shift = 1;
    }

    RawHeader_H265 *sps = m_pH265VideoDecoder->GetSPS();
    RawHeader_H265 *pps = m_pH265VideoDecoder->GetPPS();

    mfxExtCodingOptionSPSPPS * spsPps = (mfxExtCodingOptionSPSPPS *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_CODING_OPTION_SPSPPS);
    if (spsPps)
    {
        if (sps->GetSize())
        {
            spsPps->SPSBufSize = (mfxU16)sps->GetSize();
            spsPps->SPSBuffer = sps->GetPointer();
        }
        else
        {
            spsPps->SPSBufSize = 0;
        }

        if (pps->GetSize())
        {
            spsPps->PPSBufSize = (mfxU16)pps->GetSize();
            spsPps->PPSBuffer = pps->GetPointer();
        }
        else
        {
            spsPps->PPSBufSize = 0;
        }
    }
}

// Fill up frame parameters before returning it to application
void VideoDECODEH265::FillOutputSurface(mfxFrameSurface1 **surf_out, mfxFrameSurface1 *surface_work, H265DecoderFrame * pFrame)
{
    m_stat.NumFrame++;
    m_stat.NumError += pFrame->GetError() ? 1 : 0;
    const UMC::FrameData * fd = pFrame->GetFrameData();

    *surf_out = m_FrameAllocator->GetSurface(fd->GetFrameMID(), surface_work, &m_vPar);
    if(m_isOpaq)
       *surf_out = m_core->GetOpaqSurface((*surf_out)->Data.MemId);
    VM_ASSERT(*surf_out);

    mfxFrameSurface1 *surface_out = *surf_out;

    surface_out->Info.FrameId.TemporalId = 0;

    surface_out->Info.CropH = (mfxU16)(pFrame->lumaSize().height - pFrame->m_crop_bottom - pFrame->m_crop_top);
    surface_out->Info.CropW = (mfxU16)(pFrame->lumaSize().width - pFrame->m_crop_right - pFrame->m_crop_left);
    surface_out->Info.CropX = (mfxU16)(pFrame->m_crop_left);
    surface_out->Info.CropY = (mfxU16)(pFrame->m_crop_top);

    bool isShouldUpdate = !(m_vFirstPar.mfx.FrameInfo.AspectRatioH || m_vFirstPar.mfx.FrameInfo.AspectRatioW);

    surface_out->Info.AspectRatioH = isShouldUpdate ? (mfxU16)pFrame->m_aspect_height : m_vFirstPar.mfx.FrameInfo.AspectRatioH;
    surface_out->Info.AspectRatioW = isShouldUpdate ? (mfxU16)pFrame->m_aspect_width : m_vFirstPar.mfx.FrameInfo.AspectRatioW;

    isShouldUpdate = !(m_vFirstPar.mfx.FrameInfo.FrameRateExtD || m_vFirstPar.mfx.FrameInfo.FrameRateExtN);

    surface_out->Info.FrameRateExtD = isShouldUpdate ? m_vPar.mfx.FrameInfo.FrameRateExtD : m_vFirstPar.mfx.FrameInfo.FrameRateExtD;
    surface_out->Info.FrameRateExtN = isShouldUpdate ? m_vPar.mfx.FrameInfo.FrameRateExtN : m_vFirstPar.mfx.FrameInfo.FrameRateExtN;

    surface_out->Info.PicStruct = 0;
    switch(pFrame->m_chroma_format)
    {
    case 0:
        surface_out->Info.ChromaFormat = MFX_CHROMAFORMAT_YUV400;
        break;
    case 2:
        surface_out->Info.ChromaFormat = MFX_CHROMAFORMAT_YUV422;
        break;
    default:
        surface_out->Info.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        break;
    }

    surface_out->Info.PicStruct =
        UMC2MFX_PicStruct(pFrame->m_DisplayPictureStruct_H265, !!m_vPar.mfx.ExtendedPicStruct);

    surface_out->Data.TimeStamp = GetMfxTimeStamp(pFrame->m_dFrameTime);
    surface_out->Data.FrameOrder = (mfxU32)MFX_FRAMEORDER_UNKNOWN;

    surface_out->Data.DataFlag = (mfxU16)(pFrame->m_isOriginalPTS ? MFX_FRAMEDATA_ORIGINAL_TIMESTAMP : 0);

    SEI_Storer_H265 * storer = m_pH265VideoDecoder->GetSEIStorer();
    if (storer)
        storer->SetTimestamp(pFrame);

    mfxExtDecodedFrameInfo* info = (mfxExtDecodedFrameInfo*)GetExtendedBuffer(surface_out->Data.ExtParam, surface_out->Data.NumExtParam, MFX_EXTBUFF_DECODED_FRAME_INFO);
    if (info)
    {
        switch (pFrame->m_FrameType)
        {
            case UMC::I_PICTURE:
                info->FrameType = MFX_FRAMETYPE_I;
                if (pFrame->GetAU()->m_IsIDR)
                    info->FrameType |= MFX_FRAMETYPE_IDR;

                break;

            case UMC::P_PICTURE:
                info->FrameType = MFX_FRAMETYPE_P;
                break;

            case UMC::B_PICTURE:
                info->FrameType = MFX_FRAMETYPE_B;
                break;

            default:
                VM_ASSERT(!"Unknown frame type");
                info->FrameType = MFX_FRAMETYPE_UNKNOWN;
        }

        if (pFrame->m_isUsedAsReference)
            info->FrameType |= MFX_FRAMETYPE_REF;
   }

}

// Wait until a frame is ready to be output and set necessary surface flags
mfxStatus VideoDECODEH265::DecodeFrame(mfxFrameSurface1 *surface_out, H265DecoderFrame * pFrame)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VideoDECODEH265::DecodeFrame");
    MFX_CHECK_NULL_PTR1(surface_out);

    mfxI32 index;
    if (pFrame)
    {
        index = pFrame->GetFrameData()->GetFrameMID();
    }
    else
    {
        index = m_FrameAllocator->FindSurface(surface_out, m_isOpaq);
        pFrame = m_pH265VideoDecoder->FindSurface((UMC::FrameMemID)index);
        if (!pFrame)
        {
            VM_ASSERT(false);
            return MFX_ERR_NOT_FOUND;
        }
    }

    surface_out->Data.Corrupted = 0;
    int32_t const error = pFrame->GetError();

    if (error & UMC::ERROR_FRAME_DEVICE_FAILURE)
    {
        surface_out->Data.Corrupted |= MFX_CORRUPTION_MAJOR;
        if (error == UMC::UMC_ERR_GPU_HANG)
            return MFX_ERR_GPU_HANG;
        else
            return MFX_ERR_DEVICE_FAILED;
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

    mfxStatus sts = m_FrameAllocator->PrepareToOutput(surface_out, index, &m_vPar, m_isOpaq);

    pFrame->setWasDisplayed();

    return sts;
}

// Wait until a frame is ready to be output and set necessary surface flags
mfxStatus VideoDECODEH265::DecodeFrame(mfxBitstream *, mfxFrameSurface1 *, mfxFrameSurface1 *surface_out)
{
    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR1(surface_out);
    mfxStatus sts = DecodeFrame(surface_out);

    return sts;
}

// Returns closed caption data
mfxStatus VideoDECODEH265::GetUserData(mfxU8 *ud, mfxU32 *sz, mfxU64 *ts)
{
    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR3(ud, sz, ts);

    mfxStatus       MFXSts = MFX_ERR_NONE;

    UMC::MediaData data;
    UMC::Status umcRes = m_pH265VideoDecoder->GetUserData(&data);

    if (umcRes == UMC::UMC_ERR_NOT_ENOUGH_DATA)
        return MFX_ERR_MORE_DATA;

    if (*sz < data.GetDataSize())
        return MFX_ERR_NOT_ENOUGH_BUFFER;

    *sz = (mfxU32)data.GetDataSize();
    *ts = GetMfxTimeStamp(data.GetTime());
    memcpy_s(ud, *sz, data.GetDataPointer(), data.GetDataSize());

    return MFXSts;
}

// Returns stored SEI messages
mfxStatus VideoDECODEH265::GetPayload( mfxU64 *ts, mfxPayload *payload )
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR3(ts, payload, payload->Data);

    SEI_Storer_H265 * storer = m_pH265VideoDecoder->GetSEIStorer();

    if (!storer)
        return MFX_ERR_UNKNOWN;

    const SEI_Storer_H265::SEI_Message * msg = storer->GetPayloadMessage();

    if (msg)
    {
        if (payload->BufSize < msg->size)
            return MFX_ERR_NOT_ENOUGH_BUFFER;

        *ts = GetMfxTimeStamp(msg->timestamp);

        memcpy_s(payload->Data, payload->BufSize, msg->data, msg->size);

        payload->CtrlFlags =
            msg->nal_type == NAL_UT_SEI_SUFFIX ? MFX_PAYLOAD_CTRL_SUFFIX : 0;
        payload->NumBit = (mfxU32)(msg->size * 8);
        payload->Type = (mfxU16)msg->type;
    }
    else
    {
        payload->NumBit = 0;
        *ts = MFX_TIME_STAMP_INVALID;
    }

    return MFX_ERR_NONE;
}

// Find a next frame ready to be output from decoder
H265DecoderFrame * VideoDECODEH265::GetFrameToDisplay_H265(bool force)
{
    H265DecoderFrame * pFrame = m_pH265VideoDecoder->GetFrameToDisplayInternal(force);
    if (!pFrame)
    {
        return 0;
    }

    pFrame->setWasOutputted();
    m_pH265VideoDecoder->PostProcessDisplayFrame(pFrame);

    return pFrame;
}

// MediaSDK DECODE_SetSkipMode API function
mfxStatus VideoDECODEH265::SetSkipMode(mfxSkipMode mode)
{
    UMC::AutomaticUMCMutex guard(m_mGuard);

    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    int32_t test_num = 0;
    m_pH265VideoDecoder->ChangeVideoDecodingSpeed(test_num);

    int32_t num = 0;

    switch(mode)
    {
    case MFX_SKIPMODE_NOSKIP:
        num = -10;
        break;
    case MFX_SKIPMODE_MORE:
        num = 1;
        break;
    case MFX_SKIPMODE_LESS:
        num = -1;
        break;

    default:
        return MFX_ERR_UNSUPPORTED;
    }

    m_pH265VideoDecoder->ChangeVideoDecodingSpeed(num);

    return test_num == num ? MFX_WRN_VALUE_NOT_CHANGED : MFX_ERR_NONE;
}

// Check if new parameters are compatible with new parameters
bool VideoDECODEH265::IsSameVideoParam(mfxVideoParam * newPar, mfxVideoParam * oldPar, eMFXHWType type)
{
    if ((newPar->IOPattern & (MFX_IOPATTERN_OUT_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY)) !=
        (oldPar->IOPattern & (MFX_IOPATTERN_OUT_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY)) )
    {
        return false;
    }

    if (newPar->Protected != oldPar->Protected)
    {
        return false;
    }

    if (CalculateAsyncDepth(m_platform, newPar) != CalculateAsyncDepth(m_platform, oldPar))
    {
        return false;
    }

    mfxFrameAllocRequest requestOld;
    memset(&requestOld, 0, sizeof(requestOld));
    mfxFrameAllocRequest requestNew;
    memset(&requestNew, 0, sizeof(requestNew));

    mfxStatus mfxSts = QueryIOSurfInternal(m_platform, type, oldPar, &requestOld);
    if (mfxSts != MFX_ERR_NONE)
        return false;

    mfxSts = QueryIOSurfInternal(m_platform, type, newPar, &requestNew);
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

        if (opaqueNew && opaqueOld)
        {
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
        else
            return false;
    }

    return true;
}

// Fill up frame allocator request data
mfxStatus VideoDECODEH265::UpdateAllocRequest(mfxVideoParam *par,
                                                mfxFrameAllocRequest *request,
                                                mfxExtOpaqueSurfaceAlloc * &pOpaqAlloc,
                                                bool &mapping)
{
    mapping = false;
    if (!(par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
        return MFX_ERR_NONE;

    m_isOpaq = true;

    pOpaqAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
    if (!pOpaqAlloc)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (request->NumFrameMin > pOpaqAlloc->Out.NumSurface)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    request->Type = MFX_MEMTYPE_OPAQUE_FRAME | MFX_MEMTYPE_FROM_DECODE;
    request->Type |= (pOpaqAlloc->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY) ? MFX_MEMTYPE_SYSTEM_MEMORY : MFX_MEMTYPE_DXVA2_DECODER_TARGET;
    request->NumFrameMin = request->NumFrameSuggested = pOpaqAlloc->Out.NumSurface;
    mapping = true;
    return MFX_ERR_NONE;
}

// Get original Surface corresponding to OpaqueSurface
mfxFrameSurface1 *VideoDECODEH265::GetOriginalSurface(mfxFrameSurface1 *surface)
{
    if (m_isOpaq)
        return m_core->GetNativeSurface(surface);
    return surface;
}

#endif // MFX_ENABLE_H265_VIDEO_DECODE
