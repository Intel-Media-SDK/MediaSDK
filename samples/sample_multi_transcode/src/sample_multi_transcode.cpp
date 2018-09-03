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

#include "sample_multi_transcode.h"

#if defined(LIBVA_WAYLAND_SUPPORT)
#include "class_wayland.h"
#endif

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

using namespace std;
using namespace TranscodingSample;

Launcher::Launcher():
    m_StartTime(0),
    m_eDevType(static_cast<mfxHandleType>(0))
{
} // Launcher::Launcher()

Launcher::~Launcher()
{
    Close();
} // Launcher::~Launcher()

CTranscodingPipeline* CreatePipeline()
{
    MOD_SMT_CREATE_PIPELINE;

    return new CTranscodingPipeline;
}

mfxStatus Launcher::Init(int argc, msdk_char *argv[])
{
    mfxStatus sts;
    mfxU32 i = 0;
    SafetySurfaceBuffer* pBuffer = NULL;
    mfxU32 BufCounter = 0;
    mfxHDL hdl = NULL;
    sInputParams    InputParams;

    //parent transcode pipeline
    CTranscodingPipeline *pParentPipeline = NULL;
    // source transcode pipeline use instead parent in heterogeneous pipeline
    CTranscodingPipeline *pSinkPipeline = NULL;

    // parse input par file
    sts = m_parser.ParseCmdLine(argc, argv);
    MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, sts);
    if(sts == MFX_WRN_OUT_OF_RANGE)
    {
        // There's no error in parameters parsing, but we should not continue further. For instance, in case of -? option
        return sts;
    }


    // get parameters for each session from parser
    while(m_parser.GetNextSessionParams(InputParams))
    {
        m_InputParamsArray.push_back(InputParams);
    }

    // check correctness of input parameters
    sts = VerifyCrossSessionsOptions();
    MSDK_CHECK_STATUS(sts, "VerifyCrossSessionsOptions failed");

#if defined(LIBVA_X11_SUPPORT) || defined(LIBVA_DRM_SUPPORT) || defined(ANDROID)
    if (m_eDevType == MFX_HANDLE_VA_DISPLAY)
    {
        mfxI32  libvaBackend = 0;

        m_pAllocParam.reset(new vaapiAllocatorParams);
        vaapiAllocatorParams *pVAAPIParams = dynamic_cast<vaapiAllocatorParams*>(m_pAllocParam.get());
        /* The last param set in vector always describe VPP+ENCODE or Only VPP
         * So, if we want to do rendering we need to do pass HWDev to CTranscodingPipeline */
        if (m_InputParamsArray[m_InputParamsArray.size() -1].eModeExt == VppCompOnly)
        {
            sInputParams& params = m_InputParamsArray[m_InputParamsArray.size() -1];
            libvaBackend = params.libvaBackend;

            /* Rendering case */
            m_hwdev.reset(CreateVAAPIDevice(params.libvaBackend));
            if(!m_hwdev.get()) {
                msdk_printf(MSDK_STRING("error: failed to initialize VAAPI device\n"));
                return MFX_ERR_DEVICE_FAILED;
            }
            sts = m_hwdev->Init(&params.monitorType, 1, MSDKAdapter::GetNumber(0) );
#if defined(LIBVA_X11_SUPPORT) || defined(LIBVA_DRM_SUPPORT)
            if (params.libvaBackend == MFX_LIBVA_DRM_MODESET) {
                CVAAPIDeviceDRM* drmdev = dynamic_cast<CVAAPIDeviceDRM*>(m_hwdev.get());
                pVAAPIParams->m_export_mode = vaapiAllocatorParams::CUSTOM_FLINK;
                pVAAPIParams->m_exporter = dynamic_cast<vaapiAllocatorParams::Exporter*>(drmdev->getRenderer());

            }
#endif
#if defined(LIBVA_WAYLAND_SUPPORT)
            else if (params.libvaBackend == MFX_LIBVA_WAYLAND) {
                VADisplay va_dpy = NULL;
                sts = m_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, (mfxHDL *)&va_dpy);
                MSDK_CHECK_STATUS(sts, "m_hwdev->GetHandle failed");
                hdl = pVAAPIParams->m_dpy =(VADisplay)va_dpy;

                mfxHDL whdl = NULL;
                mfxHandleType hdlw_t = (mfxHandleType)HANDLE_WAYLAND_DRIVER;
                Wayland *wld;
                sts = m_hwdev->GetHandle(hdlw_t, &whdl);
                MSDK_CHECK_STATUS(sts, "m_hwdev->GetHandle failed");
                wld = (Wayland*)whdl;
                wld->SetRenderWinPos(params.nRenderWinX, params.nRenderWinY);
                wld->SetPerfMode(params.bPerfMode);

                pVAAPIParams->m_export_mode = vaapiAllocatorParams::PRIME;
            }
#endif // LIBVA_WAYLAND_SUPPORT
            params.m_hwdev = m_hwdev.get();
        }
        else /* NO RENDERING*/
        {
            m_hwdev.reset(CreateVAAPIDevice());
            if(!m_hwdev.get()) {
                msdk_printf(MSDK_STRING("error: failed to initialize VAAPI device\n"));
                return MFX_ERR_DEVICE_FAILED;
            }
            sts = m_hwdev->Init(NULL, 0, MSDKAdapter::GetNumber(0));
        }
        if (libvaBackend != MFX_LIBVA_WAYLAND) {
        MSDK_CHECK_STATUS(sts, "m_hwdev->Init failed");
        sts = m_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, (mfxHDL*)&hdl);
        MSDK_CHECK_STATUS(sts, "m_hwdev->GetHandle failed");
        // set Device to external vaapi allocator
        pVAAPIParams->m_dpy =(VADisplay)hdl;
    }
    }
