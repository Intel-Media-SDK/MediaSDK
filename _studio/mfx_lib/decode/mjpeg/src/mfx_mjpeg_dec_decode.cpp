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

#include <limits>
#include "mfx_mjpeg_dec_decode.h"

#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)


#include "mfx_common.h"
#include "mfx_common_decode_int.h"

#ifdef MFX_ENABLE_SW_FALLBACK
#include <ippcore.h> // for MfxIppInit()
#include "mfx_mjpeg_task.h"
#include "umc_mjpeg_mfx_decode.h"
#include "mfx_thread_task.h"
#endif

#include "mfx_enc_common.h"

#include "umc_jpeg_frame_constructor.h"
#include "umc_mjpeg_mfx_decode_hw.h"
#include "mfx_session.h"

// Declare skipping constants
enum
{
    JPEG_MIN_SKIP_RATE = 0,
    JPEG_MAX_SKIP_RATE = 9,

    JPEG_SKIP_BOUND = JPEG_MAX_SKIP_RATE + 1
};


struct ThreadTaskInfo
{
    mfxFrameSurface1 *surface_work;
    mfxFrameSurface1 *surface_out;
    UMC::FrameData   *dst;
    mfxU32            decodeTaskID;
    mfxU32            vppTaskID;
    bool              needCheckVppStatus;
    mfxU32            numDecodeTasksToCheck;
};

static void SetFrameType(mfxFrameSurface1 &surface_out)
{
    auto extFrameInfo = reinterpret_cast<mfxExtDecodedFrameInfo *>(GetExtendedBuffer(surface_out.Data.ExtParam, surface_out.Data.NumExtParam, MFX_EXTBUFF_DECODED_FRAME_INFO));
    if (extFrameInfo == nullptr)
        return;

    // terms I/P/B frames not applicable for JPEG,
    // so all frames marked as I
    extFrameInfo->FrameType = MFX_FRAMETYPE_I;
}

class MFX_JPEG_Utility
{
public:

    static eMFXPlatform GetPlatform(VideoCORE * core, mfxVideoParam * par);
    static mfxStatus Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out, eMFXHWType type);
    static bool CheckVideoParam(mfxVideoParam *in, eMFXHWType type);

private:

    static bool IsNeedPartialAcceleration(VideoCORE * core, mfxVideoParam * par);
};

VideoDECODEMJPEG::VideoDECODEMJPEG(VideoCORE *core, mfxStatus * sts)
    : VideoDECODE()
    , m_core(core)
    , m_isInit(false)
    , m_isOpaq(false)
    , m_frameOrder((mfxU16)MFX_FRAMEORDER_UNKNOWN)
    , m_response()
    , m_response_alien()
    , m_platform(MFX_PLATFORM_SOFTWARE)
{
    if (sts)
    {
        *sts = MFX_ERR_NONE;
    }

    m_isHeaderFound = false;
    m_isHeaderParsed = false;

    m_skipRate = 0;
    m_skipCount = 0;
}

VideoDECODEMJPEG::~VideoDECODEMJPEG(void)
{
    Close();
}

