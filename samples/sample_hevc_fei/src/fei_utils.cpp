/******************************************************************************\
Copyright (c) 2017-2019, Intel Corporation
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

#include "fei_utils.h"

SurfacesPool::SurfacesPool(MFXFrameAllocator* allocator)
    : m_pAllocator(allocator)
{
    MSDK_ZERO_MEMORY(m_response);
}

SurfacesPool::~SurfacesPool()
{
    if (m_pAllocator)
    {
        m_pAllocator->Free(m_pAllocator->pthis, &m_response);
        m_pAllocator = NULL;
    }
}

void SurfacesPool::SetAllocator(MFXFrameAllocator* allocator)
{
    m_pAllocator = allocator;
}

mfxStatus SurfacesPool::AllocSurfaces(mfxFrameAllocRequest& request)
{
    MSDK_CHECK_POINTER(m_pAllocator, MFX_ERR_NULL_PTR);

    // alloc frames
    mfxStatus sts = m_pAllocator->Alloc(m_pAllocator->pthis, &request, &m_response);
    MSDK_CHECK_STATUS(sts, "m_pAllocator->Alloc failed");

    mfxFrameSurface1 surf;
    MSDK_ZERO_MEMORY(surf);

    MSDK_MEMCPY_VAR(surf.Info, &request.Info, sizeof(mfxFrameInfo));

    m_pool.reserve(m_response.NumFrameActual);
    for (mfxU16 i = 0; i < m_response.NumFrameActual; i++)
    {
        if (m_response.mids)
        {
            surf.Data.MemId = m_response.mids[i];
        }
        m_pool.push_back(surf);
    }

    return MFX_ERR_NONE;
}

mfxFrameSurface1* SurfacesPool::GetFreeSurface()
{
    if (m_pool.empty()) // seems AllocSurfaces wasn't called
        return NULL;

    mfxU16 idx = GetFreeSurfaceIndex(m_pool.data(), m_pool.size());

    return (idx != MSDK_INVALID_SURF_IDX) ? &m_pool[idx] : NULL;
}

mfxStatus SurfacesPool::LockSurface(mfxFrameSurface1* pSurf)
{
    mfxStatus sts = MFX_ERR_NOT_INITIALIZED;

    if (m_pAllocator)
    {
        sts = m_pAllocator->Lock(m_pAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
        MSDK_CHECK_STATUS(sts, "m_pAllocator->Lock failed");
    }

    return sts;
}

mfxStatus SurfacesPool::UnlockSurface(mfxFrameSurface1* pSurf)
{
    mfxStatus sts = MFX_ERR_NOT_INITIALIZED;

    if (m_pAllocator)
    {
        sts = m_pAllocator->Unlock(m_pAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
        MSDK_CHECK_STATUS(sts, "m_pAllocator->Unlock failed");
    }

    return sts;
}

/**********************************************************************************/

YUVReader::~YUVReader()
{
    Close();
}

void YUVReader::Close()
{
    m_FileReader.Close();
}

mfxStatus YUVReader::QueryIOSurf(mfxFrameAllocRequest* request)
{
    if (request)
    {
        MSDK_ZERO_MEMORY(*request);

        MSDK_MEMCPY_VAR(request->Info, &m_frameInfo, sizeof(mfxFrameInfo));
        request->NumFrameSuggested = request->NumFrameMin = 1;
    }

    return MFX_ERR_NONE;
}

// fill FrameInfo structure with user parameters taking into account that input stream sequence
// will be stored in MSDK surfaces, i.e. width/height should be aligned, FourCC within supported formats
mfxStatus YUVReader::FillInputFrameInfo(mfxFrameInfo& fi)
{
    MSDK_ZERO_MEMORY(fi);

    bool isProgressive = (MFX_PICSTRUCT_PROGRESSIVE == m_inPars.nPicStruct);
    fi.FourCC          = MFX_FOURCC_NV12;
    fi.ChromaFormat    = FourCCToChroma(fi.FourCC);
    fi.PicStruct       = m_inPars.nPicStruct;

    fi.CropX  = 0;
    fi.CropY  = 0;
    fi.CropW  = m_inPars.nWidth;
    fi.CropH  = m_inPars.nHeight;
    fi.Width  = MSDK_ALIGN16(fi.CropW);
    fi.Height = isProgressive ? MSDK_ALIGN16(fi.CropH) : MSDK_ALIGN32(fi.CropH);

    mfxStatus sts = ConvertFrameRate(m_inPars.dFrameRate, &fi.FrameRateExtN, &fi.FrameRateExtD);
    MSDK_CHECK_STATUS(sts, "ConvertFrameRate failed");

    return MFX_ERR_NONE;
}

