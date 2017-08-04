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

#include "pipeline_region_encode.h"
#include "sysmem_allocator.h"

#if D3D_SURFACES_SUPPORT
#include "d3d_allocator.h"
#include "d3d11_allocator.h"

#include "d3d_device.h"
#include "d3d11_device.h"
#endif

#ifdef LIBVA_SUPPORT
#include "vaapi_allocator.h"
#include "vaapi_device.h"
#endif

#include "plugin_loader.h"

mfxStatus CResourcesPool::GetFreeTask(int resourceNum,sTask **ppTask)
{
    // get a pointer to a free task (bit stream and sync point for encoder)
    mfxStatus sts = m_resources[resourceNum].TaskPool.GetFreeTask(ppTask);
    if (MFX_ERR_NOT_FOUND == sts)
    {
        // We should syncrhonize every first task in all task pools to write regions (slices) into destination in correct order
        for(int i=0; i < size; i++)
        {
            sts = m_resources[i].TaskPool.SynchronizeFirstTask();
            MSDK_CHECK_STATUS(sts, "m_resources[i].TaskPool.SynchronizeFirstTask failed");
        }

        // try again
        sts = m_resources[resourceNum].TaskPool.GetFreeTask(ppTask);
    }

    return sts;
}

mfxStatus CResourcesPool::Init(int sz,mfxIMPL impl, mfxVersion *pVer)
{
    MSDK_CHECK_NOT_EQUAL(m_resources, NULL , MFX_ERR_INVALID_HANDLE);
    this->size= sz;
    m_resources = new CMSDKResource[sz];
    for (int i = 0; i < sz; i++)
    {
        mfxStatus sts = m_resources[i].Session.Init(impl, pVer);
        MSDK_CHECK_STATUS(sts, "m_resources[i].Session.Init failed");
    }
    return MFX_ERR_NONE;
}

mfxStatus CResourcesPool::InitTaskPools(CSmplBitstreamWriter* pWriter, mfxU32 nPoolSize, mfxU32 nBufferSize, CSmplBitstreamWriter *pOtherWriter)
{
    for (int i = 0; i < size; i++)
    {
        mfxStatus sts = m_resources[i].TaskPool.Init(&m_resources[i].Session,pWriter,nPoolSize,nBufferSize,pOtherWriter);
        MSDK_CHECK_STATUS(sts, "m_resources[i].TaskPool.Init failed");
    }
    return MFX_ERR_NONE;
}

mfxStatus CResourcesPool::CreateEncoders()
{
    for (int i = 0; i < size; i++)
    {
        MFXVideoENCODE* pEnc = new MFXVideoENCODE(m_resources[i].Session);
        MSDK_CHECK_POINTER(pEnc, MFX_ERR_MEMORY_ALLOC);
        m_resources[i].pEncoder=pEnc;
    }
    return MFX_ERR_NONE;
}

mfxStatus CResourcesPool::CreatePlugins(mfxPluginUID pluginGUID, mfxChar* pluginPath)
{
    for (int i = 0; i < size; i++)
    {
        MFXPlugin* pPlugin = pluginPath ?
            LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, m_resources[i].Session, pluginGUID, 1, pluginPath, (mfxU32)msdk_strnlen(pluginPath,1024)):
            LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, m_resources[i].Session, pluginGUID, 1);

        if (pPlugin == NULL)
        {
            return MFX_ERR_UNSUPPORTED;
        }
        m_resources[i].pPlugin=pPlugin;
    }
    return MFX_ERR_NONE;
}

void CResourcesPool::CloseAndDeleteEverything()
{
    for(int i = 0; i < size; i++)
    {
        m_resources[i].TaskPool.Close();
        MSDK_SAFE_DELETE(m_resources[i].pEncoder);
        MSDK_SAFE_DELETE(m_resources[i].pPlugin);
        m_resources[i].Session.Close();
    }
}

//------------------- Pipeline -----------------------------------------------------------------------------

