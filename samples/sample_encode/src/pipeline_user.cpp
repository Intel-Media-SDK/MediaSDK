/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
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

#include "pipeline_user.h"
#include "sysmem_allocator.h"

mfxStatus CUserPipeline::InitRotateParam(sInputParams *pInParams)
{
    MSDK_CHECK_POINTER(pInParams, MFX_ERR_NULL_PTR);

    MSDK_ZERO_MEMORY(m_pluginVideoParams);
    m_pluginVideoParams.AsyncDepth = pInParams->nAsyncDepth; // the maximum number of tasks that can be submitted before any task execution finishes
    m_pluginVideoParams.vpp.In.FourCC = MFX_FOURCC_NV12;
    m_pluginVideoParams.vpp.In.Width = m_pluginVideoParams.vpp.In.CropW = pInParams->nWidth;
    m_pluginVideoParams.vpp.In.Height = m_pluginVideoParams.vpp.In.CropH = pInParams->nHeight;
    m_pluginVideoParams.vpp.Out.FourCC = MFX_FOURCC_NV12;
    m_pluginVideoParams.vpp.Out.Width = m_pluginVideoParams.vpp.Out.CropW = pInParams->nWidth;
    m_pluginVideoParams.vpp.Out.Height = m_pluginVideoParams.vpp.Out.CropH = pInParams->nHeight;
    if (pInParams->memType != SYSTEM_MEMORY)
        m_pluginVideoParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    m_RotateParams.Angle = pInParams->nRotationAngle;

    return MFX_ERR_NONE;
}