mfxStatus YUVReader::PreInit()
{
    return FillInputFrameInfo(m_frameInfo);
}

mfxStatus YUVReader::GetActualFrameInfo(mfxFrameInfo & info)
{
    info = m_frameInfo;

    return MFX_ERR_NONE;
}

mfxStatus YUVReader::Init()
{
    std::list<msdk_string> in_file_names;
    in_file_names.push_back(msdk_string(m_inPars.strSrcFile));
    return m_FileReader.Init(in_file_names, m_srcColorFormat);
}

mfxStatus YUVReader::GetFrame(mfxFrameSurface1* & pSurf)
{
    mfxStatus sts = MFX_ERR_NONE;

    // point pSurf to surface from shared surface pool
    pSurf = m_pOutSurfPool->GetFreeSurface();
    MSDK_CHECK_POINTER(pSurf, MFX_ERR_MEMORY_ALLOC);

    // need to call Lock to access surface data and...
    mfxStatus lock_sts = m_pOutSurfPool->LockSurface(pSurf);
    MSDK_CHECK_STATUS(lock_sts, "LockSurface failed");

    // load frame from file to surface data
    sts = m_FileReader.LoadNextFrame(pSurf);

    // ... after we're done call Unlock
    lock_sts = m_pOutSurfPool->UnlockSurface(pSurf);
    MSDK_CHECK_STATUS(lock_sts, "UnlockSurface failed");

    if (MFX_ERR_MORE_DATA == sts) // reach the end of input file
        pSurf = NULL;

    return sts;
}

mfxStatus YUVReader::ResetIOState()
{
    m_FileReader.Reset();

    return MFX_ERR_NONE;
}

/**********************************************************************************/

mfxStatus Decoder::QueryIOSurf(mfxFrameAllocRequest* request)
{
    return m_DEC->QueryIOSurf(&m_par, request);
}

mfxStatus Decoder::PreInit()
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_FileReader.Init(m_inPars.strSrcFile);
    MSDK_CHECK_STATUS(sts, "Can't open input file");

    m_Bitstream.Extend(1024 * 1024);

    m_DEC.reset(new MFXVideoDECODE(*m_session));

    m_par.AsyncDepth  = 1;
    m_par.mfx.CodecId = m_inPars.DecodeId;
    m_par.IOPattern   = MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    sts = InitDecParams(m_par);
    MSDK_CHECK_STATUS(sts, "Can't initialize decoder params");

    // if framerate is not specified in bitstream, use framerate from SourceFrameInfo structure
    // that always contain valid one (user-defined or default - 30fps)
    if (0 == (m_par.mfx.FrameInfo.FrameRateExtN | m_par.mfx.FrameInfo.FrameRateExtD))
    {
        sts = ConvertFrameRate(m_inPars.dFrameRate, &m_par.mfx.FrameInfo.FrameRateExtN, &m_par.mfx.FrameInfo.FrameRateExtD);
        MSDK_CHECK_STATUS(sts, "ConvertFrameRate failed");
    }

    return sts;
}

mfxStatus Decoder::GetActualFrameInfo(mfxFrameInfo & info)
{
    info = m_par.mfx.FrameInfo;

    return MFX_ERR_NONE;
}

mfxStatus Decoder::Init()
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_DEC->Init(&m_par);
    MSDK_CHECK_STATUS(sts, "Decoder initialization failed");

    return sts;
}

mfxStatus Decoder::GetFrame(mfxFrameSurface1* & outSrf)
{
    mfxStatus sts = MFX_ERR_MORE_SURFACE;
    mfxFrameSurface1 * workSrf = NULL;
    mfxSyncPoint syncp = NULL;
    bool bEOS = false;

    while (MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE < sts)
    {
        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            WaitForDeviceToBecomeFree(*m_session, m_LastSyncp, sts);
        }
        else if (MFX_ERR_MORE_DATA == sts)
        {
            sts = m_FileReader.ReadNextFrame(&m_Bitstream);
            if (MFX_ERR_MORE_DATA == sts)
            {
                bEOS = true;
                sts = MFX_ERR_NONE;
            }
            MSDK_BREAK_ON_ERROR(sts);
        }
        else if (MFX_ERR_MORE_SURFACE == sts)
        {
            workSrf = m_pOutSurfPool->GetFreeSurface();
            MSDK_CHECK_POINTER(workSrf, MFX_ERR_MEMORY_ALLOC);
        }

        sts = m_DEC->DecodeFrameAsync(bEOS ? NULL : &m_Bitstream, workSrf, &outSrf, &syncp);

        if (bEOS && MFX_ERR_MORE_DATA == sts)
            break;

        if (MFX_ERR_NONE == sts)
        {
            m_LastSyncp = syncp;
        }

        if (syncp && MFX_ERR_NONE < sts)
        {
            sts = MFX_ERR_NONE;
        }

    }
    if (MFX_ERR_NONE == sts && syncp)
    {
        sts = m_session->SyncOperation(syncp, MSDK_WAIT_INTERVAL);
        MSDK_CHECK_STATUS(sts, "Decoder SyncOperation failed");
    }

    return sts;
}