#endif
    if (!m_pAllocParam.get())
    {
        m_pAllocParam.reset(new SysMemAllocatorParams);
    }

    // each pair of source and sink has own safety buffer
    sts = CreateSafetyBuffers();
    MSDK_CHECK_STATUS(sts, "CreateSafetyBuffers failed");

    /* One more hint. Example you have 3 dec + 1 enc sessions
    * (enc means vpp_comp call invoked. m_InputParamsArray.size() is 4.
    * You don't need take vpp comp params from last one session as it is enc session.
    * But you need process {0, 1, 2} sessions - totally 3.
    * So, you need start from 0 and end at 2.
    * */
    for(mfxI32 jj = 0; jj<(mfxI32)m_InputParamsArray.size() - 1; jj++)
    {
        /* Save params for VPP composition */
        sVppCompDstRect tempDstRect;
        tempDstRect.DstX   = m_InputParamsArray[jj].nVppCompDstX;
        tempDstRect.DstY   = m_InputParamsArray[jj].nVppCompDstY;
        tempDstRect.DstW   = m_InputParamsArray[jj].nVppCompDstW;
        tempDstRect.DstH   = m_InputParamsArray[jj].nVppCompDstH;
        tempDstRect.TileId = m_InputParamsArray[jj].nVppCompTileId;
        m_VppDstRects.push_back(tempDstRect);
    }

    // create sessions, allocators
    for (i = 0; i < m_InputParamsArray.size(); i++)
    {
        GeneralAllocator* pAllocator = new GeneralAllocator;
        sts = pAllocator->Init(m_pAllocParam.get());
        MSDK_CHECK_STATUS(sts, "pAllocator->Init failed");
        m_pAllocArray.push_back(pAllocator);

        std::unique_ptr<ThreadTranscodeContext> pThreadPipeline(new ThreadTranscodeContext);
        // extend BS processing init
        m_pExtBSProcArray.push_back(new FileBitstreamProcessor);

        pThreadPipeline->pPipeline.reset(CreatePipeline());

        pThreadPipeline->pBSProcessor = m_pExtBSProcArray.back();

        std::unique_ptr<CSmplBitstreamReader> reader;
        std::unique_ptr<CSmplYUVReader> yuvreader;
        if (m_InputParamsArray[i].DecodeId == MFX_CODEC_VP9)
        {
            reader.reset(new CIVFFrameReader());
        }
        else if (m_InputParamsArray[i].DecodeId == MFX_CODEC_RGB4)
        {
            // YUV reader for RGB4 overlay
            yuvreader.reset(new CSmplYUVReader());
        }
        else
        {
            reader.reset(new CSmplBitstreamReader());
        }

        if (reader.get())
        {
            sts = reader->Init(m_InputParamsArray[i].strSrcFile);
            MSDK_CHECK_STATUS(sts, "reader->Init failed");
            sts = m_pExtBSProcArray.back()->SetReader(reader);
            MSDK_CHECK_STATUS(sts, "m_pExtBSProcArray.back()->SetReader failed");
        }
        else if (yuvreader.get())
        {
            std::list<msdk_string> input;
            input.push_back(m_InputParamsArray[i].strSrcFile);
            sts = yuvreader->Init(input, MFX_FOURCC_RGB4);
            MSDK_CHECK_STATUS(sts, "m_YUVReader->Init failed");
            sts = m_pExtBSProcArray.back()->SetReader(yuvreader);
            MSDK_CHECK_STATUS(sts, "m_pExtBSProcArray.back()->SetReader failed");
        }

        std::unique_ptr<CSmplBitstreamWriter> writer(new CSmplBitstreamWriter());
        sts = writer->Init(m_InputParamsArray[i].strDstFile);

        sts = m_pExtBSProcArray.back()->SetWriter(writer);
        MSDK_CHECK_STATUS(sts, "m_pExtBSProcArray.back()->SetWriter failed");

        if (Sink == m_InputParamsArray[i].eMode)
        {
            /* N_to_1 mode */
            if ((VppComp == m_InputParamsArray[i].eModeExt) ||
                (VppCompOnly == m_InputParamsArray[i].eModeExt))
            {
                // Taking buffers from tail because they are stored in m_pBufferArray in reverse order
                // So, by doing this we'll fill buffers properly according to order from par file
                pBuffer = m_pBufferArray[m_pBufferArray.size()-1-BufCounter];
                BufCounter++;
            }
            else /* 1_to_N mode*/
            {
                pBuffer = m_pBufferArray[m_pBufferArray.size() - 1];
            }
            pSinkPipeline = pThreadPipeline->pPipeline.get();
        }
        else if (Source == m_InputParamsArray[i].eMode)
        {
            /* N_to_1 mode */
            if ((VppComp == m_InputParamsArray[i].eModeExt) ||
                (VppCompOnly == m_InputParamsArray[i].eModeExt))
            {
                pBuffer = m_pBufferArray[m_pBufferArray.size() - 1];
            }
            else /* 1_to_N mode*/
            {
                pBuffer = m_pBufferArray[BufCounter];
                BufCounter++;
            }
        }
        else
        {
            pBuffer = NULL;
        }

        /**/
        /* Vector stored linearly in the memory !*/
        m_InputParamsArray[i].pVppCompDstRects = m_VppDstRects.empty() ? NULL : &m_VppDstRects[0];

        // if session has VPP plus ENCODE only (-i::source option)
        // use decode source session as input
        sts = MFX_ERR_MORE_DATA;
        if (Source == m_InputParamsArray[i].eMode)
        {
            sts = pThreadPipeline->pPipeline->Init(&m_InputParamsArray[i],
                                                   m_pAllocArray[i],
                                                   hdl,
                                                   pSinkPipeline,
                                                   pBuffer,
                                                   m_pExtBSProcArray.back());
        }
        else
        {
            sts =  pThreadPipeline->pPipeline->Init(&m_InputParamsArray[i],
                                                    m_pAllocArray[i],
                                                    hdl,
                                                    pParentPipeline,
                                                    pBuffer,
                                                    m_pExtBSProcArray.back());
        }

        MSDK_CHECK_STATUS(sts, "pThreadPipeline->pPipeline->Init failed");

        if (!pParentPipeline && m_InputParamsArray[i].bIsJoin)
            pParentPipeline = pThreadPipeline->pPipeline.get();

        // set the session's start status (like it is waiting)
        pThreadPipeline->startStatus = MFX_WRN_DEVICE_BUSY;
        // set other session's parameters
        pThreadPipeline->implType = m_InputParamsArray[i].libType;
        m_pSessionArray.push_back(pThreadPipeline.release());

        mfxVersion ver = {{0, 0}};
        sts = m_pSessionArray[i]->pPipeline->QueryMFXVersion(&ver);
        MSDK_CHECK_STATUS(sts, "m_pSessionArray[i]->pPipeline->QueryMFXVersion failed");

        PrintInfo(i, &m_InputParamsArray[i], &ver);
    }

    for (i = 0; i < m_InputParamsArray.size(); i++)
    {
        sts = m_pSessionArray[i]->pPipeline->CompleteInit();
        MSDK_CHECK_STATUS(sts, "m_pSessionArray[i]->pPipeline->CompleteInit failed");

        if (m_pSessionArray[i]->pPipeline->GetJoiningFlag())
            msdk_printf(MSDK_STRING("Session %d was joined with other sessions\n"), i);
        else
            msdk_printf(MSDK_STRING("Session %d was NOT joined with other sessions\n"), i);

        m_pSessionArray[i]->pPipeline->SetPipelineID(i);
    }

    msdk_printf(MSDK_STRING("\n"));

    return sts;

} // mfxStatus Launcher::Init()

