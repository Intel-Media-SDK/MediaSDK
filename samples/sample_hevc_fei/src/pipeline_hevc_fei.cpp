/******************************************************************************\
Copyright (c) 2017, Intel Corporation
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

#include "pipeline_hevc_fei.h"
#include <version.h>

CEncodingPipeline::CEncodingPipeline(sInputParams& userInput)
    : m_inParams(userInput)
    , m_impl(MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_VAAPI)
    , m_EncSurfPool()
    , m_processedFrames(0)
{
}

CEncodingPipeline::~CEncodingPipeline()
{
    Close();
}

mfxStatus CEncodingPipeline::Init()
{
    mfxStatus sts = MFX_ERR_NONE;

    try
    {
        sts = m_mfxSession.Init(m_impl, NULL);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.Init failed");

        // create and init frame allocator
        sts = CreateAllocator();
        MSDK_CHECK_STATUS(sts, "CreateAllocator failed");

        m_EncSurfPool.SetAllocator(m_pMFXAllocator.get());

        {
            mfxHDL hdl = NULL;
            sts = m_pHWdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
            MSDK_CHECK_STATUS(sts, "m_pHWdev->GetHandle failed");
            m_pParamChecker.reset(new HEVCEncodeParamsChecker(m_impl, hdl));
        }

        mfxFrameInfo frameInfo;

        m_pYUVSource.reset(CreateYUVSource());
        MSDK_CHECK_POINTER(m_pYUVSource.get(), MFX_ERR_UNSUPPORTED);

        {
            sts = m_pYUVSource->PreInit();
            MSDK_CHECK_STATUS(sts, "m_pYUVSource PreInit failed");

            sts = m_pYUVSource->GetActualFrameInfo(frameInfo);
            MSDK_CHECK_STATUS(sts, "m_pYUVSource GetActualFrameInfo failed");
        }

        m_pFEI_PreENC.reset(CreatePreENC(frameInfo));
        if (m_pFEI_PreENC.get())
        {
            sts = m_pFEI_PreENC->PreInit();
            MSDK_CHECK_STATUS(sts, "FEI PreENC PreInit failed");
        }

        if (m_inParams.bEncodedOrder)
        {
            MfxVideoParamsWrapper param = GetEncodeParams(m_inParams, frameInfo);

            sts = m_pParamChecker->Query(param);
            MSDK_CHECK_STATUS(sts, "m_pParamChecker->Query failed");

            m_pOrderCtrl.reset(new EncodeOrderControl(param, !!(m_pFEI_PreENC.get())));
        }

        m_pFEI_Encode.reset(CreateEncode(frameInfo));
        if (m_pFEI_Encode.get())
        {
            sts = m_pFEI_Encode->PreInit();
            MSDK_CHECK_STATUS(sts, "FEI ENCODE PreInit failed");
        }

        sts = AllocFrames();
        MSDK_CHECK_STATUS(sts, "AllocFrames failed");

        sts = InitComponents();
        MSDK_CHECK_STATUS(sts, "InitComponents failed");
    }
    catch (mfxError& ex)
    {
        MSDK_CHECK_STATUS(ex.GetStatus(), ex.GetMessage());
    }
    catch(std::exception& ex)
    {
        MSDK_CHECK_STATUS(MFX_ERR_UNDEFINED_BEHAVIOR, ex.what());
    }
    catch(...)
    {
        MSDK_CHECK_STATUS(MFX_ERR_UNDEFINED_BEHAVIOR, "Undefined exception in Pipeline::Init");
    }

    return sts;
}

void CEncodingPipeline::Close()
{
    msdk_printf(MSDK_STRING("\nFrames processed: %u\n"), m_processedFrames);
}

void CEncodingPipeline::PrintInfo()
{
    msdk_printf(MSDK_STRING("\nIntel(R) Media SDK HEVC FEI Encoding Sample Version %s\n"), GetMSDKSampleVersion().c_str());

    const msdk_char* sPipeline = m_inParams.bPREENC && m_inParams.bENCODE ? MSDK_STRING("PreENC + Encode") : (m_inParams.bPREENC ? MSDK_STRING("PreENC") : MSDK_STRING("Encode"));
    msdk_printf(MSDK_STRING("\nPipeline         :\t%s"), sPipeline);

    mfxFrameInfo sourceFrameInfo;
    m_pYUVSource->GetActualFrameInfo(sourceFrameInfo);

    msdk_printf(MSDK_STRING("\nResolution       :\t%dx%d"), sourceFrameInfo.Width, sourceFrameInfo.Height);
    msdk_printf(MSDK_STRING("\nFrame rate       :\t%d/%d = %.2f"), sourceFrameInfo.FrameRateExtN, sourceFrameInfo.FrameRateExtD, CalculateFrameRate(sourceFrameInfo.FrameRateExtN, sourceFrameInfo.FrameRateExtD));

    MfxVideoParamsWrapper param;
    if (m_pFEI_Encode.get())
        param = m_pFEI_Encode->GetVideoParam();
    else if (m_pFEI_PreENC.get())
        param = m_pFEI_PreENC->GetVideoParam();

    msdk_printf(MSDK_STRING("\nProcessing order :\t%s"), param.mfx.EncodedOrder ? MSDK_STRING("Encoded") : MSDK_STRING("Display"));
    msdk_printf(MSDK_STRING("\nGop Size         :\t%d"), param.mfx.GopPicSize);

    mfxU16 gopOptFlag = param.mfx.GopOptFlag;
    const msdk_char* GopClosed = (gopOptFlag & MFX_GOP_CLOSED) == 0 ? MSDK_STRING("Open") : MSDK_STRING("Closed");
    const msdk_char* GopStrict = (gopOptFlag & MFX_GOP_STRICT) == 0 ? MSDK_STRING("Not Strict") : MSDK_STRING("Strict");
    msdk_printf(MSDK_STRING("\nGopOptFlag       :\t%s, %s"), GopClosed, GopStrict);

    msdk_char idrInterval[6];
    msdk_itoa_decimal(param.mfx.IdrInterval, idrInterval);
    msdk_printf(MSDK_STRING("\nIDR Interval     :\t%s"), param.mfx.IdrInterval == 0xffff ? MSDK_STRING("Infinite") : MSDK_STRING(idrInterval));

    msdk_printf(MSDK_STRING("\nGopRefDist       :\t%d"), param.mfx.GopRefDist);
    msdk_printf(MSDK_STRING("\nNumRefFrame      :\t%d"), param.mfx.NumRefFrame);

    msdk_printf(MSDK_STRING("\nNumRefActiveP    :\t%d"), param.GetExtBuffer<mfxExtCodingOption3>()->NumRefActiveP[0]);
    msdk_printf(MSDK_STRING("\nNumRefActiveBL0  :\t%d"), param.GetExtBuffer<mfxExtCodingOption3>()->NumRefActiveBL0[0]);
    msdk_printf(MSDK_STRING("\nNumRefActiveBL1  :\t%d"), param.GetExtBuffer<mfxExtCodingOption3>()->NumRefActiveBL1[0]);

    mfxU16 bRefType = param.GetExtBuffer<mfxExtCodingOption2>()->BRefType;
    msdk_printf(MSDK_STRING("\nB-pyramid        :\t%s"), bRefType ? (bRefType == MFX_B_REF_OFF ? MSDK_STRING("Off") : MSDK_STRING("On")) : MSDK_STRING("MSDK default"));

    static std::string PRefStrs[] = {"DEFAULT", "SIMPLE", "PYRAMID" };
    mfxU16 pRefType = param.GetExtBuffer<mfxExtCodingOption3>()->PRefType;
    msdk_printf(MSDK_STRING("\nP-pyramid        :\t%s"), PRefStrs[pRefType].c_str());

    mfxVersion ver;
    m_mfxSession.QueryVersion(&ver);
    msdk_printf(MSDK_STRING("\n\nMedia SDK version\t%d.%d"), ver.Major, ver.Minor);

    msdk_printf(MSDK_STRING("\n"));
}

mfxStatus CEncodingPipeline::LoadFEIPlugin()
{
    mfxPluginUID pluginGuid = msdkGetPluginUID(m_impl, MSDK_VENCODE | MSDK_FEI, MFX_CODEC_HEVC); // remove '| MSDK_FEI' to use HW HEVC for debug
    MSDK_CHECK_ERROR(AreGuidsEqual(pluginGuid, MSDK_PLUGINGUID_NULL), true, MFX_ERR_NOT_FOUND);

    m_pHEVCePlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, m_mfxSession, pluginGuid, 1));
    MSDK_CHECK_POINTER(m_pHEVCePlugin.get(), MFX_ERR_UNSUPPORTED);

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::CreateAllocator()
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = CreateHWDevice();
    MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");

    mfxHDL hdl = NULL;
    sts = m_pHWdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
    MSDK_CHECK_STATUS(sts, "m_pHWdev->GetHandle failed");

    // provide device manager to MediaSDK
    sts = m_mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
    MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");

    /* Only video memory is supported now. So application must provide MediaSDK
    with external allocator and call SetFrameAllocator */
    // create VAAPI allocator
    m_pMFXAllocator.reset(new vaapiFrameAllocator);

    std::auto_ptr<vaapiAllocatorParams> p_vaapiAllocParams(new vaapiAllocatorParams);

    p_vaapiAllocParams->m_dpy = (VADisplay)hdl;
    m_pMFXAllocatorParams = p_vaapiAllocParams;

    // Call SetAllocator to pass external allocator to MediaSDK
    sts = m_mfxSession.SetFrameAllocator(m_pMFXAllocator.get());
    MSDK_CHECK_STATUS(sts, "m_mfxSession.SetFrameAllocator failed");

    // initialize memory allocator
    sts = m_pMFXAllocator->Init(m_pMFXAllocatorParams.get());
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Init failed");

    return sts;
}

