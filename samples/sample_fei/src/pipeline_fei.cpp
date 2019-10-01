/******************************************************************************\
Copyright (c) 2005-2019, Intel Corporation
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

#include "pipeline_fei.h"
#include "version.h"

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif

CEncodingPipeline::CEncodingPipeline(AppConfig* pAppConfig)
    : m_appCfg(*pAppConfig)
    , m_nAsyncDepth(1)
    , m_picStruct(pAppConfig->nPicStruct)
    , m_refDist((std::max)(pAppConfig->refDist, mfxU16(1)))
    , m_numRefFrame((std::max)(pAppConfig->numRef, mfxU16(1)))
    , m_gopSize((std::max)(pAppConfig->gopSize, mfxU16(1)))
    , m_gopOptFlag(pAppConfig->GopOptFlag)
    , m_idrInterval(pAppConfig->nIdrInterval)
    , m_numRefActiveP(pAppConfig->NumRefActiveP)
    , m_numRefActiveBL0(pAppConfig->NumRefActiveBL0)
    , m_numRefActiveBL1(pAppConfig->NumRefActiveBL1)
    , m_bRefType(pAppConfig->bRefType)
    , m_numOfFields((pAppConfig->nPicStruct & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2)
    , m_decodePoolSize(0)
    , m_heightMB(0)
    , m_widthMB(0)
    , m_widthMBpreenc(0)
    , m_heightMBpreenc(0)
    , m_maxQueueLength(0)
    , m_log2frameNumMax(8)
    , m_frameCount(0)
#if (MFX_VERSION >= 1024)
    , m_frameCountInEncodedOrder(0)
#endif
    , m_frameOrderIdrInDisplayOrder(0)
    , m_frameType((mfxU8)MFX_FRAMETYPE_UNKNOWN, (mfxU8)MFX_FRAMETYPE_UNKNOWN)
    , m_commonFrameInfo()
    , m_preencBufs(m_numOfFields)
    , m_encodeBufs(m_numOfFields)

    , m_pExtBufDecodeStreamout(NULL)

    , m_nDRC_idx(0)
    , m_bNeedDRC(pAppConfig->bDynamicRC)
    , m_DRCqueue(pAppConfig->DRCqueue)

    , m_insertIDR(false)
    , m_bVPPneeded(pAppConfig->bVPP)
    , m_bSeparatePreENCSession(pAppConfig->bPREENC && (pAppConfig->bENCPAK || pAppConfig->bOnlyENC || (pAppConfig->preencDSstrength && m_bVPPneeded)))
    , m_mfxSessionParent((mfxSession)0)
    , m_pPreencSession(m_bSeparatePreENCSession ? &m_preenc_mfxSession : &m_mfxSession)

    , m_pFEI_PreENC(NULL)
    , m_pFEI_ENCODE(NULL)
    , m_pFEI_ENCPAK(NULL)
    , m_pVPP(NULL)
    , m_pDECODE(NULL)
    , m_pYUVReader(NULL)

    , m_pMFXAllocator(NULL)
    , m_pmfxAllocatorParams(NULL)
    , m_bUseHWmemory(pAppConfig->bUseHWmemory) //only HW memory is supported (ENCODE supports SW memory)
    , m_bExternalAlloc(pAppConfig->bUseHWmemory)
    , m_bParametersAdjusted(false)
    , m_bRecoverDeviceFailWithInputReset(true)
    , m_hwdev(NULL)

    , m_surfPoolStrategy((pAppConfig->nReconSurf || pAppConfig->nInputSurf) ? PREFER_NEW : PREFER_FIRST_FREE)
    , m_DecSurfaces(m_surfPoolStrategy)
    , m_DSSurfaces(m_surfPoolStrategy)
    , m_VppSurfaces(m_surfPoolStrategy)
    , m_EncSurfaces(m_surfPoolStrategy)
    , m_ReconSurfaces(m_surfPoolStrategy)

    , m_DecResponse()
    , m_VppResponse()
    , m_dsResponse()
    , m_EncResponse()
    , m_ReconResponse()
    , m_BaseAllocID(0)
{
    m_appCfg.PipelineCfg.mixedPicstructs = m_appCfg.nPicStruct == MFX_PICSTRUCT_UNKNOWN;
}

CEncodingPipeline::~CEncodingPipeline()
{
    Close();
}

void CEncodingPipeline::Close()
{
    msdk_printf(MSDK_STRING("Frames processed: %u\r"), m_frameCount);

    MSDK_SAFE_DELETE(m_pFEI_PreENC);
    MSDK_SAFE_DELETE(m_pFEI_ENCODE);
    MSDK_SAFE_DELETE(m_pFEI_ENCPAK);
    MSDK_SAFE_DELETE(m_pVPP);
    MSDK_SAFE_DELETE(m_pDECODE);
    MSDK_SAFE_DELETE(m_pYUVReader);

    m_mfxSession.Close();
    if (m_bSeparatePreENCSession){
        m_preenc_mfxSession.Close();
    }

    ReleaseResources();
    DeleteFrames();

    // external allocator should be deleted after SDK components
    DeleteAllocator();
}


mfxStatus CEncodingPipeline::GetEncodeSession(mfxSession &session)
{
    if(m_mfxSession)
    {
        session = m_mfxSession;
        return MFX_ERR_NONE;
    }
    else
        return MFX_ERR_NOT_INITIALIZED;
}

mfxStatus CEncodingPipeline::Init(mfxSession parentSession)
{
    mfxStatus sts = MFX_ERR_NONE;

    // Section below initialize sessions and sets proper allocId for components

    sts = InitSessions();
    MSDK_CHECK_STATUS(sts, "InitSessions failed");
    mfxSession akaSession = m_mfxSession.operator mfxSession();
    if(parentSession != NULL)
    {
        sts = MFXJoinSession(parentSession, akaSession);
        if(sts != MFX_ERR_NONE)
            msdk_printf(MSDK_STRING("WARINING! Joining Sessions failed\n"));
        m_mfxSessionParent = parentSession;
    }
    m_BaseAllocID = (mfxU64)&akaSession & 0xffffffff;

    // create and init frame allocator
    sts = CreateAllocator();
    MSDK_CHECK_STATUS(sts, "CreateAllocator failed");

    if (m_appCfg.bDECODE)
    {
        m_pDECODE = new MFX_DecodeInterface(&m_mfxSession, m_BaseAllocID, &m_appCfg, &m_DecSurfaces);
        sts = m_pDECODE->FillParameters();
        MSDK_CHECK_STATUS(sts, "DECODE: Parameters initialization failed");
        m_commonFrameInfo = m_pDECODE->GetCommonVideoParams()->mfx.FrameInfo;
    }
    else
    {
        m_pYUVReader = new YUVreader(&m_appCfg, m_bVPPneeded ? &m_VppSurfaces : &m_EncSurfaces, m_pMFXAllocator);
    }

    // Сreate preprocessor if resizing was requested from command line
    if (m_bVPPneeded)
    {
        m_pVPP = new MFX_VppInterface(&m_mfxSession, m_BaseAllocID, &m_appCfg);
        sts = m_pVPP->FillParameters();
        MSDK_CHECK_STATUS(sts, "VPP: Parameters initialization failed");
    }

    if (m_appCfg.bPREENC)
    {
        m_pFEI_PreENC = new FEI_PreencInterface(m_pPreencSession, &m_inputTasks, m_BaseAllocID, &m_preencBufs, &m_encodeBufs, &m_appCfg);
        sts = m_pFEI_PreENC->FillParameters();
        MSDK_CHECK_STATUS(sts, "PreENC: Parameters initialization failed");
        m_commonFrameInfo = m_pFEI_PreENC->GetCommonVideoParams()->mfx.FrameInfo;
    }

    if (m_appCfg.bENCODE)
    {
        m_pFEI_ENCODE = new FEI_EncodeInterface(&m_mfxSession, m_BaseAllocID, &m_encodeBufs, &m_appCfg);
        sts = m_pFEI_ENCODE->FillParameters();
        MSDK_CHECK_STATUS(sts, "ENCODE: Parameters initialization failed");
        m_commonFrameInfo = m_pFEI_ENCODE->GetCommonVideoParams()->mfx.FrameInfo;
    }

    if (m_appCfg.bENCPAK || m_appCfg.bOnlyENC || m_appCfg.bOnlyPAK)
    {
        m_pFEI_ENCPAK = new FEI_EncPakInterface(&m_mfxSession, &m_inputTasks, m_BaseAllocID, &m_encodeBufs, &m_appCfg);
        sts = m_pFEI_ENCPAK->FillParameters();
        MSDK_CHECK_STATUS(sts, "ENCPAK: Parameters initialization failed");
        m_commonFrameInfo = m_pFEI_ENCPAK->GetCommonVideoParams()->mfx.FrameInfo;
        sts = m_pFEI_ENCPAK->SetFrameAllocator(m_pMFXAllocator);
    }

#if (MFX_VERSION >= 1024)
    //BRC for PAK only
    if (m_appCfg.bOnlyPAK && m_appCfg.RateControlMethod == MFX_RATECONTROL_VBR)
    {
        //prepare MfxVideoParamsWrapper for BRC
        MfxVideoParamsWrapper tmp = *m_appCfg.PipelineCfg.pPakVideoParam;
        tmp.mfx.RateControlMethod = m_appCfg.RateControlMethod;
        tmp.mfx.TargetKbps        = m_appCfg.TargetKbps;

        auto co = tmp.AddExtBuffer<mfxExtCodingOption>();
        co->NalHrdConformance = MFX_CODINGOPTION_OFF;

        sts = m_BRC.Init(&tmp);
        MSDK_CHECK_STATUS(sts, "BRC initialization failed");
    }
#endif

    sts = ResetMFXComponents();
    MSDK_CHECK_STATUS(sts, "ResetMFXComponents failed");

    sts = SetSequenceParameters();
    MSDK_CHECK_STATUS(sts, "SetSequenceParameters failed");

    sts = AllocExtBuffers();
    MSDK_CHECK_STATUS(sts, "InitInterfaces failed");

    return sts;
}

mfxStatus CEncodingPipeline::ResetMFXComponents()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_bRecoverDeviceFailWithInputReset)
    {
        sts = ResetIOFiles();
        MSDK_CHECK_STATUS(sts, "ResetIOFiles failed");

        // Reset state indexes
        m_frameCount = 0;
        m_nDRC_idx   = 0;
    }

    if (m_pYUVReader)
    {
        m_pYUVReader->Close();
    }

    if (m_pDECODE)
    {
        sts = m_pDECODE->Close();
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_STATUS(sts, "DECODE: Close failed");
    }

    if (m_pVPP)
    {
        sts = m_pVPP->Close();
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_STATUS(sts, "VPP: Close failed");
    }

    if (m_pFEI_PreENC)
    {
        sts = m_pFEI_PreENC->Close();
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_STATUS(sts, "FEI PreENC: Close failed");
    }

    if (m_pFEI_ENCODE)
    {
        sts = m_pFEI_ENCODE->Close();
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_STATUS(sts, "FEI ENCODE: Close failed");
    }

    if (m_pFEI_ENCPAK)
    {
        sts = m_pFEI_ENCPAK->Close();
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_INITIALIZED);
        MSDK_CHECK_STATUS(sts, "FEI ENCPAK: Close failed");
    }

    DeleteFrames();

    sts = AllocFrames();
    MSDK_CHECK_STATUS(sts, "AllocFrames failed");

    if (m_pYUVReader)
    {
        sts = m_pYUVReader->Init();
        MSDK_CHECK_STATUS(sts, "YUVReader: Init failed");
    }

    if (m_pDECODE)
    {
        sts = m_pDECODE->Init();
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: DECODE partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "FEI DECODE: Init failed");
    }

    if (m_pVPP)
    {
        sts = m_pVPP->Init();
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: VPP partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        MSDK_CHECK_STATUS(sts, "VPP: Init failed");
    }

    if (m_pFEI_PreENC)
    {
        sts = m_pFEI_PreENC->Init();
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: FEI PreENC partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        m_bParametersAdjusted |= sts == MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        MSDK_CHECK_STATUS(sts, "FEI PreENC: Init failed");
    }

    if (m_pFEI_ENCODE)
    {
        sts = m_pFEI_ENCODE->Init();
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: FEI ENCODE partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        m_bParametersAdjusted |= sts == MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        MSDK_CHECK_STATUS(sts, "FEI ENCODE: Init failed");
    }

    if (m_pFEI_ENCPAK)
    {
        sts = m_pFEI_ENCPAK->Init();
        if (MFX_WRN_PARTIAL_ACCELERATION == sts)
        {
            msdk_printf(MSDK_STRING("WARNING: FEI ENCPAK partial acceleration\n"));
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        }
        m_bParametersAdjusted |= sts == MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
        MSDK_CHECK_STATUS(sts, "FEI ENCPAK: Init failed");
    }

    // Mark all buffers as vacant without reallocation
    ResetExtBuffers();

    return sts;
}

mfxStatus CEncodingPipeline::CreateHWDevice()
{
    mfxStatus sts = MFX_ERR_NONE;

    m_hwdev = CreateVAAPIDevice();
    if (NULL == m_hwdev)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }
    sts = m_hwdev->Init(NULL, 0, MSDKAdapter::GetNumber(m_mfxSession));
    MSDK_CHECK_STATUS(sts, "m_hwdev->Init failed");

    return sts;
}

mfxStatus CEncodingPipeline::ResetDevice()
{
    if (m_bUseHWmemory)
    {
        return m_hwdev->Reset();
    }
    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::AllocFrames()
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameAllocRequest EncRequest;
    MSDK_ZERO_MEMORY(EncRequest);

    mfxU16 nEncSurfNum = 0; // number of surfaces for encoder

    // Calculate the number of surfaces for components.
    // QueryIOSurf functions tell how many surfaces are required to produce at least 1 output.

    if (m_pFEI_ENCODE)
    {
        sts = m_pFEI_ENCODE->QueryIOSurf(&EncRequest);
        MSDK_CHECK_STATUS(sts, "m_pmfxENCODE->QueryIOSurf failed");
    }

    mfxFrameAllocRequest PakRequest[2];
    if (m_pFEI_ENCPAK)
    {
        MSDK_ZERO_MEMORY(PakRequest[0]); // ENCPAK input request
        MSDK_ZERO_MEMORY(PakRequest[1]); // ENCPAK reconstruct request

        sts = m_pFEI_ENCPAK->QueryIOSurf(PakRequest);
        MSDK_CHECK_STATUS(sts, "m_pFEI_ENCPAK->QueryIOSurf failed");
    }

    m_maxQueueLength = m_refDist * 2 + m_nAsyncDepth + m_numRefFrame + 1;

    // The number of surfaces shared by vpp output and encode input.
    // When surfaces are shared 1 surface at first component output contains output frame that goes to next component input
    if (m_appCfg.nInputSurf == 0)
    {
        nEncSurfNum = EncRequest.NumFrameSuggested + (m_nAsyncDepth - 1) + 2;
        if ((m_appCfg.bENCPAK) || (m_appCfg.bOnlyPAK) || (m_appCfg.bOnlyENC))
        {
            // Some additional surfaces required, because eTask holds frames till the destructor.
            // More optimal approaches are possible
            nEncSurfNum  = PakRequest[0].NumFrameSuggested + m_numRefFrame + m_refDist + 1;
            nEncSurfNum += m_pVPP ? m_refDist + 1 : 0;
        }
        else if ((m_appCfg.bPREENC) || (m_appCfg.bENCODE))
        {
            nEncSurfNum  = m_maxQueueLength;
            nEncSurfNum += m_pVPP ? m_refDist + 1 : 0;
        }
    }
    else
    {
        nEncSurfNum = m_appCfg.nInputSurf;
    }

    // prepare allocation requests
    EncRequest.NumFrameMin       = nEncSurfNum;
    EncRequest.NumFrameSuggested = nEncSurfNum;
    MSDK_MEMCPY_VAR(EncRequest.Info, &m_commonFrameInfo, sizeof(mfxFrameInfo));

    EncRequest.AllocId = m_BaseAllocID;
    EncRequest.Type |= MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
    if (m_pFEI_PreENC)
        EncRequest.Type |= m_pFEI_PreENC->m_pmfxDS ? MFX_MEMTYPE_FROM_VPPIN : MFX_MEMTYPE_FROM_ENC;
    if (m_pFEI_ENCPAK)
        EncRequest.Type |= (MFX_MEMTYPE_FROM_ENC | MFX_MEMTYPE_FROM_PAK);

    if (m_pFEI_PreENC && m_pFEI_PreENC->m_pmfxDS)
    {
        mfxFrameAllocRequest DSRequest[2];
        MSDK_ZERO_MEMORY(DSRequest[0]);
        MSDK_ZERO_MEMORY(DSRequest[1]);

        sts = m_pFEI_PreENC->QueryIOSurf(DSRequest);
        MSDK_CHECK_STATUS(sts, "m_pmfxDS->QueryIOSurf failed");

        // these surfaces are input surfaces for PREENC
        DSRequest[1].NumFrameMin = DSRequest[1].NumFrameSuggested = EncRequest.NumFrameSuggested;
        DSRequest[1].Type        = EncRequest.Type | MFX_MEMTYPE_FROM_ENC;
        DSRequest[1].AllocId     = m_BaseAllocID;

        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &(DSRequest[1]), &m_dsResponse);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");

        m_DSSurfaces.PoolSize = m_dsResponse.NumFrameActual;
        sts = FillSurfacePool(m_DSSurfaces.SurfacesPool, &m_dsResponse, &(m_pFEI_PreENC->m_DSParams.vpp.Out));
        MSDK_CHECK_STATUS(sts, "FillSurfacePool failed");
    }

    if (m_pDECODE)
    {
        mfxFrameAllocRequest DecRequest;
        MSDK_ZERO_MEMORY(DecRequest);

        sts = m_pDECODE->QueryIOSurf(&DecRequest);
        MSDK_CHECK_STATUS(sts, "m_pmfxDECODE->QueryIOSurf failed");
        DecRequest.NumFrameMin = m_decodePoolSize = DecRequest.NumFrameSuggested;

        if (!m_pVPP)
        {
            // in case of Decode and absence of VPP we use the same surface pool for the entire pipeline
            DecRequest.NumFrameMin = DecRequest.NumFrameSuggested = m_decodePoolSize + nEncSurfNum;
        }
        MSDK_MEMCPY_VAR(DecRequest.Info, &(m_pDECODE->m_videoParams.mfx.FrameInfo), sizeof(mfxFrameInfo));

        if (m_pVPP || (m_pFEI_PreENC && m_pFEI_PreENC->m_pmfxDS))
        {
            DecRequest.Type = MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET | MFX_MEMTYPE_FROM_DECODE;
        }
        else
            DecRequest.Type = EncRequest.Type | MFX_MEMTYPE_FROM_DECODE;
        DecRequest.AllocId = m_BaseAllocID;

        // alloc frames for decoder
        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &DecRequest, &m_DecResponse);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");

        // prepare mfxFrameSurface1 array for decoder
        m_DecSurfaces.PoolSize = m_DecResponse.NumFrameActual;
        sts = FillSurfacePool(m_DecSurfaces.SurfacesPool, &m_DecResponse, &(m_pDECODE->m_videoParams.mfx.FrameInfo));
        MSDK_CHECK_STATUS(sts, "FillSurfacePool failed");

        if (!(m_pVPP || m_pFEI_ENCPAK))
            return MFX_ERR_NONE;
    }

    if (!m_pDECODE && m_pVPP)
    {
        // in case of absence of DECODE we need to allocate input surfaces for VPP
        mfxFrameAllocRequest VppRequest[2];
        MSDK_ZERO_MEMORY(VppRequest[0]); //VPP in
        MSDK_ZERO_MEMORY(VppRequest[1]); //VPP out

        sts = m_pVPP->QueryIOSurf(VppRequest);
        MSDK_CHECK_STATUS(sts, "m_pmfxVPP->QueryIOSurf failed");

        VppRequest[0].NumFrameMin = VppRequest[0].NumFrameSuggested;
        VppRequest[0].AllocId = m_BaseAllocID;

        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &(VppRequest[0]), &m_VppResponse);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");

        // prepare mfxFrameSurface1 array for VPP
        m_VppSurfaces.PoolSize = m_VppResponse.NumFrameActual;
        sts = FillSurfacePool(m_VppSurfaces.SurfacesPool, &m_VppResponse, &(m_pVPP->m_videoParams.mfx.FrameInfo));
        MSDK_CHECK_STATUS(sts, "FillSurfacePool failed");
    }

    if (!m_pDECODE || m_pVPP)
    {
        // alloc frames for encoder
        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &EncRequest, &m_EncResponse);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");

        // prepare mfxFrameSurface1 array for encoder
        m_EncSurfaces.PoolSize = m_EncResponse.NumFrameActual;
        sts = FillSurfacePool(m_EncSurfaces.SurfacesPool, &m_EncResponse, &m_commonFrameInfo);
        MSDK_CHECK_STATUS(sts, "FillSurfacePool failed");
    }

    /* ENC use input source surfaces only & does not generate real reconstructed surfaces.
     * But surface from reconstruct pool is required by driver for ENC and the same surface should also be passed to PAK.
     * PAK generate real reconstructed surfaces.
     * */
    if (m_pFEI_ENCPAK)
    {
        PakRequest[1].AllocId = m_BaseAllocID;
        MSDK_MEMCPY_VAR(PakRequest[1].Info, &m_commonFrameInfo, sizeof(mfxFrameInfo));

        if (m_appCfg.nReconSurf != 0)
        {
            if (m_appCfg.nReconSurf > PakRequest[1].NumFrameMin)
            {
                PakRequest[1].NumFrameMin = PakRequest[1].NumFrameSuggested = m_appCfg.nReconSurf;
            }
            else
            {
                msdk_printf(MSDK_STRING("\nWARNING: User input reconstruct surface num invalid!\n"));
            }
        }

        if (m_appCfg.bENCPAK || m_appCfg.bOnlyENC)
        {
            PakRequest[1].Type |= MFX_MEMTYPE_FROM_ENC;
        }

        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &PakRequest[1], &m_ReconResponse);
        MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Alloc failed");

        m_ReconSurfaces.PoolSize = m_ReconResponse.NumFrameActual;
        sts = FillSurfacePool(m_ReconSurfaces.SurfacesPool, &m_ReconResponse, &m_commonFrameInfo);
        MSDK_CHECK_STATUS(sts, "FillSurfacePool failed");
    }

    return MFX_ERR_NONE;
}