void Launcher::Run()
{
    msdk_printf(MSDK_STRING("Transcoding started\n"));

    // mark start time
    m_StartTime = GetTick();

    // Robust flag is applied to every seession if enabled in one
    if (m_pSessionArray[0]->pPipeline->GetRobustFlag())
    {
        DoRobustTranscoding();
    }
    else
    {
        DoTranscoding();
    }

    msdk_printf(MSDK_STRING("\nTranscoding finished\n"));

} // mfxStatus Launcher::Init()

void Launcher::DoTranscoding()
{
    mfxStatus sts = MFX_ERR_NONE;

    // get parallel sessions parameters
    mfxU32 totalSessions = (mfxU32)m_pSessionArray.size();
    MSDKThread* pthread = 0;

    for (mfxU32 i = 0; i < totalSessions; i++)
    {
        pthread = new MSDKThread(sts, TranscodeRoutine, (void *)m_pSessionArray[i]);
        m_HDLArray.push_back(pthread);
    }

    // Need to determine overlay threads count first
    size_t nOverlayThreads = 0;
    for (size_t i = 0; i < m_pSessionArray.size(); i++)
    {
        if (m_pSessionArray[i]->pPipeline->IsOverlayUsed())
            nOverlayThreads++;
    }

    // Transcoding threads waiting cycle
    size_t aliveSessionsCount = totalSessions;
    while (aliveSessionsCount)
    {
        aliveSessionsCount = 0;
        for (mfxU32 i = 0; i < totalSessions; i++)
        {
            if (m_HDLArray[i])
            {
                aliveSessionsCount++;

                sts = m_HDLArray[i]->TimedWait(1);
                if (sts <= 0)
                {
                    // Session is completed, let's check for its status
                    if (m_pSessionArray[i]->transcodingSts < 0)
                    {
                        // Stop all the sessions if an error happened in one
                        // But do not stop in robust mode when gpu hang's happened
                        if (m_pSessionArray[i]->transcodingSts != MFX_ERR_GPU_HANG ||
                            !m_pSessionArray[i]->pPipeline->GetRobustFlag())
                        {
                            for (size_t j = 0; j < m_pSessionArray.size(); j++)
                            {
                                m_pSessionArray[j]->pPipeline->StopSession();
                            }
                        }
                    }

                    // Now clear its handle and thread info
                    MSDK_SAFE_DELETE(m_HDLArray[i]);
                    m_HDLArray[i] = NULL;
                }
            }
        }

        // If all sessions are already stopped (no matter with error or not) - we need to forcibly stop all overlay sessions
        if (aliveSessionsCount <= nOverlayThreads + 1)
        {
            // Sending stop message
            for (size_t i = 0; i < totalSessions; i++)
            {
                if (m_HDLArray[i] && m_pSessionArray[i]->pPipeline->IsOverlayUsed())
                {
                    m_pSessionArray[i]->pPipeline->StopSession();
                }
            }

            // Waiting for them to be stopped
            for (size_t i = 0; i < totalSessions; i++)
            {
                if (m_HDLArray[i])
                {
                    m_HDLArray[i]->Wait();
                    MSDK_SAFE_DELETE(m_HDLArray[i]);
                    m_HDLArray[i]=NULL;
                }
            }
        }
    }
    m_HDLArray.clear();
}

