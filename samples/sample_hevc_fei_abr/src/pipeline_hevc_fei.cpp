/******************************************************************************\
Copyright (c) 2018, Intel Corporation
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

#include <version.h>

#include "hevc_sw_dso.h"
#include "buffer_pool.h"
#include "pipeline_hevc_fei.h"

CFeiTranscodingPipeline::CFeiTranscodingPipeline(const std::vector<sInputParams> & inputParamsArray)
    : m_inParamsArray(std::move(inputParamsArray))
    , m_impl(MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_VAAPI)
    , m_EncSurfPool()
    , m_mvpPool(new MVPPool)
    , m_ctuCtrlPool(nullptr)
    , m_FramesToProcess(0)
    , m_processedFrames(0)
{
    mfxU16 LookAheadDepth = 1;
    for (auto const & param : m_inParamsArray)
    {
        if (param.sBRCparams.eBrcType != NONE)
        {
            LookAheadDepth = std::max(param.sBRCparams.LookAheadDepth, LookAheadDepth);
        }
    }
    m_la_queue.reset(new LA_queue(LookAheadDepth, m_inParamsArray[0].sBRCparams.strYUVFile));

    for (auto const & param : m_inParamsArray)
    {
        if (param.input.forceToIntra || param.input.forceToInter)
        {
            m_ctuCtrlPool.reset(new CTUCtrlPool);
            break;
        }
    }
}

CFeiTranscodingPipeline::~CFeiTranscodingPipeline()
{
    Close();
}

mfxStatus CFeiTranscodingPipeline::Init()
{
    mfxStatus sts = MFX_ERR_NONE;

    try
    {
        sts = m_mfxSession.Init(m_impl, NULL);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.Init failed");

        sts = CreateHWDevice();
        MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");

        sts = CreateAllocator();
        MSDK_CHECK_STATUS(sts, "CreateAllocator failed");

        m_EncSurfPool.SetAllocator(m_pMFXAllocator.get());

        mfxHDL hdl = NULL;
        sts = m_pHWdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
        MSDK_CHECK_STATUS(sts, "m_pHWdev->GetHandle failed");
        m_pParamChecker.reset(new HEVCEncodeParamsChecker(m_impl, hdl));

        for (auto const & param : m_inParamsArray)
        {
            switch (param.pipeMode)
            {
                case Full:
                {
                    m_FramesToProcess = param.nNumFrames;

                    m_source.reset(CreateYUVSource());
                    MSDK_CHECK_POINTER(m_source.get(), MFX_ERR_NOT_INITIALIZED);

                    m_dso.reset(new HevcSwDso(param.input, &m_EncSurfPool, m_mvpPool, m_ctuCtrlPool, param.sBRCparams.eBrcType == LOOKAHEAD, param.dumpMVP, !param.sBRCparams.strYUVFile[0], param.sBRCparams.eAlgType));

                    sts = m_source->PreInit();
                    MSDK_CHECK_STATUS(sts, "m_source PreInit failed");

                    mfxFrameInfo frameInfo;
                    sts = m_source->GetActualFrameInfo(frameInfo);
                    MSDK_CHECK_STATUS(sts, "m_source GetActualFrameInfo failed");

                    sts = CreateBufferAllocator(frameInfo.Width, frameInfo.Height);
                    MSDK_CHECK_STATUS(sts, "CreateBufferAllocator failed");

                    m_dso->SetBufferAllocator(m_bufferAllocator);

                    std::unique_ptr<EncoderContext> encoder(new EncoderContext);

                    sts = encoder->PreInit(m_pMFXAllocator.get(), hdl, m_bufferAllocator, param, frameInfo);
                    MSDK_CHECK_STATUS(sts, "FEI ENCODE Init failed");

                    m_encoders.push_back(std::move(encoder));
                    break;
                }
                case Producer:
                {
                    m_FramesToProcess = param.nNumFrames;

                    m_source.reset(CreateYUVSource());
                    MSDK_CHECK_POINTER(m_source.get(), MFX_ERR_NOT_INITIALIZED);

                    m_dso.reset(new HevcSwDso(param.input, &m_EncSurfPool, m_mvpPool, m_ctuCtrlPool, param.sBRCparams.eBrcType == LOOKAHEAD, param.dumpMVP, !param.sBRCparams.strYUVFile[0], param.sBRCparams.eAlgType));

                    sts = m_source->PreInit();
                    MSDK_CHECK_STATUS(sts, "m_source PreInit failed");

                    mfxFrameInfo frameInfo;
                    sts = m_source->GetActualFrameInfo(frameInfo);
                    MSDK_CHECK_STATUS(sts, "m_source GetActualFrameInfo failed");

                    sts = CreateBufferAllocator(frameInfo.Width, frameInfo.Height);
                    MSDK_CHECK_STATUS(sts, "CreateBufferAllocator failed");

                    m_dso->SetBufferAllocator(m_bufferAllocator);
                    break;
                }
                case Consumer:
                {
                    mfxFrameInfo frameInfo;
                    sts = m_source->GetActualFrameInfo(frameInfo);
                    MSDK_CHECK_STATUS(sts, "m_source GetActualFrameInfo failed");

                    std::unique_ptr<EncoderContext> encoder(new EncoderContext);

                    sts = encoder->PreInit(m_pMFXAllocator.get(), hdl, m_bufferAllocator, param, frameInfo);
                    MSDK_CHECK_STATUS(sts, "FEI ENCODE Init failed");

                    m_encoders.push_back(std::move(encoder));
                    break;
                }
                default:
                    msdk_printf(MSDK_STRING("ERROR: Unsupported pipeline mode"));
                    return MFX_ERR_UNSUPPORTED;
            }
        }

        sts = AllocFrames();
        MSDK_CHECK_STATUS(sts, "AllocFrames failed");

        sts = AllocBuffers();
        MSDK_CHECK_STATUS(sts, "AllocBuffers failed");

        sts = InitComponents();
        MSDK_CHECK_STATUS(sts, "InitComponents failed");

        // We need an allocator to lock surface for MSE calculation
        m_la_queue->SetAllocator(m_pMFXAllocator.get());
    }
    catch (mfxError& ex)
    {
        MSDK_CHECK_STATUS(ex.GetStatus(), ex.what());
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

void CFeiTranscodingPipeline::Close()
{
    msdk_printf(MSDK_STRING("\nFrames processed: %u\n"), m_processedFrames);
}

void CFeiTranscodingPipeline::PrintInfo()
{
    msdk_printf(MSDK_STRING("\nIntel(R) Media SDK HEVC FEI Encoding Sample Version %s\n"), GetMSDKSampleVersion().c_str());

    std::string pipeline = "Decode";
    pipeline += m_dso.get() ? " + DSO" : "";
    pipeline += " + encode";
    switch (m_inParamsArray[0].sBRCparams.eBrcType)
    {
    case LOOKAHEAD:
        pipeline += " (LA BRC)";
        break;
    case MSDKSW:
        pipeline += " (SW BRC)";
        break;
    default:
    case NONE:
        pipeline += " (NO BRC)";
        break;
    }
    msdk_printf(MSDK_STRING("\nPipeline         :\t%s"), pipeline.c_str());

    mfxFrameInfo sourceFrameInfo;
    m_source->GetActualFrameInfo(sourceFrameInfo);

    msdk_printf(MSDK_STRING("\nResolution       :\t%dx%d"), sourceFrameInfo.Width, sourceFrameInfo.Height);
    msdk_printf(MSDK_STRING("\nFrame rate       :\t%d/%d = %.2f"), sourceFrameInfo.FrameRateExtN, sourceFrameInfo.FrameRateExtD, CalculateFrameRate(sourceFrameInfo.FrameRateExtN, sourceFrameInfo.FrameRateExtD));

    mfxU32 id = 0;
    for (auto & encoder : m_encoders)
    {
        msdk_printf(MSDK_STRING("\n\nEncoder #%d:\t"), id);
        MfxVideoParamsWrapper param;
        param = encoder->GetVideoParam();

        msdk_printf(MSDK_STRING("\nProcessing order :\t%s"), param.mfx.EncodedOrder ? MSDK_STRING("Encoded") : MSDK_STRING("Display"));
        if (m_inParamsArray[id].sBRCparams.eBrcType)
        {
            msdk_printf(MSDK_STRING("\nTargetKbps       :\t%d"), m_inParamsArray[id].sBRCparams.TargetKbps);
            if (m_inParamsArray[id].sBRCparams.LookAheadDepth)
            {
                msdk_printf(MSDK_STRING("\nBRC type         :\tLook Ahead"));
                msdk_printf(MSDK_STRING("\nLookAheadDepth   :\t%d"), m_inParamsArray[id].sBRCparams.LookAheadDepth);
                msdk_printf(MSDK_STRING("\nLookBackDepth    :\t%d"), m_inParamsArray[id].sBRCparams.LookBackDepth);
                msdk_printf(MSDK_STRING("\nAdaptationLength :\t%d"), m_inParamsArray[id].sBRCparams.AdaptationLength);
                static std::string sAlgType[] = { "MSE", "NNZ", "SSC" };
                msdk_printf(MSDK_STRING("\nEstimation Algo  :\t%s"), sAlgType[m_inParamsArray[id].sBRCparams.eAlgType].c_str());
            }
            else
            {
                msdk_printf(MSDK_STRING("\nBRC type         :\tSW"));
            }
        }
        ++id;
        msdk_printf(MSDK_STRING("\nGop Size         :\t%d"), param.mfx.GopPicSize);

        mfxU16 gopOptFlag = param.mfx.GopOptFlag;
        const msdk_char* GopClosed = (gopOptFlag & MFX_GOP_CLOSED) == 0 ? MSDK_STRING("Open") : MSDK_STRING("Closed");
        const msdk_char* GopStrict = (gopOptFlag & MFX_GOP_STRICT) == 0 ? MSDK_STRING("Not Strict") : MSDK_STRING("Strict");
        msdk_printf(MSDK_STRING("\nGopOptFlag       :\t%s, %s"), GopClosed, GopStrict);

	if (param.mfx.IdrInterval == 0xffff)
            msdk_printf(MSDK_STRING("\nIDR Interval     :\tInfinite"));
	else
	    msdk_printf(MSDK_STRING("\nIDR Interval     :\t%d"), param.mfx.IdrInterval);

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
    }

    mfxVersion ver;
    m_mfxSession.QueryVersion(&ver);
    msdk_printf(MSDK_STRING("\n\nMedia SDK version\t%d.%d"), ver.Major, ver.Minor);

    msdk_printf(MSDK_STRING("\n"));
}

mfxStatus CFeiTranscodingPipeline::CreateAllocator()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pMFXAllocator.reset(new vaapiFrameAllocator);

    std::unique_ptr<mfxAllocatorParams> allocatorParam(new vaapiAllocatorParams);

    mfxHDL hdl = NULL;
    sts = m_pHWdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
    MSDK_CHECK_STATUS(sts, "m_pHWdev->GetHandle failed");

    vaapiAllocatorParams *vaapiParams = dynamic_cast<vaapiAllocatorParams*>(allocatorParam.get());
    vaapiParams->m_dpy = (VADisplay)hdl;

    sts = m_mfxSession.SetFrameAllocator(m_pMFXAllocator.get());
    MSDK_CHECK_STATUS(sts, "m_mfxSession.SetFrameAllocator failed");

    sts = m_pMFXAllocator->Init(allocatorParam.get());
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Init failed");

    return sts;
}

mfxStatus CFeiTranscodingPipeline::CreateBufferAllocator(mfxU32 w, mfxU32 h)
{
    mfxStatus sts = MFX_ERR_NONE;
    try
    {
        mfxHDL hdl = NULL;
        sts = m_pHWdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
        MSDK_CHECK_STATUS(sts, "m_pHWdev->GetHandle failed");

        m_bufferAllocator.reset(new FeiBufferAllocator(hdl, w, h));
        m_mvpPool->SetDeleter(m_bufferAllocator);

        if (m_ctuCtrlPool.get())
        {
            m_ctuCtrlPool->SetDeleter(m_bufferAllocator);
        }
    }
    catch (mfxError& ex)
    {
        MSDK_CHECK_STATUS(ex.GetStatus(), ex.what());
    }
    catch(...)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    return sts;
}

mfxStatus CFeiTranscodingPipeline::CreateHWDevice()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_pHWdev.reset(CreateVAAPIDevice());
    MSDK_CHECK_POINTER(m_pHWdev.get(), MFX_ERR_MEMORY_ALLOC);

    sts = m_pHWdev->Init(NULL, 0, MSDKAdapter::GetNumber(m_mfxSession));
    MSDK_CHECK_STATUS(sts, "m_pHWdev->Init failed");

    mfxHDL hdl = NULL;
    sts = m_pHWdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
    MSDK_CHECK_STATUS(sts, "m_pHWdev->GetHandle failed");

    sts = m_mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
    MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");

    return sts;
}

mfxStatus CFeiTranscodingPipeline::AllocFrames()
{
    mfxStatus sts = MFX_ERR_NOT_INITIALIZED;

    // Here we build a final alloc request from alloc requests from several components.
    // TODO: This code is a good candidate to become a 'builder' utility class.

    mfxFrameAllocRequest allocRequest;
    MSDK_ZERO_MEMORY(allocRequest);

    {
        mfxFrameAllocRequest request;
        MSDK_ZERO_MEMORY(request);

        sts = m_source->QueryIOSurf(&request);
        MSDK_CHECK_STATUS(sts, "m_source->QueryIOSurf failed");

        allocRequest = request;
    }

    {
        mfxFrameAllocRequest encodeRequest;
        MSDK_ZERO_MEMORY(encodeRequest);

        // Considering the case when all encoders have the same resolution so far
        sts = m_encoders.front()->QueryIOSurf(&encodeRequest);
        MSDK_CHECK_STATUS(sts, "m_encoder->QueryIOSurf failed");

        allocRequest.Type |= encodeRequest.Type | MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
        allocRequest.NumFrameSuggested += encodeRequest.NumFrameSuggested;
    }

    {
        mfxFrameAllocRequest lookaheadRequest;
        MSDK_ZERO_MEMORY(lookaheadRequest);

        sts = m_la_queue->QueryIOSurf(&lookaheadRequest);
        MSDK_CHECK_STATUS(sts, "m_la_queue.QueryIOSurf failed");

        allocRequest.NumFrameSuggested += lookaheadRequest.NumFrameSuggested;
        allocRequest.NumFrameMin = allocRequest.NumFrameSuggested;
    }

    sts = m_EncSurfPool.AllocSurfaces(allocRequest);
    MSDK_CHECK_STATUS(sts, "AllocFrames::EncSurfPool->AllocSurfaces failed");

    return sts;
}

mfxStatus CFeiTranscodingPipeline::AllocBuffers()
{
    mfxStatus sts = MFX_ERR_NONE;
    try
    {
        mfxFrameAllocRequest lookaheadRequest;
        MSDK_ZERO_MEMORY(lookaheadRequest);

        sts = m_la_queue->QueryIOSurf(&lookaheadRequest);
        MSDK_CHECK_STATUS(sts, "m_la_queue.QueryIOSurf failed");

        BufferAllocRequest request;
        MSDK_ZERO_MEMORY(request);

        // Considering the case when all encoders have the same resolution so far
        MfxVideoParamsWrapper param = m_encoders.front()->GetVideoParam();

        request.Width = param.mfx.FrameInfo.CropW;
        request.Height = param.mfx.FrameInfo.CropH;

        for (mfxU32 i = 0; i < lookaheadRequest.NumFrameSuggested; ++i)
        {
            std::unique_ptr<mfxExtFeiHevcEncMVPredictors> mvp(new mfxExtFeiHevcEncMVPredictors);
            init_ext_buffer(*mvp);

            m_bufferAllocator->Alloc(mvp.get(), request);
            m_mvpPool->Add(std::move(mvp));

            if (m_ctuCtrlPool.get())
            {
                std::unique_ptr<mfxExtFeiHevcEncCtuCtrl> ctuCtrl(new mfxExtFeiHevcEncCtuCtrl);
                init_ext_buffer(*ctuCtrl);

                m_bufferAllocator->Alloc(ctuCtrl.get(), request);
                m_ctuCtrlPool->Add(std::move(ctuCtrl));
            }
        }
    }
    catch (mfxError& ex)
    {
        MSDK_CHECK_STATUS(ex.GetStatus(), ex.what());
    }
    catch(...)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    return sts;
}

mfxStatus CFeiTranscodingPipeline::InitComponents()
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_source->Init();
    MSDK_CHECK_STATUS(sts, "m_source->Init failed");

    sts = m_dso->Init();
    MSDK_CHECK_STATUS(sts, "m_dso->Init failed");

    for (auto & encoder : m_encoders)
    {
        sts = encoder->Init();
        MSDK_CHECK_STATUS(sts, "FEI ENCODE Init failed");
    }

    return sts;
}

mfxStatus CFeiTranscodingPipeline::Execute()
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxU32 numSubmitted = 0;

    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts)
    {
        if (m_FramesToProcess <= numSubmitted) // frame encoding limit
            break;

        mfxFrameSurface1* surface = NULL;

        sts = m_source->GetFrame(surface);
        MSDK_BREAK_ON_ERROR(sts);

        std::shared_ptr<HevcTaskDSO> task(new HevcTaskDSO);
        task->m_surf = surface;

        sts = m_dso->GetFrame(*task.get()); // TODO: need to change GetFrame to accept shared_ptr
        MSDK_BREAK_ON_ERROR(sts);

        numSubmitted++;

        m_la_queue->AddTask(std::move(task));
        for (auto & encoder : m_encoders)
        {
            encoder->UpdateBRCStat(m_la_queue->Back()->m_statData);
        }

        if (!m_la_queue->GetTask(task))
        {
            continue;
        }

        for (auto & encoder : m_encoders)
        {
            sts = encoder->SubmitFrame(task); // asynchronous call
        }
        MSDK_BREAK_ON_ERROR(sts);

        m_processedFrames++;
    }

    // Drain frames from LA queue
    if (sts == MFX_ERR_MORE_DATA || m_FramesToProcess <= numSubmitted)
    {
        m_la_queue->StartDrain();

        std::shared_ptr<HevcTaskDSO> task;
        while (m_la_queue->GetTask(task))
        {
            for (auto & encoder : m_encoders)
            {
                sts = encoder->SubmitFrame(task); // asynchronous call
            }
            MSDK_BREAK_ON_ERROR(sts);

            m_processedFrames++;
        }
    }

    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA); // reached end of input file
    // exit in case of other errors
    MSDK_CHECK_STATUS(sts, "Frame processing failed");

    return sts;
}

IYUVSource* CFeiTranscodingPipeline::CreateYUVSource()
{
    std::unique_ptr<IYUVSource> pSource;

    for (auto const & it : m_inParamsArray)
    {
        if (it.pipeMode == Full || it.pipeMode == Producer)
        {
            if (it.input.DecodeId)
            {
                mfxPluginUID pluginGuid = MSDK_PLUGINGUID_NULL;
                pluginGuid = msdkGetPluginUID(m_impl, MSDK_VDECODE, it.input.DecodeId);

                if (!AreGuidsEqual(pluginGuid, MSDK_PLUGINGUID_NULL))
                {
                    m_pDecoderPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_DECODE, m_mfxSession, pluginGuid, 1));
                    if (m_pDecoderPlugin.get() == NULL)
                        return NULL;
                }

                pSource.reset(new Decoder(it.input, &m_EncSurfPool, &m_mfxSession));
            }
            break;
        }
    }



    return pSource.release();
}

/**********************************************************************************/

