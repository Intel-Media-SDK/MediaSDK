/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include "mfx_samples_config.h"
#include "sample_defs.h"
#include <algorithm>


#include <ctime>
#include <algorithm>
#include "pipeline_decode.h"
#include "sysmem_allocator.h"


#if defined LIBVA_SUPPORT
#include "vaapi_allocator.h"
#include "vaapi_device.h"
#include "vaapi_utils.h"
#endif

#if defined(LIBVA_WAYLAND_SUPPORT)
#include "class_wayland.h"
#endif

#include "version.h"

#pragma warning(disable : 4100)

#define __SYNC_WA // avoid sync issue on Media SDK side

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

CDecodingPipeline::CDecodingPipeline()
{
    m_nFrames=0;
    m_export_mode=0;
    m_bVppFullColorRange=false;
    m_bVppIsUsed = false;
    MSDK_ZERO_MEMORY(m_mfxBS);

    m_pmfxDEC = NULL;
    m_pmfxVPP = NULL;
    m_impl = 0;

    MSDK_ZERO_MEMORY(m_mfxVideoParams);
    MSDK_ZERO_MEMORY(m_mfxVppVideoParams);

    m_pGeneralAllocator = NULL;
    m_pmfxAllocatorParams = NULL;
    m_memType = SYSTEM_MEMORY;
    m_bExternalAlloc = false;
    m_bDecOutSysmem = false;
    MSDK_ZERO_MEMORY(m_mfxResponse);
    MSDK_ZERO_MEMORY(m_mfxVppResponse);

    m_pCurrentFreeSurface = NULL;
    m_pCurrentFreeVppSurface = NULL;
    m_pCurrentFreeOutputSurface = NULL;
    m_pCurrentOutputSurface = NULL;

    m_pDeliverOutputSemaphore = NULL;
    m_pDeliveredEvent = NULL;
    m_error = MFX_ERR_NONE;
    m_bStopDeliverLoop = false;

    m_eWorkMode = MODE_PERFORMANCE;
    m_bIsMVC = false;
    m_bIsExtBuffers = false;
    m_bIsVideoWall = false;
    m_bIsCompleteFrame = false;
    m_bPrintLatency = false;
    m_fourcc = 0;

    m_nTimeout = 0;
    m_nMaxFps = 0;

    m_diMode = 0;
    m_bRenderWin = false;
    m_vppOutWidth  = 0;
    m_vppOutHeight = 0;

    m_nRenderWinX = 0;
    m_nRenderWinY = 0;
    m_nRenderWinH = 0;
    m_nRenderWinW = 0;

    m_bResetFileWriter = false;
    m_bResetFileReader = false;

    m_startTick = 0;
    m_delayTicks = 0;

    MSDK_ZERO_MEMORY(m_VppDoNotUse);
    m_VppDoNotUse.Header.BufferId = MFX_EXTBUFF_VPP_DONOTUSE;
    m_VppDoNotUse.Header.BufferSz = sizeof(m_VppDoNotUse);

    MSDK_ZERO_MEMORY(m_VppDeinterlacing)
    m_VppDeinterlacing.Header.BufferId = MFX_EXTBUFF_VPP_DEINTERLACING;
    m_VppDeinterlacing.Header.BufferSz = sizeof(m_VppDeinterlacing);

    MSDK_ZERO_MEMORY(m_VppVideoSignalInfo);
    m_VppVideoSignalInfo.Header.BufferId = MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO;
    m_VppVideoSignalInfo.Header.BufferSz = sizeof(m_VppVideoSignalInfo);

#if MFX_VERSION >= 1022
    MSDK_ZERO_MEMORY(m_DecoderPostProcessing);
    m_DecoderPostProcessing.Header.BufferId = MFX_EXTBUFF_DEC_VIDEO_PROCESSING;
    m_DecoderPostProcessing.Header.BufferSz = sizeof(mfxExtDecVideoProcessing);
#endif //MFX_VERSION >= 1022

#if (MFX_VERSION >= 1025)
    MSDK_ZERO_MEMORY(m_DecodeErrorReport);
    m_DecodeErrorReport.Header.BufferId = MFX_EXTBUFF_DECODE_ERROR_REPORT;
#endif

    m_hwdev = NULL;

    m_bOutI420 = false;

#ifdef LIBVA_SUPPORT
    m_export_mode = vaapiAllocatorParams::DONOT_EXPORT;
    m_libvaBackend = 0;
    m_bPerfMode = false;
#endif

    m_monitorType = 0;
    totalBytesProcessed = 0;
    m_vLatency.reserve(1000); // reserve some space to reduce dynamic reallocation impact on pipeline execution
}

CDecodingPipeline::~CDecodingPipeline()
{
    Close();
}