mfxStatus CEncodingPipeline::CreateHWDevice()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pHWdev.reset(CreateVAAPIDevice());
    MSDK_CHECK_POINTER(m_pHWdev.get(), MFX_ERR_MEMORY_ALLOC);

    sts = m_pHWdev->Init(NULL, 0, MSDKAdapter::GetNumber(m_mfxSession));
    MSDK_CHECK_STATUS(sts, "m_pHWdev->Init failed");

    return sts;
}


mfxStatus CEncodingPipeline::AllocFrames()
{
    mfxStatus sts = MFX_ERR_NOT_INITIALIZED;

    // Here we build a final alloc request from alloc requests from several components.
    // TODO: This code is a good candidate to become a 'builder' utility class.
    // While it's safe to use it, it needs to be optimized for reasonable resource allocation.
    // TODO: Find a reasonable minimal number of required surfaces for YUVSource+Reorderer+PreENC+ENCODE pipeline_hevc_fei

    mfxFrameAllocRequest allocRequest;
    MSDK_ZERO_MEMORY(allocRequest);

    {
        mfxFrameAllocRequest request;
        MSDK_ZERO_MEMORY(request);

        sts = m_pYUVSource->QueryIOSurf(&request);
        MSDK_CHECK_STATUS(sts, "m_pYUVSource->QueryIOSurf failed");

        allocRequest = request;
    }
    if (m_pFEI_Encode.get())
    {
        mfxFrameAllocRequest encodeRequest;
        MSDK_ZERO_MEMORY(encodeRequest);

        sts = m_pFEI_Encode->QueryIOSurf(&encodeRequest);
        MSDK_CHECK_STATUS(sts, "m_pFEI_Encode->QueryIOSurf failed");

        allocRequest.Type |= encodeRequest.Type | MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
        allocRequest.NumFrameSuggested += encodeRequest.NumFrameSuggested;
        allocRequest.NumFrameMin = allocRequest.NumFrameSuggested;
    }
    if (m_pFEI_PreENC.get())
    {
        mfxFrameAllocRequest preEncRequest;
        MSDK_ZERO_MEMORY(preEncRequest);
        sts = m_pFEI_PreENC->QueryIOSurf(&preEncRequest);
        MSDK_CHECK_STATUS(sts, "m_pFEI_PreENC->QueryIOSurf failed");

        allocRequest.Type |= preEncRequest.Type | MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET | MFX_MEMTYPE_FROM_ENC;
        allocRequest.NumFrameSuggested += preEncRequest.NumFrameSuggested;
        allocRequest.NumFrameMin = allocRequest.NumFrameSuggested;
    }
    if (m_pOrderCtrl.get())
    {
        // encode order ctrl buffering surfaces in own pools,
        // so add number of required frames for it
        allocRequest.NumFrameSuggested += m_pOrderCtrl->GetNumReorderFrames();
        allocRequest.NumFrameMin = allocRequest.NumFrameSuggested;
    }

    sts = m_EncSurfPool.AllocSurfaces(allocRequest);
    MSDK_CHECK_STATUS(sts, "AllocFrames::EncSurfPool->AllocSurfaces failed");

    return sts;
}