mfxStatus CEncodingPipeline::FillSurfacePool(mfxFrameSurface1* & surfacesPool, mfxFrameAllocResponse* allocResponse, mfxFrameInfo* FrameInfo)
{
    mfxStatus sts = MFX_ERR_NONE;

    surfacesPool = new mfxFrameSurface1[allocResponse->NumFrameActual];
    MSDK_ZERO_ARRAY(surfacesPool, allocResponse->NumFrameActual);

    for (int i = 0; i < allocResponse->NumFrameActual; i++)
    {
        MSDK_MEMCPY_VAR(surfacesPool[i].Info, FrameInfo, sizeof(mfxFrameInfo));

        if (m_bExternalAlloc)
        {
            surfacesPool[i].Data.MemId = allocResponse->mids[i];
        }
        else
        {
            // get YUV pointers
            sts = m_pMFXAllocator->Lock(m_pMFXAllocator->pthis, allocResponse->mids[i], &(surfacesPool[i].Data));
            MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Lock failed");
        }
    }

    return sts;
}

mfxStatus CEncodingPipeline::CreateAllocator()
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxHDL hdl = NULL;
    if(m_mfxSessionParent != NULL)
    {
        //get handle from parent session if exist
        sts = MFXVideoCORE_GetHandle(m_mfxSessionParent, MFX_HANDLE_VA_DISPLAY, &hdl);

        MSDK_CHECK_STATUS(sts, "m_mfxSession.GetHandle failed");
    }
    else
    {
        //if no parent session(single session, or sessions not joined) - create new handle
        sts = CreateHWDevice();
        MSDK_CHECK_STATUS(sts, "CreateHWDevice failed");
        sts = m_hwdev->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
    }
    // provide device manager to MediaSDK

    sts = m_mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
    MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");

    if (m_bSeparatePreENCSession){
        sts = m_preenc_mfxSession.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
        MSDK_CHECK_STATUS(sts, "m_preenc_mfxSession.SetHandle failed");
    }

    if (m_bUseHWmemory)
    {
        // create VAAPI allocator
        m_pMFXAllocator = new vaapiFrameAllocator;

        vaapiAllocatorParams *p_vaapiAllocParams = new vaapiAllocatorParams;

        p_vaapiAllocParams->m_dpy = (VADisplay)hdl;
        m_pmfxAllocatorParams = p_vaapiAllocParams;

        /* In case of video memory we must provide MediaSDK with external allocator
        thus we demonstrate "external allocator" usage model.
        Call SetAllocator to pass allocator to MediaSDK */
        sts = m_mfxSession.SetFrameAllocator(m_pMFXAllocator);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetFrameAllocator failed");
        if (m_bSeparatePreENCSession){
            sts = m_preenc_mfxSession.SetFrameAllocator(m_pMFXAllocator);
            MSDK_CHECK_STATUS(sts, "m_preenc_mfxSession.SetFrameAllocator failed");
        }

        m_bExternalAlloc = true;
    }
    else
    {
        //in case of system memory allocator we also have to pass MFX_HANDLE_VA_DISPLAY to HW library
        mfxIMPL impl;
        m_mfxSession.QueryIMPL(&impl);

        // create system memory allocator
        m_pMFXAllocator = new SysMemFrameAllocator;

        /* In case of system memory we demonstrate "no external allocator" usage model.
        We don't call SetAllocator, Media SDK uses internal allocator.
        We use system memory allocator simply as a memory manager for application*/
    }

    // initialize memory allocator
    sts = m_pMFXAllocator->Init(m_pmfxAllocatorParams);
    MSDK_CHECK_STATUS(sts, "m_pMFXAllocator->Init failed");

    return sts;
}