mfxStatus EncoderContext::PreInit(MFXFrameAllocator * mfxAllocator,
                                  mfxHDL hdl,
                                  std::shared_ptr<FeiBufferAllocator> & bufferAllocator,
                                  const sInputParams & params,
                                  const mfxFrameInfo & inFrameInfo)
{
    mfxInitParam initPar;
    MSDK_ZERO_MEMORY(initPar);

    mfxExtThreadsParam threadsPar;
    MSDK_ZERO_MEMORY(threadsPar);
    threadsPar.Header.BufferId = MFX_EXTBUFF_THREADS_PARAM;
    threadsPar.Header.BufferSz = sizeof(threadsPar);
    threadsPar.NumThread = 2;

    mfxExtBuffer* extBufs[1];
    extBufs[0] = (mfxExtBuffer*)&threadsPar;

    initPar.Version.Major = 1;
    initPar.Version.Minor = 0;
    initPar.Implementation = MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_VAAPI;

    initPar.ExtParam = extBufs;
    initPar.NumExtParam = 1;

    mfxStatus sts = m_mfxSession.InitEx(initPar);
    MSDK_CHECK_STATUS(sts, "session.Init failed");

    sts = m_mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
    MSDK_CHECK_STATUS(sts, "session.SetHandle failed");

    sts = m_mfxSession.SetFrameAllocator(mfxAllocator);
    MSDK_CHECK_STATUS(sts, "session.SetFrameAllocator failed");

    m_MFXAllocator = mfxAllocator;
    m_hdl = hdl;
    m_bufferAllocator = bufferAllocator;

    sts = CreateEncoder(params, inFrameInfo);
    MSDK_CHECK_STATUS(sts, "CreateEncoder failed");

    sts = m_encoder->PreInit();
    MSDK_CHECK_STATUS(sts, "m_encoder->PreInit failed");

    return sts;
}