void Launcher::DoRobustTranscoding()
{
    mfxStatus sts = MFX_ERR_NONE;

    // Cycle for handling MFX_ERR_GPU_HANG during transcoding
    // If it's returned, reset all the pipelines and start over from the last point
    bool bGPUHang = false;
    for ( ; ; )
    {
        if (bGPUHang)
        {
            for (size_t i = 0; i < m_pSessionArray.size(); i++)
            {
                sts = m_pSessionArray[i]->pPipeline->Reset();
                if (sts)
                {
                    msdk_printf(MSDK_STRING("\n[WARNING] GPU Hang recovery wasn't succeed. Exiting...\n"));
                    return;
                }
            }
            bGPUHang = false;
            msdk_printf(MSDK_STRING("\n[WARNING] Successfully recovered. Continue transcoding.\n"));
        }

        DoTranscoding();

        for (size_t i = 0; i < m_pSessionArray.size(); i++)
        {
            if (m_pSessionArray[i]->transcodingSts == MFX_ERR_GPU_HANG)
            {
                bGPUHang = true;
            }
        }
        if (!bGPUHang)
            break;
        msdk_printf(MSDK_STRING("\n[WARNING] GPU Hang has happened. Trying to recover...\n"));
    }
}

mfxStatus Launcher::ProcessResult()
{
    FILE* pPerfFile = m_parser.GetPerformanceFile();

    msdk_stringstream ssTranscodingTime;
    ssTranscodingTime << std::endl << MSDK_STRING("Common transcoding time is ") << GetTime(m_StartTime) << MSDK_STRING(" sec") << std::endl;

    m_parser.PrintParFileName();

    msdk_printf(MSDK_STRING("%s"),ssTranscodingTime.str().c_str());
    if (pPerfFile)
    {
        msdk_fprintf(pPerfFile, MSDK_STRING("%s"), ssTranscodingTime.str().c_str());
    }

    mfxStatus FinalSts = MFX_ERR_NONE;
    msdk_printf(MSDK_STRING("-------------------------------------------------------------------------------\n"));
    msdk_char buffer[11] = {};

    for (mfxU32 i = 0; i < m_pSessionArray.size(); i++)
    {
        mfxStatus _sts = m_pSessionArray[i]->transcodingSts;

        if (!FinalSts)
            FinalSts = _sts;

        msdk_string SessionStsStr = _sts ? msdk_string(MSDK_STRING("FAILED"))
            : msdk_string((MSDK_STRING("PASSED")));

        m_pSessionArray[i]->pPipeline->GetSessionText(buffer, 11);
        msdk_stringstream ss;
        ss << MSDK_STRING("*** session ") << i << MSDK_STRING(" [") << msdk_string(buffer) << MSDK_STRING("] ") << SessionStsStr <<MSDK_STRING(" (") << StatusToString(_sts) << MSDK_STRING(") ")
            << m_pSessionArray[i]->working_time << MSDK_STRING(" sec, ") << m_pSessionArray[i]->numTransFrames << MSDK_STRING(" frames") << std::endl
            << m_parser.GetLine(i) << std::endl << std::endl;

        msdk_printf(MSDK_STRING("%s"),ss.str().c_str());
        if (pPerfFile)
        {
            msdk_fprintf(pPerfFile, MSDK_STRING("%s"), ss.str().c_str());
        }

    }
    msdk_printf(MSDK_STRING("-------------------------------------------------------------------------------\n"));

    msdk_stringstream ssTest;
    ssTest << std::endl << MSDK_STRING("The test ") << (FinalSts ? msdk_string(MSDK_STRING("FAILED")) : msdk_string(MSDK_STRING("PASSED"))) << std::endl;

    msdk_printf(MSDK_STRING("%s"),ssTest.str().c_str());
    if (pPerfFile)
    {
        msdk_fprintf(pPerfFile, MSDK_STRING("%s"), ssTest.str().c_str());
    }
    return FinalSts;
} // mfxStatus Launcher::ProcessResult()