void CEncodingPipeline::DeleteFrames()
{
    // delete all allocated surfaces
    m_DecSurfaces.DeleteFrames();
    m_VppSurfaces.DeleteFrames();
    m_DSSurfaces.DeleteFrames();

    if (m_EncSurfaces.SurfacesPool != m_ReconSurfaces.SurfacesPool)
    {
        m_EncSurfaces.DeleteFrames();
        m_ReconSurfaces.DeleteFrames();
    }
    else
    {
        m_EncSurfaces.DeleteFrames();

        /* Prevent double-free in m_pReconSurfaces.DeleteFrames */
        m_ReconSurfaces.SurfacesPool = NULL;
        m_ReconSurfaces.DeleteFrames();
    }

    // delete frames
    if (m_pMFXAllocator)
    {
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_DecResponse);
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_VppResponse);
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_dsResponse);
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_EncResponse);
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_ReconResponse);
    }
}

void CEncodingPipeline::ReleaseResources()
{
    //unlock last frames
    m_inputTasks.Clear();

    m_preencBufs.Clear();
    m_encodeBufs.Clear();

    ClearDecoderBuffers();
}

void CEncodingPipeline::ResetExtBuffers()
{
    m_inputTasks.Clear();
    m_preencBufs.UnlockAll();
    m_encodeBufs.UnlockAll();

    m_pExtBufDecodeStreamout = nullptr;
}

void CEncodingPipeline::DeleteHWDevice()
{
    MSDK_SAFE_DELETE(m_hwdev);
}

void CEncodingPipeline::DeleteAllocator()
{
    // delete allocator
    MSDK_SAFE_DELETE(m_pMFXAllocator);
    MSDK_SAFE_DELETE(m_pmfxAllocatorParams);

    DeleteHWDevice();
}

mfxStatus CEncodingPipeline::ResetIOFiles()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_pYUVReader)
    {
        sts = m_pYUVReader->ResetState();
        MSDK_CHECK_STATUS(sts, "YUVReader: ResetState failed");
    }
    else if (m_pDECODE)
    {
        sts = m_pDECODE->ResetState();
        MSDK_CHECK_STATUS(sts, "DECODE: ResetState failed");
    }

    if (m_pFEI_PreENC)
    {
        sts = m_pFEI_PreENC->ResetState();
        MSDK_CHECK_STATUS(sts, "FEI PreENC: ResetState failed");
    }

    if (m_pFEI_ENCODE)
    {
        sts = m_pFEI_ENCODE->ResetState();
        MSDK_CHECK_STATUS(sts, "FEI ENCODE: ResetState failed");
    }

    if (m_pFEI_ENCPAK)
    {
        sts = m_pFEI_ENCPAK->ResetState();
        MSDK_CHECK_STATUS(sts, "FEI ENCPAK: ResetState failed");
    }

    return sts;
}