mfxStatus CEncodingPipeline::InitComponents()
{
    mfxStatus sts = MFX_ERR_NONE;

    MSDK_CHECK_POINTER(m_pYUVSource.get(), MFX_ERR_UNSUPPORTED);
    sts = m_pYUVSource->Init();
    MSDK_CHECK_STATUS(sts, "m_pYUVSource->Init failed");

    if (m_pFEI_PreENC.get())
    {
        sts = m_pFEI_PreENC->Init();
        MSDK_CHECK_STATUS(sts, "FEI PreENC Init failed");
    }

    if (m_pFEI_Encode.get())
    {
        sts = m_pFEI_Encode->Init();
        MSDK_CHECK_STATUS(sts, "FEI ENCODE Init failed");
    }

    return sts;
}

mfxStatus CEncodingPipeline::Execute()
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1* pSurf = NULL; // points to frame being processed

    mfxU32 numSubmitted = 0;

    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts)
    {
        if (m_inParams.nNumFrames <= numSubmitted) // frame encoding limit
            break;

        sts = m_pYUVSource->GetFrame(pSurf);
        MSDK_BREAK_ON_ERROR(sts);

        numSubmitted++;

        HevcTask* task = NULL;
        if (m_pOrderCtrl.get())
        {
            task = m_pOrderCtrl->GetTask(pSurf);
            if (!task)
                continue; // frame is buffered here
            pSurf = task->m_surf;
        }

        if (m_pFEI_PreENC.get())
        {
            sts = m_pFEI_PreENC->PreEncFrame(task);
            MSDK_BREAK_ON_ERROR(sts);
        }

        if (m_pFEI_Encode.get())
        {
            sts = task ? m_pFEI_Encode->EncodeFrame(task) : m_pFEI_Encode->EncodeFrame(pSurf);
            MSDK_BREAK_ON_ERROR(sts);
        }

        if (m_pOrderCtrl.get())
            m_pOrderCtrl->FreeTask(task);
        m_processedFrames++;
    } // while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts)

    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA); // reached end of input file
    // exit in case of other errors
    MSDK_CHECK_STATUS(sts, "Frame processing failed");

    sts = DrainBufferedFrames();
    MSDK_CHECK_STATUS(sts, "Buffered frames processing failed");

    return sts;
}