mfxStatus CDecodingPipeline::Init(sInputParams *pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    // prepare input stream file reader
    // for VP8 complete and single frame reader is a requirement
    // create reader that supports completeframe mode for latency oriented scenarios
    if (pParams->bLowLat || pParams->bCalLat)
    {
        switch (pParams->videoType)
        {
        case MFX_CODEC_AVC:
            m_FileReader.reset(new CH264FrameReader());
            m_bIsCompleteFrame = true;
            m_bPrintLatency = pParams->bCalLat;
            break;
        case MFX_CODEC_JPEG:
            m_FileReader.reset(new CJPEGFrameReader());
            m_bIsCompleteFrame = true;
            m_bPrintLatency = pParams->bCalLat;
            break;
        case MFX_CODEC_VP8:
        case MFX_CODEC_VP9:
            m_FileReader.reset(new CIVFFrameReader());
            m_bIsCompleteFrame = true;
            m_bPrintLatency = pParams->bCalLat;
            break;
        default:
            return MFX_ERR_UNSUPPORTED; // latency mode is supported only for H.264 and JPEG codecs
        }
    }
    else
    {
        switch (pParams->videoType)
        {
        case MFX_CODEC_VP8:
        case MFX_CODEC_VP9:
            m_FileReader.reset(new CIVFFrameReader());
            break;
        default:
            m_FileReader.reset(new CSmplBitstreamReader());
            break;
        }
    }

    if (pParams->fourcc)
        m_fourcc = pParams->fourcc;

#ifdef LIBVA_SUPPORT
    if(pParams->bPerfMode)
        m_bPerfMode = true;
#endif

    if (pParams->Width)
        m_vppOutWidth = pParams->Width;
    if (pParams->Height)
        m_vppOutHeight = pParams->Height;


    m_memType = pParams->memType;

    m_nMaxFps = pParams->nMaxFPS;
    m_nFrames = pParams->nFrames ? pParams->nFrames : MFX_INFINITE;

    m_bOutI420 = pParams->outI420;

    m_nTimeout = pParams->nTimeout;

    // Initializing file reader
    totalBytesProcessed = 0;
    sts = m_FileReader->Init(pParams->strSrcFile);
    MSDK_CHECK_STATUS(sts, "m_FileReader->Init failed");

    mfxInitParam initPar;
    mfxExtThreadsParam threadsPar;
    mfxExtBuffer* extBufs[1];
    mfxVersion version;     // real API version with which library is initialized

    MSDK_ZERO_MEMORY(initPar);
    MSDK_ZERO_MEMORY(threadsPar);

    // we set version to 1.0 and later we will query actual version of the library which will got leaded
    initPar.Version.Major = 1;
    initPar.Version.Minor = 0;

    initPar.GPUCopy = pParams->gpuCopy;

    init_ext_buffer(threadsPar);

    bool needInitExtPar = false;

    if (pParams->eDeinterlace)
    {
        m_diMode = pParams->eDeinterlace;
    }

    if (pParams->bUseFullColorRange)
    {
        m_bVppFullColorRange = pParams->bUseFullColorRange;
    }

    if (pParams->nThreadsNum) {
        threadsPar.NumThread = pParams->nThreadsNum;
        needInitExtPar = true;
    }
    if (pParams->SchedulingType) {
        threadsPar.SchedulingType = pParams->SchedulingType;
        needInitExtPar = true;
    }
    if (pParams->Priority) {
        threadsPar.Priority = pParams->Priority;
        needInitExtPar = true;
    }
    if (needInitExtPar) {
        extBufs[0] = (mfxExtBuffer*)&threadsPar;
        initPar.ExtParam = extBufs;
        initPar.NumExtParam = 1;
    }

    // Init session
    if (pParams->bUseHWLib)
    {
        // try searching on all display adapters
        initPar.Implementation = MFX_IMPL_HARDWARE_ANY;

        // if d3d11 surfaces are used ask the library to run acceleration through D3D11
        // feature may be unsupported due to OS or MSDK API version

        if (D3D11_MEMORY == pParams->memType)
            initPar.Implementation |= MFX_IMPL_VIA_D3D11;

        // Library should pick first available compatible adapter during InitEx call with MFX_IMPL_HARDWARE_ANY
        sts = m_mfxSession.InitEx(initPar);
    }
    else
    {
        initPar.Implementation = MFX_IMPL_SOFTWARE;
        sts = m_mfxSession.InitEx(initPar);
    }

    MSDK_CHECK_STATUS(sts, "m_mfxSession.Init failed");

    sts = m_mfxSession.QueryVersion(&version); // get real API version of the loaded library
    MSDK_CHECK_STATUS(sts, "m_mfxSession.QueryVersion failed");

    sts = m_mfxSession.QueryIMPL(&m_impl); // get actual library implementation
    MSDK_CHECK_STATUS(sts, "m_mfxSession.QueryIMPL failed");

    if (pParams->bIsMVC && !CheckVersion(&version, MSDK_FEATURE_MVC)) {
        msdk_printf(MSDK_STRING("error: MVC is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;

    }
    if ((pParams->videoType == MFX_CODEC_JPEG) && !CheckVersion(&version, MSDK_FEATURE_JPEG_DECODE)) {
        msdk_printf(MSDK_STRING("error: Jpeg is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }
    if (pParams->bLowLat && !CheckVersion(&version, MSDK_FEATURE_LOW_LATENCY)) {
        msdk_printf(MSDK_STRING("error: Low Latency mode is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }

    if (pParams->eDeinterlace &&
        (pParams->eDeinterlace != MFX_DEINTERLACING_ADVANCED) &&
        (pParams->eDeinterlace != MFX_DEINTERLACING_BOB) )
    {
        msdk_printf(MSDK_STRING("error: Unsupported deinterlace value: %d\n"), pParams->eDeinterlace);
        return MFX_ERR_UNSUPPORTED;
    }

    if (pParams->bRenderWin) {
        m_bRenderWin = pParams->bRenderWin;
        // note: currently position is unsupported for Wayland
#if !defined(LIBVA_WAYLAND_SUPPORT)
        m_nRenderWinX = pParams->nRenderWinX;
        m_nRenderWinY = pParams->nRenderWinY;
#endif
    }

    m_delayTicks = pParams->nMaxFPS ? msdk_time_get_frequency() / pParams->nMaxFPS : 0;

    // create decoder
    m_pmfxDEC = new MFXVideoDECODE(m_mfxSession);
    MSDK_CHECK_POINTER(m_pmfxDEC, MFX_ERR_MEMORY_ALLOC);

    // set video type in parameters
    m_mfxVideoParams.mfx.CodecId = pParams->videoType;

    // prepare bit stream
    sts = InitMfxBitstream(&m_mfxBS, 8 * 1024 * 1024);
    MSDK_CHECK_STATUS(sts, "InitMfxBitstream failed");

    if (CheckVersion(&version, MSDK_FEATURE_PLUGIN_API)) {
        /* Here we actually define the following codec initialization scheme:
        *  1. If plugin path or guid is specified: we load user-defined plugin (example: VP8 sample decoder plugin)
        *  2. If plugin path not specified:
        *    2.a) we check if codec is distributed as a mediasdk plugin and load it if yes
        *    2.b) if codec is not in the list of mediasdk plugins, we assume, that it is supported inside mediasdk library
        */
        // Load user plug-in, should go after CreateAllocator function (when all callbacks were initialized)
        if (pParams->pluginParams.type == MFX_PLUGINLOAD_TYPE_FILE && msdk_strnlen(pParams->pluginParams.strPluginPath,sizeof(pParams->pluginParams.strPluginPath)))
        {
            m_pUserModule.reset(new MFXVideoUSER(m_mfxSession));
            if (pParams->videoType == MFX_CODEC_HEVC || pParams->videoType == MFX_CODEC_VP8 ||
                pParams->videoType == MFX_CODEC_VP9)
            {
                m_pPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_DECODE, m_mfxSession, pParams->pluginParams.pluginGuid, 1, pParams->pluginParams.strPluginPath, (mfxU32)msdk_strnlen(pParams->pluginParams.strPluginPath,sizeof(pParams->pluginParams.strPluginPath))));
            }
            if (m_pPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
        }
        else
        {
            bool isDefaultPlugin = false;
            if (AreGuidsEqual(pParams->pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                mfxIMPL impl = pParams->bUseHWLib ? MFX_IMPL_HARDWARE : MFX_IMPL_SOFTWARE;
                pParams->pluginParams.pluginGuid = msdkGetPluginUID(impl, MSDK_VDECODE, pParams->videoType);
                isDefaultPlugin = true;
            }
            if (!AreGuidsEqual(pParams->pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                m_pPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_DECODE, m_mfxSession, pParams->pluginParams.pluginGuid, 1));
                if (m_pPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
            }
            if(sts==MFX_ERR_UNSUPPORTED)
            {
                msdk_printf(isDefaultPlugin ?
                    MSDK_STRING("Default plugin cannot be loaded (possibly you have to define plugin explicitly)\n")
                    : MSDK_STRING("Explicitly specified plugin cannot be loaded.\n"));
            }
        }
        MSDK_CHECK_STATUS(sts, "Plugin load failed");
    }

    // Populate parameters. Involves DecodeHeader call
    sts = InitMfxParams(pParams);
    MSDK_CHECK_STATUS(sts, "InitMfxParams failed");

    if (m_bVppIsUsed)
        m_bDecOutSysmem = pParams->bUseHWLib ? false : true;
    else
        m_bDecOutSysmem = pParams->memType == SYSTEM_MEMORY;

    if (m_bVppIsUsed)
    {
        m_pmfxVPP = new MFXVideoVPP(m_mfxSession);
        if (!m_pmfxVPP) return MFX_ERR_MEMORY_ALLOC;
    }

    m_eWorkMode = pParams->mode;
    if (m_eWorkMode == MODE_FILE_DUMP) {
        // prepare YUV file writer
        sts = m_FileWriter.Init(pParams->strDstFile, pParams->numViews);
        MSDK_CHECK_STATUS(sts, "m_FileWriter.Init failed");
    } else if ((m_eWorkMode != MODE_PERFORMANCE) && (m_eWorkMode != MODE_RENDERING)) {
        msdk_printf(MSDK_STRING("error: unsupported work mode\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    m_monitorType = pParams->monitorType;
    // create device and allocator
#if defined(LIBVA_SUPPORT)
    m_libvaBackend = pParams->libvaBackend;
#endif // defined(MFX_LIBVA_SUPPORT)

    sts = CreateAllocator();
    MSDK_CHECK_STATUS(sts, "CreateAllocator failed");

    // in case of HW accelerated decode frames must be allocated prior to decoder initialization
    sts = AllocFrames();
    MSDK_CHECK_STATUS(sts, "AllocFrames failed");

    sts = m_pmfxDEC->Init(&m_mfxVideoParams);
    if (MFX_WRN_PARTIAL_ACCELERATION == sts)
    {
        msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    }
    MSDK_CHECK_STATUS(sts, "m_pmfxDEC->Init failed");

    if (m_bVppIsUsed)
    {
        if (m_diMode)
            m_mfxVppVideoParams.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

        sts = m_pmfxVPP->Init(&m_mfxVppVideoParams);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Init failed");
    }

    sts = m_pmfxDEC->GetVideoParam(&m_mfxVideoParams);
    MSDK_CHECK_STATUS(sts, "m_pmfxDEC->GetVideoParam failed");

    if (m_eWorkMode == MODE_RENDERING)
    {
        sts = CreateRenderingWindow(pParams);
        MSDK_CHECK_STATUS(sts, "CreateRenderingWindow failed");
    }

    return sts;
}

bool CDecodingPipeline::IsVppRequired(sInputParams *pParams)
{
    bool bVppIsUsed = false;
    /* Re-size */
    if ( (m_mfxVideoParams.mfx.FrameInfo.CropW != pParams->Width) ||
        (m_mfxVideoParams.mfx.FrameInfo.CropH != pParams->Height))
    {
        bVppIsUsed = pParams->Width && pParams->Height;
#if MFX_VERSION >= 1022
        if ((MODE_DECODER_POSTPROC_AUTO == pParams->nDecoderPostProcessing) ||
            (MODE_DECODER_POSTPROC_FORCE == pParams->nDecoderPostProcessing) )
        {
            /* Decoder will make decision about internal post-processing usage slightly later */
            bVppIsUsed = false;
        }
#endif //MFX_VERSION >= 1022
    }

    // JPEG and Capture decoders can provide output in nv12 and rgb4 formats
    if (pParams->videoType == MFX_CODEC_JPEG )
    {
        bVppIsUsed |= m_fourcc && (m_fourcc != MFX_FOURCC_NV12) && (m_fourcc != MFX_FOURCC_RGB4);
    }
    else
    {
        bVppIsUsed |= m_fourcc && (m_fourcc != m_mfxVideoParams.mfx.FrameInfo.FourCC);
    }

    if (pParams->eDeinterlace)
    {
        bVppIsUsed = true;
    }
    return bVppIsUsed;
}

void CDecodingPipeline::Close()
{
#if D3D_SURFACES_SUPPORT
    m_d3dRender.Close();
#endif
    WipeMfxBitstream(&m_mfxBS);
    MSDK_SAFE_DELETE(m_pmfxDEC);
    MSDK_SAFE_DELETE(m_pmfxVPP);

    DeleteFrames();

    if (m_bIsExtBuffers)
    {
        DeallocateExtMVCBuffers();
        DeleteExtBuffers();
    }

    m_ExtBuffersMfxBS.clear();

    m_pPlugin.reset();
    m_mfxSession.Close();
    m_FileWriter.Close();
    if (m_FileReader.get())
        m_FileReader->Close();

    MSDK_SAFE_DELETE_ARRAY(m_VppDoNotUse.AlgList);

    // allocator if used as external for MediaSDK must be deleted after decoder
    DeleteAllocator();

    return;
}

mfxStatus CDecodingPipeline::CreateRenderingWindow(sInputParams *pParams)
{
    mfxStatus sts = MFX_ERR_NONE;

#if D3D_SURFACES_SUPPORT    
    sWindowParams windowParams;

    windowParams.lpWindowName = pParams->bWallNoTitle ? NULL : MSDK_STRING("sample_decode");
    windowParams.nx           = pParams->nWallW;
    windowParams.ny           = pParams->nWallH;
    if (m_bVppIsUsed)
    {
        windowParams.nWidth       = m_mfxVppVideoParams.vpp.Out.Width;
        windowParams.nHeight      = m_mfxVppVideoParams.vpp.Out.Height;
    }
    else
    {
        windowParams.nWidth       = m_mfxVideoParams.mfx.FrameInfo.Width;
        windowParams.nHeight      = m_mfxVideoParams.mfx.FrameInfo.Height;
    }

    windowParams.ncell        = pParams->nWallCell;
    windowParams.nAdapter     = pParams->nWallMonitor;

    windowParams.lpClassName  = MSDK_STRING("Render Window Class");
    windowParams.dwStyle      = WS_OVERLAPPEDWINDOW;
    windowParams.hWndParent   = NULL;
    windowParams.hMenu        = NULL;
    windowParams.hInstance    = GetModuleHandle(NULL);
    windowParams.lpParam      = NULL;
    windowParams.bFullScreen  = FALSE;

    sts = m_d3dRender.Init(windowParams);
    MSDK_CHECK_STATUS(sts, "m_d3dRender.Init failed");

    //setting videowall flag
    m_bIsVideoWall = 0 != windowParams.nx;

#endif
    return sts;
}

mfxStatus CDecodingPipeline::InitMfxParams(sInputParams *pParams)
{
    MSDK_CHECK_POINTER(m_pmfxDEC, MFX_ERR_NULL_PTR);
    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 &numViews = pParams->numViews;

#if (MFX_VERSION >= 1025)
    if (pParams->bErrorReport)
    {
        m_ExtBuffersMfxBS.push_back((mfxExtBuffer *)&m_DecodeErrorReport);
        m_mfxBS.ExtParam = reinterpret_cast<mfxExtBuffer **>(&m_ExtBuffersMfxBS[0]);
        m_mfxBS.NumExtParam = static_cast<mfxU16>(m_ExtBuffersMfxBS.size());
    }
#endif

    // try to find a sequence header in the stream
    // if header is not found this function exits with error (e.g. if device was lost and there's no header in the remaining stream)
    for (;;)
    {
        // trying to find PicStruct information in AVI headers
        if ( m_mfxVideoParams.mfx.CodecId == MFX_CODEC_JPEG )
            MJPEG_AVI_ParsePicStruct(&m_mfxBS);
#if (MFX_VERSION >= 1025)
        if (pParams->bErrorReport)
        {
            mfxExtDecodeErrorReport *pDecodeErrorReport = (mfxExtDecodeErrorReport *)GetExtBuffer(m_mfxBS.ExtParam, m_mfxBS.NumExtParam, MFX_EXTBUFF_DECODE_ERROR_REPORT);

            if (pDecodeErrorReport)
                pDecodeErrorReport->ErrorTypes = 0;

            // parse bit stream and fill mfx params
            sts = m_pmfxDEC->DecodeHeader(&m_mfxBS, &m_mfxVideoParams);

            PrintDecodeErrorReport(pDecodeErrorReport);
        }
        else
        {
        // parse bit stream and fill mfx params
        sts = m_pmfxDEC->DecodeHeader(&m_mfxBS, &m_mfxVideoParams);
        }
#else
        // parse bit stream and fill mfx params
        sts = m_pmfxDEC->DecodeHeader(&m_mfxBS, &m_mfxVideoParams);
#endif

        if (!sts)
        {
            m_bVppIsUsed = IsVppRequired(pParams);
        }

        if (!sts &&
            !(m_impl & MFX_IMPL_SOFTWARE) &&                        // hw lib
            (m_mfxVideoParams.mfx.FrameInfo.BitDepthLuma == 10) &&  // hevc 10 bit
            (m_mfxVideoParams.mfx.CodecId == MFX_CODEC_HEVC) &&
            AreGuidsEqual(pParams->pluginParams.pluginGuid, MFX_PLUGINID_HEVCD_SW) && // sw hevc decoder
            m_bVppIsUsed )
        {
            sts = MFX_ERR_UNSUPPORTED;
            msdk_printf(MSDK_STRING("Error: Combination of (SW HEVC plugin in 10bit mode + HW lib VPP) isn't supported. Use -sw option.\n"));
        }
        if (m_pPlugin.get() && pParams->videoType == CODEC_VP8 && !sts) {
            // force set format to nv12 as the vp8 plugin uses yv12
            m_mfxVideoParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
        }
        if (MFX_ERR_MORE_DATA == sts)
        {
            if (m_mfxBS.MaxLength == m_mfxBS.DataLength)
            {
                sts = ExtendMfxBitstream(&m_mfxBS, m_mfxBS.MaxLength * 2);
                MSDK_CHECK_STATUS(sts, "ExtendMfxBitstream failed");
            }
            // read a portion of data
            totalBytesProcessed += m_mfxBS.DataOffset;
            sts = m_FileReader->ReadNextFrame(&m_mfxBS);
            if (MFX_ERR_MORE_DATA == sts &&
                !(m_mfxBS.DataFlag & MFX_BITSTREAM_EOS))
            {
                m_mfxBS.DataFlag |= MFX_BITSTREAM_EOS;
                sts = MFX_ERR_NONE;
            }
            MSDK_CHECK_STATUS(sts, "m_FileReader->ReadNextFrame failed");

            continue;
        }
        else
        {
            // Enter MVC mode
            if (m_bIsMVC)
            {
                // Check for attached external parameters - if we have them already,
                // we don't need to attach them again
                if (NULL != m_mfxVideoParams.ExtParam)
                    break;

                // allocate and attach external parameters for MVC decoder
                sts = AllocateExtBuffer<mfxExtMVCSeqDesc>();
                MSDK_CHECK_STATUS(sts, "AllocateExtBuffer<mfxExtMVCSeqDesc> failed");

                AttachExtParam();
                sts = m_pmfxDEC->DecodeHeader(&m_mfxBS, &m_mfxVideoParams);

                if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
                {
                    sts = AllocateExtMVCBuffers();
                    SetExtBuffersFlag();

                    MSDK_CHECK_STATUS(sts, "AllocateExtMVCBuffers failed");
                    MSDK_CHECK_POINTER(m_mfxVideoParams.ExtParam, MFX_ERR_MEMORY_ALLOC);
                    continue;
                }
            }

            // if input is interlaced JPEG stream
            if ( m_mfxBS.PicStruct == MFX_PICSTRUCT_FIELD_TFF || m_mfxBS.PicStruct == MFX_PICSTRUCT_FIELD_BFF)
            {
                m_mfxVideoParams.mfx.FrameInfo.CropH *= 2;
                m_mfxVideoParams.mfx.FrameInfo.Height = MSDK_ALIGN16(m_mfxVideoParams.mfx.FrameInfo.CropH);
                m_mfxVideoParams.mfx.FrameInfo.PicStruct = m_mfxBS.PicStruct;
            }

            switch(pParams->nRotation)
            {
            case 0:
                m_mfxVideoParams.mfx.Rotation = MFX_ROTATION_0;
                break;
            case 90:
                m_mfxVideoParams.mfx.Rotation = MFX_ROTATION_90;
                break;
            case 180:
                m_mfxVideoParams.mfx.Rotation = MFX_ROTATION_180;
                break;
            case 270:
                m_mfxVideoParams.mfx.Rotation = MFX_ROTATION_270;
                break;
            default:
                return MFX_ERR_UNSUPPORTED;
            }

            break;
        }
    }

    // check DecodeHeader status
    if (MFX_WRN_PARTIAL_ACCELERATION == sts)
    {
        msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    }
    MSDK_CHECK_STATUS(sts, "m_pmfxDEC->DecodeHeader failed");

    if (!m_mfxVideoParams.mfx.FrameInfo.FrameRateExtN || !m_mfxVideoParams.mfx.FrameInfo.FrameRateExtD) {
        msdk_printf(MSDK_STRING("pretending that stream is 30fps one\n"));
        m_mfxVideoParams.mfx.FrameInfo.FrameRateExtN = 30;
        m_mfxVideoParams.mfx.FrameInfo.FrameRateExtD = 1;
    }
    if (!m_mfxVideoParams.mfx.FrameInfo.AspectRatioW || !m_mfxVideoParams.mfx.FrameInfo.AspectRatioH) {
        msdk_printf(MSDK_STRING("pretending that aspect ratio is 1:1\n"));
        m_mfxVideoParams.mfx.FrameInfo.AspectRatioW = 1;
        m_mfxVideoParams.mfx.FrameInfo.AspectRatioH = 1;
    }

    // Videoparams for RGB4 JPEG decoder output
    if ((pParams->fourcc == MFX_FOURCC_RGB4) && (pParams->videoType == MFX_CODEC_JPEG))
    {
        m_mfxVideoParams.mfx.FrameInfo.FourCC = MFX_FOURCC_RGB4;
        m_mfxVideoParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
        if (pParams->chromaType == MFX_JPEG_COLORFORMAT_RGB)
        {
            m_mfxVideoParams.mfx.JPEGColorFormat = pParams->chromaType;
        }
    }

    // Only shifted P010 is supported now
    if (m_mfxVideoParams.mfx.FrameInfo.FourCC == MFX_FOURCC_P010) {
        m_mfxVideoParams.mfx.FrameInfo.Shift = 1;
    }

#if MFX_VERSION >= 1022
    /* Lets make final decision how to use VPP...*/
    if (!m_bVppIsUsed)
    {

        if ( (m_mfxVideoParams.mfx.FrameInfo.CropW != pParams->Width && pParams->Width) ||
            (m_mfxVideoParams.mfx.FrameInfo.CropH != pParams->Height && pParams->Height) )
        {
            /* By default VPP used for resize */
            m_bVppIsUsed = true;
            /* But... lets try to use decoder's post processing */
            if ( ((MODE_DECODER_POSTPROC_AUTO == pParams->nDecoderPostProcessing) ||
                  (MODE_DECODER_POSTPROC_FORCE == pParams->nDecoderPostProcessing)) &&
                 (MFX_CODEC_AVC == m_mfxVideoParams.mfx.CodecId) && /* Only for AVC */
                 (MFX_PICSTRUCT_PROGRESSIVE == m_mfxVideoParams.mfx.FrameInfo.PicStruct)) /* ...And only for progressive!*/
            {   /* it is possible to use decoder's post-processing */
                m_bVppIsUsed = false;
                m_DecoderPostProcessing.In.CropX = 0;
                m_DecoderPostProcessing.In.CropY = 0;
                m_DecoderPostProcessing.In.CropW = m_mfxVideoParams.mfx.FrameInfo.CropW;
                m_DecoderPostProcessing.In.CropH = m_mfxVideoParams.mfx.FrameInfo.CropH;

                m_DecoderPostProcessing.Out.FourCC = m_mfxVideoParams.mfx.FrameInfo.FourCC;
                m_DecoderPostProcessing.Out.ChromaFormat = m_mfxVideoParams.mfx.FrameInfo.ChromaFormat;
                m_DecoderPostProcessing.Out.Width = MSDK_ALIGN16(pParams->Width);
                m_DecoderPostProcessing.Out.Height = MSDK_ALIGN16(pParams->Height);
                m_DecoderPostProcessing.Out.CropX = 0;
                m_DecoderPostProcessing.Out.CropY = 0;
                m_DecoderPostProcessing.Out.CropW = pParams->Width;
                m_DecoderPostProcessing.Out.CropH = pParams->Height;

                m_ExtBuffers.push_back((mfxExtBuffer *)&m_DecoderPostProcessing);
                AttachExtParam();
                msdk_printf(MSDK_STRING("Decoder's post-processing is used for resizing\n") );
            }
            /* POSTPROC_FORCE */
            if (MODE_DECODER_POSTPROC_FORCE == pParams->nDecoderPostProcessing)
            {
               if ((MFX_CODEC_AVC != m_mfxVideoParams.mfx.CodecId) ||
                   (MFX_PICSTRUCT_PROGRESSIVE != m_mfxVideoParams.mfx.FrameInfo.PicStruct))
               {
                   /* it is impossible to use decoder's post-processing */
                   msdk_printf(MSDK_STRING("ERROR: decoder postprocessing (-dec_postproc forced) cannot resize this stream!\n") );
                   return MFX_ERR_UNSUPPORTED;
               }
            }
            if ((m_bVppIsUsed) && (MODE_DECODER_POSTPROC_AUTO == pParams->nDecoderPostProcessing))
                msdk_printf(MSDK_STRING("Decoder post-processing is unsupported for this stream, VPP is used for resizing\n") );
        }
    }
#endif //MFX_VERSION >= 1022

    // If MVC mode we need to detect number of views in stream
    if (m_bIsMVC)
    {
        mfxExtMVCSeqDesc* pSequenceBuffer;
        pSequenceBuffer = (mfxExtMVCSeqDesc*) GetExtBuffer(m_mfxVideoParams.ExtParam, m_mfxVideoParams.NumExtParam, MFX_EXTBUFF_MVC_SEQ_DESC);
        MSDK_CHECK_POINTER(pSequenceBuffer, MFX_ERR_INVALID_VIDEO_PARAM);

        mfxU32 i = 0;
        numViews = 0;
        for (i = 0; i < pSequenceBuffer->NumView; ++i)
        {
            /* Some MVC streams can contain different information about
               number of views and view IDs, e.x. numVews = 2
               and ViewId[0, 1] = 0, 2 instead of ViewId[0, 1] = 0, 1.
               numViews should be equal (max(ViewId[i]) + 1)
               to prevent crashes during output files writing */
            if (pSequenceBuffer->View[i].ViewId >= numViews)
                numViews = pSequenceBuffer->View[i].ViewId + 1;
        }
    }
    else
    {
        numViews = 1;
    }

    // specify memory type
    if (!m_bVppIsUsed)
        m_mfxVideoParams.IOPattern = (mfxU16)(m_memType != SYSTEM_MEMORY ? MFX_IOPATTERN_OUT_VIDEO_MEMORY : MFX_IOPATTERN_OUT_SYSTEM_MEMORY);
    else
        m_mfxVideoParams.IOPattern = (mfxU16)(pParams->bUseHWLib ? MFX_IOPATTERN_OUT_VIDEO_MEMORY : MFX_IOPATTERN_OUT_SYSTEM_MEMORY);

    m_mfxVideoParams.AsyncDepth = pParams->nAsyncDepth;

    return MFX_ERR_NONE;
}

mfxStatus CDecodingPipeline::AllocAndInitVppFilters()
{
    m_VppDoNotUse.NumAlg = 4;

    /* In case of Reset() this code called twice!
     * But required to have only one allocation to prevent memleaks
     * Deallocation done in Close() */
    if (NULL == m_VppDoNotUse.AlgList)
        m_VppDoNotUse.AlgList = new mfxU32 [m_VppDoNotUse.NumAlg];

    if (!m_VppDoNotUse.AlgList) return MFX_ERR_NULL_PTR;

    m_VppDoNotUse.AlgList[0] = MFX_EXTBUFF_VPP_DENOISE; // turn off denoising (on by default)
    m_VppDoNotUse.AlgList[1] = MFX_EXTBUFF_VPP_SCENE_ANALYSIS; // turn off scene analysis (on by default)
    m_VppDoNotUse.AlgList[2] = MFX_EXTBUFF_VPP_DETAIL; // turn off detail enhancement (on by default)
    m_VppDoNotUse.AlgList[3] = MFX_EXTBUFF_VPP_PROCAMP; // turn off processing amplified (on by default)

    if (m_diMode)
    {
        m_VppDeinterlacing.Mode = m_diMode;
    }

    return MFX_ERR_NONE;
}

mfxStatus CDecodingPipeline::InitVppParams()
{
    m_mfxVppVideoParams.IOPattern = (mfxU16)( m_bDecOutSysmem ?
            MFX_IOPATTERN_IN_SYSTEM_MEMORY
            : MFX_IOPATTERN_IN_VIDEO_MEMORY );

    m_mfxVppVideoParams.IOPattern |= (m_memType != SYSTEM_MEMORY) ?
            MFX_IOPATTERN_OUT_VIDEO_MEMORY
            : MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    MSDK_MEMCPY_VAR(m_mfxVppVideoParams.vpp.In, &m_mfxVideoParams.mfx.FrameInfo, sizeof(mfxFrameInfo));
    MSDK_MEMCPY_VAR(m_mfxVppVideoParams.vpp.Out, &m_mfxVppVideoParams.vpp.In, sizeof(mfxFrameInfo));

    if (m_fourcc)
    {
        m_mfxVppVideoParams.vpp.Out.FourCC  = m_fourcc;
    }

    if (m_vppOutWidth && m_vppOutHeight)
    {

        m_mfxVppVideoParams.vpp.Out.CropW = m_vppOutWidth;
        m_mfxVppVideoParams.vpp.Out.Width = MSDK_ALIGN16(m_vppOutWidth);
        m_mfxVppVideoParams.vpp.Out.CropH = m_vppOutHeight;
        m_mfxVppVideoParams.vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE == m_mfxVppVideoParams.vpp.Out.PicStruct)?
                        MSDK_ALIGN16(m_vppOutHeight) : MSDK_ALIGN32(m_vppOutHeight);
    }

    m_mfxVppVideoParams.AsyncDepth = m_mfxVideoParams.AsyncDepth;

    m_VppExtParams.clear();
    AllocAndInitVppFilters();
    m_VppExtParams.push_back((mfxExtBuffer*)&m_VppDoNotUse);
    if (m_diMode)
    {
        m_VppExtParams.push_back((mfxExtBuffer*)&m_VppDeinterlacing);
    }

    m_mfxVppVideoParams.ExtParam = &m_VppExtParams[0];
    m_mfxVppVideoParams.NumExtParam = (mfxU16)m_VppExtParams.size();

    m_VppSurfaceExtParams.clear();
    if (m_bVppFullColorRange)
    {
        //Let MSDK figure out the transfer matrix to use
        m_VppVideoSignalInfo.TransferMatrix = MFX_TRANSFERMATRIX_UNKNOWN;
        m_VppVideoSignalInfo.NominalRange = MFX_NOMINALRANGE_0_255;

        m_VppSurfaceExtParams.push_back((mfxExtBuffer*)&m_VppVideoSignalInfo);
    }

    // P010 video surfaces should be shifted
    if ((m_mfxVppVideoParams.vpp.Out.FourCC == MFX_FOURCC_P010 
#if (MFX_VERSION >= 1027)
        || m_mfxVppVideoParams.vpp.Out.FourCC == MFX_FOURCC_Y210
#endif
        )&& m_memType != SYSTEM_MEMORY)
    {
        m_mfxVppVideoParams.vpp.Out.Shift = 1;
    }

    return MFX_ERR_NONE;
}

mfxStatus CDecodingPipeline::CreateHWDevice()
{
#if D3D_SURFACES_SUPPORT
    mfxStatus sts = MFX_ERR_NONE;

    HWND window = NULL;
    bool render = (m_eWorkMode == MODE_RENDERING);

    if (render) {
        window = (D3D11_MEMORY == m_memType) ? NULL : m_d3dRender.GetWindowHandle();
    }

#if MFX_D3D11_SUPPORT
    if (D3D11_MEMORY == m_memType)
        m_hwdev = new CD3D11Device();
    else
#endif // #if MFX_D3D11_SUPPORT
        m_hwdev = new CD3D9Device();

    if (NULL == m_hwdev)
        return MFX_ERR_MEMORY_ALLOC;

    sts = m_hwdev->Init(
        window,
        render ? (m_bIsMVC ? 2 : 1) : 0,
        MSDKAdapter::GetNumber(m_mfxSession));
    MSDK_CHECK_STATUS(sts, "m_hwdev->Init failed");

    if (render)
        m_d3dRender.SetHWDevice(m_hwdev);
#elif LIBVA_SUPPORT
    mfxStatus sts = MFX_ERR_NONE;
    m_hwdev = CreateVAAPIDevice(m_libvaBackend);

    if (NULL == m_hwdev) {
        return MFX_ERR_MEMORY_ALLOC;
    }

    sts = m_hwdev->Init(&m_monitorType, (m_eWorkMode == MODE_RENDERING) ? 1 : 0, MSDKAdapter::GetNumber(m_mfxSession));
    MSDK_CHECK_STATUS(sts, "m_hwdev->Init failed");

#if defined(LIBVA_WAYLAND_SUPPORT)
    if (m_eWorkMode == MODE_RENDERING && m_libvaBackend == MFX_LIBVA_WAYLAND) {
        mfxHDL hdl = NULL;
        mfxHandleType hdlw_t = (mfxHandleType)HANDLE_WAYLAND_DRIVER;
        Wayland *wld;
        sts = m_hwdev->GetHandle(hdlw_t, &hdl);
        MSDK_CHECK_STATUS(sts, "m_hwdev->GetHandle failed");
        wld = (Wayland*)hdl;
        wld->SetRenderWinPos(m_nRenderWinX, m_nRenderWinY);
        wld->SetPerfMode(m_bPerfMode);
    }
#endif //LIBVA_WAYLAND_SUPPORT

#endif
    return MFX_ERR_NONE;
}

mfxStatus CDecodingPipeline::ResetDevice()
{
    return m_hwdev->Reset();
}

mfxStatus CDecodingPipeline::AllocFrames()
{
    MSDK_CHECK_POINTER(m_pmfxDEC, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameAllocRequest Request;
    mfxFrameAllocRequest VppRequest[2];

    mfxU16 nSurfNum = 0; // number of surfaces for decoder
    mfxU16 nVppSurfNum = 0; // number of surfaces for vpp

    MSDK_ZERO_MEMORY(Request);

    MSDK_ZERO_MEMORY(VppRequest[0]);
    MSDK_ZERO_MEMORY(VppRequest[1]);

    sts = m_pmfxDEC->Query(&m_mfxVideoParams, &m_mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    MSDK_CHECK_STATUS(sts, "m_pmfxDEC->Query failed");

    // Workaround for VP9 codec
    if ((m_mfxVideoParams.mfx.FrameInfo.FourCC == MFX_FOURCC_P010 
#if (MFX_VERSION >= 1027)
        || m_mfxVideoParams.mfx.FrameInfo.FourCC == MFX_FOURCC_Y210
#endif
        ) && m_mfxVideoParams.mfx.CodecId==MFX_CODEC_VP9)
    {
        m_mfxVideoParams.mfx.FrameInfo.Shift = 1;
    }

    // calculate number of surfaces required for decoder
    sts = m_pmfxDEC->QueryIOSurf(&m_mfxVideoParams, &Request);
    if (MFX_WRN_PARTIAL_ACCELERATION == sts)
    {
        msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        m_bDecOutSysmem = true;
    }
    MSDK_CHECK_STATUS(sts, "m_pmfxDEC->QueryIOSurf failed");

    if (m_nMaxFps)
    {
        // Add surfaces for rendering smoothness
        Request.NumFrameSuggested += m_nMaxFps / 3;
    }

    if (m_bVppIsUsed)
    {
        // respecify memory type between Decoder and VPP
        m_mfxVideoParams.IOPattern = (mfxU16)( m_bDecOutSysmem ?
                MFX_IOPATTERN_OUT_SYSTEM_MEMORY:
                MFX_IOPATTERN_OUT_VIDEO_MEMORY);

        // recalculate number of surfaces required for decoder
        sts = m_pmfxDEC->QueryIOSurf(&m_mfxVideoParams, &Request);
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        MSDK_CHECK_STATUS(sts, "m_pmfxDEC->QueryIOSurf failed");


        sts = InitVppParams();
        MSDK_CHECK_STATUS(sts, "InitVppParams failed");

        sts = m_pmfxVPP->Query(&m_mfxVppVideoParams, &m_mfxVppVideoParams);
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Query failed");

        // VppRequest[0] for input frames request, VppRequest[1] for output frames request
        sts = m_pmfxVPP->QueryIOSurf(&m_mfxVppVideoParams, VppRequest);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts) {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->QueryIOSurf failed");

        if ((VppRequest[0].NumFrameSuggested < m_mfxVppVideoParams.AsyncDepth) ||
            (VppRequest[1].NumFrameSuggested < m_mfxVppVideoParams.AsyncDepth))
            return MFX_ERR_MEMORY_ALLOC;


        // If surfaces are shared by 2 components, c1 and c2. NumSurf = c1_out + c2_in - AsyncDepth + 1
        // The number of surfaces shared by vpp input and decode output
        nSurfNum = Request.NumFrameSuggested + VppRequest[0].NumFrameSuggested - m_mfxVideoParams.AsyncDepth + 1;

        // The number of surfaces for vpp output
        nVppSurfNum = VppRequest[1].NumFrameSuggested;

        // prepare allocation request
        Request.NumFrameSuggested = Request.NumFrameMin = nSurfNum;

        // surfaces are shared between vpp input and decode output
        Request.Type = MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_FROM_DECODE | MFX_MEMTYPE_FROM_VPPIN;
    }

    if ((Request.NumFrameSuggested < m_mfxVideoParams.AsyncDepth) &&
        (m_impl & MFX_IMPL_HARDWARE_ANY))
        return MFX_ERR_MEMORY_ALLOC;

    Request.Type |= (m_bDecOutSysmem) ?
        MFX_MEMTYPE_SYSTEM_MEMORY
        : MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;

#ifdef LIBVA_SUPPORT
    if (!m_bVppIsUsed &&
        (m_export_mode != vaapiAllocatorParams::DONOT_EXPORT))
    {
        Request.Type |= MFX_MEMTYPE_EXPORT_FRAME;
    }
#endif

    // alloc frames for decoder
    sts = m_pGeneralAllocator->Alloc(m_pGeneralAllocator->pthis, &Request, &m_mfxResponse);
    MSDK_CHECK_STATUS(sts, "m_pGeneralAllocator->Alloc failed");

    if (m_bVppIsUsed)
    {
        // alloc frames for VPP
#ifdef LIBVA_SUPPORT
        if (m_export_mode != vaapiAllocatorParams::DONOT_EXPORT)
        {
            VppRequest[1].Type |= MFX_MEMTYPE_EXPORT_FRAME;
        }
#endif
        VppRequest[1].NumFrameSuggested = VppRequest[1].NumFrameMin = nVppSurfNum;
        MSDK_MEMCPY_VAR(VppRequest[1].Info, &(m_mfxVppVideoParams.vpp.Out), sizeof(mfxFrameInfo));

        sts = m_pGeneralAllocator->Alloc(m_pGeneralAllocator->pthis, &VppRequest[1], &m_mfxVppResponse);
        MSDK_CHECK_STATUS(sts, "m_pGeneralAllocator->Alloc failed");

        // prepare mfxFrameSurface1 array for decoder
        nVppSurfNum = m_mfxVppResponse.NumFrameActual;

        // AllocVppBuffers should call before AllocBuffers to set the value of m_OutputSurfacesNumber
        sts = AllocVppBuffers(nVppSurfNum);
        MSDK_CHECK_STATUS(sts, "AllocVppBuffers failed");
    }

    // prepare mfxFrameSurface1 array for decoder
    nSurfNum = m_mfxResponse.NumFrameActual;

    sts = AllocBuffers(nSurfNum);
    MSDK_CHECK_STATUS(sts, "AllocBuffers failed");

    for (int i = 0; i < nSurfNum; i++)
    {
        // initating each frame:
        MSDK_MEMCPY_VAR(m_pSurfaces[i].frame.Info, &(Request.Info), sizeof(mfxFrameInfo));
        if (m_bExternalAlloc) {
            m_pSurfaces[i].frame.Data.MemId = m_mfxResponse.mids[i];
            if (m_bVppFullColorRange)
            {
                m_pSurfaces[i].frame.Data.ExtParam = &m_VppSurfaceExtParams[0];
                m_pSurfaces[i].frame.Data.NumExtParam = (mfxU16)m_VppSurfaceExtParams.size();
            }
        }
        else {
            sts = m_pGeneralAllocator->Lock(m_pGeneralAllocator->pthis, m_mfxResponse.mids[i], &(m_pSurfaces[i].frame.Data));
            MSDK_CHECK_STATUS(sts, "m_pGeneralAllocator->Lock failed");
        }
    }

    // prepare mfxFrameSurface1 array for VPP
    for (int i = 0; i < nVppSurfNum; i++) {
        MSDK_MEMCPY_VAR(m_pVppSurfaces[i].frame.Info, &(VppRequest[1].Info), sizeof(mfxFrameInfo));
        if (m_bExternalAlloc) {
            m_pVppSurfaces[i].frame.Data.MemId = m_mfxVppResponse.mids[i];
            if (m_bVppFullColorRange)
            {
                m_pVppSurfaces[i].frame.Data.ExtParam = &m_VppSurfaceExtParams[0];
                m_pVppSurfaces[i].frame.Data.NumExtParam = (mfxU16)m_VppSurfaceExtParams.size();
            }
        }
        else {
            sts = m_pGeneralAllocator->Lock(m_pGeneralAllocator->pthis, m_mfxVppResponse.mids[i], &(m_pVppSurfaces[i].frame.Data));
            if (MFX_ERR_NONE != sts) {
                return sts;
            }
        }
    }
    return MFX_ERR_NONE;
}

mfxStatus CDecodingPipeline::CreateAllocator()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pGeneralAllocator = new GeneralAllocator();
    if (m_memType != SYSTEM_MEMORY || !m_bDecOutSysmem)
    {
#if D3D_SURFACES_SUPPORT
        sts = CreateHWDevice();
        MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");

        // provide device manager to MediaSDK
        mfxHDL hdl = NULL;
        mfxHandleType hdl_t =
#if MFX_D3D11_SUPPORT
            D3D11_MEMORY == m_memType ? MFX_HANDLE_D3D11_DEVICE :
#endif // #if MFX_D3D11_SUPPORT
            MFX_HANDLE_D3D9_DEVICE_MANAGER;

        sts = m_hwdev->GetHandle(hdl_t, &hdl);
        MSDK_CHECK_STATUS(sts, "m_hwdev->GetHandle failed");
        sts = m_mfxSession.SetHandle(hdl_t, hdl);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");

        // create D3D allocator
#if MFX_D3D11_SUPPORT
        if (D3D11_MEMORY == m_memType)
        {
            D3D11AllocatorParams *pd3dAllocParams = new D3D11AllocatorParams;
            MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
            pd3dAllocParams->pDevice = reinterpret_cast<ID3D11Device *>(hdl);

            m_pmfxAllocatorParams = pd3dAllocParams;
        }
        else
#endif // #if MFX_D3D11_SUPPORT
        {
            D3DAllocatorParams *pd3dAllocParams = new D3DAllocatorParams;
            MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
            pd3dAllocParams->pManager = reinterpret_cast<IDirect3DDeviceManager9 *>(hdl);

            m_pmfxAllocatorParams = pd3dAllocParams;
        }

        /* In case of video memory we must provide MediaSDK with external allocator
        thus we demonstrate "external allocator" usage model.
        Call SetAllocator to pass allocator to mediasdk */
        sts = m_mfxSession.SetFrameAllocator(m_pGeneralAllocator);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetFrameAllocator failed");

        m_bExternalAlloc = true;
#elif LIBVA_SUPPORT
        sts = CreateHWDevice();
        MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");
        /* It's possible to skip failed result here and switch to SW implementation,
           but we don't process this way */

        // provide device manager to MediaSDK
        VADisplay va_dpy = NULL;
        sts = m_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, (mfxHDL *)&va_dpy);
        MSDK_CHECK_STATUS(sts, "m_hwdev->GetHandle failed");
        sts = m_mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, va_dpy);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");

        vaapiAllocatorParams *p_vaapiAllocParams = new vaapiAllocatorParams;
        MSDK_CHECK_POINTER(p_vaapiAllocParams, MFX_ERR_MEMORY_ALLOC);

        p_vaapiAllocParams->m_dpy = va_dpy;
        if (m_eWorkMode == MODE_RENDERING)
        {
            if (m_libvaBackend == MFX_LIBVA_DRM_MODESET)
            {
#if defined(LIBVA_DRM_SUPPORT)
                CVAAPIDeviceDRM* drmdev = dynamic_cast<CVAAPIDeviceDRM*>(m_hwdev);
                p_vaapiAllocParams->m_export_mode = vaapiAllocatorParams::CUSTOM_FLINK;
                p_vaapiAllocParams->m_exporter = dynamic_cast<vaapiAllocatorParams::Exporter*>(drmdev->getRenderer());
#endif
            }
            else if (m_libvaBackend == MFX_LIBVA_WAYLAND || m_libvaBackend == MFX_LIBVA_X11)
            {
                p_vaapiAllocParams->m_export_mode = vaapiAllocatorParams::PRIME;
            }
        }
        m_export_mode = p_vaapiAllocParams->m_export_mode;
        m_pmfxAllocatorParams = p_vaapiAllocParams;

        /* In case of video memory we must provide MediaSDK with external allocator
        thus we demonstrate "external allocator" usage model.
        Call SetAllocator to pass allocator to mediasdk */
        sts = m_mfxSession.SetFrameAllocator(m_pGeneralAllocator);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetFrameAllocator failed");

        m_bExternalAlloc = true;
#endif
    }
    else
    {
#ifdef LIBVA_SUPPORT
        //in case of system memory allocator we also have to pass MFX_HANDLE_VA_DISPLAY to HW library

        if(MFX_IMPL_HARDWARE == MFX_IMPL_BASETYPE(m_impl))
        {
            sts = CreateHWDevice();
            MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");

            // provide device manager to MediaSDK
            VADisplay va_dpy = NULL;
            sts = m_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, (mfxHDL *)&va_dpy);
            MSDK_CHECK_STATUS(sts, "m_hwdev->GetHandle failed");
            sts = m_mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, va_dpy);
            MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");
        }
#endif
        // create system memory allocator
        //m_pGeneralAllocator = new SysMemFrameAllocator;
        //MSDK_CHECK_POINTER(m_pGeneralAllocator, MFX_ERR_MEMORY_ALLOC);

        /* In case of system memory we demonstrate "no external allocator" usage model.
        We don't call SetAllocator, MediaSDK uses internal allocator.
        We use system memory allocator simply as a memory manager for application*/
    }

    // initialize memory allocator
    sts = m_pGeneralAllocator->Init(m_pmfxAllocatorParams);
    MSDK_CHECK_STATUS(sts, "m_pGeneralAllocator->Init failed");

    return MFX_ERR_NONE;
}

void CDecodingPipeline::DeleteFrames()
{
    FreeBuffers();

    m_pCurrentFreeSurface = NULL;
    MSDK_SAFE_FREE(m_pCurrentFreeOutputSurface);

    m_pCurrentFreeVppSurface = NULL;

    // delete frames
    if (m_pGeneralAllocator)
    {
        m_pGeneralAllocator->Free(m_pGeneralAllocator->pthis, &m_mfxResponse);
    }

    return;
}

void CDecodingPipeline::DeleteAllocator()
{
    // delete allocator
    MSDK_SAFE_DELETE(m_pGeneralAllocator);
    MSDK_SAFE_DELETE(m_pmfxAllocatorParams);
    MSDK_SAFE_DELETE(m_hwdev);
}

void CDecodingPipeline::SetMultiView()
{
    m_FileWriter.SetMultiView();
    m_bIsMVC = true;
}

// function for allocating a specific external buffer
template <typename Buffer>
mfxStatus CDecodingPipeline::AllocateExtBuffer()
{
    std::unique_ptr<Buffer> pExtBuffer (new Buffer());
    if (!pExtBuffer.get())
        return MFX_ERR_MEMORY_ALLOC;

    init_ext_buffer(*pExtBuffer);

    m_ExtBuffers.push_back(reinterpret_cast<mfxExtBuffer*>(pExtBuffer.release()));

    return MFX_ERR_NONE;
}

void CDecodingPipeline::AttachExtParam()
{
    m_mfxVideoParams.ExtParam = reinterpret_cast<mfxExtBuffer**>(&m_ExtBuffers[0]);
    m_mfxVideoParams.NumExtParam = static_cast<mfxU16>(m_ExtBuffers.size());
}

void CDecodingPipeline::DeleteExtBuffers()
{
    for (std::vector<mfxExtBuffer *>::iterator it = m_ExtBuffers.begin(); it != m_ExtBuffers.end(); ++it)
        delete *it;
    m_ExtBuffers.clear();
}

mfxStatus CDecodingPipeline::AllocateExtMVCBuffers()
{
    mfxU32 i;

    mfxExtMVCSeqDesc* pExtMVCBuffer = (mfxExtMVCSeqDesc*) m_mfxVideoParams.ExtParam[0];
    MSDK_CHECK_POINTER(pExtMVCBuffer, MFX_ERR_MEMORY_ALLOC);

    pExtMVCBuffer->View = new mfxMVCViewDependency[pExtMVCBuffer->NumView];
    MSDK_CHECK_POINTER(pExtMVCBuffer->View, MFX_ERR_MEMORY_ALLOC);
    for (i = 0; i < pExtMVCBuffer->NumView; ++i)
    {
        MSDK_ZERO_MEMORY(pExtMVCBuffer->View[i]);
    }
    pExtMVCBuffer->NumViewAlloc = pExtMVCBuffer->NumView;

    pExtMVCBuffer->ViewId = new mfxU16[pExtMVCBuffer->NumViewId];
    MSDK_CHECK_POINTER(pExtMVCBuffer->ViewId, MFX_ERR_MEMORY_ALLOC);
    for (i = 0; i < pExtMVCBuffer->NumViewId; ++i)
    {
        MSDK_ZERO_MEMORY(pExtMVCBuffer->ViewId[i]);
    }
    pExtMVCBuffer->NumViewIdAlloc = pExtMVCBuffer->NumViewId;

    pExtMVCBuffer->OP = new mfxMVCOperationPoint[pExtMVCBuffer->NumOP];
    MSDK_CHECK_POINTER(pExtMVCBuffer->OP, MFX_ERR_MEMORY_ALLOC);
    for (i = 0; i < pExtMVCBuffer->NumOP; ++i)
    {
        MSDK_ZERO_MEMORY(pExtMVCBuffer->OP[i]);
    }
    pExtMVCBuffer->NumOPAlloc = pExtMVCBuffer->NumOP;

    return MFX_ERR_NONE;
}

void CDecodingPipeline::DeallocateExtMVCBuffers()
{
    mfxExtMVCSeqDesc* pExtMVCBuffer = (mfxExtMVCSeqDesc*) m_mfxVideoParams.ExtParam[0];
    if (pExtMVCBuffer != NULL)
    {
        MSDK_SAFE_DELETE_ARRAY(pExtMVCBuffer->View);
        MSDK_SAFE_DELETE_ARRAY(pExtMVCBuffer->ViewId);
        MSDK_SAFE_DELETE_ARRAY(pExtMVCBuffer->OP);
    }

    MSDK_SAFE_DELETE(m_mfxVideoParams.ExtParam[0]);

    m_bIsExtBuffers = false;
}

mfxStatus CDecodingPipeline::ResetDecoder(sInputParams *pParams)
{
    mfxStatus sts = MFX_ERR_NONE;

    // close decoder
    sts = m_pmfxDEC->Close();
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_STATUS(sts, "m_pmfxDEC->Close failed");

    // close VPP
    if(m_pmfxVPP)
    {
        sts = m_pmfxVPP->Close();
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Close failed");
    }

    // free allocated frames
    DeleteFrames();

    // initialize parameters with values from parsed header
    sts = InitMfxParams(pParams);
    MSDK_CHECK_STATUS(sts, "InitMfxParams failed");

    // in case of HW accelerated decode frames must be allocated prior to decoder initialization
    sts = AllocFrames();
    MSDK_CHECK_STATUS(sts, "AllocFrames failed");

    // init decoder
    sts = m_pmfxDEC->Init(&m_mfxVideoParams);
    if (MFX_WRN_PARTIAL_ACCELERATION == sts)
    {
        msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    }
    MSDK_CHECK_STATUS(sts, "m_pmfxDEC->Init failed");

    if(m_pmfxVPP)
    {
        sts = m_pmfxVPP->Init(&m_mfxVppVideoParams);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->Init failed");
    }

    return MFX_ERR_NONE;
}

mfxStatus CDecodingPipeline::DeliverOutput(mfxFrameSurface1* frame)
{
    CAutoTimer timer_fwrite(m_tick_fwrite);

    mfxStatus res = MFX_ERR_NONE, sts = MFX_ERR_NONE;

    if (!frame) {
        return MFX_ERR_NULL_PTR;
    }


    if (m_bResetFileWriter)
    {
        sts = m_FileWriter.Reset();
        MSDK_CHECK_STATUS(sts, "");
        m_bResetFileWriter = false;
    }

    if (m_bExternalAlloc) {
        if (m_eWorkMode == MODE_FILE_DUMP) {
            res = m_pGeneralAllocator->Lock(m_pGeneralAllocator->pthis, frame->Data.MemId, &(frame->Data));
            if (MFX_ERR_NONE == res) {
                res = m_bOutI420 ? m_FileWriter.WriteNextFrameI420(frame)
                    : m_FileWriter.WriteNextFrame(frame);
                sts = m_pGeneralAllocator->Unlock(m_pGeneralAllocator->pthis, frame->Data.MemId, &(frame->Data));
            }
            if ((MFX_ERR_NONE == res) && (MFX_ERR_NONE != sts)) {
                res = sts;
            }
        } else if (m_eWorkMode == MODE_RENDERING) {
#if D3D_SURFACES_SUPPORT
            res = m_d3dRender.RenderFrame(frame, m_pGeneralAllocator);
#elif LIBVA_SUPPORT
            res = m_hwdev->RenderFrame(frame, m_pGeneralAllocator);
#endif

            while( m_delayTicks && (m_startTick + m_delayTicks > msdk_time_get_tick()) )
            {
                MSDK_SLEEP(0);
            };
            m_startTick=msdk_time_get_tick();
        }
    }
    else {
        res = m_bOutI420 ? m_FileWriter.WriteNextFrameI420(frame)
            : m_FileWriter.WriteNextFrame(frame);
    }

    return res;
}

mfxStatus CDecodingPipeline::DeliverLoop(void)
{
    mfxStatus res = MFX_ERR_NONE;

    while (!m_bStopDeliverLoop) {
        m_pDeliverOutputSemaphore->Wait();
        if (m_bStopDeliverLoop) {
            continue;
        }
        if (MFX_ERR_NONE != m_error) {
            continue;
        }
        msdkOutputSurface* pCurrentDeliveredSurface = m_DeliveredSurfacesPool.GetSurface();
        if (!pCurrentDeliveredSurface) {
            m_error = MFX_ERR_NULL_PTR;
            continue;
        }
        mfxFrameSurface1* frame = &(pCurrentDeliveredSurface->surface->frame);

        m_error = DeliverOutput(frame);
        ReturnSurfaceToBuffers(pCurrentDeliveredSurface);

        pCurrentDeliveredSurface = NULL;
        msdk_atomic_inc32(&m_output_count);
        m_pDeliveredEvent->Signal();
    }
    return res;
}

unsigned int MFX_STDCALL CDecodingPipeline::DeliverThreadFunc(void* ctx)
{
    CDecodingPipeline* pipeline = (CDecodingPipeline*)ctx;

    mfxStatus sts;
    sts = pipeline->DeliverLoop();

    return 0;
}

void CDecodingPipeline::PrintPerFrameStat(bool force)
{
#define MY_COUNT 1 // TODO: this will be cmd option
#define MY_THRESHOLD 10000.0
    if ((!(m_output_count % MY_COUNT) && (m_eWorkMode != MODE_PERFORMANCE)) || force) {
        double fps, fps_fread, fps_fwrite;

        m_timer_overall.Sync();

        fps = (m_tick_overall)? m_output_count/CTimer::ConvertToSeconds(m_tick_overall): 0.0;
        fps_fread = (m_tick_fread)? m_output_count/CTimer::ConvertToSeconds(m_tick_fread): 0.0;
        fps_fwrite = (m_tick_fwrite)? m_output_count/CTimer::ConvertToSeconds(m_tick_fwrite): 0.0;
        // decoding progress
        msdk_printf(MSDK_STRING("Frame number: %4d, fps: %0.3f, fread_fps: %0.3f, fwrite_fps: %.3f\r"),
            m_output_count,
            fps,
            (fps_fread < MY_THRESHOLD)? fps_fread: 0.0,
            (fps_fwrite < MY_THRESHOLD)? fps_fwrite: 0.0);
        fflush(NULL);
#if D3D_SURFACES_SUPPORT
        m_d3dRender.UpdateTitle(fps);
#elif LIBVA_SUPPORT
        if (m_hwdev) m_hwdev->UpdateTitle(fps);
#endif
    }
}

mfxStatus CDecodingPipeline::SyncOutputSurface(mfxU32 wait)
{
    if (!m_pCurrentOutputSurface) {
        m_pCurrentOutputSurface = m_OutputSurfacesPool.GetSurface();
    }
    if (!m_pCurrentOutputSurface) {
        return MFX_ERR_MORE_DATA;
    }

    mfxStatus sts = m_mfxSession.SyncOperation(m_pCurrentOutputSurface->syncp, wait);

    if (MFX_ERR_GPU_HANG == sts) {
        msdk_printf(MSDK_STRING("GPU hang happened\n"));
        // Output surface can be corrupted
        // But should be delivered to output anyway
        sts = MFX_ERR_NONE;
    }

    if (MFX_WRN_IN_EXECUTION == sts) {
        return sts;
    }
    if (MFX_ERR_NONE == sts) {
        // we got completely decoded frame - pushing it to the delivering thread...
        ++m_synced_count;
        if (m_bPrintLatency) {
            m_vLatency.push_back(m_timer_overall.Sync() - m_pCurrentOutputSurface->surface->submit);
        }
        else {
            PrintPerFrameStat();
        }

        if (m_eWorkMode == MODE_PERFORMANCE) {
            m_output_count = m_synced_count;
            ReturnSurfaceToBuffers(m_pCurrentOutputSurface);
        } else if (m_eWorkMode == MODE_FILE_DUMP) {
            sts = DeliverOutput(&(m_pCurrentOutputSurface->surface->frame));
            if (MFX_ERR_NONE != sts) {
                sts = MFX_ERR_UNKNOWN;
            } else {
                m_output_count = m_synced_count;
            }
            ReturnSurfaceToBuffers(m_pCurrentOutputSurface);
        } else if (m_eWorkMode == MODE_RENDERING) {
            m_DeliveredSurfacesPool.AddSurface(m_pCurrentOutputSurface);
            m_pDeliveredEvent->Reset();
            m_pDeliverOutputSemaphore->Post();
        }
        m_pCurrentOutputSurface = NULL;
    }

    return sts;
}

mfxStatus CDecodingPipeline::RunDecoding()
{
    mfxFrameSurface1*   pOutSurface = NULL;
    mfxBitstream*       pBitstream = &m_mfxBS;
    mfxStatus           sts = MFX_ERR_NONE;
    bool                bErrIncompatibleVideoParams = false;
    CTimeInterval<>     decodeTimer(m_bIsCompleteFrame);
    time_t start_time = time(0);
    MSDKThread * pDeliverThread = NULL;
#if (MFX_VERSION >= 1025)
    mfxExtDecodeErrorReport *pDecodeErrorReport = NULL;
#endif

    if (m_eWorkMode == MODE_RENDERING) {
        m_pDeliverOutputSemaphore = new MSDKSemaphore(sts);
        m_pDeliveredEvent = new MSDKEvent(sts, false, false);
        pDeliverThread = new MSDKThread(sts, DeliverThreadFunc, this);
        if (!pDeliverThread || !m_pDeliverOutputSemaphore || !m_pDeliveredEvent) {
            MSDK_SAFE_DELETE(pDeliverThread);
            MSDK_SAFE_DELETE(m_pDeliverOutputSemaphore);
            MSDK_SAFE_DELETE(m_pDeliveredEvent);
            return MFX_ERR_MEMORY_ALLOC;
        }
    }

    while (((sts == MFX_ERR_NONE) || (MFX_ERR_MORE_DATA == sts) || (MFX_ERR_MORE_SURFACE == sts)) && (m_nFrames > m_output_count))
    {
        if (MFX_ERR_NONE != m_error) {
            msdk_printf(MSDK_STRING("DeliverOutput return error = %d\n"),m_error);
            break;
        }

        if (pBitstream && ((MFX_ERR_MORE_DATA == sts) || (m_bIsCompleteFrame && !pBitstream->DataLength))) {
            CAutoTimer timer_fread(m_tick_fread);
            sts = m_FileReader->ReadNextFrame(pBitstream); // read more data to input bit stream

            if (MFX_ERR_MORE_DATA == sts) {
                sts = MFX_ERR_NONE;
                // Timeout has expired or videowall mode
                m_timer_overall.Sync();
                if ( ((CTimer::ConvertToSeconds(m_tick_overall) < m_nTimeout) && m_nTimeout ) || m_bIsVideoWall)
                {
                    m_FileReader->Reset();
                    m_bResetFileWriter = true;
                    continue;
                }

                // we almost reached end of stream, need to pull buffered data now
                pBitstream = NULL;
            }
        }

        if ((MFX_ERR_NONE == sts) || (MFX_ERR_MORE_DATA == sts) || (MFX_ERR_MORE_SURFACE == sts)) {
            // here we check whether output is ready, though we do not wait...
#ifndef __SYNC_WA
            mfxStatus _sts = SyncOutputSurface(0);
            if (MFX_ERR_UNKNOWN == _sts) {
                sts = _sts;
                break;
            } else if (MFX_ERR_NONE == _sts) {
                continue;
            }
#endif
        }
        else
        {
            MSDK_CHECK_STATUS_NO_RET(sts, "ReadNextFrame failed");
        }

        if ((MFX_ERR_NONE == sts) || (MFX_ERR_MORE_DATA == sts) || (MFX_ERR_MORE_SURFACE == sts)) {
            SyncFrameSurfaces();
            SyncVppFrameSurfaces();
            if (!m_pCurrentFreeSurface) {
                m_pCurrentFreeSurface = m_FreeSurfacesPool.GetSurface();
            }
            if (!m_pCurrentFreeVppSurface) {
              m_pCurrentFreeVppSurface = m_FreeVppSurfacesPool.GetSurface();
            }
#ifndef __SYNC_WA
            if (!m_pCurrentFreeSurface || !m_pCurrentFreeVppSurface) {
#else
            if (!m_pCurrentFreeSurface || (!m_pCurrentFreeVppSurface && m_bVppIsUsed) || (m_OutputSurfacesPool.GetSurfaceCount() == m_mfxVideoParams.AsyncDepth)) {
#endif
                // we stuck with no free surface available, now we will sync...
                sts = SyncOutputSurface(MSDK_DEC_WAIT_INTERVAL);
                if (MFX_ERR_MORE_DATA == sts) {
                    if ((m_eWorkMode == MODE_PERFORMANCE) || (m_eWorkMode == MODE_FILE_DUMP)) {
                        sts = MFX_ERR_NOT_FOUND;
                    } else if (m_eWorkMode == MODE_RENDERING) {
                        if (m_synced_count != m_output_count) {
                            sts = m_pDeliveredEvent->TimedWait(MSDK_DEC_WAIT_INTERVAL);
                        } else {
                            sts = MFX_ERR_NOT_FOUND;
                        }
                    }
                    if (MFX_ERR_NOT_FOUND == sts) {
                        msdk_printf(MSDK_STRING("fatal: failed to find output surface, that's a bug!\n"));
                        break;
                    }
                }
                // note: MFX_WRN_IN_EXECUTION will also be treated as an error at this point
                continue;
            }

            if (!m_pCurrentFreeOutputSurface) 
            {
                m_pCurrentFreeOutputSurface = GetFreeOutputSurface();
            }
            if (!m_pCurrentFreeOutputSurface) 
            {
                sts = MFX_ERR_NOT_FOUND;
                break;
            }
        }

        // exit by timeout
        if ((MFX_ERR_NONE == sts) && m_bIsVideoWall && (time(0)-start_time) >= m_nTimeout) {
            sts = MFX_ERR_NONE;
            break;
        }

        if ((MFX_ERR_NONE == sts) || (MFX_ERR_MORE_DATA == sts) || (MFX_ERR_MORE_SURFACE == sts)) {
            if (m_bIsCompleteFrame) 
            {
                m_pCurrentFreeSurface->submit = m_timer_overall.Sync();
            }
            pOutSurface = NULL;
            do {
#if (MFX_VERSION >= 1025)
                if (pBitstream) {
                    pDecodeErrorReport = (mfxExtDecodeErrorReport *)GetExtBuffer(pBitstream->ExtParam, pBitstream->NumExtParam, MFX_EXTBUFF_DECODE_ERROR_REPORT);
                }
#endif
                sts = m_pmfxDEC->DecodeFrameAsync(pBitstream, &(m_pCurrentFreeSurface->frame), &pOutSurface, &(m_pCurrentFreeOutputSurface->syncp));

#if (MFX_VERSION >= 1025)
                PrintDecodeErrorReport(pDecodeErrorReport);
#endif

                if (pBitstream && MFX_ERR_MORE_DATA == sts && pBitstream->MaxLength == pBitstream->DataLength)
                {
                    mfxStatus stsExt = ExtendMfxBitstream(pBitstream, pBitstream->MaxLength * 2);
                    MSDK_CHECK_STATUS_SAFE(stsExt, "ExtendMfxBitstream failed", MSDK_SAFE_DELETE(pDeliverThread));
                }

                if (MFX_WRN_DEVICE_BUSY == sts) {
                    if (m_bIsCompleteFrame) {
                        //in low latency mode device busy leads to increasing of latency
                        //msdk_printf(MSDK_STRING("Warning : latency increased due to MFX_WRN_DEVICE_BUSY\n"));
                    }
                    mfxStatus _sts = SyncOutputSurface(MSDK_DEC_WAIT_INTERVAL);
                    // note: everything except MFX_ERR_NONE are errors at this point
                    if (MFX_ERR_NONE == _sts) {
                        sts = MFX_WRN_DEVICE_BUSY;
                    } else {
                        sts = _sts;
                        if (MFX_ERR_MORE_DATA == sts) {
                            // we can't receive MFX_ERR_MORE_DATA and have no output - that's a bug
                            sts = MFX_WRN_DEVICE_BUSY;//MFX_ERR_NOT_FOUND;
                        }
                    }
                }
            } while (MFX_WRN_DEVICE_BUSY == sts);

            if (sts > MFX_ERR_NONE) {
                // ignoring warnings...
                if (m_pCurrentFreeOutputSurface->syncp) {
                    MSDK_SELF_CHECK(pOutSurface);
                    // output is available
                    sts = MFX_ERR_NONE;
                } else {
                    // output is not available
                    sts = MFX_ERR_MORE_SURFACE;
                }
            } else if ((MFX_ERR_MORE_DATA == sts) && pBitstream) {
                if (m_bIsCompleteFrame && pBitstream->DataLength)
                {
                    // In low_latency mode decoder have to process bitstream completely
                    msdk_printf(MSDK_STRING("error: Incorrect decoder behavior in low latency mode (bitstream length is not equal to 0 after decoding)\n"));
                    sts = MFX_ERR_UNDEFINED_BEHAVIOR;
                    continue;
                }
            } else if ((MFX_ERR_MORE_DATA == sts) && !pBitstream) {
                // that's it - we reached end of stream; now we need to render bufferred data...
                do {
                    sts = SyncOutputSurface(MSDK_DEC_WAIT_INTERVAL);
                } while (MFX_ERR_NONE == sts);

                MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
                if (sts) MSDK_PRINT_WRN_MSG(sts, "SyncOutputSurface failed")

                while (m_synced_count != m_output_count) {
                    m_pDeliveredEvent->Wait();
                }
                break;
            } else if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == sts) {
                bErrIncompatibleVideoParams = true;
                // need to go to the buffering loop prior to reset procedure
                pBitstream = NULL;
                sts = MFX_ERR_NONE;
                continue;
            }
        }

        if ((MFX_ERR_NONE == sts) || (MFX_ERR_MORE_DATA == sts) || (MFX_ERR_MORE_SURFACE == sts)) {
            // if current free surface is locked we are moving it to the used surfaces array
            /*if (m_pCurrentFreeSurface->frame.Data.Locked)*/ {
                m_UsedSurfacesPool.AddSurface(m_pCurrentFreeSurface);
                m_pCurrentFreeSurface = NULL;
            }
        }
        else
        {
            MSDK_CHECK_STATUS_NO_RET(sts, "DecodeFrameAsync returned error status");
        }

        if (MFX_ERR_NONE == sts)
        {
            if (m_bVppIsUsed)
            {
                if(m_pCurrentFreeVppSurface)
                {
                    do
                    {
                        if ((m_pCurrentFreeVppSurface->frame.Info.CropW == 0) ||
                            (m_pCurrentFreeVppSurface->frame.Info.CropH == 0)) {
                                m_pCurrentFreeVppSurface->frame.Info.CropW = pOutSurface->Info.CropW;
                                m_pCurrentFreeVppSurface->frame.Info.CropH = pOutSurface->Info.CropH;
                                m_pCurrentFreeVppSurface->frame.Info.CropX = pOutSurface->Info.CropX;
                                m_pCurrentFreeVppSurface->frame.Info.CropY = pOutSurface->Info.CropY;
                        }
                        if (pOutSurface->Info.PicStruct != m_pCurrentFreeVppSurface->frame.Info.PicStruct) {
                            m_pCurrentFreeVppSurface->frame.Info.PicStruct = pOutSurface->Info.PicStruct;
                        }
                        if ((pOutSurface->Info.PicStruct == 0) && (m_pCurrentFreeVppSurface->frame.Info.PicStruct == 0)) {
                            m_pCurrentFreeVppSurface->frame.Info.PicStruct = pOutSurface->Info.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
                        }

                        if (m_diMode)
                            m_pCurrentFreeVppSurface->frame.Info.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

                        // WA: RunFrameVPPAsync doesn't copy ViewId from input to output
                        m_pCurrentFreeVppSurface->frame.Info.FrameId.ViewId = pOutSurface->Info.FrameId.ViewId;
                        sts = m_pmfxVPP->RunFrameVPPAsync(pOutSurface, &(m_pCurrentFreeVppSurface->frame), NULL, &(m_pCurrentFreeOutputSurface->syncp));

                        if (MFX_WRN_DEVICE_BUSY == sts)
                        {
                            MSDK_SLEEP(1); // just wait and then repeat the same call to RunFrameVPPAsync
                        }
                    } while (MFX_WRN_DEVICE_BUSY == sts);

                    // process errors
                    if (MFX_ERR_MORE_DATA == sts) 
                    { // will never happen actually
                        continue;
                    }
                    else if (MFX_ERR_NONE != sts) 
                    {
                        MSDK_PRINT_RET_MSG(sts, "RunFrameVPPAsync failed");
                        break;
                    }

                    m_UsedVppSurfacesPool.AddSurface(m_pCurrentFreeVppSurface);
                    msdk_atomic_inc16(&(m_pCurrentFreeVppSurface->render_lock));

                    m_pCurrentFreeOutputSurface->surface = m_pCurrentFreeVppSurface;
                    m_OutputSurfacesPool.AddSurface(m_pCurrentFreeOutputSurface);

                    m_pCurrentFreeOutputSurface = NULL;
                    m_pCurrentFreeVppSurface = NULL;
                }
            }
            else
            {
                msdkFrameSurface* surface = FindUsedSurface(pOutSurface);

                msdk_atomic_inc16(&(surface->render_lock));

                m_pCurrentFreeOutputSurface->surface = surface;
                m_OutputSurfacesPool.AddSurface(m_pCurrentFreeOutputSurface);
                m_pCurrentFreeOutputSurface = NULL;
            }
        }
    } //while processing

    if (m_nFrames == m_output_count)
    {
        if (!sts)
            msdk_printf(MSDK_STRING("[WARNING] Decoder returned error %s that could be compensated during next iterations of decoding process.\
                                    But requested amount of frames is already successfully decoded, so whole process is finished successfully."),
                        StatusToString(sts).c_str());
        sts = MFX_ERR_NONE;
    }

    PrintPerFrameStat(true);

    if (m_bPrintLatency && m_vLatency.size() > 0) {
        unsigned int frame_idx = 0;
        msdk_tick sum = 0;
        for (std::vector<msdk_tick>::iterator it = m_vLatency.begin(); it != m_vLatency.end(); ++it)
        {
            sum += *it;
            msdk_printf(MSDK_STRING("Frame %4d, latency=%5.5f ms\n"), ++frame_idx, CTimer::ConvertToSeconds(*it)*1000);
        }
        msdk_printf(MSDK_STRING("\nLatency summary:\n"));
        msdk_printf(MSDK_STRING("\nAVG=%5.5f ms, MAX=%5.5f ms, MIN=%5.5f ms"),
            CTimer::ConvertToSeconds((msdk_tick)((mfxF64)sum/m_vLatency.size()))*1000,
            CTimer::ConvertToSeconds(*std::max_element(m_vLatency.begin(), m_vLatency.end()))*1000,
            CTimer::ConvertToSeconds(*std::min_element(m_vLatency.begin(), m_vLatency.end()))*1000);
    }

    if (m_eWorkMode == MODE_RENDERING) {
        m_bStopDeliverLoop = true;
        m_pDeliverOutputSemaphore->Post();
        if (pDeliverThread)
            pDeliverThread->Wait();
    }

    MSDK_SAFE_DELETE(m_pDeliverOutputSemaphore);
    MSDK_SAFE_DELETE(m_pDeliveredEvent);
    MSDK_SAFE_DELETE(pDeliverThread);

    // exit in case of other errors
    MSDK_CHECK_STATUS(sts, "Unexpected error!!");

    // if we exited main decoding loop with ERR_INCOMPATIBLE_PARAM we need to send this status to caller
    if (bErrIncompatibleVideoParams) {
        sts = MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;
    }

    return sts; // ERR_NONE or ERR_INCOMPATIBLE_VIDEO_PARAM
}

void CDecodingPipeline::PrintInfo()
{
    msdk_printf(MSDK_STRING("Decoding Sample Version %s\n\n"), GetMSDKSampleVersion().c_str());
    msdk_printf(MSDK_STRING("\nInput video\t%s\n"), CodecIdToStr(m_mfxVideoParams.mfx.CodecId).c_str());
    if (m_bVppIsUsed)
    {
        msdk_printf(MSDK_STRING("Output format\t%s (using vpp)\n"), m_bOutI420 ? "I420(YUV)" : CodecIdToStr(m_mfxVppVideoParams.vpp.Out.FourCC).c_str());
    }
    else
    {
        msdk_printf(MSDK_STRING("Output format\t%s\n"), m_bOutI420 ? "I420(YUV)" : CodecIdToStr(m_mfxVideoParams.mfx.FrameInfo.FourCC).c_str());
    }

    mfxFrameInfo Info = m_mfxVideoParams.mfx.FrameInfo;
    msdk_printf(MSDK_STRING("Input:\n"));
    msdk_printf(MSDK_STRING("  Resolution\t%dx%d\n"), Info.Width, Info.Height);
    msdk_printf(MSDK_STRING("  Crop X,Y,W,H\t%d,%d,%d,%d\n"), Info.CropX, Info.CropY, Info.CropW, Info.CropH);
    msdk_printf(MSDK_STRING("Output:\n"));
    if (m_vppOutHeight && m_vppOutWidth)
    {
        msdk_printf(MSDK_STRING("  Resolution\t%dx%d\n"), m_vppOutWidth, m_vppOutHeight);
    }
    else
    {
        msdk_printf(MSDK_STRING("  Resolution\t%dx%d\n"),
                    Info.CropW ? Info.CropW : Info.Width,
                    Info.CropH ? Info.CropH : Info.Height);
    }

    mfxF64 dFrameRate = CalculateFrameRate(Info.FrameRateExtN, Info.FrameRateExtD);
    msdk_printf(MSDK_STRING("Frame rate\t%.2f\n"), dFrameRate);

    const msdk_char* sMemType =
        m_memType == D3D9_MEMORY  ? MSDK_STRING("vaapi")
        : (m_memType == D3D11_MEMORY ? MSDK_STRING("d3d11")
        : MSDK_STRING("system"));
    msdk_printf(MSDK_STRING("Memory type\t\t%s\n"), sMemType);


    const msdk_char* sImpl = (MFX_IMPL_VIA_D3D11 == MFX_IMPL_VIA_MASK(m_impl)) ? MSDK_STRING("hw_d3d11")
                           : (MFX_IMPL_SOFTWARE == MFX_IMPL_BASETYPE(m_impl))  ? MSDK_STRING("sw")
                                                         : MSDK_STRING("hw");
    msdk_printf(MSDK_STRING("MediaSDK impl\t\t%s\n"), sImpl);

    mfxVersion ver;
    m_mfxSession.QueryVersion(&ver);
    msdk_printf(MSDK_STRING("MediaSDK version\t%d.%d\n"), ver.Major, ver.Minor);

    msdk_printf(MSDK_STRING("\n"));

    return;
}