mfxStatus CEncodingPipeline::SetSequenceParameters()
{
    mfxStatus sts = UpdateVideoParam(); // update settings according to those that exposed by MSDK library
    MSDK_CHECK_STATUS(sts, "UpdateVideoParam failed");

    mfxU16 cached_picstruct = m_picStruct;

    // Get adjusted BRef type, active P/B refs, picstruct and other
    if (m_pFEI_PreENC)
    {
        m_pFEI_PreENC->GetRefInfo(m_picStruct, m_refDist, m_numRefFrame, m_gopSize, m_gopOptFlag, m_idrInterval,
            m_numRefActiveP, m_numRefActiveBL0, m_numRefActiveBL1, m_bRefType, m_appCfg.bFieldProcessingMode);
    }

    if (m_pFEI_ENCPAK)
    {
        m_pFEI_ENCPAK->GetRefInfo(m_picStruct, m_refDist, m_numRefFrame, m_gopSize, m_gopOptFlag, m_idrInterval,
            m_numRefActiveP, m_numRefActiveBL0, m_numRefActiveBL1, m_bRefType, m_appCfg.bFieldProcessingMode);
    }

    if (m_pFEI_ENCODE)
    {
        m_pFEI_ENCODE->GetRefInfo(m_picStruct, m_refDist, m_numRefFrame, m_gopSize, m_gopOptFlag, m_idrInterval,
            m_numRefActiveP, m_numRefActiveBL0, m_numRefActiveBL1, m_bRefType, m_appCfg.bFieldProcessingMode);
    }

    // Adjustment interlaced to progressive by MSDK lib is possible.
    // So keep this value to fit resulting number of MBs to surface passed on Init stage.
    mfxU16 cached_numOfFields = m_numOfFields;

    m_numOfFields = m_preencBufs.num_of_fields = m_encodeBufs.num_of_fields = (m_picStruct & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2;

    if (m_bParametersAdjusted)
    {
        msdk_printf(MSDK_STRING("\nWARNING: Incompatible video parameters adjusted by MSDK library!\n"));
    }

    // Update picstruct in allocated frames
    if (cached_picstruct != m_picStruct)
    {
        sts = m_DecSurfaces.UpdatePicStructs(m_picStruct);
        MSDK_CHECK_STATUS(sts, "m_DecSurfaces.UpdatePicStructs failed");

        sts = m_DSSurfaces.UpdatePicStructs(m_picStruct);
        MSDK_CHECK_STATUS(sts, "m_DSSurfaces.UpdatePicStructs failed");

        sts = m_VppSurfaces.UpdatePicStructs(m_picStruct);
        MSDK_CHECK_STATUS(sts, "m_VppSurfaces.UpdatePicStructs failed");

        sts = m_EncSurfaces.UpdatePicStructs(m_picStruct);
        MSDK_CHECK_STATUS(sts, "m_EncSurfaces.UpdatePicStructs failed");

        sts = m_ReconSurfaces.UpdatePicStructs(m_picStruct);
        MSDK_CHECK_STATUS(sts, "m_ReconSurfaces.UpdatePicStructs failed");
    }

    /* Initialize task pool */
    m_inputTasks.Init(m_appCfg.EncodedOrder,
                    2 + !m_appCfg.preencDSstrength, // (ENC + PAK structs always present) + 1 (if DS not present)
                    m_refDist, m_gopOptFlag, m_numRefFrame, m_bRefType, m_numRefFrame + 1, m_log2frameNumMax);

    m_taskInitializationParams.PicStruct       = m_picStruct;
    m_taskInitializationParams.GopPicSize      = m_gopSize;
    m_taskInitializationParams.GopRefDist      = m_refDist;
    m_taskInitializationParams.NumRefActiveP   = m_numRefActiveP;
    m_taskInitializationParams.NumRefActiveBL0 = m_numRefActiveBL0;
    m_taskInitializationParams.NumRefActiveBL1 = m_numRefActiveBL1;

    m_taskInitializationParams.NumMVPredictorsP   = m_appCfg.PipelineCfg.NumMVPredictorsP = m_appCfg.bNPredSpecified_Pl0 ?
        m_appCfg.NumMVPredictors_Pl0 : (std::min)(mfxU16(m_numRefFrame*m_numOfFields), MaxFeiEncMVPNum);

    m_taskInitializationParams.NumMVPredictorsBL0 = m_appCfg.PipelineCfg.NumMVPredictorsBL0 = m_appCfg.bNPredSpecified_Bl0 ?
        m_appCfg.NumMVPredictors_Bl0 : (std::min)(mfxU16(m_numRefFrame*m_numOfFields), MaxFeiEncMVPNum);

    m_taskInitializationParams.NumMVPredictorsBL1 = m_appCfg.PipelineCfg.NumMVPredictorsBL1 = m_appCfg.bNPredSpecified_l1 ?
        m_appCfg.NumMVPredictors_Bl1 : (std::min)(mfxU16(m_numRefFrame*m_numOfFields), MaxFeiEncMVPNum);

    m_taskInitializationParams.BRefType           = m_bRefType;
    m_taskInitializationParams.NoPRefB            = m_appCfg.bNoPtoBref;

    /* Section below calculates number of macroblocks for extension buffers allocation */

    // Copy source frame size from decoder
    if (m_pDECODE)
    {
        m_appCfg.nWidth  = m_pDECODE->m_videoParams.mfx.FrameInfo.CropW;
        m_appCfg.nHeight = m_pDECODE->m_videoParams.mfx.FrameInfo.CropH;

        if (m_appCfg.nDstWidth  == 0) { m_appCfg.nDstWidth  = m_appCfg.nWidth;  }
        if (m_appCfg.nDstHeight == 0) { m_appCfg.nDstHeight = m_appCfg.nHeight; }
    }

    m_widthMB  = MSDK_ALIGN16(m_appCfg.nDstWidth);
    m_heightMB = cached_numOfFields == 2 ? MSDK_ALIGN32(m_appCfg.nDstHeight) : MSDK_ALIGN16(m_appCfg.nDstHeight);

    m_appCfg.PipelineCfg.numMB_refPic  = m_appCfg.PipelineCfg.numMB_frame = (m_widthMB * m_heightMB) >> 8;
    m_appCfg.PipelineCfg.numMB_refPic /= m_numOfFields;

    // num of MBs in reference frame/field
    m_widthMB  >>= 4;
    m_heightMB >>= m_numOfFields == 2 ? 5 : 4;

    if (m_appCfg.bPREENC)
    {
        if (m_appCfg.preencDSstrength)
        {
            m_widthMBpreenc  = MSDK_ALIGN16(m_appCfg.nDstWidth / m_appCfg.preencDSstrength);
            m_heightMBpreenc = m_numOfFields == 2 ? MSDK_ALIGN32(m_appCfg.nDstHeight / m_appCfg.preencDSstrength) :
                               MSDK_ALIGN16(m_appCfg.nDstHeight / m_appCfg.preencDSstrength);

            m_appCfg.PipelineCfg.numMB_preenc_refPic  = m_appCfg.PipelineCfg.numMB_preenc_frame = (m_widthMBpreenc * m_heightMBpreenc) >> 8;
            m_appCfg.PipelineCfg.numMB_preenc_refPic /= m_numOfFields;

            m_widthMBpreenc  >>= 4;
            m_heightMBpreenc >>= m_numOfFields == 2 ? 5 : 4;
        }
        else
        {
            m_widthMBpreenc  = m_widthMB;
            m_heightMBpreenc = m_heightMB;

            m_appCfg.PipelineCfg.numMB_preenc_frame  = m_appCfg.PipelineCfg.numMB_frame;
            m_appCfg.PipelineCfg.numMB_preenc_refPic = m_appCfg.PipelineCfg.numMB_refPic;
        }
    }

    return sts;
}

mfxStatus CEncodingPipeline::InitSessions()
{
    mfxIMPL impl = MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_VAAPI;

    mfxStatus sts = m_mfxSession.Init(impl, NULL);
    MSDK_CHECK_STATUS(sts, "m_mfxSession.Init failed");

    if (m_bSeparatePreENCSession)
    {
        sts = m_preenc_mfxSession.Init(impl, NULL);
        MSDK_CHECK_STATUS(sts, "m_preenc_mfxSession.Init failed");
    }

    return sts;
}

mfxStatus CEncodingPipeline::ResetSessions()
{
    mfxStatus sts = m_mfxSession.Close();
    MSDK_CHECK_STATUS(sts, "m_mfxSession.Close failed");

    if (m_bSeparatePreENCSession)
    {
        sts = m_preenc_mfxSession.Close();
        MSDK_CHECK_STATUS(sts, "m_preenc_mfxSession.Close failed");
    }

    sts = InitSessions();
    MSDK_CHECK_STATUS(sts, "InitSessions failed");

    return sts;
}

mfxStatus CEncodingPipeline::UpdateVideoParam()
{
    if (m_pFEI_ENCODE) { return m_pFEI_ENCODE->UpdateVideoParam(); }

    if (m_pFEI_ENCPAK) { return m_pFEI_ENCPAK->UpdateVideoParam(); }

    if (m_pFEI_PreENC) { return m_pFEI_PreENC->UpdateVideoParam(); }

    if (m_pDECODE)     { return m_pDECODE->UpdateVideoParam();     }

    return MFX_ERR_NOT_FOUND;
}

PairU8 ExtendFrameType(mfxU32 type)
{
    mfxU32 type1 = type & 0xff;
    mfxU32 type2 = type >> 8;

    if (type2 == 0)
    {
        type2 = type1 & ~MFX_FRAMETYPE_IDR; // second field can't be IDR

        if (type1 & MFX_FRAMETYPE_I)
        {
            type2 &= ~MFX_FRAMETYPE_I;
            type2 |= MFX_FRAMETYPE_P;
        }
    }

    return PairU8(type1, type2);
}

PairU8 CEncodingPipeline::GetFrameType(mfxU32 frameOrder)
{
    static mfxU32 idrPicDist = (m_gopSize != 0xffff) ? (std::max)(mfxU32(m_gopSize * (m_idrInterval + 1)), mfxU32(1))
                                                     : 0xffffffff; // infinite GOP

    if (frameOrder % idrPicDist == 0)
        return ExtendFrameType(MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR);

    if (frameOrder % m_gopSize == 0)
        return ExtendFrameType(MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF);

    if (frameOrder % m_gopSize % m_refDist == 0)
        return ExtendFrameType(MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);

    if ((m_gopOptFlag & MFX_GOP_STRICT) == 0)
        if (((frameOrder + 1) % m_gopSize == 0 && (m_gopOptFlag & MFX_GOP_CLOSED)) ||
            (frameOrder + 1) % idrPicDist == 0)
            return ExtendFrameType(MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF); // switch last B frame to P frame

    bool adjust_last_B = !(m_gopOptFlag & MFX_GOP_STRICT) && m_appCfg.nNumFrames && (frameOrder + 1) >= m_appCfg.nNumFrames;
    return adjust_last_B ? ExtendFrameType(MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF) : ExtendFrameType(MFX_FRAMETYPE_B);
}

mfxStatus CEncodingPipeline::AllocExtBuffers()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_appCfg.bDECODESTREAMOUT)
    {
        m_StreamoutBufs.reserve(m_DecResponse.NumFrameActual);
        for (mfxU32 i = 0; i < m_DecResponse.NumFrameActual; i++) // alloc a streamout buffer per decoder surface
        {
            m_pExtBufDecodeStreamout = new mfxExtFeiDecStreamOut;
            MSDK_ZERO_MEMORY(*m_pExtBufDecodeStreamout);

            m_pExtBufDecodeStreamout->Header.BufferId = MFX_EXTBUFF_FEI_DEC_STREAM_OUT;
            m_pExtBufDecodeStreamout->Header.BufferSz = sizeof(mfxExtFeiDecStreamOut);
            m_pExtBufDecodeStreamout->PicStruct   = m_picStruct;
            m_pExtBufDecodeStreamout->RemapRefIdx = MFX_CODINGOPTION_ON; /* turn on refIdx remapping in library */
            /* streamout uses one buffer to store info about both fields */
            m_pExtBufDecodeStreamout->NumMBAlloc  = (m_commonFrameInfo.Width * m_commonFrameInfo.Height) >> 8;

            m_pExtBufDecodeStreamout->MB = new mfxFeiDecStreamOutMBCtrl[m_pExtBufDecodeStreamout->NumMBAlloc];
            MSDK_ZERO_ARRAY(m_pExtBufDecodeStreamout->MB, m_pExtBufDecodeStreamout->NumMBAlloc);

            m_StreamoutBufs.push_back(m_pExtBufDecodeStreamout);

            /* attach created buffer to decoder surface */
            m_DecSurfaces.SurfacesPool[i].Data.ExtParam    = reinterpret_cast<mfxExtBuffer**>(&(m_StreamoutBufs[i]));
            m_DecSurfaces.SurfacesPool[i].Data.NumExtParam = 1;
        }
    }

    mfxU32 fieldId = 0;

    if (m_appCfg.bPREENC)
    {
        //setup control structures
        bool disableMVoutPreENC  = (m_appCfg.mvoutFile     == NULL) && !(m_appCfg.bENCODE || m_appCfg.bENCPAK || m_appCfg.bOnlyENC);
        bool disableMBStatPreENC = (m_appCfg.mbstatoutFile == NULL) && !(m_appCfg.bENCODE || m_appCfg.bENCPAK || m_appCfg.bOnlyENC);
        bool enableMVpredPreENC  = m_appCfg.mvinFile != NULL;
        bool enableMBQP          = m_appCfg.mbQpFile != NULL        && !(m_appCfg.bENCODE || m_appCfg.bENCPAK || m_appCfg.bOnlyENC);

        bufSet*                      tmpForInit = NULL;
        mfxExtFeiPreEncCtrl*         preENCCtr  = NULL;
        mfxExtFeiPreEncMVPredictors* mvPreds    = NULL;
        mfxExtFeiEncQP*              qps        = NULL;
        mfxExtFeiPreEncMV*           mvs        = NULL;
        mfxExtFeiPreEncMBStat*       mbdata     = NULL;

        mfxU32 num_buffers = m_maxQueueLength + (m_appCfg.bDECODE ? m_decodePoolSize : 0) + (m_pVPP ? 2 : 0) + 4;
        num_buffers = (std::max)(num_buffers, mfxU32(m_maxQueueLength*m_numRefFrame));

        m_preencBufs.Clear();

        for (mfxU32 k = 0; k < num_buffers; k++)
        {
            tmpForInit = new bufSet(m_numOfFields);

            for (fieldId = 0; fieldId < m_numOfFields; fieldId++)
            {
                /* We allocate buffer of progressive frame size for the first field if mixed picstructs are used */
                mfxU32 numMB = (m_appCfg.PipelineCfg.mixedPicstructs && !fieldId) ? m_appCfg.PipelineCfg.numMB_preenc_frame : m_appCfg.PipelineCfg.numMB_preenc_refPic;

                if (fieldId == 0){
                    preENCCtr = new mfxExtFeiPreEncCtrl[m_numOfFields];
                    MSDK_ZERO_ARRAY(preENCCtr, m_numOfFields);
                }

                preENCCtr[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PREENC_CTRL;
                preENCCtr[fieldId].Header.BufferSz = sizeof(mfxExtFeiPreEncCtrl);
                preENCCtr[fieldId].PictureType             = GetCurPicType(fieldId);
                preENCCtr[fieldId].RefPictureType[0]       = preENCCtr[fieldId].PictureType;
                preENCCtr[fieldId].RefPictureType[1]       = preENCCtr[fieldId].PictureType;
                preENCCtr[fieldId].DownsampleInput         = MFX_CODINGOPTION_ON;  // Default is ON
                preENCCtr[fieldId].DownsampleReference[0]  = MFX_CODINGOPTION_OFF; // In sample_fei PreENC works only in encoded order
                preENCCtr[fieldId].DownsampleReference[1]  = MFX_CODINGOPTION_OFF; // so all references would be already downsampled
                preENCCtr[fieldId].DisableMVOutput         = disableMVoutPreENC;
                preENCCtr[fieldId].DisableStatisticsOutput = disableMBStatPreENC;
                preENCCtr[fieldId].FTEnable                = m_appCfg.FTEnable;
                preENCCtr[fieldId].AdaptiveSearch          = m_appCfg.AdaptiveSearch;
                preENCCtr[fieldId].LenSP                   = m_appCfg.LenSP;
                preENCCtr[fieldId].MBQp                    = enableMBQP;
                preENCCtr[fieldId].MVPredictor             = enableMVpredPreENC;
                preENCCtr[fieldId].RefHeight               = m_appCfg.RefHeight;
                preENCCtr[fieldId].RefWidth                = m_appCfg.RefWidth;
                preENCCtr[fieldId].SubPelMode              = m_appCfg.SubPelMode;
                preENCCtr[fieldId].SearchWindow            = m_appCfg.SearchWindow;
                preENCCtr[fieldId].SearchPath              = m_appCfg.SearchPath;
                preENCCtr[fieldId].Qp                      = m_appCfg.QP;
                preENCCtr[fieldId].IntraSAD                = m_appCfg.IntraSAD;
                preENCCtr[fieldId].InterSAD                = m_appCfg.InterSAD;
                preENCCtr[fieldId].SubMBPartMask           = m_appCfg.SubMBPartMask;
                preENCCtr[fieldId].IntraPartMask           = m_appCfg.IntraPartMask;
                preENCCtr[fieldId].Enable8x8Stat           = m_appCfg.Enable8x8Stat;

                if (preENCCtr[fieldId].MVPredictor)
                {
                    if (fieldId == 0){
                        mvPreds = new mfxExtFeiPreEncMVPredictors[m_numOfFields];
                        MSDK_ZERO_ARRAY(mvPreds, m_numOfFields);
                    }

                    mvPreds[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PREENC_MV_PRED;
                    mvPreds[fieldId].Header.BufferSz = sizeof(mfxExtFeiPreEncMVPredictors);
                    mvPreds[fieldId].NumMBAlloc = numMB;
                    mvPreds[fieldId].MB = new mfxExtFeiPreEncMVPredictors::mfxExtFeiPreEncMVPredictorsMB[mvPreds[fieldId].NumMBAlloc];
                    MSDK_ZERO_ARRAY(mvPreds[fieldId].MB, mvPreds[fieldId].NumMBAlloc);
                }

                if (preENCCtr[fieldId].MBQp)
                {
                    if (fieldId == 0){
                        qps = new mfxExtFeiEncQP[m_numOfFields];
                        MSDK_ZERO_ARRAY(qps, m_numOfFields);
                    }

                    qps[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_ENC_QP;
                    qps[fieldId].Header.BufferSz = sizeof(mfxExtFeiEncQP);
#if MFX_VERSION >= 1023
                    qps[fieldId].NumMBAlloc = numMB;
                    qps[fieldId].MB = new mfxU8[qps[fieldId].NumMBAlloc];
                    MSDK_ZERO_ARRAY(qps[fieldId].MB, qps[fieldId].NumMBAlloc);
#else
                    qps[fieldId].NumQPAlloc = numMB;
                    qps[fieldId].QP = new mfxU8[qps[fieldId].NumQPAlloc];
                    MSDK_ZERO_ARRAY(qps[fieldId].QP, qps[fieldId].NumQPAlloc);
#endif
                }

                if (!preENCCtr[fieldId].DisableMVOutput)
                {
                    if (fieldId == 0){
                        mvs = new mfxExtFeiPreEncMV[m_numOfFields];
                        MSDK_ZERO_ARRAY(mvs, m_numOfFields);
                    }

                    mvs[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PREENC_MV;
                    mvs[fieldId].Header.BufferSz = sizeof(mfxExtFeiPreEncMV);
                    mvs[fieldId].NumMBAlloc = numMB;
                    mvs[fieldId].MB = new mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB[mvs[fieldId].NumMBAlloc];
                    MSDK_ZERO_ARRAY(mvs[fieldId].MB, mvs[fieldId].NumMBAlloc);
                }

                if (!preENCCtr[fieldId].DisableStatisticsOutput)
                {
                    if (fieldId == 0){
                        mbdata = new mfxExtFeiPreEncMBStat[m_numOfFields];
                        MSDK_ZERO_ARRAY(mbdata, m_numOfFields);
                    }

                    mbdata[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PREENC_MB;
                    mbdata[fieldId].Header.BufferSz = sizeof(mfxExtFeiPreEncMBStat);
                    mbdata[fieldId].NumMBAlloc = numMB;
                    mbdata[fieldId].MB = new mfxExtFeiPreEncMBStat::mfxExtFeiPreEncMBStatMB[mbdata[fieldId].NumMBAlloc];
                    MSDK_ZERO_ARRAY(mbdata[fieldId].MB, mbdata[fieldId].NumMBAlloc);
                }

            } // for (fieldId = 0; fieldId < m_numOfFields; fieldId++)

            for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(preENCCtr + fieldId));
                tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(preENCCtr + fieldId));
            }
            if (mvPreds){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(mvPreds + fieldId));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(mvPreds + fieldId));
                }
            }
            if (qps){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(qps + fieldId));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(qps + fieldId));
                }
            }
            if (mvs){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit->PB_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(mvs + fieldId));
                }
            }
            if (mbdata){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(mbdata + fieldId));
                    tmpForInit->PB_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(mbdata + fieldId));
                }
            }

            m_preencBufs.AddSet(tmpForInit);
        } // for (int k = 0; k < num_buffers; k++)
    } // if (m_appCfg.bPREENC)

    if ((m_appCfg.bENCODE) || (m_appCfg.bENCPAK) ||
        (m_appCfg.bOnlyPAK) || (m_appCfg.bOnlyENC))
    {
        bufSet*                   tmpForInit         = NULL;
        //FEI buffers
        mfxExtFeiPPS*             feiPPS             = NULL;
        mfxExtFeiSliceHeader*     feiSliceHeader     = NULL;
        mfxExtFeiEncFrameCtrl*    feiEncCtrl         = NULL;
        mfxExtFeiEncMVPredictors* feiEncMVPredictors = NULL;
        mfxExtFeiEncMBCtrl*       feiEncMBCtrl       = NULL;
        mfxExtFeiEncMBStat*       feiEncMbStat       = NULL;
        mfxExtFeiEncMV*           feiEncMV           = NULL;
        mfxExtFeiEncQP*           feiEncMbQp         = NULL;
        mfxExtFeiPakMBCtrl*       feiEncMBCode       = NULL;
        mfxExtFeiRepackCtrl*      feiRepack          = NULL;
        mfxExtPredWeightTable*    feiWeights         = NULL;
#if (MFX_VERSION >= 1025)
        mfxExtFeiRepackStat*      feiRepackStat      = NULL;
#endif

        bool MVPredictors = (m_appCfg.mvinFile   != NULL) || m_appCfg.bPREENC; //couple with PREENC
        bool MBCtrl       = m_appCfg.mbctrinFile != NULL;
        bool MBQP         = m_appCfg.mbQpFile    != NULL;

        bool MBStatOut    = m_appCfg.mbstatoutFile  != NULL;
        bool MVOut        = m_appCfg.mvoutFile      != NULL;
        bool MBCodeOut    = m_appCfg.mbcodeoutFile  != NULL;
        bool RepackCtrl   = m_appCfg.repackctrlFile != NULL;
#if (MFX_VERSION >= 1025)
        bool RepackStat   = m_appCfg.repackstatFile != NULL;
#endif
        bool Weights      = m_appCfg.weightsFile    != NULL;


        mfxU32 num_buffers = m_maxQueueLength + (m_appCfg.bDECODE ? m_decodePoolSize : 0) + (m_pVPP ? 2 : 0);

        m_encodeBufs.Clear();

        for (mfxU32 k = 0; k < num_buffers; k++)
        {
            tmpForInit = new bufSet(m_numOfFields);

            for (fieldId = 0; fieldId < m_numOfFields; fieldId++)
            {
                /* We allocate buffer of progressive frame size for the first field if mixed picstructs are used */
                mfxU32 numMB = (m_appCfg.PipelineCfg.mixedPicstructs && !fieldId) ? m_appCfg.PipelineCfg.numMB_frame : m_appCfg.PipelineCfg.numMB_refPic;

                if (!m_appCfg.bOnlyPAK)
                {
                    if (fieldId == 0){
                        feiEncCtrl = new mfxExtFeiEncFrameCtrl[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiEncCtrl, m_numOfFields);
                    }

                    feiEncCtrl[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_ENC_CTRL;
                    feiEncCtrl[fieldId].Header.BufferSz = sizeof(mfxExtFeiEncFrameCtrl);

                    feiEncCtrl[fieldId].SearchPath             = m_appCfg.SearchPath;
                    feiEncCtrl[fieldId].LenSP                  = m_appCfg.LenSP;
                    feiEncCtrl[fieldId].SubMBPartMask          = m_appCfg.SubMBPartMask;
                    feiEncCtrl[fieldId].MultiPredL0            = m_appCfg.MultiPredL0;
                    feiEncCtrl[fieldId].MultiPredL1            = m_appCfg.MultiPredL1;
                    feiEncCtrl[fieldId].SubPelMode             = m_appCfg.SubPelMode;
                    feiEncCtrl[fieldId].InterSAD               = m_appCfg.InterSAD;
                    feiEncCtrl[fieldId].IntraSAD               = m_appCfg.IntraSAD;
                    feiEncCtrl[fieldId].IntraPartMask          = m_appCfg.IntraPartMask;
                    feiEncCtrl[fieldId].DistortionType         = m_appCfg.DistortionType;
                    feiEncCtrl[fieldId].RepartitionCheckEnable = m_appCfg.RepartitionCheckEnable;
                    feiEncCtrl[fieldId].AdaptiveSearch         = m_appCfg.AdaptiveSearch;
                    feiEncCtrl[fieldId].MVPredictor            = MVPredictors;
                    feiEncCtrl[fieldId].NumMVPredictors[0]     = m_taskInitializationParams.NumMVPredictorsP;
                    feiEncCtrl[fieldId].NumMVPredictors[1]     = m_taskInitializationParams.NumMVPredictorsBL1;
                    feiEncCtrl[fieldId].PerMBQp                = MBQP;
                    feiEncCtrl[fieldId].PerMBInput             = MBCtrl;
                    feiEncCtrl[fieldId].MBSizeCtrl             = m_appCfg.bMBSize;
                    feiEncCtrl[fieldId].ColocatedMbDistortion  = m_appCfg.ColocatedMbDistortion;

                    //Note:
                    //(RefHeight x RefWidth) should not exceed 2048 for P frames and 1024 for B frames
                    feiEncCtrl[fieldId].RefHeight    = m_appCfg.RefHeight;
                    feiEncCtrl[fieldId].RefWidth     = m_appCfg.RefWidth;
                    feiEncCtrl[fieldId].SearchWindow = m_appCfg.SearchWindow;
                }

                /* PPS Header */
                if (m_appCfg.bENCPAK || m_appCfg.bOnlyENC || m_appCfg.bOnlyPAK)
                {
                    if (fieldId == 0){
                        feiPPS = new mfxExtFeiPPS[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiPPS, m_numOfFields);
                    }

                    feiPPS[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PPS;
                    feiPPS[fieldId].Header.BufferSz = sizeof(mfxExtFeiPPS);

                    feiPPS[fieldId].SPSId = 0;
                    feiPPS[fieldId].PPSId = 0;

                    /* PicInitQP should be always 26 !!!
                     * Adjusting of QP parameter should be done via Slice header */
                    feiPPS[fieldId].PicInitQP = 26;

                    feiPPS[fieldId].NumRefIdxL0Active = 1;
                    feiPPS[fieldId].NumRefIdxL1Active = 1;

                    feiPPS[fieldId].ChromaQPIndexOffset       = m_appCfg.ChromaQPIndexOffset;
                    feiPPS[fieldId].SecondChromaQPIndexOffset = m_appCfg.SecondChromaQPIndexOffset;
                    feiPPS[fieldId].Transform8x8ModeFlag      = m_appCfg.Transform8x8ModeFlag;

#if MFX_VERSION >= 1023
                    memset(feiPPS[fieldId].DpbBefore, 0xffff, sizeof(feiPPS[fieldId].DpbBefore));
                    memset(feiPPS[fieldId].DpbAfter,  0xffff, sizeof(feiPPS[fieldId].DpbAfter));
#else
                    memset(feiPPS[fieldId].ReferenceFrames, 0xffff, 16 * sizeof(mfxU16));
#endif // MFX_VERSION >= 1023
                }

#if (MFX_VERSION >= 1025)
                /* Repack Stat */
                if (m_appCfg.bENCODE && RepackStat)
                {
                    if (fieldId == 0){
                        feiRepackStat = new mfxExtFeiRepackStat[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiRepackStat, m_numOfFields);
                    }
                    feiRepackStat[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_REPACK_STAT;
                    feiRepackStat[fieldId].Header.BufferSz = sizeof(mfxExtFeiRepackStat);

                    feiRepackStat[fieldId].NumPasses = 0;
                }
#endif

                /* Slice Header */
                if (m_appCfg.bENCPAK || m_appCfg.bOnlyENC || m_appCfg.bOnlyPAK)
                {
                    if (fieldId == 0){
                        feiSliceHeader = new mfxExtFeiSliceHeader[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiSliceHeader, m_numOfFields);
                    }

                    feiSliceHeader[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_SLICE;
                    feiSliceHeader[fieldId].Header.BufferSz = sizeof(mfxExtFeiSliceHeader);

                    feiSliceHeader[fieldId].NumSlice = m_appCfg.numSlices;
                    feiSliceHeader[fieldId].Slice = new mfxExtFeiSliceHeader::mfxSlice[feiSliceHeader[fieldId].NumSlice];
                    MSDK_ZERO_ARRAY(feiSliceHeader[fieldId].Slice, feiSliceHeader[fieldId].NumSlice);

                    // TODO: Improve slice divider
                    mfxU16 nMBrows = (m_heightMB + feiSliceHeader[fieldId].NumSlice - 1) / feiSliceHeader[fieldId].NumSlice,
                        nMBremain = m_heightMB;
                    for (mfxU16 numSlice = 0; numSlice < feiSliceHeader[fieldId].NumSlice; numSlice++)
                    {
                        feiSliceHeader[fieldId].Slice[numSlice].MBAddress = numSlice*(nMBrows*m_widthMB);
                        feiSliceHeader[fieldId].Slice[numSlice].NumMBs    = (std::min)(nMBrows, nMBremain)*m_widthMB;
                        feiSliceHeader[fieldId].Slice[numSlice].SliceType = 0;
                        feiSliceHeader[fieldId].Slice[numSlice].PPSId     = feiPPS ? feiPPS[fieldId].PPSId : 0;
                        feiSliceHeader[fieldId].Slice[numSlice].IdrPicId  = 0;

                        feiSliceHeader[fieldId].Slice[numSlice].CabacInitIdc = 0;
                        mfxU32 initQP = (m_appCfg.QP != 0) ? m_appCfg.QP : 26;
                        feiSliceHeader[fieldId].Slice[numSlice].SliceQPDelta               = mfxI16(initQP - feiPPS[fieldId].PicInitQP);
                        feiSliceHeader[fieldId].Slice[numSlice].DisableDeblockingFilterIdc = m_appCfg.DisableDeblockingIdc;
                        feiSliceHeader[fieldId].Slice[numSlice].SliceAlphaC0OffsetDiv2     = m_appCfg.SliceAlphaC0OffsetDiv2;
                        feiSliceHeader[fieldId].Slice[numSlice].SliceBetaOffsetDiv2        = m_appCfg.SliceBetaOffsetDiv2;

                        nMBremain -= nMBrows;
                    }
                }

                if (MVPredictors)
                {
                    if (fieldId == 0){
                        feiEncMVPredictors = new mfxExtFeiEncMVPredictors[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiEncMVPredictors, m_numOfFields);
                    }

                    feiEncMVPredictors[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_ENC_MV_PRED;
                    feiEncMVPredictors[fieldId].Header.BufferSz = sizeof(mfxExtFeiEncMVPredictors);
                    feiEncMVPredictors[fieldId].NumMBAlloc = numMB;
                    feiEncMVPredictors[fieldId].MB = new mfxExtFeiEncMVPredictors::mfxExtFeiEncMVPredictorsMB[feiEncMVPredictors[fieldId].NumMBAlloc];
                    MSDK_ZERO_ARRAY(feiEncMVPredictors[fieldId].MB, feiEncMVPredictors[fieldId].NumMBAlloc);
                }

                if (RepackCtrl)
                {
                    if (fieldId == 0){
                        feiRepack = new mfxExtFeiRepackCtrl[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiRepack, m_numOfFields);
                    }

                    feiRepack[fieldId].Header.BufferId =  MFX_EXTBUFF_FEI_REPACK_CTRL;
                    feiRepack[fieldId].Header.BufferSz = sizeof(mfxExtFeiRepackCtrl);
                    feiRepack[fieldId].MaxFrameSize = 0;
                    feiRepack[fieldId].NumPasses    = 1;
                    MSDK_ZERO_ARRAY(feiRepack[fieldId].DeltaQP, 8);
                }

                if (MBCtrl)
                {
                    if (fieldId == 0){
                        feiEncMBCtrl = new mfxExtFeiEncMBCtrl[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiEncMBCtrl, m_numOfFields);
                    }

                    feiEncMBCtrl[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_ENC_MB;
                    feiEncMBCtrl[fieldId].Header.BufferSz = sizeof(mfxExtFeiEncMBCtrl);
                    feiEncMBCtrl[fieldId].NumMBAlloc = numMB;
                    feiEncMBCtrl[fieldId].MB = new mfxExtFeiEncMBCtrl::mfxExtFeiEncMBCtrlMB[feiEncMBCtrl[fieldId].NumMBAlloc];
                    MSDK_ZERO_ARRAY(feiEncMBCtrl[fieldId].MB, feiEncMBCtrl[fieldId].NumMBAlloc);
                }

                if (MBQP)
                {
                    if (fieldId == 0){
                        feiEncMbQp = new mfxExtFeiEncQP[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiEncMbQp, m_numOfFields);
                    }

                    feiEncMbQp[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_ENC_QP;
                    feiEncMbQp[fieldId].Header.BufferSz = sizeof(mfxExtFeiEncQP);
#if MFX_VERSION >= 1023
                    feiEncMbQp[fieldId].NumMBAlloc = numMB;
                    feiEncMbQp[fieldId].MB = new mfxU8[feiEncMbQp[fieldId].NumMBAlloc];
                    MSDK_ZERO_ARRAY(feiEncMbQp[fieldId].MB, feiEncMbQp[fieldId].NumMBAlloc);
#else
                    feiEncMbQp[fieldId].NumQPAlloc = numMB;
                    feiEncMbQp[fieldId].QP = new mfxU8[feiEncMbQp[fieldId].NumQPAlloc];
                    MSDK_ZERO_ARRAY(feiEncMbQp[fieldId].QP, feiEncMbQp[fieldId].NumQPAlloc);
#endif
                }

                if (Weights)
                {
                    if (fieldId == 0){
                        feiWeights = new mfxExtPredWeightTable[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiWeights, m_numOfFields);
                    }

                    feiWeights[fieldId].Header.BufferId = MFX_EXTBUFF_PRED_WEIGHT_TABLE;
                    feiWeights[fieldId].Header.BufferSz = sizeof(mfxExtPredWeightTable);
                }

                if (MBStatOut)
                {
                    if (fieldId == 0){
                        feiEncMbStat = new mfxExtFeiEncMBStat[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiEncMbStat, m_numOfFields);
                    }

                    feiEncMbStat[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_ENC_MB_STAT;
                    feiEncMbStat[fieldId].Header.BufferSz = sizeof(mfxExtFeiEncMBStat);
                    feiEncMbStat[fieldId].NumMBAlloc = numMB;
                    feiEncMbStat[fieldId].MB = new mfxExtFeiEncMBStat::mfxExtFeiEncMBStatMB[feiEncMbStat[fieldId].NumMBAlloc];
                    MSDK_ZERO_ARRAY(feiEncMbStat[fieldId].MB, feiEncMbStat[fieldId].NumMBAlloc);
                }

                if ((MVOut) || (m_appCfg.bENCPAK) || (m_appCfg.bOnlyPAK))
                {
                    MVOut = true; /* ENC_PAK need MVOut and MBCodeOut buffer */
                    if (fieldId == 0){
                        feiEncMV = new mfxExtFeiEncMV[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiEncMV, m_numOfFields);
                    }

                    feiEncMV[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_ENC_MV;
                    feiEncMV[fieldId].Header.BufferSz = sizeof(mfxExtFeiEncMV);
                    feiEncMV[fieldId].NumMBAlloc = numMB;
                    feiEncMV[fieldId].MB = new mfxExtFeiEncMV::mfxExtFeiEncMVMB[feiEncMV[fieldId].NumMBAlloc];
                    MSDK_ZERO_ARRAY(feiEncMV[fieldId].MB, feiEncMV[fieldId].NumMBAlloc);
                }

                //distortion buffer have to be always provided - current limitation
                if ((MBCodeOut) || (m_appCfg.bENCPAK) || (m_appCfg.bOnlyPAK))
                {
                    MBCodeOut = true; /* ENC_PAK need MVOut and MBCodeOut buffer */
                    if (fieldId == 0){
                        feiEncMBCode = new mfxExtFeiPakMBCtrl[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiEncMBCode, m_numOfFields);
                    }

                    feiEncMBCode[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PAK_CTRL;
                    feiEncMBCode[fieldId].Header.BufferSz = sizeof(mfxExtFeiPakMBCtrl);
                    feiEncMBCode[fieldId].NumMBAlloc = numMB;
                    feiEncMBCode[fieldId].MB = new mfxFeiPakMBCtrl[feiEncMBCode[fieldId].NumMBAlloc];
                    MSDK_ZERO_ARRAY(feiEncMBCode[fieldId].MB, feiEncMBCode[fieldId].NumMBAlloc);
                }

            } // for (fieldId = 0; fieldId < m_numOfFields; fieldId++)

            if (feiEncCtrl)
            {
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncCtrl[fieldId]));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncCtrl[fieldId]));
                }
            }

            if (feiPPS){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiPPS[fieldId]));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiPPS[fieldId]));
                }
            }
            if (feiSliceHeader){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiSliceHeader[fieldId]));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiSliceHeader[fieldId]));
                }
            }
            if (MVPredictors){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    //tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(feiEncMVPredictors[fieldId]);
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMVPredictors[fieldId]));
                }
            }
            if (MBCtrl){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMBCtrl[fieldId]));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMBCtrl[fieldId]));
                }
            }
            if (MBQP){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMbQp[fieldId]));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMbQp[fieldId]));
                }
            }
            if (RepackCtrl){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiRepack[fieldId]));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiRepack[fieldId]));
                }
            }
            if (Weights){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    //weight table is never for I frame
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiWeights[fieldId]));
                }
            }
            if (MBStatOut){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMbStat[fieldId]));
                    tmpForInit->PB_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMbStat[fieldId]));
                }
            }
            if (MVOut){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMV[fieldId]));
                    tmpForInit->PB_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMV[fieldId]));
                }
            }
            if (MBCodeOut){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMBCode[fieldId]));
                    tmpForInit->PB_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMBCode[fieldId]));
                }
            }