mfxStatus CEncodingPipeline::DrainBufferedFrames()
{
    mfxStatus sts = MFX_ERR_NONE;

    // encode order
    if (m_pOrderCtrl.get())
    {
        HevcTask* task;
        while ( NULL != (task = m_pOrderCtrl->GetTask(NULL)))
        {
            if (m_pFEI_PreENC.get())
            {
                sts = m_pFEI_PreENC->PreEncFrame(task);
                MSDK_CHECK_STATUS(sts, "PreEncFrame drain failed");
            }

            if (m_pFEI_Encode.get())
            {
                sts = m_pFEI_Encode->EncodeFrame(task);
                // exit in case of other errors
                MSDK_CHECK_STATUS(sts, "EncodeFrame drain failed");

            }
            m_pOrderCtrl->FreeTask(task);
            m_processedFrames++;
        }
        return sts;
    }

    // display order
    if (m_pFEI_Encode.get())
    {
        mfxFrameSurface1* null_surf = NULL;
        while (MFX_ERR_NONE <= sts)
        {
            sts = m_pFEI_Encode->EncodeFrame(null_surf);
        }

        // MFX_ERR_MORE_DATA indicates that there are no more buffered frames
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
        // exit in case of other errors
        MSDK_CHECK_STATUS(sts, "EncodeFrame failed");
    }

    return sts;
}