mfxStatus CUserPipeline::AllocFrames()
{
    MSDK_CHECK_POINTER(m_pmfxENC, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameAllocRequest EncRequest, RotateRequest;

    mfxU16 nEncSurfNum = 0; // number of frames at encoder input (rotate output)
    mfxU16 nRotateSurfNum = 0; // number of frames at rotate input

    MSDK_ZERO_MEMORY(EncRequest);

    sts = m_pmfxENC->QueryIOSurf(&m_mfxEncParams, &EncRequest);
    MSDK_CHECK_STATUS(sts, "m_pmfxENC->QueryIOSurf failed");

    if (EncRequest.NumFrameSuggested < m_mfxEncParams.AsyncDepth)
        return MFX_ERR_MEMORY_ALLOC;

    nEncSurfNum = EncRequest.NumFrameSuggested;

    // The number of surfaces for plugin input - so that plugin can work at async depth = m_nAsyncDepth
    nRotateSurfNum = MSDK_MAX(m_mfxEncParams.AsyncDepth, m_nMemBuffer);

    // If surfaces are shared by 2 components, c1 and c2. NumSurf = c1_out + c2_in - AsyncDepth + 1
    nEncSurfNum += nRotateSurfNum - m_mfxEncParams.AsyncDepth + 1;

    // prepare allocation requests
    EncRequest.NumFrameSuggested = EncRequest.NumFrameMin = nEncSurfNum;
    RotateRequest.NumFrameSuggested = RotateRequest.NumFrameMin = nRotateSurfNum;

    mfxU16 mem_type = MFX_MEMTYPE_EXTERNAL_FRAME;
    mem_type |= (SYSTEM_MEMORY == m_memType) ?
        (mfxU16)MFX_MEMTYPE_SYSTEM_MEMORY
        :(mfxU16)MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;

    EncRequest.Type = RotateRequest.Type = mem_type;

    EncRequest.Type |= MFX_MEMTYPE_FROM_ENCODE;
    RotateRequest.Type |= MFX_MEMTYPE_FROM_VPPOUT; // THIS IS A WORKAROUND, NEED TO ADJUST ALLOCATOR

    MSDK_MEMCPY_VAR(EncRequest.Info, &(m_mfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
    MSDK_MEMCPY_VAR(RotateRequest.Info, &(m_pluginVideoParams.vpp.In), sizeof(mfxFrameInfo));

    // alloc frames for encoder input
    sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &EncRequest, &m_EncResponse);
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");

    // alloc frames for rotate input
    sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &(RotateRequest), &m_PluginResponse);
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");

    // prepare mfxFrameSurface1 array for components
    m_pEncSurfaces = new mfxFrameSurface1 [nEncSurfNum];
    MSDK_CHECK_POINTER(m_pEncSurfaces, MFX_ERR_MEMORY_ALLOC);
    m_pPluginSurfaces = new mfxFrameSurface1 [nRotateSurfNum];
    MSDK_CHECK_POINTER(m_pPluginSurfaces, MFX_ERR_MEMORY_ALLOC);

    for (int i = 0; i < nEncSurfNum; i++)
    {
        MSDK_ZERO_MEMORY(m_pEncSurfaces[i]);
        MSDK_MEMCPY_VAR(m_pEncSurfaces[i].Info, &(m_mfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
        if (SYSTEM_MEMORY != m_memType)
        {
            // external allocator used - provide just MemIds
            m_pEncSurfaces[i].Data.MemId = m_EncResponse.mids[i];
        }
        else
        {
            sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, m_EncResponse.mids[i], &(m_pEncSurfaces[i].Data));
            MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Lock failed");
        }
    }

    for (int i = 0; i < nRotateSurfNum; i++)
    {
        MSDK_ZERO_MEMORY(m_pPluginSurfaces[i]);
        MSDK_MEMCPY_VAR(m_pPluginSurfaces[i].Info, &(m_pluginVideoParams.vpp.In), sizeof(mfxFrameInfo));
        if (SYSTEM_MEMORY != m_memType)
        {
            // external allocator used - provide just MemIds
            m_pPluginSurfaces[i].Data.MemId = m_PluginResponse.mids[i];
        }
        else
        {
            sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, m_PluginResponse.mids[i], &(m_pPluginSurfaces[i].Data));
            MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Lock failed");
        }
    }

    return MFX_ERR_NONE;
}

void CUserPipeline::DeleteFrames()
{
    MSDK_SAFE_DELETE_ARRAY(m_pPluginSurfaces);

    CEncodingPipeline::DeleteFrames();
}

CUserPipeline::CUserPipeline() : CEncodingPipeline()
{
    m_pPluginSurfaces = NULL;
    m_PluginModule = NULL;
    m_pusrPlugin = NULL;
    MSDK_ZERO_MEMORY(m_PluginResponse);
    MSDK_ZERO_MEMORY(m_pluginVideoParams);
    MSDK_ZERO_MEMORY(m_RotateParams);
    m_MVCflags = MVC_DISABLED;
}

CUserPipeline::~CUserPipeline()
{
    Close();
}

mfxStatus CUserPipeline::Init(sInputParams *pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    m_PluginModule = msdk_so_load(pParams->strPluginDLLPath);
    MSDK_CHECK_POINTER(m_PluginModule, MFX_ERR_NOT_FOUND);

    PluginModuleTemplate::fncCreateGenericPlugin pCreateFunc = (PluginModuleTemplate::fncCreateGenericPlugin)msdk_so_get_addr(m_PluginModule, "mfxCreateGenericPlugin");

    MSDK_CHECK_POINTER(pCreateFunc, MFX_ERR_NOT_FOUND);

    m_pusrPlugin = (*pCreateFunc)();
    MSDK_CHECK_POINTER(m_pusrPlugin, MFX_ERR_NOT_FOUND);

    // prepare input file reader
    sts = m_FileReader.Init(pParams->InputFiles,
                            pParams->FileInputFourCC );
    MSDK_CHECK_STATUS(sts, "m_FileReader.Init failed");

    // set memory type
    m_memType = pParams->memType;
    m_nMemBuffer = pParams->nMemBuf;
    m_nTimeout = pParams->nTimeout;
    m_bCutOutput = !pParams->bUncut;

    // prepare output file writer
    sts = InitFileWriters(pParams);
    MSDK_CHECK_STATUS(sts, "InitFileWriters failed");

    mfxIMPL impl = pParams->bUseHWLib ? MFX_IMPL_HARDWARE : MFX_IMPL_SOFTWARE;

    // if d3d11 surfaces are used ask the library to run acceleration through D3D11
    // feature may be unsupported due to OS or MSDK API version
    if (D3D11_MEMORY == pParams->memType)
        impl |= MFX_IMPL_VIA_D3D11;

    mfxVersion min_version;
    mfxVersion version;     // real API version with which library is initialized

    // we set version to 1.0 and later we will query actual version of the library which will got leaded
    min_version.Major = 1;
    min_version.Minor = 0;

    // create a session for the second vpp and encode
    sts = m_mfxSession.Init(impl, &min_version);
    MSDK_CHECK_STATUS(sts, "m_mfxSession.Init failed");

    sts = MFXQueryVersion(m_mfxSession , &version); // get real API version of the loaded library
    MSDK_CHECK_STATUS(sts, "MFXQueryVersion failed");

    if (CheckVersion(&version, MSDK_FEATURE_PLUGIN_API)) {
       // we check if codec is distributed as a mediasdk plugin and load it if yes
        // else if codec is not in the list of mediasdk plugins, we assume, that it is supported inside mediasdk library

        // in case of HW library (-hw key) we will firstly try to load HW plugin
        // in case of failure - we will try SW one
        mfxIMPL impl2 = pParams->bUseHWLib ? MFX_IMPL_HARDWARE : MFX_IMPL_SOFTWARE;

        if (AreGuidsEqual(MSDK_PLUGINGUID_NULL,pParams->pluginParams.pluginGuid))
        {
            pParams->pluginParams.pluginGuid = msdkGetPluginUID(impl2, MSDK_VENCODE, pParams->CodecId);
        }
        if (AreGuidsEqual(pParams->pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL) && impl2 == MFX_IMPL_HARDWARE)
            pParams->pluginParams.pluginGuid = msdkGetPluginUID(MFX_IMPL_SOFTWARE, MSDK_VENCODE, pParams->CodecId);
        if (!AreGuidsEqual(pParams->pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
        {
            m_pPlugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, m_mfxSession, pParams->pluginParams.pluginGuid, 1));
            if (m_pPlugin.get() == NULL) sts = MFX_ERR_UNSUPPORTED;
        }
    }

    // create encoder
    m_pmfxENC = new MFXVideoENCODE(m_mfxSession);
    MSDK_CHECK_POINTER(m_pmfxENC, MFX_ERR_MEMORY_ALLOC);

    sts = InitMfxEncParams(pParams);
    MSDK_CHECK_STATUS(sts, "InitMfxEncParams failed");

    sts = InitRotateParam(pParams);
    MSDK_CHECK_STATUS(sts, "InitRotateParam failed");

    // create and init frame allocator
    sts = CreateAllocator();
    MSDK_CHECK_STATUS(sts, "CreateAllocator failed");

    sts = ResetMFXComponents(pParams);
    MSDK_CHECK_STATUS(sts, "ResetMFXComponents failed");
    // register plugin callbacks in Media SDK
    mfxPlugin plg = make_mfx_plugin_adapter(m_pusrPlugin);
    sts = MFXVideoUSER_Register(m_mfxSession, 0, &plg);
    MSDK_CHECK_STATUS(sts, "MFXVideoUSER_Register failed");

    // need to call Init after registration because mfxCore interface is needed
    sts = m_pusrPlugin->Init(&m_pluginVideoParams);
    MSDK_CHECK_STATUS(sts, "m_pusrPlugin->Init failed");

    sts = m_pusrPlugin->SetAuxParams(&m_RotateParams, sizeof(m_RotateParams));
    MSDK_CHECK_STATUS(sts, "m_pusrPlugin->SetAuxParams failed");

    return MFX_ERR_NONE;
}

void CUserPipeline::Close()
{
    MFXVideoUSER_Unregister(m_mfxSession, 0);

    CEncodingPipeline::Close();

    MSDK_SAFE_DELETE(m_pusrPlugin);
    if (m_PluginModule)
    {
        msdk_so_free(m_PluginModule);
        m_PluginModule = NULL;
    }
}

mfxStatus CUserPipeline::ResetMFXComponents(sInputParams* pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(m_pmfxENC, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = MFX_ERR_NONE;

    sts = m_pmfxENC->Close();
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_STATUS(sts, "m_pmfxENC->Close failed");

    // free allocated frames
    DeleteFrames();

    m_TaskPool.Close();

    sts = AllocFrames();
    MSDK_CHECK_STATUS(sts, "AllocFrames failed");

    sts = m_pmfxENC->Init(&m_mfxEncParams);
    MSDK_CHECK_STATUS(sts, "m_pmfxENC->Init failed");

    mfxU32 nEncodedDataBufferSize = m_mfxEncParams.mfx.FrameInfo.Width * m_mfxEncParams.mfx.FrameInfo.Height * 4;
    sts = m_TaskPool.Init(&m_mfxSession, m_FileWriters.first, m_mfxEncParams.AsyncDepth, nEncodedDataBufferSize, m_FileWriters.second);
    MSDK_CHECK_STATUS(sts, "m_TaskPool.Init failed");

    sts = FillBuffers();
    MSDK_CHECK_STATUS(sts, "FillBuffers failed");

    return MFX_ERR_NONE;
}

mfxStatus CUserPipeline::Run()
{
    m_statOverall.StartTimeMeasurement();
    MSDK_CHECK_POINTER(m_pmfxENC, MFX_ERR_NOT_INITIALIZED);

    mfxStatus sts = MFX_ERR_NONE;

    sTask *pCurrentTask = NULL; // a pointer to the current task
    mfxU16 nEncSurfIdx = 0; // index of free surface for encoder input
    mfxU16 nRotateSurfIdx = 0; // ~ for rotation plugin input

    mfxSyncPoint RotateSyncPoint = NULL; // ~ with rotation plugin call

    sts = MFX_ERR_NONE;

    // main loop, preprocessing and encoding
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts)
    {
        // get a pointer to a free task (bit stream and sync point for encoder)
        sts = GetFreeTask(&pCurrentTask);
        MSDK_BREAK_ON_ERROR(sts);

        if (m_nMemBuffer)
        {
            nRotateSurfIdx = m_nFramesRead % m_nMemBuffer;
        }
        else
        {
            nRotateSurfIdx = GetFreeSurface(m_pPluginSurfaces, m_PluginResponse.NumFrameActual);
        }
        MSDK_CHECK_ERROR(nRotateSurfIdx, MSDK_INVALID_SURF_IDX, MFX_ERR_MEMORY_ALLOC);

        m_statFile.StartTimeMeasurement();
        sts = LoadNextFrame(&m_pPluginSurfaces[nRotateSurfIdx]);
        m_statFile.StopTimeMeasurement();
        if ( (MFX_ERR_MORE_DATA == sts) && !m_bTimeOutExceed)
            continue;

        MSDK_BREAK_ON_ERROR(sts);

        nEncSurfIdx = GetFreeSurface(m_pEncSurfaces, m_EncResponse.NumFrameActual);
        MSDK_CHECK_ERROR(nEncSurfIdx, MSDK_INVALID_SURF_IDX, MFX_ERR_MEMORY_ALLOC);

        // rotation
        for(;;)
        {
            mfxHDL h1, h2;
            h1 = &m_pPluginSurfaces[nRotateSurfIdx];
            h2 = &m_pEncSurfaces[nEncSurfIdx];
            sts = MFXVideoUSER_ProcessFrameAsync(m_mfxSession, &h1, 1, &h2, 1, &RotateSyncPoint);

            if (MFX_WRN_DEVICE_BUSY == sts)
            {
                MSDK_SLEEP(1); // just wait and then repeat the same call
            }
            else
            {
                break;
            }
        }
        MSDK_BREAK_ON_ERROR(sts);

        // save the id of preceding rotate task which will produce input data for the encode task
        if (RotateSyncPoint)
        {
            pCurrentTask->DependentVppTasks.push_back(RotateSyncPoint);
            RotateSyncPoint = NULL;
        }

        for (;;)
        {
            InsertIDR(m_bInsertIDR);
            sts = m_pmfxENC->EncodeFrameAsync(&m_encCtrl, &m_pEncSurfaces[nEncSurfIdx], &pCurrentTask->mfxBS, &pCurrentTask->EncSyncP);
            m_bInsertIDR = false;

            if (MFX_ERR_NONE < sts && !pCurrentTask->EncSyncP) // repeat the call if warning and no output
            {
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1); // wait if device is busy
            }
            else if (MFX_ERR_NONE < sts && pCurrentTask->EncSyncP)
            {
                sts = MFX_ERR_NONE; // ignore warnings if output is available
                break;
            }
            else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
            {
                sts = AllocateSufficientBuffer(&pCurrentTask->mfxBS);
                MSDK_CHECK_STATUS(sts, "AllocateSufficientBuffer failed");
            }
            else
            {
                break;
            }
        }
    }

    // means that the input file has ended, need to go to buffering loops
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    // exit in case of other errors
    MSDK_CHECK_STATUS(sts, "m_pmfENC->EncodeFrameAsync failed");

    // rotate plugin doesn't buffer frames
    // loop to get buffered frames from encoder
    while (MFX_ERR_NONE <= sts)
    {
        // get a free task (bit stream and sync point for encoder)
        sts = GetFreeTask(&pCurrentTask);
        MSDK_BREAK_ON_ERROR(sts);

        for (;;)
        {
            InsertIDR(m_bInsertIDR);
            sts = m_pmfxENC->EncodeFrameAsync(&m_encCtrl, NULL, &pCurrentTask->mfxBS, &pCurrentTask->EncSyncP);
            m_bInsertIDR = false;

            if (MFX_ERR_NONE < sts && !pCurrentTask->EncSyncP) // repeat the call if warning and no output
            {
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1); // wait if device is busy
            }
            else if (MFX_ERR_NONE < sts && pCurrentTask->EncSyncP)
            {
                sts = MFX_ERR_NONE; // ignore warnings if output is available
                break;
            }
            else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
            {
                sts = AllocateSufficientBuffer(&pCurrentTask->mfxBS);
                MSDK_CHECK_STATUS(sts, "AllocateSufficientBuffer failed");
            }
            else
            {
                break;
            }
        }
        MSDK_BREAK_ON_ERROR(sts);
    }

    // MFX_ERR_MORE_DATA is the correct status to exit buffering loop with
    // indicates that there are no more buffered frames
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    // exit in case of other errors
    MSDK_CHECK_STATUS(sts, "m_pmfxENC->EncodeFrameAsync failed");

    // synchronize all tasks that are left in task pool
    while (MFX_ERR_NONE == sts)
    {
        sts = m_TaskPool.SynchronizeFirstTask();
    }

    // MFX_ERR_NOT_FOUND is the correct status to exit the loop with,
    // EncodeFrameAsync and SyncOperation don't return this status
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_FOUND);
    // report any errors that occurred in asynchronous part
    MSDK_CHECK_STATUS(sts, "m_TaskPool.SynchronizeFirstTask failed");
    m_statOverall.StopTimeMeasurement();
    return sts;
}

mfxStatus CUserPipeline::FillBuffers()
{
    if (m_nMemBuffer)
    {
        for (mfxU32 i = 0; i < m_nMemBuffer; i++)
        {
            mfxFrameSurface1* surface = &m_pPluginSurfaces[i];

            mfxStatus sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, surface->Data.MemId, &surface->Data);
            MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Lock failed");
            sts = m_FileReader.LoadNextFrame(surface);
            MSDK_CHECK_STATUS(sts, "m_FileReader.LoadNextFrame failed");
            sts = m_pMFXAllocator->Unlock(m_pMFXAllocator->pthis, surface->Data.MemId, &surface->Data);
            MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Unlock failed");
        }
    }
    return MFX_ERR_NONE;
}

void CUserPipeline::PrintInfo()
{
    msdk_printf(MSDK_STRING("\nPipeline with rotation plugin"));
    msdk_printf(MSDK_STRING("\nNOTE: Some of command line options may have been ignored as non-supported for this pipeline. For details see readme-encode.rtf.\n\n"));

    CEncodingPipeline::PrintInfo();
}