void Decoder::Close()
{
    return;
}

mfxStatus Decoder::InitDecParams(MfxVideoParamsWrapper & par)
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = m_FileReader.ReadNextFrame(&m_Bitstream);
    if (MFX_ERR_MORE_DATA == sts)
        return sts;

    MSDK_CHECK_STATUS(sts, "ReadNextFrame failed");

    for (;;)
    {
        sts = m_DEC->DecodeHeader(&m_Bitstream, &par);

        if (MFX_ERR_MORE_DATA == sts)
        {
            if (m_Bitstream.MaxLength == m_Bitstream.DataLength)
            {
                m_Bitstream.Extend(m_Bitstream.MaxLength * 2);
            }

            sts = m_FileReader.ReadNextFrame(&m_Bitstream);
            if (MFX_ERR_MORE_DATA == sts)
                return sts;
        }
        else
            break;
    }

    return sts;
}

mfxStatus Decoder::ResetIOState()
{
    m_FileReader.Reset();

    return MFX_ERR_NONE;
}

/**********************************************************************************/

mfxStatus FieldSplitter::QueryIOSurf(mfxFrameAllocRequest* request)
{
    MSDK_CHECK_POINTER(request, MFX_ERR_NOT_INITIALIZED);

    mfxFrameAllocRequest vppRequest[2];
    MSDK_ZERO_MEMORY(vppRequest);

    mfxStatus sts = m_VPP->QueryIOSurf(&m_par, vppRequest);
    MSDK_CHECK_STATUS(sts, "VPP QueryIOSurf failed");

    *request = vppRequest[1];

    return sts;
}

mfxStatus FieldSplitter::PreInit()
{
    MSDK_CHECK_POINTER(m_parentSession, MFX_ERR_NOT_INITIALIZED);

    // base initialization
    mfxStatus sts = m_pTarget->PreInit();
    MSDK_CHECK_STATUS(sts, "m_pTarget->PreInit failed");

    // Fill mfxVideoParam
    {
        m_par.AsyncDepth  = 1;
        m_par.IOPattern   = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;

        sts = m_pTarget->GetActualFrameInfo(m_par.vpp.In);
        MSDK_CHECK_STATUS(sts, "m_pTarget->GetActualFrameInfo failed");

        // If frame rate is not specified and input stream header doesn't contain valid values, then use default (30.0)
        if (0 == (m_par.vpp.In.FrameRateExtN | m_par.vpp.In.FrameRateExtD))
        {
            m_par.vpp.In.FrameRateExtN = 30;
            m_par.vpp.In.FrameRateExtD = 1;
        }

        m_par.vpp.Out = m_par.vpp.In;
        m_par.vpp.Out.Height   >>= 1;
        m_par.vpp.Out.CropH    >>= 1;
        m_par.vpp.Out.PicStruct  = MFX_PICSTRUCT_FIELD_SINGLE;
    }

    // Create a new session and VPP
    {
        sts = m_session.Init(MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_VAAPI, NULL);
        MSDK_CHECK_STATUS(sts, "m_session.Init failed");

        mfxHDL hdl = NULL;
        sts = m_parentSession->GetHandle(MFX_HANDLE_VA_DISPLAY, &hdl);
        MSDK_CHECK_STATUS(sts, "m_parentSession->GetHandle failed");

        sts = m_session.SetHandle(MFX_HANDLE_VA_DISPLAY, hdl);
        MSDK_CHECK_STATUS(sts, "m_mfxSession.SetHandle failed");

        sts = m_parentSession->JoinSession(m_session);
        MSDK_CHECK_STATUS(sts, "m_parentSession->JoinSession failed");

        MFXFrameAllocator * allocator = m_pOutSurfPool->GetAllocator();
        MSDK_CHECK_POINTER(allocator, MFX_ERR_NOT_INITIALIZED);

        sts = m_session.SetFrameAllocator(allocator);
        MSDK_CHECK_STATUS(sts, "m_session.SetFrameAllocator failed");

        m_VPP.reset(new MFXVideoVPP(m_session));
    }

    // Allocate surfaces between the source and VPP
    {
        MFXFrameAllocator * allocator = m_pOutSurfPool->GetAllocator();
        m_InSurfacePool.SetAllocator(allocator);

        m_pTarget->SetSurfacePool(&m_InSurfacePool);

        mfxFrameAllocRequest sourceRequest;
        MSDK_ZERO_MEMORY(sourceRequest);

        sts = m_pTarget->QueryIOSurf(&sourceRequest);
        MSDK_CHECK_STATUS(sts, "m_pTarget->QueryIOSurf");

        mfxFrameAllocRequest vppRequest[2];
        MSDK_ZERO_MEMORY(vppRequest[0]);
        MSDK_ZERO_MEMORY(vppRequest[1]);

        mfxStatus sts = m_VPP->QueryIOSurf(&m_par, vppRequest);
        MSDK_CHECK_STATUS(sts, "VPP QueryIOSurf failed");


        sourceRequest.Type |= vppRequest[0].Type;
        sourceRequest.NumFrameSuggested += vppRequest[0].NumFrameSuggested + 1;
        sourceRequest.NumFrameMin = sourceRequest.NumFrameSuggested;

        sts = m_InSurfacePool.AllocSurfaces(sourceRequest);
        MSDK_CHECK_STATUS(sts, "m_InSurfacePool.AllocSurfaces failed");
    }

    return sts;
}