// Function generates common mfxVideoParam ENCODE
// from user cmd line parameters and frame info from upstream component in pipeline.
MfxVideoParamsWrapper GetEncodeParams(const sInputParams& user_pars, const mfxFrameInfo& in_fi)
{
    MfxVideoParamsWrapper pars;

    // frame info from previous component
    MSDK_MEMCPY_VAR(pars.mfx.FrameInfo, &in_fi, sizeof(mfxFrameInfo));

    pars.mfx.CodecId = MFX_CODEC_HEVC;

    // default settings
    pars.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
    pars.mfx.TargetUsage       = 0;
    pars.mfx.TargetKbps        = 0;
    pars.AsyncDepth            = 1; // inherited limitation from AVC FEI
    pars.IOPattern             = MFX_IOPATTERN_IN_VIDEO_MEMORY;

    // user defined settings
    pars.mfx.QPI = pars.mfx.QPP = pars.mfx.QPB = user_pars.QP;

    pars.mfx.GopRefDist   = user_pars.nRefDist;
    pars.mfx.GopPicSize   = user_pars.nGopSize;
    pars.mfx.GopOptFlag   = user_pars.nGopOptFlag;
    pars.mfx.IdrInterval  = user_pars.nIdrInterval;
    pars.mfx.NumRefFrame  = user_pars.nNumRef;
    pars.mfx.NumSlice     = user_pars.nNumSlices;
    pars.mfx.EncodedOrder = user_pars.bEncodedOrder;

    mfxExtCodingOption* pCO = pars.AddExtBuffer<mfxExtCodingOption>();
    if (!pCO) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption");

    pCO->PicTimingSEI = user_pars.PicTimingSEI;

    mfxExtCodingOption2* pCO2 = pars.AddExtBuffer<mfxExtCodingOption2>();
    if (!pCO2) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption2");

    // configure B-pyramid settings
    pCO2->BRefType = user_pars.BRefType;

    mfxExtCodingOption3* pCO3 = pars.AddExtBuffer<mfxExtCodingOption3>();
    if (!pCO3) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption3");

    pCO3->PRefType             = user_pars.PRefType;

    std::fill(pCO3->NumRefActiveP,   pCO3->NumRefActiveP + 8,   user_pars.NumRefActiveP);
    std::fill(pCO3->NumRefActiveBL0, pCO3->NumRefActiveBL0 + 8, user_pars.NumRefActiveBL0);
    std::fill(pCO3->NumRefActiveBL1, pCO3->NumRefActiveBL1 + 8, user_pars.NumRefActiveBL1);

    pCO3->GPB = user_pars.GPB;

    return pars;
}

IYUVSource* CEncodingPipeline::CreateYUVSource()
{
    std::auto_ptr<IYUVSource> pSource;

    if (m_inParams.input.DecodeId)
    {
        mfxPluginUID pluginGuid = MSDK_PLUGINGUID_NULL;
        pluginGuid = msdkGetPluginUID(m_impl, MSDK_VDECODE, m_inParams.input.DecodeId);

        if (!AreGuidsEqual(pluginGuid, MSDK_PLUGINGUID_NULL))
        {
            m_pDecoderPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_DECODE, m_mfxSession, pluginGuid, 1));
            if (m_pDecoderPlugin.get() == NULL)
                return NULL;
        }

        pSource.reset(new Decoder(m_inParams.input, &m_EncSurfPool, &m_mfxSession));
    }
    else
        pSource.reset(new YUVReader(m_inParams.input, &m_EncSurfPool));

    if (m_inParams.input.fieldSplitting)
    {
        IYUVSource* pFieldSplitter = new FieldSplitter(pSource.get(), m_inParams.input, &m_EncSurfPool, &m_mfxSession);
        pSource.release(); // FieldSplitter takes responsibility for memory deallocation of pSource
        pSource.reset(pFieldSplitter);
    }

    return pSource.release();
}