mfxStatus VideoDECODEMJPEG::Init(mfxVideoParam *par)
{
    std::lock_guard<std::mutex> guard(m_mGuard);

    if (m_isInit)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    MFX_CHECK_NULL_PTR1(par);

    m_platform = MFX_JPEG_Utility::GetPlatform(m_core, par);

    eMFXHWType type = MFX_HW_UNKNOWN;
    if (m_platform == MFX_PLATFORM_HARDWARE)
    {
        type = m_core->GetHWType();
    }

    if (CheckVideoParamDecoders(par, m_core->IsExternalFrameAllocator(), type) < MFX_ERR_NONE)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (!MFX_JPEG_Utility::CheckVideoParam(par, type))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    m_vFirstPar = *par;
    m_vFirstPar.mfx.NumThread = 0;

    bool isNeedChangeVideoParamWarning = IsNeedChangeVideoParam(&m_vFirstPar);
    m_vPar = m_vFirstPar;

    m_vPar.mfx.NumThread = (mfxU16)(m_vPar.AsyncDepth ? m_vPar.AsyncDepth : m_core->GetAutoAsyncDepth());
    if (MFX_PLATFORM_SOFTWARE != m_platform)
        m_vPar.mfx.NumThread = 1;

    int32_t useInternal = (MFX_PLATFORM_SOFTWARE == m_platform) ?
        (m_vPar.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) :
        (m_vPar.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY);

    // allocate memory
    mfxFrameAllocRequest request;
    mfxFrameAllocRequest request_internal;
    memset(&request, 0, sizeof(request));
    memset(&m_response, 0, sizeof(m_response));
    memset(&m_response_alien, 0, sizeof(m_response_alien));
    m_isOpaq = false;

    MFX_SAFE_CALL(QueryIOSurfInternal(m_core, &m_vPar, &request));

    if (m_vPar.IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc *pOpaqAlloc = (mfxExtOpaqueSurfaceAlloc *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (!pOpaqAlloc)
            return MFX_ERR_INVALID_VIDEO_PARAM;

        useInternal = (m_platform == MFX_PLATFORM_SOFTWARE) ?
                      (pOpaqAlloc->Out.Type & MFX_MEMTYPE_DXVA2_DECODER_TARGET) :
                      (pOpaqAlloc->Out.Type & MFX_MEMTYPE_SYSTEM_MEMORY);
    }

    request_internal = request;

    if (useInternal)
        request.Type |= MFX_MEMTYPE_INTERNAL_FRAME
        ;
    else
        request.Type |= MFX_MEMTYPE_EXTERNAL_FRAME;

    // allocates external surfaces:
    bool mapOpaq = true;
    mfxExtOpaqueSurfaceAlloc *pOpqAlloc = 0;
    mfxStatus mfxSts = UpdateAllocRequest(par, &request, pOpqAlloc, mapOpaq);
    MFX_CHECK(mfxSts >= MFX_ERR_NONE, mfxSts);

    if (mapOpaq)
    {
        request_internal.NumFrameMin = request_internal.NumFrameSuggested = pOpqAlloc->Out.NumSurface;
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
            mfxSts = m_core->AllocFrames(&request, &m_response,false);
        }
    }

    if (mfxSts < MFX_ERR_NONE)
        return mfxSts;

    if (MFX_PLATFORM_SOFTWARE == m_platform)
    {
#ifdef MFX_ENABLE_SW_FALLBACK
        decoder.reset(new VideoDECODEMJPEGBase_SW);
#else
        return MFX_ERR_UNSUPPORTED;
#endif
    }
    else
    {

        VideoDECODEMJPEGBase_HW * dec = new VideoDECODEMJPEGBase_HW;
        decoder.reset(dec);
        bool usePostProcessing = GetExtendedBuffer(m_vPar.ExtParam, m_vPar.NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING);
        useInternal |= dec->AdjustFrameAllocRequest(&request_internal,
                                                  &m_vPar.mfx,
                                                  m_core->GetHWType(),
                                                  m_core->GetVAType(),
                                                  usePostProcessing);

    }

    decoder->m_vPar = m_vPar;
    decoder->m_isOpaq = m_isOpaq;

    // allocates internal surfaces:
    if (useInternal)
    {
        m_response_alien = m_response;
        decoder->m_FrameAllocator->SetExternalFramesResponse(&m_response_alien);
        request_internal.Type |= MFX_MEMTYPE_INTERNAL_FRAME;
        request = request_internal;
        mfxSts = m_core->AllocFrames(&request_internal, &m_response, (m_vPar.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY));
        if (mfxSts < MFX_ERR_NONE)
            return mfxSts;
    }
    else
    {
        decoder->m_FrameAllocator->SetExternalFramesResponse(&m_response);
    }

    mfxVideoParam decPar = *par;
    decPar.mfx.FrameInfo = request.Info;
    m_frameConstructor.reset(new UMC::JpegFrameConstructor());

    mfxSts = decoder->Init(&decPar, &request, &m_response, &request_internal, !useInternal, m_core);
    if (mfxSts < MFX_ERR_NONE)
        return mfxSts;

    m_isInit = true;
    m_isHeaderFound = false;
    m_isHeaderParsed = false;

    m_frameOrder = 0;

    if (m_platform != m_core->GetPlatformType())
    {
        VM_ASSERT(m_platform == MFX_PLATFORM_SOFTWARE);
        return MFX_WRN_PARTIAL_ACCELERATION;
    }

    if (isNeedChangeVideoParamWarning)
    {
        return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEG::Reset(mfxVideoParam *par)
{
    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR1(par);

    eMFXHWType type = MFX_HW_UNKNOWN;
    if (m_platform == MFX_PLATFORM_HARDWARE)
    {
        type = m_core->GetHWType();
    }

    if (CheckVideoParamDecoders(par, m_core->IsExternalFrameAllocator(), type) < MFX_ERR_NONE)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (!MFX_JPEG_Utility::CheckVideoParam(par, type))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if (!IsSameVideoParam(par, &m_vFirstPar))
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    // need to sw acceleration
    if (m_platform != MFX_JPEG_Utility::GetPlatform(m_core, par))
    {
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
    }

    MFX_SAFE_CALL(decoder->Reset(par));

    m_frameOrder = 0;
    bool isNeedChangeVideoParamWarning = IsNeedChangeVideoParam(par);
    m_vPar = *par;

    if (isNeedChangeVideoParamWarning)
    {
        return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    m_isHeaderFound = false;
    m_isHeaderParsed = false;

    m_skipRate = 0;
    m_skipCount = 0;

    m_frameConstructor->Reset();

    if (m_platform != m_core->GetPlatformType())
    {
        VM_ASSERT(m_platform == MFX_PLATFORM_SOFTWARE);
        return MFX_WRN_PARTIAL_ACCELERATION;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEG::Close(void)
{
    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    decoder->Close();

    if (m_response.NumFrameActual)
        m_core->FreeFrames(&m_response);

    if (m_response_alien.NumFrameActual)
        m_core->FreeFrames(&m_response_alien);

    m_isOpaq = false;
    m_isInit = false;
    m_isHeaderFound = false;
    m_isHeaderParsed = false;
    m_frameOrder = (mfxU16)MFX_FRAMEORDER_UNKNOWN;

    m_frameConstructor->Close();

    return MFX_ERR_NONE;
}

mfxTaskThreadingPolicy VideoDECODEMJPEG::GetThreadingPolicy(void)
{
    return MFX_TASK_THREADING_INTER;
}

mfxStatus VideoDECODEMJPEG::Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out)
{
    MFX_CHECK_NULL_PTR1(out);

    eMFXHWType type = core->GetHWType();
    return MFX_JPEG_Utility::Query(core, in, out, type);
}

mfxStatus VideoDECODEMJPEG::GetVideoParam(mfxVideoParam *par)
{
    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR1(par);

    par->mfx = m_vPar.mfx;

    par->Protected = m_vPar.Protected;
    par->IOPattern = m_vPar.IOPattern;
    par->AsyncDepth = m_vPar.AsyncDepth;

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

    return decoder->GetVideoParam(par);
}

mfxStatus VideoDECODEMJPEG::DecodeHeader(VideoCORE *core, mfxBitstream *bs, mfxVideoParam *par)
{
    MFX_CHECK_NULL_PTR2(bs, par);

    mfxStatus sts = CheckBitstream(bs);
    if (sts != MFX_ERR_NONE)
    {
        MFX_CHECK_INIT(sts == MFX_ERR_NULL_PTR);
        MFX_RETURN(MFX_ERR_NULL_PTR);
    }

    MFXMediaDataAdapter in(bs);

    mfx_UMC_MemAllocator  tempAllocator;
    tempAllocator.InitMem(0, core);

    mfxExtJPEGQuantTables* jpegQT = (mfxExtJPEGQuantTables*) GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_JPEG_QT );
    mfxExtJPEGHuffmanTables*  jpegHT = (mfxExtJPEGHuffmanTables*) GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_JPEG_HUFFMAN );

    // DecodeHeader
    UMC::MJPEGVideoDecoderBaseMFX decoder;

    UMC::VideoDecoderParams umcVideoParams;
    umcVideoParams.info.clip_info.height = par->mfx.FrameInfo.Height;
    umcVideoParams.info.clip_info.width = par->mfx.FrameInfo.Width;

    umcVideoParams.lpMemoryAllocator = &tempAllocator;

    UMC::Status umcRes = decoder.Init(&umcVideoParams);
    MFX_CHECK_INIT(umcRes == UMC::UMC_OK);

    umcRes = decoder.DecodeHeader(&in);

    in.Save(bs);

    MFX_CHECK_INIT(umcRes == UMC::UMC_OK);

    mfxVideoParam temp;

    umcRes = decoder.FillVideoParam(&temp, false);
    MFX_CHECK_INIT(umcRes == UMC::UMC_OK);

    if(jpegQT)
    {
        umcRes = decoder.FillQuantTableExtBuf(jpegQT);
        MFX_CHECK_INIT(umcRes == UMC::UMC_OK);
    }

    if(jpegHT)
    {
        umcRes = decoder.FillHuffmanTableExtBuf(jpegHT);
        MFX_CHECK_INIT(umcRes == UMC::UMC_OK);
    }

    decoder.Close();
    tempAllocator.Close();

    par->mfx.FrameInfo = temp.mfx.FrameInfo;
    par->mfx.JPEGChromaFormat = temp.mfx.JPEGChromaFormat;
    par->mfx.JPEGColorFormat = temp.mfx.JPEGColorFormat;
    par->mfx.Rotation  = temp.mfx.Rotation;
    par->mfx.InterleavedDec  = temp.mfx.InterleavedDec;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEG::QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_CHECK_NULL_PTR2(par, request);

    eMFXPlatform platform = MFX_JPEG_Utility::GetPlatform(core, par);

    mfxVideoParam params = *par;
    bool isNeedChangeVideoParamWarning = IsNeedChangeVideoParam(&params);

    if (!(par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && !(par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) && !(par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if ((par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if ((par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) && (par->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    if ((par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) && (par->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    MFX_SAFE_CALL(QueryIOSurfInternal(core, &params, request));

    int32_t isInternalManaging = (MFX_PLATFORM_SOFTWARE == platform) ?
        (params.IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) : (params.IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY);

    if (isInternalManaging)
    {
        request->NumFrameSuggested = request->NumFrameMin = par->AsyncDepth ? par->AsyncDepth : core->GetAutoAsyncDepth();

        request->Type = MFX_MEMTYPE_FROM_DECODE;
        if (MFX_PLATFORM_SOFTWARE == platform)
        {
            if ((request->Info.FourCC == MFX_FOURCC_RGB4 || request->Info.FourCC == MFX_FOURCC_YUY2) && MFX_HW_D3D11 == core->GetVAType())
                request->Type |= MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET;
            else
                request->Type |= MFX_MEMTYPE_DXVA2_DECODER_TARGET;
        }
        else
        {
            request->Type |= MFX_MEMTYPE_SYSTEM_MEMORY;
        }
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
        return MFX_WRN_PARTIAL_ACCELERATION;
    }

    if (isNeedChangeVideoParamWarning)
    {
        return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEG::QueryIOSurfInternal(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    eMFXPlatform platform = MFX_JPEG_Utility::GetPlatform(core, par);

    request->Info = par->mfx.FrameInfo;

    mfxU32 asyncDepth = (par->AsyncDepth ? par->AsyncDepth : core->GetAutoAsyncDepth());

    request->NumFrameMin = mfxU16 (asyncDepth);

    request->NumFrameSuggested = request->NumFrameMin;

    request->Type = MFX_MEMTYPE_FROM_DECODE;

    mfxExtDecVideoProcessing * videoProcessing = (mfxExtDecVideoProcessing *)GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING);
    if (videoProcessing)
    {
        // need to substitute output format
        // number of surfaces is same
        request->Info.FourCC = videoProcessing->Out.FourCC;

        request->Info.ChromaFormat = videoProcessing->Out.ChromaFormat;
        request->Info.Width = videoProcessing->Out.Width;
        request->Info.Height = videoProcessing->Out.Height;
        request->Info.CropX = videoProcessing->Out.CropX;
        request->Info.CropY = videoProcessing->Out.CropY;
        request->Info.CropW = videoProcessing->Out.CropW;
        request->Info.CropH = videoProcessing->Out.CropH;
    }

    if(MFX_ROTATION_90 == par->mfx.Rotation || MFX_ROTATION_270 == par->mfx.Rotation)
    {
        std::swap(request->Info.Height, request->Info.Width);
        std::swap(request->Info.AspectRatioH, request->Info.AspectRatioW);
        std::swap(request->Info.CropH, request->Info.CropW);
        std::swap(request->Info.CropY, request->Info.CropX);
    }

    request->Info.Width  = mfx::align2_value(request->Info.Width, 0x10);
    request->Info.Height = mfx::align2_value(request->Info.Height,
        (request->Info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE) ? 0x10 : 0x20);

    if (MFX_PLATFORM_SOFTWARE == platform)
    {
        request->Type |= MFX_MEMTYPE_SYSTEM_MEMORY;
    }
    else
    {
        bool needVpp = false;
        if(par->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_FIELD_TFF ||
           par->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_FIELD_BFF)
        {
            needVpp = true;
        }

        mfxFrameAllocRequest request_internal = *request;
        bool usePostProcessing = GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING);
        VideoDECODEMJPEGBase_HW::AdjustFourCC(&request_internal.Info, &par->mfx, core->GetHWType(), core->GetVAType(),
                                              usePostProcessing, &needVpp);


        if (needVpp && MFX_HW_D3D11 == core->GetVAType())
        {
            request->Type |= MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET;
            request->Type |= MFX_MEMTYPE_FROM_VPPOUT;
        }
        else
            request->Type |= MFX_MEMTYPE_DXVA2_DECODER_TARGET;

    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEG::GetDecodeStat(mfxDecodeStat *stat)
{
    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR1(stat);

    decoder->m_stat.NumCachedFrame = 0;
    decoder->m_stat.NumError = 0;

    *stat = decoder->m_stat;
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEG::MJPEGDECODERoutine(void *pState, void *pParam,
                                               mfxU32 threadNumber, mfxU32 callNumber)
{
    VideoDECODEMJPEG &obj = *((VideoDECODEMJPEG *) pState);
    MFX::AutoTimer timer("DecodeFrame");
    return obj.decoder->RunThread(pParam, threadNumber, callNumber);
} // mfxStatus VideoDECODEMJPEG::MJPEGDECODERoutine(void *pState, void *pParam,

mfxStatus VideoDECODEMJPEG::MJPEGCompleteProc(void *pState, void *pParam,
                                              mfxStatus taskRes)
{
    VideoDECODEMJPEG &obj = *((VideoDECODEMJPEG *) pState);
    return obj.decoder->CompleteTask(pParam, taskRes);
}

mfxStatus VideoDECODEMJPEG::DecodeFrameCheck(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, MFX_ENTRY_POINT *pEntryPoint)
{
    // It can be useful to run threads right after first frame receive
    MFX_SAFE_CALL(DecodeFrameCheck(bs, surface_work, surface_out));

    UMC::FrameData *dst = nullptr;
    MFX_SAFE_CALL(decoder->AllocateFrameData(dst));

    {
        // output surface is always working surface
        *surface_out = decoder->m_FrameAllocator->GetSurface(dst->GetFrameMID(),
                                                    GetOriginalSurface(surface_work),
                                                    &m_vPar);
        *surface_out = (m_isOpaq) ? m_core->GetOpaqSurface((*surface_out)->Data.MemId) : (*surface_out);

        if (!(*surface_out))
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        SetFrameType(**surface_out);

        (*surface_out)->Info.FrameId.ViewId = 0; // (mfxU16)pFrame->m_viewId;

        bool isShouldUpdate = !(m_vFirstPar.mfx.FrameInfo.AspectRatioH || m_vFirstPar.mfx.FrameInfo.AspectRatioW);

        if(MFX_ROTATION_0 == m_vFirstPar.mfx.Rotation || MFX_ROTATION_180 == m_vFirstPar.mfx.Rotation)
        {
            (*surface_out)->Info.CropH = m_vPar.mfx.FrameInfo.CropH;
            (*surface_out)->Info.CropW = m_vPar.mfx.FrameInfo.CropW;
            (*surface_out)->Info.AspectRatioH = isShouldUpdate ? (mfxU16) 1 : m_vFirstPar.mfx.FrameInfo.AspectRatioH;
            (*surface_out)->Info.AspectRatioW = isShouldUpdate ? (mfxU16) 1 : m_vFirstPar.mfx.FrameInfo.AspectRatioW;
        }
        else
        {
            (*surface_out)->Info.CropH = m_vPar.mfx.FrameInfo.CropW;
            (*surface_out)->Info.CropW = m_vPar.mfx.FrameInfo.CropH;
            (*surface_out)->Info.AspectRatioH = isShouldUpdate ? (mfxU16) 1 : m_vFirstPar.mfx.FrameInfo.AspectRatioW;
            (*surface_out)->Info.AspectRatioW = isShouldUpdate ? (mfxU16) 1 : m_vFirstPar.mfx.FrameInfo.AspectRatioH;
        }

        (*surface_out)->Info.CropX = 0;
        (*surface_out)->Info.CropY = 0;

        mfxExtDecVideoProcessing * videoProcessing = (mfxExtDecVideoProcessing *)GetExtendedBuffer(m_vFirstPar.ExtParam, m_vFirstPar.NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING);
        if (videoProcessing)
        {
            (*surface_out)->Info.CropH = videoProcessing->Out.CropH;
            (*surface_out)->Info.CropW = videoProcessing->Out.CropW;
            (*surface_out)->Info.CropX = videoProcessing->Out.CropX;
            (*surface_out)->Info.CropY = videoProcessing->Out.CropY;
        }

        isShouldUpdate = !(m_vFirstPar.mfx.FrameInfo.FrameRateExtD || m_vFirstPar.mfx.FrameInfo.FrameRateExtN);

        (*surface_out)->Info.FrameRateExtD = isShouldUpdate ? m_vPar.mfx.FrameInfo.FrameRateExtD : m_vFirstPar.mfx.FrameInfo.FrameRateExtD;
        (*surface_out)->Info.FrameRateExtN = isShouldUpdate ? m_vPar.mfx.FrameInfo.FrameRateExtN : m_vFirstPar.mfx.FrameInfo.FrameRateExtN;
        (*surface_out)->Info.PicStruct = (mfxU16) ((MFX_PICSTRUCT_PROGRESSIVE == m_vPar.mfx.FrameInfo.PicStruct) ?
                                                    (MFX_PICSTRUCT_PROGRESSIVE) :
                                                    (MFX_PICSTRUCT_FIELD_TFF));
        (*surface_out)->Data.TimeStamp = GetMfxTimeStamp(dst->GetTime());

        if(MFX_TIME_STAMP_INVALID == (*surface_out)->Data.TimeStamp)
        {
            (*surface_out)->Data.TimeStamp = ((mfxU64)m_frameOrder * m_vPar.mfx.FrameInfo.FrameRateExtD * MFX_TIME_STAMP_FREQUENCY) / m_vPar.mfx.FrameInfo.FrameRateExtN;
        }

        (*surface_out)->Data.FrameOrder = m_frameOrder;
        m_frameOrder++;

        (*surface_out)->Data.Corrupted = 0;
    }

    // prepare output structure
    pEntryPoint->pRoutine = &MJPEGDECODERoutine;
    pEntryPoint->pCompleteProc = &MJPEGCompleteProc;
    pEntryPoint->pState = this;
    pEntryPoint->pRoutineName = (char *)"DecodeMJPEG";

    return decoder->FillEntryPoint(pEntryPoint, GetOriginalSurface(surface_work), GetOriginalSurface(*surface_out));
}

mfxStatus VideoDECODEMJPEG::DecodeFrameCheck(mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out)
{
    UMC::Status umcRes = UMC::UMC_OK;

    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    // make sure that there is a free task
    MFX_SAFE_CALL(decoder->CheckTaskAvailability(m_vPar.AsyncDepth ? m_vPar.AsyncDepth : m_core->GetAutoAsyncDepth()));

    MFX_CHECK_NULL_PTR2(surface_work, surface_out);

    if (bs)
        MFX_SAFE_CALL(CheckBitstream(bs));

    *surface_out = 0;

    if (m_isOpaq)
    {
        MFX_CHECK_COND(CheckFrameInfoCodecs(&surface_work->Info, MFX_CODEC_JPEG) == MFX_ERR_NONE);

        if (surface_work->Data.MemId || surface_work->Data.Y || surface_work->Data.R || surface_work->Data.A || surface_work->Data.UV) // opaq surface
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        surface_work = GetOriginalSurface(surface_work);
        if (!surface_work)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    MFX_CHECK_COND(CheckFrameInfoCodecs(&surface_work->Info, MFX_CODEC_JPEG) == MFX_ERR_NONE);

    MFX_SAFE_CALL(CheckFrameData(surface_work));

    UMC::MJPEGVideoDecoderBaseMFX* pMJPEGVideoDecoder;
    MFX_SAFE_CALL(decoder->ReserveUMCDecoder(pMJPEGVideoDecoder, surface_work, m_isOpaq));

    mfxU32 numPic = 0;
    mfxU32 picToCollect = (MFX_PICSTRUCT_PROGRESSIVE == m_vPar.mfx.FrameInfo.PicStruct) ? 1 : 2;

    //gpu session priority 
    UMC::VideoDecoderParams* videoDecoderParams = &(decoder->umcVideoParams);
    if (videoDecoderParams != nullptr && videoDecoderParams->pVideoAccelerator != nullptr)
        videoDecoderParams->pVideoAccelerator->m_ContextPriority = m_core->GetSession()->m_priority;

    do
    {
        MFXMediaDataAdapter src(bs);
        UMC::MediaDataEx *pSrcData;

        if (!m_isHeaderFound && bs)
        {
            umcRes = pMJPEGVideoDecoder->FindStartOfImage(&src);
            if (umcRes != UMC::UMC_OK)
            {
                if(umcRes != UMC::UMC_ERR_NOT_ENOUGH_DATA)
                    decoder->ReleaseReservedTask();
                return (umcRes == UMC::UMC_ERR_NOT_ENOUGH_DATA) ? MFX_ERR_MORE_DATA : ConvertUMCStatusToMfx(umcRes);
            }
            src.Save(bs);
            m_isHeaderFound = true;
        }

        if (!m_isHeaderParsed && bs)
        {
            umcRes = pMJPEGVideoDecoder->_GetFrameInfo((uint8_t*)src.GetDataPointer(), src.GetDataSize());
            if (umcRes != UMC::UMC_OK)
            {
                if(umcRes != UMC::UMC_ERR_NOT_ENOUGH_DATA)
                    decoder->ReleaseReservedTask();
                return (umcRes == UMC::UMC_ERR_NOT_ENOUGH_DATA) ? MFX_ERR_MORE_DATA : ConvertUMCStatusToMfx(umcRes);
            }

            mfxVideoParam temp;
            pMJPEGVideoDecoder->FillVideoParam(&temp, false);

            if(m_vPar.mfx.FrameInfo.CropW != temp.mfx.FrameInfo.CropW ||
                m_vPar.mfx.FrameInfo.CropH != temp.mfx.FrameInfo.CropH)
            {
                decoder->ReleaseReservedTask();
                return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
            }
            m_isHeaderParsed = true;
        }

        mfxU32 maxBitstreamSize = m_vPar.mfx.FrameInfo.CropW * m_vPar.mfx.FrameInfo.CropH;
        switch(m_vPar.mfx.JPEGChromaFormat)
        {
        case CHROMA_TYPE_YUV411:
        case CHROMA_TYPE_YUV420:
            maxBitstreamSize = maxBitstreamSize * 3 / 2;
            break;
        case CHROMA_TYPE_YUV422H_2Y:
        case CHROMA_TYPE_YUV422V_2Y:
        case CHROMA_TYPE_YUV422H_4Y:
        case CHROMA_TYPE_YUV422V_4Y:
            maxBitstreamSize = maxBitstreamSize * 2;
            break;
        case CHROMA_TYPE_YUV444:
            maxBitstreamSize = maxBitstreamSize * 3;
            break;
        case CHROMA_TYPE_YUV400:
        default:
            break;
        };

        pSrcData = m_frameConstructor->GetFrame(bs ? &src : 0, maxBitstreamSize);

        // update the bytes used in bitstream
        src.Save(bs);
        if (!pSrcData)
        {
            decoder->ReleaseReservedTask();
            return MFX_ERR_MORE_DATA;
        }

        m_isHeaderFound = false;
        m_isHeaderParsed = false;

        try
        {
            MFX_SAFE_CALL(decoder->AddPicture(pSrcData, numPic));
        }
        catch(const UMC::eUMC_Status& sts)
        {
            if(sts == UMC::UMC_ERR_INVALID_STREAM)
            {
                continue;
            }
            else
            {
                return ConvertUMCStatusToMfx(sts);
            }
        }
    // make sure, that we collected BOTH fields
    } while (picToCollect > numPic);

    // check if skipping is enabled
    m_skipCount += m_skipRate;
    if (JPEG_SKIP_BOUND <= m_skipCount)
    {
        m_skipCount -= JPEG_SKIP_BOUND;

        decoder->m_stat.NumSkippedFrame++;

        // it is time to skip a frame
        decoder->ReleaseReservedTask();
        return MFX_ERR_MORE_DATA;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEG::DecodeFrame(mfxBitstream *, mfxFrameSurface1 *, mfxFrameSurface1 *)
{
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEG::GetUserData(mfxU8 *ud, mfxU32 *sz, mfxU64 *ts)
{
    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR3(ud, sz, ts);
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus VideoDECODEMJPEG::GetPayload( mfxU64 *ts, mfxPayload *payload )
{
    if (!m_isInit)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR3(ts, payload, payload->Data);
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus VideoDECODEMJPEG::SetSkipMode(mfxSkipMode mode)
{
    // check error(s)
    if (!m_isInit)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    // check if we reached bounds of skipping
    if (((JPEG_MIN_SKIP_RATE == m_skipRate) && (MFX_SKIPMODE_LESS == mode)) ||
        ((JPEG_MAX_SKIP_RATE == m_skipRate) && (MFX_SKIPMODE_MORE == mode)) ||
        ((JPEG_MIN_SKIP_RATE == m_skipRate) && (MFX_SKIPMODE_NOSKIP == mode)))
    {
        return MFX_WRN_VALUE_NOT_CHANGED;
    }

    // set new skip rate
    switch (mode)
    {
    case MFX_SKIPMODE_LESS:
        m_skipRate -= 1;
        break;

    case MFX_SKIPMODE_MORE:
        m_skipRate += 1;
        break;

    default:
        m_skipRate = 0;
        break;
    }

    return MFX_ERR_NONE;

} // mfxStatus VideoDECODEMJPEG::SetSkipMode(mfxSkipMode mode)

bool VideoDECODEMJPEG::IsSameVideoParam(mfxVideoParam * newPar, mfxVideoParam * oldPar)
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

    if (newPar->AsyncDepth != oldPar->AsyncDepth)
    {
        return false;
    }

    mfxFrameAllocRequest requestOld;
    memset(&requestOld, 0, sizeof(requestOld));
    mfxFrameAllocRequest requestNew;
    memset(&requestNew, 0, sizeof(requestNew));

    mfxStatus mfxSts = QueryIOSurfInternal(m_core, oldPar, &requestOld);
    if (mfxSts != MFX_ERR_NONE)
        return false;

    mfxSts = QueryIOSurfInternal(m_core, newPar, &requestNew);
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MFX_JPEG_Utility implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MFX_JPEG_Utility::IsNeedPartialAcceleration(VideoCORE * core, mfxVideoParam * par)
{
    if (!par)
        return false;

    if (par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_RGB &&
        par->mfx.JPEGChromaFormat != MFX_CHROMAFORMAT_YUV444)
        return true;

#if defined (MFX_VA_LINUX)
    if (par->mfx.FrameInfo.FourCC != MFX_FOURCC_NV12 &&
        par->mfx.FrameInfo.FourCC != MFX_FOURCC_RGB4 &&
        par->mfx.FrameInfo.FourCC != MFX_FOURCC_YUY2)
        return true;

    /* on Linux in case of multiscan turn off HW support until because some functionality is missed so far */
    if (par->mfx.InterleavedDec == MFX_SCANTYPE_NONINTERLEAVED)
        return true;

    if (core->GetHWType() == MFX_HW_APL
        || core->GetHWType() >= MFX_HW_KBL
       )
    {
        switch (par->mfx.FrameInfo.FourCC)
        {
            case MFX_FOURCC_NV12:
                if  ((par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_YCbCr &&
                        (par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV420  ||
                         par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV422H ||
                         par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV422V ||
                         par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV444 )) ||
                    (par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_RGB &&
                         par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV444))
                    return false;
                else
                    return true;
            case MFX_FOURCC_YUY2:
            case MFX_FOURCC_UYVY:
                if( par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_YCbCr &&
                   (par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV420 || par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV422H)
                  ) return false;
                else
                    return true;
            case MFX_FOURCC_RGB4:
                if ((par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_RGB && par->mfx.JPEGChromaFormat != MFX_CHROMAFORMAT_YUV444) ||
                    (par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_YCbCr && par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV422V) ||
                    (par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_YCbCr && par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_MONOCHROME)
                    || (par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_YCbCr && par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV411)
                    )
                    return true;
                else
                    return false;
            default:
                return true;
        }
    }
    else if (core->GetHWType() == MFX_HW_BDW || core->GetHWType() == MFX_HW_SCL)
    {
        if (par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_YCbCr &&
            par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_MONOCHROME)
            return true;
        if (par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV411)
            return true;
        if (par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_YCbCr &&
            par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV444)
            return true;

        if (par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_RGB &&
            par->mfx.JPEGChromaFormat == MFX_CHROMAFORMAT_YUV444 &&
            par->mfx.FrameInfo.FourCC != MFX_FOURCC_RGB4)
            return true;
    }
    else if (core->GetHWType() == MFX_HW_VLV)
    {
        if (par->mfx.JPEGColorFormat == MFX_JPEG_COLORFORMAT_RGB) return true;
    }
#else
    (void)core;
#endif

    return false;
}

eMFXPlatform MFX_JPEG_Utility::GetPlatform(VideoCORE * core, mfxVideoParam * par)
{
    eMFXPlatform platform = core->GetPlatformType();

    if (platform != MFX_PLATFORM_SOFTWARE)
    {
        if (IsNeedPartialAcceleration(core, par))
        {
            return MFX_PLATFORM_SOFTWARE;
        }

        if (MFX_ERR_NONE != core->IsGuidSupported(sDXVA2_Intel_IVB_ModeJPEG_VLD_NoFGT, par))
        {
            return MFX_PLATFORM_SOFTWARE;
        }

        bool needVpp = false;
        if (par->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_FIELD_TFF ||
            par->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_FIELD_BFF)
        {
            needVpp = true;
        }
        mfxFrameAllocRequest request;
        memset(&request, 0, sizeof(request));
        request.Info = par->mfx.FrameInfo;
        bool usePostProcessing = GetExtendedBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING);

        VideoDECODEMJPEGBase_HW::AdjustFourCC(&request.Info, &par->mfx, core->GetHWType(), core->GetVAType(),
                                              usePostProcessing, &needVpp);

        if (needVpp)
        {

            if (MFX_ERR_NONE != VideoDECODEMJPEGBase_HW::CheckVPPCaps(core, par))
            {
                return MFX_PLATFORM_SOFTWARE;
            }
        }
    }

    return platform;
}

mfxStatus MFX_JPEG_Utility::Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out, eMFXHWType type)
{
    MFX_CHECK_NULL_PTR1(out);
    mfxStatus  sts = MFX_ERR_NONE;

    if (in == out)
    {
        mfxVideoParam in1 = *in;
        return Query(core, &in1, out, type);
    }

    memset(&out->mfx, 0, sizeof(mfxInfoMFX));

    if (in)
    {
        if (in->mfx.CodecId == MFX_CODEC_JPEG)
            out->mfx.CodecId = in->mfx.CodecId;

        if ((MFX_PROFILE_JPEG_BASELINE == in->mfx.CodecProfile))
            out->mfx.CodecProfile = in->mfx.CodecProfile;

        switch (in->mfx.CodecLevel)
        {
        case MFX_LEVEL_UNKNOWN:
            out->mfx.CodecLevel = in->mfx.CodecLevel;
            break;
        }

        if (in->mfx.NumThread < 128)
            out->mfx.NumThread = in->mfx.NumThread;

        if (in->AsyncDepth < MFX_MAX_ASYNC_DEPTH_VALUE) // Actually AsyncDepth > 5-7 is for debugging only.
            out->AsyncDepth = in->AsyncDepth;

        if ((in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) || (in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
        {
            if ( !((in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && (in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY)) )
                out->IOPattern = in->IOPattern;
        }

        mfxU32 fourCC = in->mfx.FrameInfo.FourCC;
        mfxU16 chromaFormat = in->mfx.FrameInfo.ChromaFormat;

        if ((fourCC == 0 && chromaFormat == 0) ||
            (fourCC == MFX_FOURCC_NV12 && chromaFormat == MFX_CHROMAFORMAT_MONOCHROME) ||
            (fourCC == MFX_FOURCC_NV12 && chromaFormat == MFX_CHROMAFORMAT_YUV420) ||
            (fourCC == MFX_FOURCC_RGB4 && chromaFormat == MFX_CHROMAFORMAT_YUV444) ||
            (fourCC == MFX_FOURCC_YUY2 && chromaFormat == MFX_CHROMAFORMAT_YUV422H))
        {
            out->mfx.FrameInfo.FourCC = in->mfx.FrameInfo.FourCC;
            out->mfx.FrameInfo.ChromaFormat = in->mfx.FrameInfo.ChromaFormat;
        }
        else
        {
            sts = MFX_ERR_UNSUPPORTED;
        }

        out->mfx.FrameInfo.Width  = mfx::align2_value(in->mfx.FrameInfo.Width, 0x10);
        out->mfx.FrameInfo.Height = mfx::align2_value(in->mfx.FrameInfo.Height,
            (in->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE) ? 0x10 : 0x20);

        if (in->mfx.FrameInfo.CropX <= out->mfx.FrameInfo.Width)
            out->mfx.FrameInfo.CropX = in->mfx.FrameInfo.CropX;

        if (in->mfx.FrameInfo.CropY <= out->mfx.FrameInfo.Height)
            out->mfx.FrameInfo.CropY = in->mfx.FrameInfo.CropY;

        if (out->mfx.FrameInfo.CropX + in->mfx.FrameInfo.CropW <= out->mfx.FrameInfo.Width)
            out->mfx.FrameInfo.CropW = in->mfx.FrameInfo.CropW;

        if (out->mfx.FrameInfo.CropY + in->mfx.FrameInfo.CropH <= out->mfx.FrameInfo.Height)
            out->mfx.FrameInfo.CropH = in->mfx.FrameInfo.CropH;

        out->mfx.FrameInfo.FrameRateExtN = in->mfx.FrameInfo.FrameRateExtN;
        out->mfx.FrameInfo.FrameRateExtD = in->mfx.FrameInfo.FrameRateExtD;

        out->mfx.FrameInfo.AspectRatioW = in->mfx.FrameInfo.AspectRatioW;
        out->mfx.FrameInfo.AspectRatioH = in->mfx.FrameInfo.AspectRatioH;

        switch (in->mfx.FrameInfo.PicStruct)
        {
        case MFX_PICSTRUCT_PROGRESSIVE:
        case MFX_PICSTRUCT_FIELD_TFF:
        case MFX_PICSTRUCT_FIELD_BFF:
        //case MFX_PICSTRUCT_FIELD_REPEATED:
        //case MFX_PICSTRUCT_FRAME_DOUBLING:
        //case MFX_PICSTRUCT_FRAME_TRIPLING:
            out->mfx.FrameInfo.PicStruct = in->mfx.FrameInfo.PicStruct;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }

        switch (in->mfx.JPEGChromaFormat)
        {
        case MFX_CHROMAFORMAT_MONOCHROME:
        case MFX_CHROMAFORMAT_YUV420:
        case MFX_CHROMAFORMAT_YUV422:
        case MFX_CHROMAFORMAT_YUV444:
        case MFX_CHROMAFORMAT_YUV411:
        case MFX_CHROMAFORMAT_YUV422V:
            out->mfx.JPEGChromaFormat = in->mfx.JPEGChromaFormat;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }

        switch (in->mfx.JPEGColorFormat)
        {
        case MFX_JPEG_COLORFORMAT_UNKNOWN:
        case MFX_JPEG_COLORFORMAT_YCbCr:
        case MFX_JPEG_COLORFORMAT_RGB:
            out->mfx.JPEGColorFormat = in->mfx.JPEGColorFormat;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }

        switch (in->mfx.InterleavedDec)
        {
        case MFX_SCANTYPE_UNKNOWN:
        case MFX_SCANTYPE_INTERLEAVED:
        case MFX_SCANTYPE_NONINTERLEAVED:
            out->mfx.InterleavedDec = in->mfx.InterleavedDec;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }

        switch (in->mfx.Rotation)
        {
        case MFX_ROTATION_0:
        case MFX_ROTATION_90:
        case MFX_ROTATION_180:
        case MFX_ROTATION_270:
            out->mfx.Rotation = in->mfx.Rotation;
            break;
        default:
            sts = MFX_ERR_UNSUPPORTED;
            break;
        }

        if(in->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE &&
           in->mfx.Rotation != MFX_ROTATION_0)
            sts = MFX_ERR_UNSUPPORTED;

        mfxStatus stsExt = CheckDecodersExtendedBuffers(in);
        if (stsExt < MFX_ERR_NONE)
            sts = MFX_ERR_UNSUPPORTED;

        if (in->Protected)
        {
            sts = MFX_ERR_UNSUPPORTED;
        }

        if (GetPlatform(core, out) != core->GetPlatformType() && sts == MFX_ERR_NONE)
        {
            VM_ASSERT(GetPlatform(core, out) == MFX_PLATFORM_SOFTWARE);
            sts = MFX_WRN_PARTIAL_ACCELERATION;
        }

        /*SFC*/
        mfxExtDecVideoProcessing * videoProcessingTargetIn = (mfxExtDecVideoProcessing *)GetExtendedBuffer(in->ExtParam, in->NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING);
        mfxExtDecVideoProcessing * videoProcessingTargetOut = (mfxExtDecVideoProcessing *)GetExtendedBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING);
        if (videoProcessingTargetIn && videoProcessingTargetOut)
        {
            // limits are from media_driver/agnostic/common/hw/mhw_sfc.h
            // TODO: get them via API
            const short unsigned MHW_SFC_MIN_HEIGHT        = 128;
            const short unsigned MHW_SFC_MIN_WIDTH         = 128;
            const short unsigned MHW_SFC_MAX_HEIGHT        = 4096;
            const short unsigned MHW_SFC_MAX_WIDTH         = 4096;

            if ( (MFX_PICSTRUCT_PROGRESSIVE == in->mfx.FrameInfo.PicStruct) &&
                 (in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) &&
                 // FtrSFCPipe is not supported on BDW, and there is a known issue on SCL
                 (core->GetHWType() > MFX_HW_SCL) &&
                 (videoProcessingTargetIn->Out.Width  >= MHW_SFC_MIN_WIDTH  &&
                  videoProcessingTargetIn->Out.Width  <= MHW_SFC_MAX_WIDTH)  &&
                 (videoProcessingTargetIn->Out.Height >= MHW_SFC_MIN_HEIGHT &&
                  videoProcessingTargetIn->Out.Height <= MHW_SFC_MAX_HEIGHT) &&
                 // only conversion to RGB4 is supported by driver
                 (fourCC == MFX_FOURCC_RGB4) &&
                 // resize is not supported by driver
                 (videoProcessingTargetIn->In.CropX == videoProcessingTargetIn->Out.CropX &&
                  videoProcessingTargetIn->In.CropY == videoProcessingTargetIn->Out.CropY &&
                  videoProcessingTargetIn->In.CropW == videoProcessingTargetIn->Out.CropW &&
                  videoProcessingTargetIn->In.CropH == videoProcessingTargetIn->Out.CropH)
                )
            {
                *videoProcessingTargetOut = *videoProcessingTargetIn;
            }
            else
            {
                sts = MFX_ERR_UNSUPPORTED;
            }
        }
    }
    else
    {
        out->mfx.CodecId = MFX_CODEC_JPEG;
        out->mfx.CodecProfile = 1;
        out->mfx.CodecLevel = 1;

        out->mfx.NumThread = 1;

        out->AsyncDepth = 1;

        // mfxFrameInfo
        out->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
        out->mfx.FrameInfo.Width = 1;
        out->mfx.FrameInfo.Height = 1;

        //out->mfx.FrameInfo.CropX = 1;
        //out->mfx.FrameInfo.CropY = 1;
        //out->mfx.FrameInfo.CropW = 1;
        //out->mfx.FrameInfo.CropH = 1;

        out->mfx.FrameInfo.FrameRateExtN = 1;
        out->mfx.FrameInfo.FrameRateExtD = 1;

        out->mfx.FrameInfo.AspectRatioW = 1;
        out->mfx.FrameInfo.AspectRatioH = 1;

        out->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

        out->mfx.JPEGChromaFormat = 1;
        out->mfx.JPEGColorFormat = 1;
        out->mfx.Rotation = 1;

        if (type == MFX_HW_UNKNOWN)
        {
            out->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
        }
        else
        {
            out->IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
        }

        mfxExtOpaqueSurfaceAlloc * opaqueOut = (mfxExtOpaqueSurfaceAlloc *)GetExtBuffer(out->ExtParam, out->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
        if (opaqueOut)
        {
        }
    }

    return sts;
}

bool MFX_JPEG_Utility::CheckVideoParam(mfxVideoParam *in, eMFXHWType )
{
    if (!in)
        return false;

    if (in->Protected)
       return false;

    if (MFX_CODEC_JPEG != in->mfx.CodecId)
        return false;

    if (in->mfx.FrameInfo.Width % 16)
        return false;

    if (in->mfx.FrameInfo.Height % 16)
        return false;

    // both zero or not zero
    if ((in->mfx.FrameInfo.AspectRatioW || in->mfx.FrameInfo.AspectRatioH) && !(in->mfx.FrameInfo.AspectRatioW && in->mfx.FrameInfo.AspectRatioH))
        return false;

    switch (in->mfx.FrameInfo.PicStruct)
    {
    case MFX_PICSTRUCT_PROGRESSIVE:
    case MFX_PICSTRUCT_FIELD_TFF:
    case MFX_PICSTRUCT_FIELD_BFF:
        break;
    default:
        return false;
    }

    if(in->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE &&
       in->mfx.Rotation != MFX_ROTATION_0)
        return false;

    mfxU32 fourCC = in->mfx.FrameInfo.FourCC;
    mfxU16 chromaFormat = in->mfx.FrameInfo.ChromaFormat;

#if defined (MFX_VA_LINUX)
    if ((fourCC != MFX_FOURCC_NV12 || chromaFormat != MFX_CHROMAFORMAT_MONOCHROME) &&
        (fourCC != MFX_FOURCC_NV12 || chromaFormat != MFX_CHROMAFORMAT_YUV420) &&
        (fourCC != MFX_FOURCC_RGB4 || chromaFormat != MFX_CHROMAFORMAT_YUV444) &&
        (fourCC != MFX_FOURCC_YUY2 || chromaFormat != MFX_CHROMAFORMAT_YUV422H))
           return false;
#else
    if ((fourCC != MFX_FOURCC_NV12 || chromaFormat != MFX_CHROMAFORMAT_MONOCHROME) &&
        (fourCC != MFX_FOURCC_NV12 || chromaFormat != MFX_CHROMAFORMAT_YUV420) &&
        (fourCC != MFX_FOURCC_RGB4 || chromaFormat != MFX_CHROMAFORMAT_YUV444) &&
        (fourCC != MFX_FOURCC_YUY2 || chromaFormat != MFX_CHROMAFORMAT_YUV422H))
        return false;

#endif

    if (!(in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && !(in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY) && !(in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
        return false;

    if ((in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY) && (in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
        return false;

    if ((in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) && (in->IOPattern & MFX_IOPATTERN_OUT_SYSTEM_MEMORY))
        return false;

    if ((in->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY) && (in->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
        return false;

    return true;
}

mfxStatus VideoDECODEMJPEG::UpdateAllocRequest(mfxVideoParam *par,
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

    if (pOpaqAlloc->Out.Type & MFX_MEMTYPE_FROM_VPPOUT)
        request->Type = MFX_MEMTYPE_OPAQUE_FRAME | MFX_MEMTYPE_FROM_VPPOUT;
    else
        request->Type = MFX_MEMTYPE_OPAQUE_FRAME | MFX_MEMTYPE_FROM_DECODE;

    switch (pOpaqAlloc->Out.Type & (MFX_MEMTYPE_DXVA2_DECODER_TARGET|MFX_MEMTYPE_SYSTEM_MEMORY|MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET))
    {
    case MFX_MEMTYPE_SYSTEM_MEMORY:
        request->Type |= MFX_MEMTYPE_SYSTEM_MEMORY;
        break;
    case MFX_MEMTYPE_DXVA2_DECODER_TARGET:
        request->Type |= MFX_MEMTYPE_DXVA2_DECODER_TARGET;
        break;
    case MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET:
        request->Type |= MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET;
        break;
    default:
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }
    request->NumFrameMin = request->NumFrameSuggested = pOpaqAlloc->Out.NumSurface;
    mapping = true;
    return MFX_ERR_NONE;
}

mfxFrameSurface1 *VideoDECODEMJPEG::GetOriginalSurface(mfxFrameSurface1 *surface)
{
    if (m_isOpaq)
        return m_core->GetNativeSurface(surface);
    return surface;
}


VideoDECODEMJPEGBase::VideoDECODEMJPEGBase()
{
     memset(&m_stat, 0, sizeof(m_stat));
     m_isOpaq = 0;
}

mfxStatus VideoDECODEMJPEGBase::GetVideoParam(mfxVideoParam *par, UMC::MJPEGVideoDecoderBaseMFX * mjpegDecoder)
{
    mfxExtJPEGQuantTables* jpegQT = (mfxExtJPEGQuantTables*) GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_JPEG_QT );
    mfxExtJPEGHuffmanTables*  jpegHT = (mfxExtJPEGHuffmanTables*) GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_JPEG_HUFFMAN );

    if(!jpegQT && !jpegHT)
        return MFX_ERR_NONE;

    UMC::Status umcRes = UMC::UMC_OK;

    if(jpegQT)
    {
        umcRes = mjpegDecoder->FillQuantTableExtBuf(jpegQT);
        if (umcRes != UMC::UMC_OK)
            return ConvertUMCStatusToMfx(umcRes);
    }

    if(jpegHT)
    {
        umcRes = mjpegDecoder->FillHuffmanTableExtBuf(jpegHT);
        if (umcRes != UMC::UMC_OK)
            return ConvertUMCStatusToMfx(umcRes);
    }

    return MFX_ERR_NONE;
}

VideoDECODEMJPEGBase_HW::VideoDECODEMJPEGBase_HW()
{
    m_pMJPEGVideoDecoder.reset(new UMC::MJPEGVideoDecoderMFX_HW()); // HW
    m_FrameAllocator.reset(new mfx_UMC_FrameAllocator_D3D);
    m_va = 0;
    m_dst = 0;
    m_numPic = 0;
    m_needVpp = false;
}

mfxStatus VideoDECODEMJPEGBase_HW::Init(mfxVideoParam *decPar, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response, mfxFrameAllocRequest *request_internal,
                                            bool isUseExternalFrames, VideoCORE *core)
{
    UMC::Status umcSts = m_FrameAllocator->InitMfx(0, core, decPar, request, response, isUseExternalFrames, false);
    if (umcSts != UMC::UMC_OK)
        return MFX_ERR_MEMORY_ALLOC;

    ConvertMFXParamsToUMC(decPar, &umcVideoParams);
    umcVideoParams.numThreads = m_vPar.mfx.NumThread;

    mfxStatus mfxSts = core->CreateVA(decPar, request, response, m_FrameAllocator.get());
    if (mfxSts < MFX_ERR_NONE)
        return mfxSts;

    core->GetVA((mfxHDL*)&m_va, MFX_MEMTYPE_FROM_DECODE);

    m_pMJPEGVideoDecoder->SetFrameAllocator(m_FrameAllocator.get());
    umcVideoParams.pVideoAccelerator = m_va;

    umcSts = m_pMJPEGVideoDecoder->Init(&umcVideoParams);
    if (umcSts != UMC::UMC_OK)
    {
        return ConvertUMCStatusToMfx(umcSts);
    }
    m_pMJPEGVideoDecoder->SetFourCC(request_internal->Info.FourCC);
    m_numPic = 0;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_HW::Reset(mfxVideoParam *par)
{
    m_pMJPEGVideoDecoder->Reset();
    m_numPic = 0;

    m_vPar = *par;

    {
        std::lock_guard<std::mutex> guard(m_guard);

        mfxU32 picToCollect = (MFX_PICSTRUCT_PROGRESSIVE == m_vPar.mfx.FrameInfo.PicStruct) ?
            (1) : (2);

        while(!m_dsts.empty())
        {
            for(mfxU32 i=0; i<picToCollect; i++)
                m_pMJPEGVideoDecoder->CloseFrame(&(m_dsts.back()), i);
            delete [] m_dsts.back();
            m_dsts.pop_back();
        }
    }

    if (m_FrameAllocator->Reset() != UMC::UMC_OK)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    memset(&m_stat, 0, sizeof(mfxDecodeStat));
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_HW::Close(void)
{
    if (!m_pMJPEGVideoDecoder.get())
        return MFX_ERR_NOT_INITIALIZED;

    m_pMJPEGVideoDecoder->Close();
    m_numPic = 0;
    m_isOpaq = false;

    {
        std::lock_guard<std::mutex> guard(m_guard);

        mfxU32 picToCollect = (MFX_PICSTRUCT_PROGRESSIVE == m_vPar.mfx.FrameInfo.PicStruct) ?
            (1) : (2);

        while(!m_dsts.empty())
        {
            for(mfxU32 i=0; i<picToCollect; i++)
                m_pMJPEGVideoDecoder->CloseFrame(&(m_dsts.back()), i);
            delete [] m_dsts.back();
            m_dsts.pop_back();
        }
    }

    memset(&m_stat, 0, sizeof(mfxDecodeStat));

    m_va = 0;
    m_FrameAllocator->Close();

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_HW::GetVideoParam(mfxVideoParam *par)
{
    return VideoDECODEMJPEGBase::GetVideoParam(par, m_pMJPEGVideoDecoder.get());
}

mfxStatus VideoDECODEMJPEGBase_HW::CheckVPPCaps(VideoCORE * core, mfxVideoParam * par)
{
    VideoVppJpegD3D9 cc(core, false, bool(par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY));

    return cc.Init(par);
}

mfxU32 VideoDECODEMJPEGBase_HW::AdjustFrameAllocRequest(mfxFrameAllocRequest *request,
                                               mfxInfoMFX *info,
                                               eMFXHWType hwType,
                                               eMFXVAType vaType,
                                               bool usePostProcessing)
{
    bool needVpp = false;

    // update request in case of interlaced stream
    if(request->Info.PicStruct == MFX_PICSTRUCT_FIELD_TFF || request->Info.PicStruct == MFX_PICSTRUCT_FIELD_BFF)
    {
        request->Info.Height >>= 1;
        request->Info.CropH >>= 1;
        request->NumFrameMin <<= 1;
        request->NumFrameSuggested <<= 1;

        needVpp = true;
    }

    // set FourCC
    AdjustFourCC(&request->Info, info, hwType, vaType, usePostProcessing, &needVpp);

    if(info->Rotation == MFX_ROTATION_90 || info->Rotation == MFX_ROTATION_180 || info->Rotation == MFX_ROTATION_270)
    {
        needVpp = true;
    }

    if(info->Rotation == MFX_ROTATION_90 || info->Rotation == MFX_ROTATION_270)
    {
        std::swap(request->Info.Height, request->Info.Width);
        std::swap(request->Info.AspectRatioH, request->Info.AspectRatioW);
        std::swap(request->Info.CropH, request->Info.CropW);
        std::swap(request->Info.CropY, request->Info.CropX);
    }

    m_needVpp = needVpp;
    if (m_needVpp)
    {
        m_FrameAllocator.reset(new mfx_UMC_FrameAllocator_D3D_Converter());

        if (request->Type & MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)
        {
            request->Type = request->Type &~ MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
            request->Type = request->Type &~ MFX_MEMTYPE_FROM_VPPOUT;
        }
        request->Type |= MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
    }

    return m_needVpp ? 1 : 0;
}

void VideoDECODEMJPEGBase_HW::AdjustFourCC(mfxFrameInfo *requestFrameInfo, const mfxInfoMFX *info, eMFXHWType /*hwType*/, eMFXVAType /*vaType*/, bool usePostProc, bool *needVpp)
{
    if (info->JPEGColorFormat == MFX_JPEG_COLORFORMAT_UNKNOWN || info->JPEGColorFormat == MFX_JPEG_COLORFORMAT_YCbCr)
    {
        switch(info->JPEGChromaFormat)
        {
        case MFX_CHROMAFORMAT_MONOCHROME:
            if (requestFrameInfo->FourCC == MFX_FOURCC_RGB4)
            {
                requestFrameInfo->FourCC = MFX_FOURCC_NV12;
                *needVpp = true;
            }
            break;
        case MFX_CHROMAFORMAT_YUV420:
            if (!usePostProc && info->Rotation == MFX_ROTATION_0 &&
                requestFrameInfo->FourCC == MFX_FOURCC_RGB4)
            {
                requestFrameInfo->FourCC = MFX_FOURCC_NV12;
                *needVpp = true;
            }
            break;
        case MFX_CHROMAFORMAT_YUV411:
            break;
        case MFX_CHROMAFORMAT_YUV422H:
            if (!usePostProc && info->Rotation == MFX_ROTATION_0 &&
                requestFrameInfo->FourCC == MFX_FOURCC_RGB4)
            {
                requestFrameInfo->FourCC = MFX_FOURCC_NV12;
                *needVpp = true;
            }
            break;
        case MFX_CHROMAFORMAT_YUV422V:
            break;
        case MFX_CHROMAFORMAT_YUV444:
            if (info->Rotation == MFX_ROTATION_0 &&
                ((!usePostProc && requestFrameInfo->FourCC == MFX_FOURCC_RGB4)
                 // for YUV444 jpeg, decoded stream contains 444P,
                 // to get NV12 we must use VPP
                 || requestFrameInfo->FourCC == MFX_FOURCC_NV12))
            {
                requestFrameInfo->FourCC = MFX_FOURCC_NV12;
                *needVpp = true;
            }
            break;
        default:
            VM_ASSERT(false);
            break;
        }
    }
    else if(info->JPEGColorFormat == MFX_JPEG_COLORFORMAT_RGB)
    {
        if(info->JPEGChromaFormat == MFX_CHROMAFORMAT_YUV444 && info->Rotation == MFX_ROTATION_0)
        {
            requestFrameInfo->FourCC = MFX_FOURCC_RGBP;
            *needVpp = true;
        }
    }
}


mfxStatus VideoDECODEMJPEGBase_HW::RunThread(void *params, mfxU32, mfxU32 )
{
    mfxStatus mfxSts = MFX_ERR_NONE;
    MFX_CHECK_NULL_PTR1(params);

    ThreadTaskInfo * info = (ThreadTaskInfo *)params;

    if (m_needVpp)
    {
        if(info->needCheckVppStatus)
        {
            mfxSts = ((mfx_UMC_FrameAllocator_D3D_Converter *)m_FrameAllocator.get())->CheckPreparingToOutput(info->surface_out,
                                                                                                              info->dst,
                                                                                                              &m_vPar,
                                                                                                              (mfxU16)info->vppTaskID);
            if(mfxSts != MFX_TASK_DONE)
            {
                return mfxSts;
            }
            info->needCheckVppStatus = false;
        }

        mfxU16 corruptedField = 0;
        if(info->numDecodeTasksToCheck == 2)
        {
            mfxSts = m_pMJPEGVideoDecoder->CheckStatusReportNumber(info->decodeTaskID-1, &corruptedField);
            if(mfxSts != MFX_TASK_DONE)
            {
                return mfxSts;
            }
            info->numDecodeTasksToCheck--;
            info->surface_out->Data.Corrupted |= corruptedField;
        }

        if(info->numDecodeTasksToCheck == 1)
        {
            mfxSts = m_pMJPEGVideoDecoder->CheckStatusReportNumber(info->decodeTaskID, &corruptedField);
            if(mfxSts != MFX_TASK_DONE)
            {
                return mfxSts;
            }
            info->numDecodeTasksToCheck--;
            info->surface_out->Data.Corrupted |= corruptedField;
        }
    }
    else
    {
        mfxSts = m_pMJPEGVideoDecoder->CheckStatusReportNumber(info->decodeTaskID, &(info->surface_out->Data.Corrupted));
        if(mfxSts != MFX_TASK_DONE)
        {
            return mfxSts;
        }

        mfxSts = ((mfx_UMC_FrameAllocator_D3D *)m_FrameAllocator.get())->PrepareToOutput(info->surface_out, info->dst->GetFrameMID(), &m_vPar, m_isOpaq);
        if (mfxSts < MFX_ERR_NONE)
        {
            return mfxSts;
        }
    }

    info = 0;

    return MFX_TASK_DONE;
}

mfxStatus VideoDECODEMJPEGBase_HW::ReserveUMCDecoder(UMC::MJPEGVideoDecoderBaseMFX* &pMJPEGVideoDecoder, mfxFrameSurface1 *surf, bool isOpaq)
{
    pMJPEGVideoDecoder = nullptr;
    MFX_SAFE_CALL(m_FrameAllocator->SetCurrentMFXSurface(surf, isOpaq));

    if (m_numPic == 0)
    {
        int picToCollect = (MFX_PICSTRUCT_PROGRESSIVE == m_vPar.mfx.FrameInfo.PicStruct) ?
                       (1) : (2);
        delete[] m_dst;
        m_dst = new UMC::FrameData[picToCollect];
    }

    pMJPEGVideoDecoder = m_pMJPEGVideoDecoder.get();
    return MFX_ERR_NONE;
}

void VideoDECODEMJPEGBase_HW::ReleaseReservedTask()
{
    if (m_numPic == 0)
    {
        delete[] m_dst;
        m_dst = 0;
        m_numPic = 0;
    }
}

mfxStatus VideoDECODEMJPEGBase_HW::AddPicture(UMC::MediaDataEx *pSrcData, mfxU32 & numPic)
{
    mfxU32 fieldPos = m_numPic;

    if (MFX_PICSTRUCT_FIELD_BFF == m_vPar.mfx.FrameInfo.PicStruct)
    {
        // change field order in BFF case
        fieldPos ^= 1;
    }

    UMC::Status umcRes = UMC::UMC_OK;
    umcRes = m_pMJPEGVideoDecoder->SetRotation(0);
    if (umcRes != UMC::UMC_OK)
    {
        delete[] m_dst;
        m_dst = 0;
        return ConvertUMCStatusToMfx(umcRes);
    }

    umcRes = m_pMJPEGVideoDecoder->GetFrame(pSrcData, &m_dst, fieldPos);

    mfxStatus sts = MFX_ERR_NONE;

    // convert status
    if (umcRes == UMC::UMC_ERR_NOT_ENOUGH_DATA || umcRes == UMC::UMC_ERR_SYNC)
    {
        if (m_numPic == 0)
        {
            delete[] m_dst;
            m_dst = 0;
        }

        sts = MFX_ERR_MORE_DATA;
    }
    else
    {
        if (umcRes != UMC::UMC_OK)
        {
            delete[] m_dst;
            m_dst = 0;
            sts = ConvertUMCStatusToMfx(umcRes);
        }
    }

    if (!(umcRes == UMC::UMC_OK && m_dst))    // return frame to display
        return sts;

    m_numPic++;
    numPic = m_numPic;

    return sts;
}

mfxStatus VideoDECODEMJPEGBase_HW::AllocateFrameData(UMC::FrameData *&data)
{
    std::lock_guard<std::mutex> guard(m_guard);
    m_dsts.push_back(m_dst);
    data = m_dst;
    m_dst = 0;
    m_numPic = 0;
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_HW::FillEntryPoint(MFX_ENTRY_POINT *pEntryPoint, mfxFrameSurface1 *surface_work, mfxFrameSurface1 *surface_out)
{
    mfxU16 taskId = 0;

    if (m_dsts.empty())
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    UMC::FrameData *dst = m_dsts.back();
    if (m_needVpp)
    {
        UMC::ConvertInfo * convertInfo = m_pMJPEGVideoDecoder->GetConvertInfo();
        mfx_UMC_FrameAllocator_D3D_Converter::JPEG_Info info;
        info.colorFormat = convertInfo->colorFormat;
        info.UOffset = convertInfo->UOffset;
        info.VOffset = convertInfo->VOffset;

        ((mfx_UMC_FrameAllocator_D3D_Converter *)m_FrameAllocator.get())->SetJPEGInfo(&info);

        // decoding is ready. prepare to output:
        mfxStatus mfxSts = ((mfx_UMC_FrameAllocator_D3D_Converter *)m_FrameAllocator.get())->StartPreparingToOutput(surface_out, dst, &m_vPar, &taskId, m_isOpaq);
        if (mfxSts < MFX_ERR_NONE)
        {
            return mfxSts;
        }
    }

    ThreadTaskInfo * info = new ThreadTaskInfo();
    info->surface_work = surface_work;
    info->surface_out = surface_out;
    info->decodeTaskID = m_pMJPEGVideoDecoder->GetStatusReportNumber();
    info->vppTaskID = (mfxU32)taskId;
    info->needCheckVppStatus = m_needVpp;
    info->numDecodeTasksToCheck = MFX_PICSTRUCT_PROGRESSIVE == m_vPar.mfx.FrameInfo.PicStruct ? 1 : 2;
    info->dst = dst;

    pEntryPoint->requiredNumThreads = m_vPar.mfx.NumThread;
    pEntryPoint->pParam = info;
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_HW::CheckTaskAvailability(mfxU32 maxTaskNumber)
{
    std::lock_guard<std::mutex> guard(m_guard);

    if (m_dsts.size() >= maxTaskNumber)
    {
        return MFX_WRN_DEVICE_BUSY;
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_HW::CompleteTask(void *pParam, mfxStatus )
{
    ThreadTaskInfo * info = (ThreadTaskInfo *)pParam;

    std::lock_guard<std::mutex> guard(m_guard);

    mfxI32 index = -1;
    for (size_t i = 0; i < m_dsts.size(); i++)
        if(m_dsts[i]->GetFrameMID() == info->dst->GetFrameMID())
        {
            index = (mfxI32)i;
        }

    if(index != -1)
    {
        mfxU32 picToCollect = (MFX_PICSTRUCT_PROGRESSIVE == m_vPar.mfx.FrameInfo.PicStruct) ?
            (1) : (2);

        for(mfxU32 i=0; i<picToCollect; i++)
            m_pMJPEGVideoDecoder->CloseFrame(&(m_dsts[index]), i);
        delete [] m_dsts[index];
        m_dsts.erase(m_dsts.begin() + index);
    }

    delete (ThreadTaskInfo *)pParam;
    return MFX_ERR_NONE;
}

#ifdef MFX_ENABLE_SW_FALLBACK
VideoDECODEMJPEGBase_SW::VideoDECODEMJPEGBase_SW()
{
    m_FrameAllocator.reset(new mfx_UMC_FrameAllocator);
    pLastTask = NULL;
    m_tasksCount = 0;
}

mfxStatus VideoDECODEMJPEGBase_SW::Init(mfxVideoParam *decPar, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response, mfxFrameAllocRequest *, bool isUseExternalFrames, VideoCORE *core)
{
    auto ippSt = MfxIppInit();
    MFX_CHECK(ippSt == ippStsNoErr, MFX_ERR_UNSUPPORTED);

    UMC::Status umcSts = m_FrameAllocator->InitMfx(0, core, decPar, request, response, isUseExternalFrames, true);
    MFX_CHECK(umcSts == UMC::UMC_OK, MFX_ERR_MEMORY_ALLOC);

    ConvertMFXParamsToUMC(decPar, &umcVideoParams);
    umcVideoParams.numThreads = m_vPar.mfx.NumThread;
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_SW::Reset(mfxVideoParam *par)
{
    m_tasksCount = 0;
    pLastTask = nullptr;
    {
        std::lock_guard<std::mutex> guard(m_guard);
        while(!m_freeTasks.empty())
        {
            m_freeTasks.pop();
        }
    }

    memset(&m_stat, 0, sizeof(mfxDecodeStat));
    m_vPar = *par;

    UMC::Status umcSts = m_FrameAllocator->Reset();
    MFX_CHECK(umcSts == UMC::UMC_OK, MFX_ERR_MEMORY_ALLOC);

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_SW::Close()
{
    UMC::Status umcSts = m_FrameAllocator->Close();

    m_tasksCount = 0;
    pLastTask = nullptr;
    memset(&m_stat, 0, sizeof(mfxDecodeStat));

    // delete free tasks queue
    {
        std::lock_guard<std::mutex> guard(m_guard);
        while (!m_freeTasks.empty())
        {
            m_freeTasks.pop();
        }
    }

    m_isOpaq = false;

    MFX_CHECK(umcSts == UMC::UMC_OK, ConvertUMCStatusToMfx(umcSts));

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_SW::GetVideoParam(mfxVideoParam *par)
{
    mfxExtJPEGQuantTables* jpegQT = (mfxExtJPEGQuantTables*) GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_JPEG_QT );
    mfxExtJPEGHuffmanTables*  jpegHT = (mfxExtJPEGHuffmanTables*) GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_JPEG_HUFFMAN );

    if(!jpegQT && !jpegHT)
        return MFX_ERR_NONE;

    MFX_CHECK(pLastTask, MFX_ERR_UNSUPPORTED);

    return VideoDECODEMJPEGBase::GetVideoParam(par, pLastTask->m_pMJPEGVideoDecoder.get());
}

mfxStatus VideoDECODEMJPEGBase_SW::ReserveUMCDecoder(UMC::MJPEGVideoDecoderBaseMFX* &pMJPEGVideoDecoder, mfxFrameSurface1 *surf, bool isOpaq)
{
    pMJPEGVideoDecoder = 0;
    MFX_SAFE_CALL(m_FrameAllocator->SetCurrentMFXSurface(surf, isOpaq));

    pMJPEGVideoDecoder = m_freeTasks.front()->m_pMJPEGVideoDecoder.get();
    //pMJPEGVideoDecoder->Reset();
    return MFX_ERR_NONE;
}

void VideoDECODEMJPEGBase_SW::ReleaseReservedTask()
{
    if (!m_freeTasks.empty())
        m_freeTasks.front()->Reset();
}

mfxStatus VideoDECODEMJPEGBase_SW::AddPicture(UMC::MediaDataEx *pSrcData, mfxU32 & numPic)
{
    // select the field position. 0 means top, 1 means bottom.
    mfxU32 fieldPos = m_freeTasks.front()->NumPicCollected();

    if (MFX_PICSTRUCT_FIELD_BFF == m_vPar.mfx.FrameInfo.PicStruct)
    {
        // change field order in BFF case
        fieldPos ^= 1;
    }

    // add picture to the task
    MFX_SAFE_CALL(m_freeTasks.front()->AddPicture(pSrcData, fieldPos));
    numPic = m_freeTasks.front()->NumPicCollected();
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_SW::FillEntryPoint(MFX_ENTRY_POINT *pEntryPoint, mfxFrameSurface1 *surface_work, mfxFrameSurface1 *surface_out)
{
    // remove the ready task from the queue
    {
        std::lock_guard<std::mutex> guard(m_guard);
        pLastTask = m_freeTasks.front().release();
        m_freeTasks.pop();
    }

    pLastTask->surface_work = surface_work;
    pLastTask->surface_out = surface_out;

    pEntryPoint->requiredNumThreads = std::min(pLastTask->m_pMJPEGVideoDecoder->NumDecodersAllocated(),
                                               pLastTask->NumPiecesCollected());
    pEntryPoint->pParam = pLastTask;

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_SW::AllocateFrameData(UMC::FrameData *&data)
{
    CJpegTask *pTask = m_freeTasks.front().get();

    // prepare the decoder(s)
    UMC::Status umcRes = pTask->m_pMJPEGVideoDecoder->AllocateFrame();
    if (UMC::UMC_OK != umcRes)
    {
        return ConvertUMCStatusToMfx(umcRes);
    }

    // save parameters to the task
    pTask->dst = pTask->m_pMJPEGVideoDecoder.get()->GetDst();

    UMC::FrameData *dst = m_freeTasks.front()->dst;
    dst->SetTime(m_freeTasks.front()->GetPictureBuffer(0).timeStamp);
    data = dst;
    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_SW::CheckTaskAvailability(mfxU32 maxTaskNumber)
{
    if (m_freeTasks.empty())
    {
        if (m_tasksCount >= maxTaskNumber)
        {
            return MFX_WRN_DEVICE_BUSY;
        }

        std::unique_ptr<CJpegTask> pTask(new CJpegTask());
        m_tasksCount++;

        // initialize the task
        MFX_SAFE_CALL(pTask->Initialize(umcVideoParams,
                                        m_FrameAllocator.get(),
                                        m_vPar.mfx.Rotation,
                                        m_vPar.mfx.JPEGChromaFormat,
                                        m_vPar.mfx.JPEGColorFormat));

        // save the task object into the queue
        {
            std::lock_guard<std::mutex> guard(m_guard);
            m_freeTasks.push(std::move(pTask));
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus VideoDECODEMJPEGBase_SW::RunThread(void *pParam, mfxU32 threadNumber, mfxU32 callNumber)
{
    CJpegTask *task = (CJpegTask *) pParam;
    if (!task)
        return MFX_ERR_NULL_PTR;

    // check the number of call. one call = one piece decoded. all extra call
    // should go exit.
    if (callNumber >= task->NumPiecesCollected())
    {
        return MFX_TASK_DONE;
    }

    // do decoding process
    UMC::Status umcSts = task->m_pMJPEGVideoDecoder->DecodePicture(*task, threadNumber, callNumber);
    MFX_CHECK(umcSts == UMC::UMC_OK, MFX_ERR_INVALID_VIDEO_PARAM);

    return ((callNumber + 1) == task->NumPiecesCollected()) ? (MFX_TASK_DONE) : (MFX_TASK_WORKING);
}

mfxStatus VideoDECODEMJPEGBase_SW::CompleteTask(void *pParam, mfxStatus taskRes)
{
    CJpegTask &task = *((CJpegTask *) pParam);

    // do color conversion and other useless stuff
    if (MFX_ERR_NONE == taskRes)
    {
        UMC::Status umcRes = UMC::UMC_OK;

        umcRes = task.m_pMJPEGVideoDecoder->PostProcessing(task.GetPictureBuffer(0).timeStamp);
        if (UMC::UMC_OK != umcRes)
        {
            return ConvertUMCStatusToMfx(umcRes);
        }

        // decoding is ready. prepare to output:
        mfxStatus mfxSts = m_FrameAllocator->PrepareToOutput(task.surface_out,
                                                                    task.dst->GetFrameMID(),
                                                                    &m_vPar,
                                                                    m_isOpaq);
        if (mfxSts < MFX_ERR_NONE)
        {
            return mfxSts;
        }

        task.m_pMJPEGVideoDecoder->CloseFrame();

        m_stat.NumFrame++;
    }

    task.Reset();
    {
        std::lock_guard<std::mutex> guard(m_guard);
        m_freeTasks.emplace(&task);
    }
    return MFX_ERR_NONE;
}
#endif //ifdef MFX_ENABLE_SW_FALLBACK

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