mfxStatus Launcher::VerifyCrossSessionsOptions()
{
    bool IsSinkPresence = false;
    bool IsSourcePresence = false;
    bool IsHeterSessionJoin = false;
    bool IsFirstInTopology = true;
    bool areAllInterSessionsOpaque = true;

    mfxU16 minAsyncDepth = 0;
    bool bUseExternalAllocator = false;
    bool bSingleTexture = false;

#if (MFX_VERSION >= 1025)
    bool allMFEModesEqual=true;
    bool allMFEFramesEqual=true;
    bool allMFESessionsJoined = true;

    mfxU16 usedMFEMaxFrames = 0;
    mfxU16 usedMFEMode = 0;

    for (mfxU32 i = 0; i < m_InputParamsArray.size(); i++)
    {
        // loop over all sessions and check mfe-specific params
        // for mfe is required to have sessions joined, HW impl
        if(m_InputParamsArray[i].numMFEFrames > 1)
        {
            usedMFEMaxFrames = m_InputParamsArray[i].numMFEFrames;
            for (mfxU32 j = 0; j < m_InputParamsArray.size(); j++)
            {
                if(m_InputParamsArray[j].numMFEFrames &&
                   m_InputParamsArray[j].numMFEFrames != usedMFEMaxFrames)
                {
                    m_InputParamsArray[j].numMFEFrames = usedMFEMaxFrames;
                    allMFEFramesEqual = false;
                    m_InputParamsArray[j].MFMode = m_InputParamsArray[j].MFMode < MFX_MF_AUTO
                      ? MFX_MF_AUTO : m_InputParamsArray[j].MFMode;
                }
                if(m_InputParamsArray[j].bIsJoin == false)
                {
                    allMFESessionsJoined = false;
                    m_InputParamsArray[j].bIsJoin = true;
                }
            }
        }
        if(m_InputParamsArray[i].MFMode >= MFX_MF_AUTO)
        {
            usedMFEMode = m_InputParamsArray[i].MFMode;
            for (mfxU32 j = 0; j < m_InputParamsArray.size(); j++)
            {
                if(m_InputParamsArray[j].MFMode &&
                   m_InputParamsArray[j].MFMode != usedMFEMode)
                {
                    m_InputParamsArray[j].MFMode = usedMFEMode;
                    allMFEModesEqual = false;
                }
                if(m_InputParamsArray[j].bIsJoin == false)
                {
                    allMFESessionsJoined = false;
                    m_InputParamsArray[j].bIsJoin = true;
                }
            }
        }
    }
    if(!allMFEFramesEqual)
        msdk_printf(MSDK_STRING("WARNING: All sessions for MFE should have the same number of MFE frames!\n used ammount of frame for MFE: %d\n"),  (int)usedMFEMaxFrames);
    if(!allMFEModesEqual)
        msdk_printf(MSDK_STRING("WARNING: All sessions for MFE should have the same mode!\n, used mode: %d\n"),  (int)usedMFEMode);
    if(!allMFESessionsJoined)
        msdk_printf(MSDK_STRING("WARNING: Sessions for MFE should be joined! All sessions forced to be joined\n"));
#endif

    for (mfxU32 i = 0; i < m_InputParamsArray.size(); i++)
    {
        if (!m_InputParamsArray[i].bUseOpaqueMemory &&
            ((m_InputParamsArray[i].eMode == Source) || (m_InputParamsArray[i].eMode == Sink)))
        {
            areAllInterSessionsOpaque = false;
        }

        // Any plugin or static frame alpha blending
        // CPU rotate plugin works with opaq frames in native mode
        if ((m_InputParamsArray[i].nRotationAngle && m_InputParamsArray[i].eMode != Native) ||
            m_InputParamsArray[i].bOpenCL ||
            m_InputParamsArray[i].EncoderFourCC ||
            m_InputParamsArray[i].DecoderFourCC ||
            m_InputParamsArray[i].nVppCompSrcH ||
            m_InputParamsArray[i].nVppCompSrcW)
        {
            bUseExternalAllocator = true;
        }

        if (m_InputParamsArray[i].bSingleTexture)
        {
            bSingleTexture = true;
        }

        // All sessions have to know about timeout
        if (m_InputParamsArray[i].nTimeout && (m_InputParamsArray[i].eMode == Sink))
        {
            for (mfxU32 j = 0; j < m_InputParamsArray.size(); j++)
            {
                if (m_InputParamsArray[j].MaxFrameNumber != MFX_INFINITE)
                {
                    msdk_printf(MSDK_STRING("\"-timeout\" option isn't compatible with \"-n\". \"-n\" will be ignored.\n"));
                    for (mfxU32 k = 0; k < m_InputParamsArray.size(); k++)
                    {
                        m_InputParamsArray[k].MaxFrameNumber = MFX_INFINITE;
                    }
                    break;
                }
            }
            msdk_printf(MSDK_STRING("Timeout %d seconds has been set to all sessions\n"), m_InputParamsArray[i].nTimeout);
            for (mfxU32 j = 0; j < m_InputParamsArray.size(); j++)
            {
                m_InputParamsArray[j].nTimeout = m_InputParamsArray[i].nTimeout;
            }
        }

        // All sessions have to know if robust mode enabled
        if (m_InputParamsArray[i].bRobustFlag)
        {
            for (mfxU32 j = 0; j < m_InputParamsArray.size(); j++)
            {
                m_InputParamsArray[j].bRobustFlag = m_InputParamsArray[i].bRobustFlag;
            }
        }

        if (Source == m_InputParamsArray[i].eMode)
        {
            if (m_InputParamsArray[i].nAsyncDepth < minAsyncDepth)
            {
                minAsyncDepth = m_InputParamsArray[i].nAsyncDepth;
            }
            // topology definition
            if (!IsSinkPresence)
            {
                PrintError(MSDK_STRING("Error in par file. Decode source session must be declared BEFORE encode sinks \n"));
                return MFX_ERR_UNSUPPORTED;
            }
            IsSourcePresence = true;

            if (IsFirstInTopology)
            {
                if (m_InputParamsArray[i].bIsJoin)
                    IsHeterSessionJoin = true;
                else
                    IsHeterSessionJoin = false;
            }
            else
            {
                if (m_InputParamsArray[i].bIsJoin && !IsHeterSessionJoin)
                {
                    PrintError(MSDK_STRING("Error in par file. All heterogeneous sessions must be joined \n"));
                    return MFX_ERR_UNSUPPORTED;
                }
                if (!m_InputParamsArray[i].bIsJoin && IsHeterSessionJoin)
                {
                    PrintError(MSDK_STRING("Error in par file. All heterogeneous sessions must be NOT joined \n"));
                    return MFX_ERR_UNSUPPORTED;
                }
            }

            if (IsFirstInTopology)
                IsFirstInTopology = false;

        }
        else if (Sink == m_InputParamsArray[i].eMode)
        {
            minAsyncDepth = m_InputParamsArray[i].nAsyncDepth;
            IsSinkPresence = true;

            if (IsFirstInTopology)
            {
                if (m_InputParamsArray[i].bIsJoin)
                    IsHeterSessionJoin = true;
                else
                    IsHeterSessionJoin = false;
            }
            else
            {
                if (m_InputParamsArray[i].bIsJoin && !IsHeterSessionJoin)
                {
                    PrintError(MSDK_STRING("Error in par file. All heterogeneous sessions must be joined \n"));
                    return MFX_ERR_UNSUPPORTED;
                }
                if (!m_InputParamsArray[i].bIsJoin && IsHeterSessionJoin)
                {
                    PrintError(MSDK_STRING("Error in par file. All heterogeneous sessions must be NOT joined \n"));
                    return MFX_ERR_UNSUPPORTED;
                }
            }

            if (IsFirstInTopology)
                IsFirstInTopology = false;
        }
        if (MFX_IMPL_SOFTWARE != m_InputParamsArray[i].libType)
        {
            // TODO: can we avoid ifdef and use MFX_IMPL_VIA_VAAPI?
#if defined(LIBVA_SUPPORT)
            m_eDevType = MFX_HANDLE_VA_DISPLAY;
#endif
        }
    }

    if (bUseExternalAllocator)
    {
        for(mfxU32 i = 0; i < m_InputParamsArray.size(); i++)
        {
            m_InputParamsArray[i].bUseOpaqueMemory = false;
        }
        msdk_printf(MSDK_STRING("External allocator will be used as some cmd line paremeters request it.\n"));
    }

    // Async depth between inter-sessions should be equal to the minimum async depth of all these sessions.
    for (mfxU32 i = 0; i < m_InputParamsArray.size(); i++)
    {
        if ((m_InputParamsArray[i].eMode == Source) || (m_InputParamsArray[i].eMode == Sink))
        {
            m_InputParamsArray[i].nAsyncDepth = minAsyncDepth;

            //--- If at least one of inter-session is not using opaque memory, all of them should not use it
            if(!areAllInterSessionsOpaque)
            {
                m_InputParamsArray[i].bUseOpaqueMemory=false;
            }
        }
    }

    if(!areAllInterSessionsOpaque)
    {
        msdk_printf(MSDK_STRING("Some inter-sessions do not use opaque memory (possibly because of -o::raw).\nOpaque memory in all inter-sessions is disabled.\n"));
    }

    if (IsSinkPresence && !IsSourcePresence)
    {
        PrintError(MSDK_STRING("Error: Sink must be defined"));
        return MFX_ERR_UNSUPPORTED;
    }

    if(bSingleTexture)
    {
        bool showWarning = false;
        for (mfxU32 j = 0; j < m_InputParamsArray.size(); j++)
        {
            if (!m_InputParamsArray[j].bSingleTexture)
            {
                showWarning = true;
            }
            m_InputParamsArray[j].bSingleTexture = true;
        }
        if (showWarning)
        {
            msdk_printf(MSDK_STRING("WARNING: At least one session has -single_texture_d3d11 option, all other sessions are modified to have this setting enabled al well.\n"));
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus Launcher::VerifyCrossSessionsOptions()

mfxStatus Launcher::CreateSafetyBuffers()
{
    SafetySurfaceBuffer* pBuffer     = NULL;
    SafetySurfaceBuffer* pPrevBuffer = NULL;

    for (mfxU32 i = 0; i < m_InputParamsArray.size(); i++)
    {
        /* this is for 1 to N case*/
        if ((Source == m_InputParamsArray[i].eMode) &&
            (Native == m_InputParamsArray[0].eModeExt))
        {
            pBuffer = new SafetySurfaceBuffer(pPrevBuffer);
            pPrevBuffer = pBuffer;
            m_pBufferArray.push_back(pBuffer);
        }

        /* And N_to_1 case: composition should be enabled!
         * else it is logic error */
        if ( (Source != m_InputParamsArray[i].eMode) &&
             ( (VppComp     == m_InputParamsArray[0].eModeExt) ||
               (VppCompOnly == m_InputParamsArray[0].eModeExt) ) )
        {
            pBuffer = new SafetySurfaceBuffer(pPrevBuffer);
            pPrevBuffer = pBuffer;
            m_pBufferArray.push_back(pBuffer);
        }
    }
    return MFX_ERR_NONE;

} // mfxStatus Launcher::CreateSafetyBuffers

void Launcher::Close()
{
    while(m_pSessionArray.size())
    {
        delete m_pSessionArray[m_pSessionArray.size()-1];
        m_pSessionArray[m_pSessionArray.size() - 1] = NULL;
        m_pSessionArray.pop_back();
    }

    while(m_pAllocArray.size())
    {
        delete m_pAllocArray[m_pAllocArray.size()-1];
        m_pAllocArray[m_pAllocArray.size() - 1] = NULL;
        m_pAllocArray.pop_back();
    }

    while(m_pBufferArray.size())
    {
        delete m_pBufferArray[m_pBufferArray.size()-1];
        m_pBufferArray[m_pBufferArray.size() - 1] = NULL;
        m_pBufferArray.pop_back();
    }

    while(m_pExtBSProcArray.size())
    {
        delete m_pExtBSProcArray[m_pExtBSProcArray.size() - 1];
        m_pExtBSProcArray[m_pExtBSProcArray.size() - 1] = NULL;
        m_pExtBSProcArray.pop_back();
    }
} // void Launcher::Close()

int main(int argc, char *argv[])
{
    mfxStatus sts;
    Launcher transcode;
    if (argc < 2)
    {
        msdk_printf(MSDK_STRING("[ERROR] Command line is empty. Use -? for getting help on available options.\n"));
        return 0;
    }

    sts = transcode.Init(argc, argv);
    if(sts == MFX_WRN_OUT_OF_RANGE)
    {
        // There's no error in parameters parsing, but we should not continue further. For instance, in case of -? option
        return MFX_ERR_NONE;
    }

    fflush(stdout);
    fflush(stderr);

    MSDK_CHECK_STATUS(sts, "transcode.Init failed");

    transcode.Run();

    sts = transcode.ProcessResult();
    fflush(stdout);
    fflush(stderr);
    MSDK_CHECK_STATUS(sts, "transcode.ProcessResult failed");

    return 0;
}