IPreENC* CEncodingPipeline::CreatePreENC(mfxFrameInfo& in_fi)
{
    if (!m_inParams.bPREENC && (0 == msdk_strlen(m_inParams.mvpInFile) || !m_inParams.bFormattedMVPin))
        return NULL;

    MfxVideoParamsWrapper pars = GetEncodeParams(m_inParams, in_fi);

    mfxStatus sts = m_pParamChecker->Query(pars);
    CHECK_STS_AND_RETURN(sts, "m_pParamChecker->Query failed", NULL);

    // PreENC specific parameters
    pars.mfx.CodecId = MFX_CODEC_AVC;

    // attach buffer to set FEI PreENC usage model
    mfxExtFeiParam* pExtBufInit = pars.AddExtBuffer<mfxExtFeiParam>();
    if (!pExtBufInit) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtFeiParam");
    pExtBufInit->Func = MFX_FEI_FUNCTION_PREENC;

    std::auto_ptr<IPreENC> pPreENC;

    if (0 == msdk_strlen(m_inParams.mvpInFile))
    {
        pPreENC.reset(new FEI_Preenc(&m_mfxSession, pars, m_inParams.preencCtrl, m_inParams.mvoutFile,
            m_inParams.bFormattedMVout, m_inParams.mbstatoutFile));

        if (m_inParams.preencDSfactor > 1)
        {
            IPreENC* pPreENC_DS = new PreencDownsampler(pPreENC.get(), m_inParams.preencDSfactor, &m_mfxSession, m_pMFXAllocator.get());
            pPreENC.release(); // PreencDownsampler saves an auto-pointer for just created FEI_Preenc
            pPreENC.reset(pPreENC_DS);
        }
    }
    else
    {
        pPreENC.reset(new Preenc_Reader(pars, m_inParams.preencCtrl, m_inParams.mvpInFile));
    }


    return pPreENC.release();
}

FEI_Encode* CEncodingPipeline::CreateEncode(mfxFrameInfo& in_fi)
{
    if (!m_inParams.bENCODE)
        return NULL;

    mfxStatus sts = MFX_ERR_NONE;

    sts = LoadFEIPlugin();
    CHECK_STS_AND_RETURN(sts, "LoadFEIPlugin failed", NULL);

    MfxVideoParamsWrapper pars = GetEncodeParams(m_inParams, in_fi);
    sts = m_pParamChecker->Query(pars);
    CHECK_STS_AND_RETURN(sts, "m_pParamChecker->Query failed", NULL);

    std::auto_ptr<PredictorsRepaking> pRepacker;

    if (m_inParams.bPREENC || (0 != msdk_strlen(m_inParams.mvpInFile) && m_inParams.bFormattedMVPin))
    {
        pRepacker.reset(new PredictorsRepaking());
        sts = pRepacker->Init(pars, m_inParams.preencDSfactor, m_inParams.encodeCtrl.NumMvPredictors);
        CHECK_STS_AND_RETURN(sts, "CreateEncode::pRepacker->Init failed", NULL);

        pRepacker->SetPerfomanceRepackingMode();
    }

    mfxHDL hdl = NULL;
    sts = m_pHWdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
    CHECK_STS_AND_RETURN(sts, "CreateEncode::m_pHWdev->GetHandle failed", NULL);

    FEI_Encode* pEncode = new FEI_Encode(&m_mfxSession, hdl, pars, m_inParams.encodeCtrl, m_inParams.strDstFile,
            m_inParams.mvpInFile, pRepacker.get());

    pRepacker.release(); // FEI_Encode takes responsibility for pRepakcer's deallocation

    return pEncode;
}