#if (MFX_VERSION >= 1025)
            if (RepackStat){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>
                                                (&feiRepackStat[fieldId]));
                    tmpForInit->PB_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>
                                                (&feiRepackStat[fieldId]));
                }
            }
#endif

            m_encodeBufs.AddSet(tmpForInit);

        } // for (int k = 0; k < num_buffers; k++)

    } // if (m_appCfg.bENCODE)

    return sts;
}

mfxStatus CEncodingPipeline::Run()
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1* pSurf = NULL; // points to frame being processed

    bool swap_fields = false;

    iTask* eTask = NULL; // encoding task
    time_t start = time(0);

    // main loop, preprocessing and encoding
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts)
    {
        /* Break if we achieved frame limit or by timeout */
        if ((m_appCfg.nNumFrames && m_frameCount >= m_appCfg.nNumFrames)
            || (m_appCfg.nTimeout && (time(0) - start >= m_appCfg.nTimeout)))
        {
            break;
        }

        /* Get income frame */
        sts = GetOneFrame(pSurf);
        if (sts == MFX_ERR_GPU_HANG)
        {
            sts = doGPUHangRecovery();
            MSDK_CHECK_STATUS(sts, "doGPUHangRecovery failed");
            continue;             // New cycle if GPU Recovered
        }

        if (sts == MFX_ERR_MORE_DATA && m_appCfg.nTimeout) // New cycle in loop mode
        {
            m_insertIDR = true;
            sts = ResetIOFiles();
            MSDK_CHECK_STATUS(sts, "ResetIOFiles failed");
            continue;
        }
        MSDK_BREAK_ON_ERROR(sts);

        if (m_bNeedDRC)
        {
            sts = ResizeFrame(m_frameCount, pSurf->Info.PicStruct & 0xf);
        }

        if (m_insertIDR)
        {
            m_frameType = PairU8(mfxU8(MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF),
                                 mfxU8(MFX_FRAMETYPE_P                     | MFX_FRAMETYPE_REF));
            m_insertIDR = false;
        }
        else
        {
            m_frameType = GetFrameType(m_frameCount - m_frameOrderIdrInDisplayOrder);
        }

        if (m_frameType[0] & MFX_FRAMETYPE_IDR)
            m_frameOrderIdrInDisplayOrder = m_frameCount;

        pSurf->Data.FrameOrder = m_frameCount; // in display order

        m_frameCount++;

        swap_fields = (pSurf->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF) && !(pSurf->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE);
        if (swap_fields)
            std::swap(m_frameType[0], m_frameType[1]);

        if (m_pVPP)
        {
            // pre-process a frame
            sts = PreProcessOneFrame(pSurf);
            if (sts == MFX_ERR_GPU_HANG)
            {
                sts = doGPUHangRecovery();
                MSDK_CHECK_STATUS(sts, "doGPUHangRecovery failed");
                continue;
            }
            MSDK_BREAK_ON_ERROR(sts);
        }

        /* Reorder income frame */
        {
            /* PreENC on downsampled surface */
            if (m_appCfg.bPREENC && m_appCfg.preencDSstrength)
            {
                m_taskInitializationParams.DSsurface = m_DSSurfaces.GetFreeSurface_FEI();
                MSDK_CHECK_POINTER(m_taskInitializationParams.DSsurface, MFX_ERR_MEMORY_ALLOC);
            }

            m_taskInitializationParams.PicStruct  = pSurf->Info.PicStruct & 0xf;
            m_taskInitializationParams.InputSurf  = pSurf;
            m_taskInitializationParams.FrameType  = m_frameType;
            m_taskInitializationParams.FrameCount = m_frameCount - 1;
            m_taskInitializationParams.FrameOrderIdrInDisplayOrder = m_frameOrderIdrInDisplayOrder;
            m_taskInitializationParams.SingleFieldMode = m_appCfg.bFieldProcessingMode;

            m_inputTasks.AddTask(new iTask(m_taskInitializationParams));
            eTask = m_inputTasks.GetTaskToEncode(false);

            // No frame to process right now (bufferizing B-frames, need more input frames)
            if (!eTask) continue;

            // Set reconstruct surface for ENC / PAK
            if (m_appCfg.bENCPAK || m_appCfg.bOnlyENC || m_appCfg.bOnlyPAK)
            {
                /* FEI ENC and (or) PAK requires separate pool for reconstruct surfaces */
                mfxFrameSurface1 * recon_surf = m_ReconSurfaces.GetFreeSurface_FEI();
                MSDK_CHECK_POINTER(recon_surf, MFX_ERR_MEMORY_ALLOC);

                eTask->SetReconSurf(recon_surf);
            }
        }

        if (m_appCfg.bPREENC)
        {
            sts = m_pFEI_PreENC->PreencOneFrame(eTask);
            if (sts == MFX_ERR_GPU_HANG)
            {
                sts = doGPUHangRecovery();
                MSDK_CHECK_STATUS(sts, "doGPUHangRecovery failed");
                continue;
            }
            MSDK_BREAK_ON_ERROR(sts);
        }

        if (m_appCfg.bENCPAK || m_appCfg.bOnlyPAK || m_appCfg.bOnlyENC)
        {
#if (MFX_VERSION >= 1024)
            mfxBRCFrameParam BRCPar;
            mfxBRCFrameCtrl BRCCtrl;
            MSDK_ZERO_MEMORY(BRCPar);
            MSDK_ZERO_MEMORY(BRCCtrl);

            if (m_appCfg.RateControlMethod == MFX_RATECONTROL_VBR){
                //get QP
                BRCPar.EncodedOrder = m_frameCountInEncodedOrder++;
                BRCPar.DisplayOrder = eTask->m_frameOrder;
                BRCPar.FrameType = eTask->m_type[eTask->m_fid[0]];       //progressive only for now
                sts = m_BRC.GetFrameCtrl (&BRCPar, &BRCCtrl);
                MSDK_BREAK_ON_ERROR(sts);
                m_appCfg.QP = BRCCtrl.QpY;
            }
#endif

            sts = m_pFEI_ENCPAK->EncPakOneFrame(eTask);
            if (sts == MFX_ERR_GPU_HANG)
            {
                sts = doGPUHangRecovery();
                MSDK_CHECK_STATUS(sts, "doGPUHangRecovery failed");
                continue;
            }
            MSDK_BREAK_ON_ERROR(sts);

#if (MFX_VERSION >= 1024)
            if (m_appCfg.RateControlMethod == MFX_RATECONTROL_VBR){
                //update QP
                BRCPar.CodedFrameSize = eTask->EncodedFrameSize;
                mfxBRCFrameStatus bsts;
                sts = m_BRC.Update(&BRCPar, &BRCCtrl, &bsts);
                MSDK_BREAK_ON_ERROR(sts);
                if(bsts.BRCStatus != MFX_BRC_OK) { //reencode is not supported
                    sts = MFX_ERR_UNDEFINED_BEHAVIOR;
                    break;
                }
            }
#endif
        }

        if (m_appCfg.bENCODE)
        {
            sts = m_pFEI_ENCODE->EncodeOneFrame(eTask);
            if (sts == MFX_ERR_GPU_HANG)
            {
                sts = doGPUHangRecovery();
                MSDK_CHECK_STATUS(sts, "doGPUHangRecovery failed");
                continue;
            }
            MSDK_BREAK_ON_ERROR(sts);
        }

        m_inputTasks.UpdatePool();

    } // while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts)

    // means that the input file has ended, need to go to buffering loops
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    // exit in case of other errors
    MSDK_CHECK_STATUS(sts, "Unexpected error!!");

    sts = ProcessBufferedFrames();
    // MFX_ERR_NOT_FOUND is the correct status to exit the loop with
    // EncodeFrameAsync and SyncOperation, so don't return this status
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_FOUND);
    // report any errors that occurred in asynchronous part
    MSDK_CHECK_STATUS(sts, "Buffered frames processing failed");

    return sts;
}