mfxStatus CRegionEncodingPipeline::InitMfxEncParams(sInputParams *pInParams)
{
    CEncodingPipeline::InitMfxEncParams(pInParams);
    m_mfxEncParams.mfx.TargetKbps              = pInParams->nBitRate / pInParams->nNumSlice; // in Kbps

    if (m_mfxEncParams.mfx.NumSlice > 1 && MFX_CODEC_HEVC == pInParams->CodecId)
    {
        m_HEVCRegion.RegionType = MFX_HEVC_REGION_SLICE;
        m_HEVCRegion.RegionId   = 0; // will be rewrited before call of encoder Init
        m_EncExtParams.push_back((mfxExtBuffer *)&m_HEVCRegion);
    }

    if (!m_EncExtParams.empty())
    {
        m_mfxEncParams.ExtParam = &m_EncExtParams[0]; // vector is stored linearly in memory
        m_mfxEncParams.NumExtParam = (mfxU16)m_EncExtParams.size();
    }

    // JPEG encoder settings overlap with other encoders settings in mfxInfoMFX structure
    if (MFX_CODEC_JPEG == pInParams->CodecId)
    {
        m_mfxEncParams.mfx.Interleaved = 1;
        m_mfxEncParams.mfx.Quality = pInParams->nQuality;
        m_mfxEncParams.mfx.RestartInterval = 0;
        MSDK_ZERO_MEMORY(m_mfxEncParams.mfx.reserved5);
    }

    m_mfxEncParams.AsyncDepth = 1;
    m_mfxEncParams.mfx.GopRefDist = 1;

    return MFX_ERR_NONE;
}

mfxStatus CRegionEncodingPipeline::CreateAllocator()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (D3D9_MEMORY == m_memType || D3D11_MEMORY == m_memType)
    {
#if D3D_SURFACES_SUPPORT
        sts = CreateHWDevice();
        MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");

        mfxHDL hdl = NULL;
        mfxHandleType hdl_t =
        #if MFX_D3D11_SUPPORT
            D3D11_MEMORY == m_memType ? MFX_HANDLE_D3D11_DEVICE :
        #endif // #if MFX_D3D11_SUPPORT
            MFX_HANDLE_D3D9_DEVICE_MANAGER;

        sts = m_hwdev->GetHandle(hdl_t, &hdl);
        MSDK_CHECK_STATUS(sts, "m_hwdev->GetHandle failed");

        // handle is needed for HW library only
        mfxIMPL impl = 0;
        m_resources[0].Session.QueryIMPL(&impl);
        if (impl != MFX_IMPL_SOFTWARE)
        {
            for (int i = 0; i < m_resources.GetSize(); i++) {
                sts = m_resources[i].Session.SetHandle(hdl_t, hdl);
                MSDK_CHECK_STATUS(sts, "m_resources[i].Session.SetHandle failed");
            }
        }

        // create D3D allocator
#if MFX_D3D11_SUPPORT
        if (D3D11_MEMORY == m_memType)
        {
            m_pMFXAllocator = new D3D11FrameAllocator;
            MSDK_CHECK_POINTER(m_pMFXAllocator, MFX_ERR_MEMORY_ALLOC);

            D3D11AllocatorParams *pd3dAllocParams = new D3D11AllocatorParams;
            MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
            pd3dAllocParams->pDevice = reinterpret_cast<ID3D11Device *>(hdl);

            m_pmfxAllocatorParams = pd3dAllocParams;
        }
        else
#endif // #if MFX_D3D11_SUPPORT
        {
            m_pMFXAllocator = new D3DFrameAllocator;
            MSDK_CHECK_POINTER(m_pMFXAllocator, MFX_ERR_MEMORY_ALLOC);

            D3DAllocatorParams *pd3dAllocParams = new D3DAllocatorParams;
            MSDK_CHECK_POINTER(pd3dAllocParams, MFX_ERR_MEMORY_ALLOC);
            pd3dAllocParams->pManager = reinterpret_cast<IDirect3DDeviceManager9 *>(hdl);

            m_pmfxAllocatorParams = pd3dAllocParams;
        }

        /* In case of video memory we must provide MediaSDK with external allocator
        thus we demonstrate "external allocator" usage model.
        Call SetAllocator to pass allocator to Media SDK */
        for (int i = 0; i < m_resources.GetSize(); i++) {
            sts = m_resources[i].Session.SetFrameAllocator(m_pMFXAllocator);
            MSDK_CHECK_STATUS(sts, "m_resources[i].Session.SetFrameAllocator failed");
        }

        m_bExternalAlloc = true;
#endif
#ifdef LIBVA_SUPPORT
        sts = CreateHWDevice();
        MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");
        /* It's possible to skip failed result here and switch to SW implementation,
        but we don't process this way */

        mfxHDL hdl = NULL;
        sts = m_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);

        for (int i = 0; i < m_resources.GetSize(); i++) {
            // provide device manager to MediaSDK
            sts = m_resources[i].Session.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
            MSDK_CHECK_STATUS(sts, "m_resources[i].Session.SetHandle failed");

            // create VAAPI allocator
            m_pMFXAllocator = new vaapiFrameAllocator;
            MSDK_CHECK_POINTER(m_pMFXAllocator, MFX_ERR_MEMORY_ALLOC);

            vaapiAllocatorParams *p_vaapiAllocParams = new vaapiAllocatorParams;
            MSDK_CHECK_POINTER(p_vaapiAllocParams, MFX_ERR_MEMORY_ALLOC);

            p_vaapiAllocParams->m_dpy = (VADisplay)hdl;
            m_pmfxAllocatorParams = p_vaapiAllocParams;

            /* In case of video memory we must provide MediaSDK with external allocator
            thus we demonstrate "external allocator" usage model.
            Call SetAllocator to pass allocator to mediasdk */
            sts = m_resources[i].Session.SetFrameAllocator(m_pMFXAllocator);
            MSDK_CHECK_STATUS(sts, "m_resources[i].Session.SetFrameAllocator failed");
        }
        m_bExternalAlloc = true;