mfxStatus EncoderContext::CreateEncoder(const sInputParams & params, const mfxFrameInfo & info)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxPluginUID pluginGuid = msdkGetPluginUID(MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_VAAPI, MSDK_VENCODE | MSDK_FEI, MFX_CODEC_HEVC);
    MSDK_CHECK_ERROR(AreGuidsEqual(pluginGuid, MSDK_PLUGINGUID_NULL), true, MFX_ERR_NOT_FOUND);

    m_pHEVCePlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, m_mfxSession, pluginGuid, 1));
    MSDK_CHECK_POINTER(m_pHEVCePlugin.get(), MFX_ERR_UNSUPPORTED);

    MfxVideoParamsWrapper pars = GetEncodeParams(params, info);

    std::unique_ptr<IEncoder> encoder;
    encoder.reset(new FEI_Encode(&m_mfxSession, pars, params.encodeCtrl, params.frameCtrl, params.strDstFile, params.sBRCparams));

    if (params.drawMVP)
    {
        std::string name = params.strDstFile;
        name += "." + std::to_string(info.CropW) + "x" + std::to_string(info.CropH) + ".mvp.nv12";

        IEncoder* overlay = new MVPOverlay(encoder.get(), m_MFXAllocator, m_bufferAllocator, name);
        encoder.release();
        encoder.reset(overlay);
    }

    m_encoder.reset(encoder.release());

    return sts;
}