mfxStatus CEncodingPipeline::ProcessBufferedFrames()
{
    mfxStatus sts = MFX_ERR_NONE;

    // loop to get buffered frames from encoder
    if (m_appCfg.EncodedOrder)
    {
        iTask* eTask = NULL;
        mfxU32 numUnencoded = m_inputTasks.CountUnencodedFrames();

        if (numUnencoded) { m_inputTasks.ProcessLastB(); }

        while (numUnencoded != 0)
        {
            eTask = m_inputTasks.GetTaskToEncode(true);
            MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

            if (m_appCfg.bPREENC)
            {
                sts = m_pFEI_PreENC->PreencOneFrame(eTask);
                MSDK_BREAK_ON_ERROR(sts);
            }

            if ((m_appCfg.bENCPAK) || (m_appCfg.bOnlyPAK) || (m_appCfg.bOnlyENC))
            {
                // Set reconstruct surface for ENC / PAK
                mfxFrameSurface1 * recon_surf = m_ReconSurfaces.GetFreeSurface_FEI();
                MSDK_CHECK_POINTER(recon_surf, MFX_ERR_MEMORY_ALLOC);

                eTask->SetReconSurf(recon_surf);

                sts = m_pFEI_ENCPAK->EncPakOneFrame(eTask);
                MSDK_BREAK_ON_ERROR(sts);
            }

            if (m_appCfg.bENCODE)
            {
                sts = m_pFEI_ENCODE->EncodeOneFrame(eTask);
                MSDK_BREAK_ON_ERROR(sts);
            }

            m_inputTasks.UpdatePool();

            --numUnencoded;
        }
    }
    else // ENCODE in display order
    {
        if (m_appCfg.bENCODE)
        {
            while (MFX_ERR_NONE <= sts)
            {
                sts = m_pFEI_ENCODE->EncodeOneFrame(NULL);
            }

            // MFX_ERR_MORE_DATA is the correct status to exit buffering loop with
            // indicates that there are no more buffered frames
            MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
            // exit in case of other errors
            MSDK_CHECK_STATUS(sts, "EncodeOneFrame failed");
        }
    }

    return sts;
}