mfxStatus FieldSplitter::GetActualFrameInfo(mfxFrameInfo & info)
{
    info = m_par.vpp.Out;
    return MFX_ERR_NONE;
}

mfxStatus FieldSplitter::Init()
{
    mfxStatus sts = m_pTarget->Init();
    MSDK_CHECK_STATUS(sts, "m_pTarget->Init");

    sts = m_VPP->Init(&m_par);
    MSDK_CHECK_STATUS(sts, "VPP initialization failed");

    return sts;
}

mfxStatus FieldSplitter::VPPOneFrame(mfxFrameSurface1* pSurfaceIn, mfxFrameSurface1** pSurfaceOut)
{
    MSDK_CHECK_POINTER(pSurfaceOut, MFX_ERR_NULL_PTR);

    mfxSyncPoint syncp = NULL;

    mfxFrameSurface1 * pOutSrf = m_pOutSurfPool->GetFreeSurface();
    MSDK_CHECK_POINTER(pOutSrf, MFX_ERR_MEMORY_ALLOC);

    pOutSrf->Info.PicStruct = m_par.vpp.Out.PicStruct;

    mfxStatus sts = MFX_ERR_NONE;
    for(;;)
    {
        sts = m_VPP->RunFrameVPPAsync(pSurfaceIn, pOutSrf, NULL, &syncp);

        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            WaitForDeviceToBecomeFree(m_session, syncp, sts);
            continue;
        }
        else if (MFX_ERR_NONE == sts || MFX_ERR_MORE_SURFACE == sts)
        {
            break;
        }
        else
        {
            return sts;
        }
    }

    if (syncp)
    {
        mfxStatus sts = m_session.SyncOperation(syncp, MSDK_WAIT_INTERVAL);
        MSDK_CHECK_STATUS(sts, "Field splitting SyncOperation failed");

        *pSurfaceOut = pOutSrf;
    }

    return sts;
}

mfxStatus FieldSplitter::GetFrame(mfxFrameSurface1* & pOutSrf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 * pInSrf = m_pInSurface;

    if (NULL == pInSrf)
    {
        sts = m_pTarget->GetFrame(pInSrf);
        if (MFX_ERR_MORE_DATA == sts) sts = MFX_ERR_NONE;
        MSDK_CHECK_STATUS(sts, "m_pTarget->GetFrame failed");

        if (pInSrf)
        {
            if (MFX_PICSTRUCT_FIELD_TFF != pInSrf->Info.PicStruct && MFX_PICSTRUCT_FIELD_BFF != pInSrf->Info.PicStruct)
            {
                msdk_printf(MSDK_STRING("\nERROR: VPP Field Splitting can not process non-interlace frame\n"));
                return MFX_ERR_UNDEFINED_BEHAVIOR;
            }
        }
    }

    sts = VPPOneFrame(pInSrf, &pOutSrf);
    MSDK_CHECK_POINTER(pOutSrf, MFX_ERR_NULL_PTR);

    if (MFX_ERR_MORE_SURFACE == sts) // even field
    {
        m_pInSurface = pInSrf; // Keep a VPP input surface in a cache to process a next (odd) field
        sts = MFX_ERR_NONE;
    }
    else if (MFX_ERR_NONE == sts) // odd field
    {
        m_pInSurface = NULL;
    }

    return sts;
}