MfxVideoParamsWrapper EncoderContext::GetEncodeParams(const sInputParams& userParam, const mfxFrameInfo& info)
{
    MfxVideoParamsWrapper pars;

    // frame info from previous component
    MSDK_MEMCPY_VAR(pars.mfx.FrameInfo, &info, sizeof(mfxFrameInfo));

    pars.mfx.CodecId = MFX_CODEC_HEVC;

    // default settings
    pars.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
    pars.mfx.TargetUsage       = 0;
    pars.mfx.TargetKbps        = 0;
    pars.AsyncDepth            = 1; // inherited limitation from AVC FEI
    pars.IOPattern             = MFX_IOPATTERN_IN_VIDEO_MEMORY;

    // user defined settings
    pars.mfx.QPI = pars.mfx.QPP = pars.mfx.QPB = userParam.QP;

    pars.mfx.GopRefDist   = userParam.nRefDist;
    pars.mfx.GopPicSize   = userParam.nGopSize;
    pars.mfx.GopOptFlag   = userParam.nGopOptFlag;
    pars.mfx.IdrInterval  = userParam.nIdrInterval;
    pars.mfx.NumRefFrame  = userParam.nNumRef;
    pars.mfx.NumSlice     = userParam.nNumSlices;
    pars.mfx.EncodedOrder = userParam.bEncodedOrder;

    mfxExtCodingOption* pCO = pars.AddExtBuffer<mfxExtCodingOption>();
    if (!pCO) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption");

    pCO->PicTimingSEI        = userParam.PicTimingSEI;

    mfxExtCodingOption2* pCO2 = pars.AddExtBuffer<mfxExtCodingOption2>();
    if (!pCO2) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption2");

    // configure B-pyramid settings
    pCO2->BRefType = userParam.BRefType;

    mfxExtCodingOption3* pCO3 = pars.AddExtBuffer<mfxExtCodingOption3>();
    if (!pCO3) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption3");

    pCO3->PRefType = userParam.PRefType;

    std::fill(std::begin(pCO3->NumRefActiveP),   std::end(pCO3->NumRefActiveP),   userParam.NumRefActiveP);
    std::fill(std::begin(pCO3->NumRefActiveBL0), std::end(pCO3->NumRefActiveBL0), userParam.NumRefActiveBL0);
    std::fill(std::begin(pCO3->NumRefActiveBL1), std::end(pCO3->NumRefActiveBL1), userParam.NumRefActiveBL1);

    pCO3->GPB = userParam.GPB;

    // qp offset per pyramid layer, default is library behavior
    pCO3->EnableQPOffset = userParam.bDisableQPOffset ? MFX_CODINGOPTION_OFF : MFX_CODINGOPTION_UNKNOWN;

    return pars;
}