/* read input frame */
mfxStatus CEncodingPipeline::GetOneFrame(mfxFrameSurface1* & pSurf)
{
    if (!m_appCfg.bDECODE) // load frame from YUV file
    {
        mfxStatus sts = m_pYUVReader->GetOneFrame(pSurf);
        // Fill picstruct
        if (pSurf) { pSurf->Info.PicStruct = m_picStruct; }
        return sts;
    }
    else // decode frame
    {
        return m_pDECODE->GetOneFrame(pSurf);
    }
}

inline mfxU16 CEncodingPipeline::GetCurPicType(mfxU32 fieldId)
{
    switch (m_picStruct & 0xf)
    {
    case MFX_PICSTRUCT_FIELD_TFF:
        return mfxU16(fieldId ? MFX_PICTYPE_BOTTOMFIELD : MFX_PICTYPE_TOPFIELD);

    case MFX_PICSTRUCT_FIELD_BFF:
        return mfxU16(fieldId ? MFX_PICTYPE_TOPFIELD : MFX_PICTYPE_BOTTOMFIELD);

    case MFX_PICSTRUCT_PROGRESSIVE:
    default:
        return MFX_PICTYPE_FRAME;
    }
}

mfxStatus CEncodingPipeline::PreProcessOneFrame(mfxFrameSurface1* & pSurf)
{
    MFX_ITT_TASK("VPPOneFrame");
    MSDK_CHECK_POINTER(pSurf, MFX_ERR_NULL_PTR);
    mfxStatus sts = MFX_ERR_NONE;

    // find/wait for a free working surface
    mfxFrameSurface1* pSurf_out = m_EncSurfaces.GetFreeSurface_FEI();
    MSDK_CHECK_POINTER(pSurf_out, MFX_ERR_MEMORY_ALLOC);
    pSurf_out->Info.PicStruct = pSurf->Info.PicStruct;

    // update output surface parameters in case of Dynamic Resolution Change
    if (m_bNeedDRC)
    {
        pSurf_out->Info.Width  = m_commonFrameInfo.Width;
        pSurf_out->Info.Height = m_commonFrameInfo.Height;
        pSurf_out->Info.CropW  = m_commonFrameInfo.CropW;
        pSurf_out->Info.CropH  = m_commonFrameInfo.CropH;
        pSurf_out->Info.CropX  = 0;
        pSurf_out->Info.CropY  = 0;
    }

    sts = m_pVPP->VPPoneFrame(pSurf, pSurf_out);
    MSDK_CHECK_STATUS(sts, "VPP: frame processing failed");

    pSurf = pSurf_out;

    return sts;
}