#endif
    }
    else
    {
#ifdef LIBVA_SUPPORT
        //in case of system memory allocator we also have to pass MFX_HANDLE_VA_DISPLAY to HW library
        mfxIMPL impl;
        m_resources[0].Session.QueryIMPL(&impl);

        if(MFX_IMPL_HARDWARE == MFX_IMPL_BASETYPE(impl))
        {
            sts = CreateHWDevice();
            MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");

            mfxHDL hdl = NULL;
            sts = m_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
            for (int i = 0; i < m_resources.GetSize(); i++) {
                // provide device manager to MediaSDK
                sts = m_resources[i].Session.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
                MSDK_CHECK_STATUS(sts, "m_resources[i].Session.SetHandle failed");
            }
        }
#endif

        // create system memory allocator
        m_pMFXAllocator = new SysMemFrameAllocator;
        MSDK_CHECK_POINTER(m_pMFXAllocator, MFX_ERR_MEMORY_ALLOC);

        /* In case of system memory we demonstrate "no external allocator" usage model.
        We don't call SetAllocator, Media SDK uses internal allocator.
        We use system memory allocator simply as a memory manager for application*/
    }

    // initialize memory allocator
    sts = m_pMFXAllocator->Init(m_pmfxAllocatorParams);
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Init failed");

    return MFX_ERR_NONE;
}


CRegionEncodingPipeline::CRegionEncodingPipeline() : CEncodingPipeline()
{
    m_timeAll = 0;

    MSDK_ZERO_MEMORY(m_HEVCRegion);
    m_HEVCRegion.Header.BufferId = MFX_EXTBUFF_HEVC_REGION;
    m_HEVCRegion.Header.BufferSz = sizeof(m_HEVCRegion);
}

CRegionEncodingPipeline::~CRegionEncodingPipeline()
{
    Close();
}