void FieldSplitter::Close()
{
    m_pTarget->Close();
}

mfxStatus FieldSplitter::ResetIOState()
{
    return m_pTarget->ResetIOState();
}

/**********************************************************************************/
MFX_VPP::MFX_VPP(MFXVideoSession* session, MfxVideoParamsWrapper& vpp_pars, SurfacesPool* sp)
    : m_pmfxSession(session)
    , m_mfxVPP(*m_pmfxSession)
    , m_pOutSurfPool(sp)
    , m_videoParams(vpp_pars)
{}

MFX_VPP::~MFX_VPP()
{
    m_mfxVPP.Close();
}

mfxStatus MFX_VPP::PreInit()
{
    mfxStatus sts = Query();
    return sts;
}

mfxStatus MFX_VPP::Init()
{
    mfxStatus sts = m_mfxVPP.Init(&m_videoParams);
    MSDK_CHECK_STATUS(sts, "VPP Init failed");
    MSDK_CHECK_WRN(sts, "VPP Init");

    // just in case, call GetVideoParam to get current VPP state
    sts = m_mfxVPP.GetVideoParam(&m_videoParams);
    MSDK_CHECK_STATUS(sts, "VPP GetVideoParam failed");

    return sts;
}

mfxStatus MFX_VPP::Query()
{
    mfxStatus sts = m_mfxVPP.Query(&m_videoParams, &m_videoParams);
    MSDK_CHECK_WRN(sts, "VPP Query");

    return sts;
}

mfxStatus MFX_VPP::Reset(mfxVideoParam& par)
{
    m_videoParams = par;

    mfxStatus sts = m_mfxVPP.Reset(&m_videoParams);
    MSDK_CHECK_STATUS(sts, "VPP Reset failed");
    MSDK_CHECK_WRN(sts, "VPP Reset");

    // just in case, call GetVideoParam to get current VPP state
    sts = m_mfxVPP.GetVideoParam(&m_videoParams);
    MSDK_CHECK_STATUS(sts, "VPP GetVideoParam failed");

    return sts;
}

mfxStatus MFX_VPP::QueryIOSurf(mfxFrameAllocRequest* request)
{
    return m_mfxVPP.QueryIOSurf(&m_videoParams, request);
}

mfxFrameInfo MFX_VPP::GetOutFrameInfo()
{
    return m_videoParams.vpp.Out;
}

mfxStatus MFX_VPP::ProcessFrame(mfxFrameSurface1* pInSurf, mfxFrameSurface1* & pOutSurf)
{
    mfxStatus sts = MFX_ERR_NONE;

    for (;;)
    {
        mfxSyncPoint syncp = nullptr;

        pOutSurf = m_pOutSurfPool->GetFreeSurface();
        MSDK_CHECK_POINTER(pOutSurf, MFX_ERR_MEMORY_ALLOC);

        sts = m_mfxVPP.RunFrameVPPAsync(pInSurf, pOutSurf, NULL, &syncp);
        MSDK_CHECK_WRN(sts, "VPP RunFrameVPPAsync");

        if (MFX_ERR_NONE < sts && !syncp) // repeat the call if warning and no output
        {
            if (MFX_WRN_DEVICE_BUSY == sts)
            {
                WaitForDeviceToBecomeFree(*m_pmfxSession, syncp, sts);
            }
            continue;
        }

        if (MFX_ERR_NONE <= sts && syncp) // ignore warnings if output is available
        {
            sts = m_pmfxSession->SyncOperation(syncp, MSDK_WAIT_INTERVAL);
            break;
        }

        MSDK_BREAK_ON_ERROR(sts);
    }

    // when pInSurf==NULL, MFX_ERR_MORE_DATA indicates all cached frames within VPP were drained,
    // return status as is
    // otherwise fetch more input for VPP, ignore status
    if (MFX_ERR_MORE_DATA == sts && pInSurf)
    {
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    }

    return sts;
}