mfxStatus CEncodingPipeline::ResizeFrame(mfxU32 m_frameCount, mfxU16 picstruct)
{
    mfxStatus sts = MFX_ERR_NONE;

    bool reset_point = (m_DRCqueue.size() > m_nDRC_idx) && (m_DRCqueue[m_nDRC_idx].start_frame == m_frameCount);

    if (!reset_point)
        return sts;

    sts = ProcessBufferedFrames();
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_NOT_FOUND);
    MSDK_CHECK_STATUS(sts, "Buffered frames processing failed");

    // Clear income queue
    m_inputTasks.Clear();

    m_appCfg.PipelineCfg.DRCresetPoint = true;

    m_insertIDR = true;

    mfxU16 n_fields = (picstruct & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2;
    mfxU16 wdt = MSDK_ALIGN16(m_DRCqueue[m_nDRC_idx].target_w),
           hgt = (n_fields == 1) ? MSDK_ALIGN16(m_DRCqueue[m_nDRC_idx].target_h) : MSDK_ALIGN32(m_DRCqueue[m_nDRC_idx].target_h);

    m_appCfg.PipelineCfg.numMB_drc_curr = ((wdt * hgt) >> 8) / n_fields;

    sts = m_pVPP->Reset(wdt, hgt, m_DRCqueue[m_nDRC_idx].target_w, m_DRCqueue[m_nDRC_idx].target_h);
    MSDK_CHECK_STATUS(sts, "VPP: Reset failed");

    m_commonFrameInfo = m_pVPP->GetCommonVideoParams()->vpp.Out;

    if (m_pFEI_PreENC) { sts = m_pFEI_PreENC->Reset(wdt, hgt, m_DRCqueue[m_nDRC_idx].target_w, m_DRCqueue[m_nDRC_idx].target_h); MSDK_CHECK_STATUS(sts, "PreENC: Reset failed"); }
    if (m_pFEI_ENCODE) { sts = m_pFEI_ENCODE->Reset(wdt, hgt, m_DRCqueue[m_nDRC_idx].target_w, m_DRCqueue[m_nDRC_idx].target_h); MSDK_CHECK_STATUS(sts, "ENCODE: Reset failed"); }
    if (m_pFEI_ENCPAK) { sts = m_pFEI_ENCPAK->Reset(wdt, hgt, m_DRCqueue[m_nDRC_idx].target_w, m_DRCqueue[m_nDRC_idx].target_h); MSDK_CHECK_STATUS(sts, "ENCPAK: Reset failed"); }

    ++m_nDRC_idx;
    return sts;
}

/* GPU Hang Recovery */

mfxStatus CEncodingPipeline::doGPUHangRecovery()
{
    msdk_printf(MSDK_STRING("GPU Hang detected. Recovering...\n"));
    mfxStatus sts = MFX_ERR_NONE;

    msdk_printf(MSDK_STRING("Recreation of pipeline...\n"));

    Close();
    sts = Init();
    MSDK_CHECK_STATUS(sts, "Init failed");

    m_frameCount = 0;

    msdk_printf(MSDK_STRING("Pipeline successfully recovered\n"));
    return sts;
}

/* free decode streamout buffers*/

void CEncodingPipeline::ClearDecoderBuffers()
{
    if (m_appCfg.bDECODESTREAMOUT)
    {
        m_pExtBufDecodeStreamout = NULL;

        for (mfxU32 i = 0; i < m_StreamoutBufs.size(); i++)
        {
            MSDK_SAFE_DELETE_ARRAY(m_StreamoutBufs[i]->MB);
            MSDK_SAFE_DELETE(m_StreamoutBufs[i]);
        }

        m_StreamoutBufs.clear();
    }
}


/* Info printing */

void CEncodingPipeline::PrintInfo()
{
    msdk_printf(MSDK_STRING("Intel(R) Media SDK FEI Sample Version %s\n"), GetMSDKSampleVersion().c_str());
    if (!m_pDECODE)
        msdk_printf(MSDK_STRING("\nInput file format\t%s\n"), ColorFormatToStr(m_pYUVReader->m_FileReader.m_ColorFormat));
    else
        msdk_printf(MSDK_STRING("\nInput  video\t\t%s\n"), CodecIdToStr(m_appCfg.DecodeId).c_str());
    msdk_printf(MSDK_STRING("Output video\t\t%s\n"), CodecIdToStr(m_appCfg.CodecId).c_str());

    mfxFrameInfo & SrcPicInfo = m_pVPP ? m_appCfg.PipelineCfg.pVppVideoParam->vpp.In : m_commonFrameInfo;
    mfxFrameInfo & DstPicInfo = m_commonFrameInfo;

    msdk_printf(MSDK_STRING("\nSource picture:\n"));
    msdk_printf(MSDK_STRING("\tResolution\t%dx%d\n"), SrcPicInfo.Width, SrcPicInfo.Height);
    msdk_printf(MSDK_STRING("\tCrop X,Y,W,H\t%d,%d,%d,%d\n"), SrcPicInfo.CropX, SrcPicInfo.CropY, SrcPicInfo.CropW, SrcPicInfo.CropH);

    msdk_printf(MSDK_STRING("\nDestination picture:\n"));
    msdk_printf(MSDK_STRING("\tResolution\t%dx%d\n"), DstPicInfo.Width, DstPicInfo.Height);
    msdk_printf(MSDK_STRING("\tCrop X,Y,W,H\t%d,%d,%d,%d\n"), DstPicInfo.CropX, DstPicInfo.CropY, DstPicInfo.CropW, DstPicInfo.CropH);

    msdk_printf(MSDK_STRING("\nFrame rate\t\t%.2f\n"), DstPicInfo.FrameRateExtN * 1.0 / DstPicInfo.FrameRateExtD);

    const msdk_char* sMemType = m_bUseHWmemory ? MSDK_STRING("hw") : MSDK_STRING("system");
    msdk_printf(MSDK_STRING("Memory type\t\t%s\n"), sMemType);

    mfxVersion ver;
    m_mfxSession.QueryVersion(&ver);
    msdk_printf(MSDK_STRING("Media SDK version\t%d.%d\n"), ver.Major, ver.Minor);

    msdk_printf(MSDK_STRING("\n"));
}