mfxStatus CRegionEncodingPipeline::Init(sInputParams *pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    // prepare input file reader
    sts = m_FileReader.Init(pParams->InputFiles,
                            pParams->FileInputFourCC );
    MSDK_CHECK_STATUS(sts, "m_FileReader.Init failed");

    m_MVCflags = pParams->MVC_flags;

    // FileReader can convert yv12->nv12 without vpp
    m_InputFourCC = (pParams->FileInputFourCC == MFX_FOURCC_I420) ? MFX_FOURCC_NV12 : pParams->FileInputFourCC;

    sts = InitFileWriters(pParams);
    MSDK_CHECK_STATUS(sts, "InitFileWriters failed");

    m_timeAll = 0;
    if (pParams->nNumSlice == 0)
        pParams->nNumSlice = 1;

    mfxVersion min_version;
    mfxVersion version;     // real API version with which library is initialized

    // we set version to 1.0 and later we will query actual version of the library which will got leaded
    min_version.Major = 1;
    min_version.Minor = 0;

    // Init session
    if (pParams->bUseHWLib)
    {
        msdk_printf(MSDK_STRING("Hardware library is unsupported in Region Encoding mode\n"));
        return MFX_ERR_UNSUPPORTED;
    }
    else {
        sts = m_resources.Init(pParams->nNumSlice,MFX_IMPL_SOFTWARE, &min_version);
        MSDK_CHECK_STATUS(sts, "m_resources.Init failed");
    }


    sts = MFXQueryVersion(m_resources[0].Session, &version); // get real API version of the loaded library
    MSDK_CHECK_STATUS(sts, "MFXQueryVersion failed");

    if ((pParams->MVC_flags & MVC_ENABLED) != 0 && !CheckVersion(&version, MSDK_FEATURE_MVC)) {
        msdk_printf(MSDK_STRING("error: MVC is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;

    }
    if ((pParams->MVC_flags & MVC_VIEWOUTPUT) != 0 && !CheckVersion(&version, MSDK_FEATURE_MVC_VIEWOUTPUT)) {
        msdk_printf(MSDK_STRING("error: MVC Viewoutput is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }
    if ((pParams->CodecId == MFX_CODEC_JPEG) && !CheckVersion(&version, MSDK_FEATURE_JPEG_ENCODE)) {
        msdk_printf(MSDK_STRING("error: Jpeg is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }

    if ((pParams->nRateControlMethod == MFX_RATECONTROL_LA) && !CheckVersion(&version, MSDK_FEATURE_LOOK_AHEAD)) {
        msdk_printf(MSDK_STRING("error: Look ahead is not supported in the %d.%d API version\n"),
            version.Major, version.Minor);
        return MFX_ERR_UNSUPPORTED;
    }

    if (CheckVersion(&version, MSDK_FEATURE_PLUGIN_API)) {
        /* Here we actually define the following codec initialization scheme:
        *  1. If plugin path or guid is specified: we load user-defined plugin (example: HEVC encoder plugin)
        *  2. If plugin path not specified:
        *    2.a) we check if codec is distributed as a mediasdk plugin and load it if yes
        *    2.b) if codec is not in the list of mediasdk plugins, we assume, that it is supported inside mediasdk library
        */
        if (pParams->pluginParams.type == MFX_PLUGINLOAD_TYPE_FILE && msdk_strnlen(pParams->pluginParams.strPluginPath,sizeof(pParams->pluginParams.strPluginPath)))
        {
            m_pUserModule.reset(new MFXVideoUSER(m_resources[0].Session));
            sts = m_resources.CreatePlugins(pParams->pluginParams.pluginGuid,pParams->pluginParams.strPluginPath);
            MSDK_CHECK_STATUS(sts, "m_resources.CreatePlugins failed");
        }
        else
        {
            bool isDefaultPlugin = false;
            if (AreGuidsEqual(pParams->pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                mfxIMPL impl = pParams->bUseHWLib ? MFX_IMPL_HARDWARE : MFX_IMPL_SOFTWARE;
                pParams->pluginParams.pluginGuid = msdkGetPluginUID(impl, MSDK_VENCODE, pParams->CodecId);
                isDefaultPlugin = true;
            }
            if (!AreGuidsEqual(pParams->pluginParams.pluginGuid, MSDK_PLUGINGUID_NULL))
            {
                sts = m_resources.CreatePlugins(pParams->pluginParams.pluginGuid,NULL);
                MSDK_CHECK_STATUS(sts, "m_resources.CreatePlugins failed");
            }
            if(sts==MFX_ERR_UNSUPPORTED)
            {
                msdk_printf(isDefaultPlugin ?
                    MSDK_STRING("Default plugin cannot be loaded (possibly you have to define plugin explicitly)\n")
                    : MSDK_STRING("Explicitly specified plugin cannot be loaded.\n"));
            }
        }
        MSDK_CHECK_STATUS(sts, "m_resources.CreatePlugins failed");
    }

    // set memory type
    m_memType = pParams->memType;
    m_nMemBuffer = pParams->nMemBuf;
    m_nTimeout = pParams->nTimeout;

    // If output isn't specified work in performance mode and do not insert idr
    m_bCutOutput = pParams->dstFileBuff.size() ? !pParams->bUncut : false;

    // create and init frame allocator
    sts = CreateAllocator();
    MSDK_CHECK_STATUS(sts, "CreateAllocator failed");

    sts = InitMfxEncParams(pParams);
    MSDK_CHECK_STATUS(sts, "InitMfxEncParams failed");

    // MVC specific options
    if (MVC_ENABLED & m_MVCflags)
    {
        sts = AllocAndInitMVCSeqDesc();
        MSDK_CHECK_STATUS(sts, "AllocAndInitMVCSeqDesc failed");
    }

    // create encoder
    sts = m_resources.CreateEncoders();
    MSDK_CHECK_STATUS(sts, "m_resources.CreateEncoders failed");

    sts = ResetMFXComponents(pParams);
    MSDK_CHECK_STATUS(sts, "ResetMFXComponents failed");

    return MFX_ERR_NONE;
}

void CRegionEncodingPipeline::Close()
{
    if (m_FileWriters.first)
    {
        mfxU32 frameNum = m_resources.GetSize() ? m_FileWriters.first->m_nProcessedFramesNum / m_resources.GetSize() : 0;
        msdk_printf(MSDK_STRING("Frame number: %u   \r"), frameNum);
        msdk_printf(MSDK_STRING("\nEncode fps: %.2lf\n"), m_timeAll ? frameNum*((double)time_get_frequency())/m_timeAll : 0);
    }

    FreeMVCSeqDesc();

    DeleteFrames();

    m_resources.CloseAndDeleteEverything();

    m_FileReader.Close();
    FreeFileWriters();

    // allocator if used as external for MediaSDK must be deleted after SDK components
    DeleteAllocator();
}

mfxStatus CRegionEncodingPipeline::ResetMFXComponents(sInputParams* pParams)
{
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    for(int i = 0; i < m_resources.GetSize(); i++) {
        if (m_resources[i].pEncoder) {
            sts = m_resources[i].pEncoder->Close();
            MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
            MSDK_CHECK_STATUS(sts, "m_resources[i].pEncoder->Close failed");
        }
    }

    // free allocated frames
    DeleteFrames();

    for(int i = 0; i < m_resources.GetSize(); i++)
    {
        m_resources[i].TaskPool.Close();
    }

    sts = AllocFrames();
    MSDK_CHECK_STATUS(sts, "AllocFrames failed");

    for (int regId = 0; regId < m_resources.GetSize(); regId++) {
        if (m_resources.GetSize() > 1)
            m_HEVCRegion.RegionId = regId;

        sts = m_resources[regId].pEncoder->Init(&m_mfxEncParams);
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }

        MSDK_CHECK_STATUS(sts, "m_resources[regId].pEncoder->Init failed");
    }

    mfxU32 nEncodedDataBufferSize = m_mfxEncParams.mfx.FrameInfo.Width * m_mfxEncParams.mfx.FrameInfo.Height * 4;

    sts = m_resources.InitTaskPools(m_FileWriters.first, m_mfxEncParams.AsyncDepth, nEncodedDataBufferSize, m_FileWriters.second);
    MSDK_CHECK_STATUS(sts, "m_resources.InitTaskPools failed");

    sts = FillBuffers();
    MSDK_CHECK_STATUS(sts, "FillBuffers failed");

    return MFX_ERR_NONE;
}

mfxStatus CRegionEncodingPipeline::Run()
{
    mfxI64 timeCurStart=0;
    mfxI64 timeCurEnd=0;
    mfxI64 timeCurMax;
    mfxI64 nFrames = 0;

    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1* pSurf = NULL; // dispatching pointer

    sTask *pCurrentTask = NULL; // a pointer to the current task
    mfxU16 nEncSurfIdx = 0;     // index of free surface for encoder input (vpp output)

    // Since in sample we support just 2 views
    // we will change this value between 0 and 1 in case of MVC
    mfxU16 currViewNum = 0;

    sts = MFX_ERR_NONE;
    m_statOverall.StartTimeMeasurement();

    // main loop, preprocessing and encoding
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts)
    {
        // find free surface for encoder input
        if (m_nMemBuffer)
        {
            nEncSurfIdx %= m_nMemBuffer;
        }
        else
        {
            nEncSurfIdx = GetFreeSurface(m_pEncSurfaces, m_EncResponse.NumFrameActual);
        }
        MSDK_CHECK_ERROR(nEncSurfIdx, MSDK_INVALID_SURF_IDX, MFX_ERR_MEMORY_ALLOC);

        // point pSurf to encoder surface
        pSurf = &m_pEncSurfaces[nEncSurfIdx];
        pSurf->Info.FrameId.ViewId = currViewNum;

        m_statFile.StartTimeMeasurement();
        sts = LoadNextFrame(pSurf);
        m_statFile.StopTimeMeasurement();

        if ( (MFX_ERR_MORE_DATA == sts) && !m_bTimeOutExceed)
            continue;
        if (MVC_ENABLED & m_MVCflags) currViewNum ^= 1; // Flip between 0 and 1 for ViewId
        MSDK_BREAK_ON_ERROR(sts);

        m_statFile.StopTimeMeasurement();

        timeCurMax = 0;
        if (m_bFileWriterReset)
        {
            if (m_FileWriters.first)
            {
                sts = m_FileWriters.first->Reset();
                MSDK_CHECK_STATUS(sts, "m_FileWriters.first->Reset failed");
            }
            if (m_FileWriters.second)
            {
                sts = m_FileWriters.second->Reset();
                MSDK_CHECK_STATUS(sts, "m_FileWriters.second->Reset failed");
            }
            m_bFileWriterReset = false;
        }
        for (int regId = 0; regId < m_resources.GetSize(); regId++)
        {
            // get a pointer to a free task (bit stream and sync point for encoder)
            sts = m_resources.GetFreeTask(regId, &pCurrentTask);
            MSDK_CHECK_STATUS(sts, "m_resources.GetFreeTask failed");

            for (;;)
            {
                timeCurStart = time_get_tick();
                // at this point surface for encoder contains a frame from a file
                InsertIDR(m_bInsertIDR);
                sts = m_resources[regId].pEncoder->EncodeFrameAsync(&m_encCtrl, &m_pEncSurfaces[nEncSurfIdx], &pCurrentTask->mfxBS, &pCurrentTask->EncSyncP);
                m_bInsertIDR = false;

                if ((sts != MFX_ERR_NOT_ENOUGH_BUFFER) && m_nMemBuffer)
                {
                    nEncSurfIdx++;
                }

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
                    continue;
                }
                else
                {
                    // get next surface and new task for 2nd bitstream in ViewOutput mode
                    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_BITSTREAM);
                    break;
                }
            }

            timeCurEnd = time_get_tick();
            if (timeCurMax < (timeCurEnd - timeCurStart))
                timeCurMax = timeCurEnd - timeCurStart;
        }
        if (timeCurMax) {
            nFrames ++;
            m_timeAll += timeCurMax;
        }
    }

    // means that the input file has ended, need to go to buffering loops
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    // exit in case of other errors
    MSDK_CHECK_STATUS(sts, "Unexpected error!!");

    // loop to get buffered frames from encoder
    while (MFX_ERR_NONE <= sts)
    {
        timeCurMax = 0;
        for (int regId = 0; regId < m_resources.GetSize(); regId++)
        {
            // get a free task (bit stream and sync point for encoder)
            sts = m_resources.GetFreeTask(regId, &pCurrentTask);
            MSDK_CHECK_STATUS(sts, "m_resources.GetFreeTask failed");

            for (;;)
            {
                timeCurStart = time_get_tick();
                InsertIDR(m_bInsertIDR);
                sts = m_resources[regId].pEncoder->EncodeFrameAsync(&m_encCtrl, NULL, &pCurrentTask->mfxBS, &pCurrentTask->EncSyncP);
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
                    continue;
                }
                else
                {
                    // get new task for 2nd bitstream in ViewOutput mode
                    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_BITSTREAM);
                    break;
                }
            }

            // MFX_ERR_MORE_DATA is the correct status to exit buffering loop with
            // indicates that there are no more buffered frames
            if (sts == MFX_ERR_MORE_DATA)
                continue;
            // exit in case of other errors
            MSDK_CHECK_STATUS(sts, "m_resources[regId].pEncoder->EncodeFrameAsync failed");

            timeCurEnd = time_get_tick();
            if (timeCurMax < (timeCurEnd - timeCurStart))
                timeCurMax = timeCurEnd - timeCurStart;
        }
        if (timeCurMax) {
            nFrames ++;
            m_timeAll += timeCurMax;
        }
    }

    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);

    // MFX_ERR_NOT_FOUND is the correct status to exit the loop with
    // EncodeFrameAsync and SyncOperation don't return this status
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_FOUND);
    // report any errors that occurred in asynchronous part
    MSDK_CHECK_STATUS(sts, "Unexpected error!!");

    m_statOverall.StopTimeMeasurement();
    return sts;
}
