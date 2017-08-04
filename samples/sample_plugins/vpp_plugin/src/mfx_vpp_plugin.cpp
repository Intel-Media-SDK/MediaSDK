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

#include <stdio.h>
#include <memory>
#include "mfx_vpp_plugin.h"
#include "mfx_plugin_module.h"

#define MY_WAIT(__Milliseconds)  MSDK_SLEEP(__Milliseconds)

MFXVideoVPPPlugin::MFXVideoVPPPlugin(mfxSession session)
    : MFXVideoMultiVPP(session)
{
    m_PluginModule = NULL;
    m_pPlugin = NULL;

    memset(&m_pmfxPluginParam, 0, sizeof(m_pmfxPluginParam));
    m_pAuxParam = NULL;
    m_AuxParamSize = 0;

    memset(&m_FrameAllocator, 0, sizeof(m_FrameAllocator));
    memset(&m_pmfxVPP1Param, 0, sizeof(m_pmfxVPP1Param));
    memset(&m_pmfxVPP2Param, 0, sizeof(m_pmfxVPP2Param));
    m_pVPP1 = NULL;
    m_pVPP2 = NULL;

    m_session2 = NULL;
    m_mfxHDL   = NULL;

    m_allocResponses[0].reset();
    m_allocResponses[1].reset();
}


mfxStatus MFXVideoVPPPlugin::LoadDLL(msdk_char *dll_path)
{
    MSDK_CHECK_POINTER(dll_path, MFX_ERR_NULL_PTR);

    // Load plugin DLL
    m_PluginModule = msdk_so_load(dll_path);
    MSDK_CHECK_POINTER(m_PluginModule, MFX_ERR_NOT_FOUND);

    // Load Create function
    PluginModuleTemplate::fncCreateGenericPlugin pCreateFunc = (PluginModuleTemplate::fncCreateGenericPlugin)msdk_so_get_addr(m_PluginModule, "mfxCreateGenericPlugin");
    MSDK_CHECK_POINTER(pCreateFunc, MFX_ERR_NOT_FOUND);

    // Create plugin object
    m_pPlugin = (*pCreateFunc)();
    MSDK_CHECK_POINTER(m_pPlugin, MFX_ERR_NOT_FOUND);

    return MFX_ERR_NONE;
}

mfxStatus MFXVideoVPPPlugin::SetAuxParam(void* auxParam, int auxParamSize)
{
    MSDK_CHECK_POINTER(auxParam, MFX_ERR_MEMORY_ALLOC);
    MSDK_CHECK_ERROR(0, auxParamSize, MFX_ERR_NOT_FOUND);

    m_pAuxParam = auxParam;
    m_AuxParamSize = auxParamSize;

    return MFX_ERR_NONE;
}

mfxStatus MFXVideoVPPPlugin::SetFrameAllocator(mfxFrameAllocator *allocator)
{
    MSDK_CHECK_POINTER(allocator, MFX_ERR_NULL_PTR);
    m_FrameAllocator = *allocator;
    return MFX_ERR_NONE;
}

MFXVideoVPPPlugin::~MFXVideoVPPPlugin(void)
{
    Close();
}

// per-component methods currently not implemented, you can add your own implementation if needed
mfxStatus MFXVideoVPPPlugin::Query(mfxVideoParam *in, mfxVideoParam *out, mfxU8 component_idx)
{
    in;out;component_idx;
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus MFXVideoVPPPlugin::GetVideoParam(mfxVideoParam *par, mfxU8 component_idx)
{
    par;component_idx;
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus MFXVideoVPPPlugin::GetVPPStat(mfxVPPStat *stat, mfxU8 component_idx)
{
    stat;component_idx;
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus MFXVideoVPPPlugin::AllocateFrames(mfxVideoParam *par, mfxVideoParam *par1, mfxVideoParam *par2)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameAllocResponse allocResponse;
    memset((void*)&allocResponse, 0, sizeof(mfxFrameAllocResponse));
    mfxFrameAllocRequest plgAllocRequest0;
    memset((void*)&plgAllocRequest0, 0, sizeof(mfxFrameAllocRequest));
    mfxFrameAllocRequest plgAllocRequest1;
    memset((void*)&plgAllocRequest1, 0, sizeof(mfxFrameAllocRequest));
    mfxFrameAllocRequest vpp1Request[2];
    memset((void*)&vpp1Request, 0, 2*sizeof(mfxFrameAllocRequest));
    mfxFrameAllocRequest vpp2Request[2];
    memset((void*)&vpp2Request, 0, 2*sizeof(mfxFrameAllocRequest));

    if (m_pPlugin)
    {
        m_pmfxPluginParam = par;

        sts = m_pPlugin->QueryIOSurf(m_pmfxPluginParam, &plgAllocRequest0, &plgAllocRequest1);
        MSDK_CHECK_STATUS(sts, "m_pPlugin->QueryIOSurf failed");
    }
    else
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    if (par1 && m_pVPP1)
    {
        m_pmfxVPP1Param = par1;

        sts = m_pVPP1->QueryIOSurf(m_pmfxVPP1Param, vpp1Request);
        MSDK_CHECK_STATUS(sts, "m_pVPP1->QueryIOSurf failed");

        plgAllocRequest0.Type = (mfxU16) ((vpp1Request[1].Type & ~MFX_MEMTYPE_EXTERNAL_FRAME) | MFX_MEMTYPE_INTERNAL_FRAME);

        plgAllocRequest0.NumFrameSuggested = plgAllocRequest0.NumFrameSuggested + vpp1Request[1].NumFrameSuggested;
        plgAllocRequest0.NumFrameMin = plgAllocRequest0.NumFrameSuggested;

        // check if opaque memory was requested for this pipeline by the application
        bool bIsOpaque = par->IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY ? true : false;

        // allocate actual memory only if not opaque
        if (!bIsOpaque)
        {
            sts = m_FrameAllocator.Alloc(m_FrameAllocator.pthis, &plgAllocRequest0, &allocResponse);
            MSDK_CHECK_STATUS(sts, "m_FrameAllocator.Alloc failed");
        }
        else
        {
            allocResponse.NumFrameActual = plgAllocRequest0.NumFrameSuggested;
        }

        if (m_allocResponses[0].get())
        {
            m_FrameAllocator.Free(m_FrameAllocator.pthis, m_allocResponses[0].get());
        }
        m_allocResponses[0].reset(new mfxFrameAllocResponse(allocResponse));
        //create pool of mfxFrameSurface1 structures
        sts = m_SurfacePool1.Alloc(allocResponse, plgAllocRequest0.Info, bIsOpaque);
        MSDK_CHECK_STATUS(sts, "m_SurfacePool1.Alloc failed");

        // fill mfxExtOpaqueAlloc buffer of vpp and plugin in case of opaque memory
        if (bIsOpaque)
        {
            mfxExtOpaqueSurfaceAlloc* vpp1OpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer
                (par1->ExtParam, par1->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

            MSDK_CHECK_POINTER(vpp1OpaqueAlloc, MFX_ERR_INVALID_VIDEO_PARAM);

            vpp1OpaqueAlloc->Out.Surfaces = m_SurfacePool1.m_ppSurfacesPool;
            vpp1OpaqueAlloc->Out.NumSurface = (mfxU16)m_SurfacePool1.m_nPoolSize;

            vpp1OpaqueAlloc->Out.Type = plgAllocRequest0.Type;

            mfxExtOpaqueSurfaceAlloc* pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer
                (par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

            MSDK_CHECK_POINTER(pluginOpaqueAlloc, MFX_ERR_INVALID_VIDEO_PARAM);
            pluginOpaqueAlloc->In = vpp1OpaqueAlloc->Out;
        }
    }
    else if (par1 || m_pVPP1) // configuration differs from the one provided in QueryIOSurf
    {
        return MFX_ERR_UNSUPPORTED;
    }

    if (par2 && par1)
    {
        //setting the same allocator in second session
        sts = MFXVideoCORE_SetFrameAllocator(m_session2, &m_FrameAllocator);
        MSDK_CHECK_STATUS(sts, "MFXVideoCORE_SetFrameAllocator failed");
        if (NULL != m_mfxHDL)
        {
            sts = MFXVideoCORE_SetHandle(m_session2, MFX_HANDLE_D3D9_DEVICE_MANAGER, m_mfxHDL);
            MSDK_CHECK_STATUS(sts, "MFXVideoCORE_SetHandle failed");
        }
    }

    if (par2 && m_pVPP2)
    {
        m_pmfxVPP2Param = par2;

        sts = m_pVPP2->QueryIOSurf(m_pmfxVPP2Param, vpp2Request);
        MSDK_CHECK_STATUS(sts, "m_pVPP2->QueryIOSurf failed");

        plgAllocRequest1.Type = (mfxU16) ((vpp2Request[1].Type & ~MFX_MEMTYPE_EXTERNAL_FRAME) | MFX_MEMTYPE_INTERNAL_FRAME);
        plgAllocRequest1.NumFrameSuggested = plgAllocRequest1.NumFrameSuggested + vpp2Request[0].NumFrameSuggested;
        plgAllocRequest1.NumFrameMin = plgAllocRequest1.NumFrameSuggested;

        // check if opaque memory was requested for this pipeline by the application
        bool bIsOpaque = par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY ? true : false;

        // check that IOPattern of VPP is consistent
        if (bIsOpaque && par1 && !(par1->IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY))
            return MFX_ERR_UNSUPPORTED;

        // allocate actual memory only if not opaque
        if (!bIsOpaque)
        {
            sts = m_FrameAllocator.Alloc(m_FrameAllocator.pthis, &plgAllocRequest1, &allocResponse);
            MSDK_CHECK_STATUS(sts, "m_FrameAllocator.Alloc failed");
        }
        else
        {
            allocResponse.NumFrameActual = plgAllocRequest1.NumFrameSuggested;
        }

        if (m_allocResponses[1].get())
        {
            m_FrameAllocator.Free(m_FrameAllocator.pthis, m_allocResponses[1].get());
        }
        m_allocResponses[1].reset(new mfxFrameAllocResponse(allocResponse));
        //create pool of mfxFrameSurface1 structures
        sts = m_SurfacePool2.Alloc(allocResponse, plgAllocRequest1.Info, bIsOpaque);
        MSDK_CHECK_STATUS(sts, "m_SurfacePool2.Alloc failed");

        // fill mfxExtOpaqueAlloc buffer of vpp and plugin in case of opaque memory
        if (bIsOpaque)
        {
            mfxExtOpaqueSurfaceAlloc* vpp2OpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer
                (par2->ExtParam, par2->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
            MSDK_CHECK_POINTER(vpp2OpaqueAlloc, MFX_ERR_INVALID_VIDEO_PARAM);

            vpp2OpaqueAlloc->In.Surfaces = m_SurfacePool2.m_ppSurfacesPool;
            vpp2OpaqueAlloc->In.NumSurface = (mfxU16)m_SurfacePool2.m_nPoolSize;
            vpp2OpaqueAlloc->In.Type = plgAllocRequest1.Type;

            mfxExtOpaqueSurfaceAlloc* pluginOpaqueAlloc = (mfxExtOpaqueSurfaceAlloc*)
                GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);
            MSDK_CHECK_POINTER(pluginOpaqueAlloc, MFX_ERR_INVALID_VIDEO_PARAM);

            pluginOpaqueAlloc->Out = vpp2OpaqueAlloc->In;
        }
    }
    else if (par2 || m_pVPP2)// configuration differs from the one provided in QueryIOSurf
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return sts;
}

mfxStatus MFXVideoVPPPlugin::QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest request[2], mfxVideoParam *par1, mfxVideoParam *par2)
{
    MSDK_CHECK_POINTER(par, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest plgAllocRequest0;
    memset((void*)&plgAllocRequest0, 0, sizeof(mfxFrameAllocRequest));
    mfxFrameAllocRequest plgAllocRequest1;
    memset((void*)&plgAllocRequest1, 0, sizeof(mfxFrameAllocRequest));
    mfxFrameAllocRequest vpp1Request[2];
    memset((void*)&vpp1Request, 0, 2*sizeof(mfxFrameAllocRequest));
    mfxFrameAllocRequest vpp2Request[2];
    memset((void*)&vpp2Request, 0, 2*sizeof(mfxFrameAllocRequest));

    MSDK_CHECK_POINTER(par, MFX_ERR_NULL_PTR);

    // create and init Plugin
    if (m_pPlugin)
    {
        m_pmfxPluginParam = par;

        sts = m_pPlugin->QueryIOSurf(m_pmfxPluginParam, &plgAllocRequest0, &plgAllocRequest1);
        MSDK_CHECK_STATUS(sts, "m_pPlugin->QueryIOSurf failed");

        request[0] = plgAllocRequest0;
        request[1] = plgAllocRequest1;
    }
    else
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    // create VPP and allocate surfaces to build VPP->Plugin pipeline
    if (par1)
    {
        m_pmfxVPP1Param = par1;
        m_pVPP1 = new MFXVideoVPP(m_session);
        MSDK_CHECK_POINTER(m_pVPP1, MFX_ERR_MEMORY_ALLOC);

        sts = m_pVPP1->QueryIOSurf(m_pmfxVPP1Param, vpp1Request);
        MSDK_CHECK_STATUS(sts, "m_pVPP1->QueryIOSurf failed");

        plgAllocRequest0.Type = (mfxU16) ((vpp1Request[1].Type & ~MFX_MEMTYPE_EXTERNAL_FRAME) | MFX_MEMTYPE_INTERNAL_FRAME);
        plgAllocRequest0.NumFrameSuggested = plgAllocRequest0.NumFrameSuggested + vpp1Request[1].NumFrameSuggested;
        plgAllocRequest0.NumFrameMin = plgAllocRequest0.NumFrameSuggested;

        // check if opaque memory was requested for this pipeline by the application
        bool bIsOpaque = par->IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY ? true : false;

        // check that IOPattern of VPP is consistent
        if (bIsOpaque && !(par1->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY))
            return MFX_ERR_UNSUPPORTED;

        // exposed to application
        request[0] = vpp1Request[0];
    }

    //need to create a new session if both vpp are used
    //second vpp will be in a separate joined session
    mfxSession session2 = m_session;
    if (par2 && par1)
    {
        sts = MFXCloneSession(m_session, &m_session2);
        MSDK_CHECK_STATUS(sts, "MFXCloneSession failed");
        sts = MFXJoinSession(m_session, m_session2);
        MSDK_CHECK_STATUS(sts, "MFXJoinSession failed");

        session2 = m_session2;
    }

    // create VPP and allocate surfaces to build Plugin->VPP pipeline
    if (par2)
    {
        m_pmfxVPP2Param = par2;
        m_pVPP2 = new MFXVideoVPP(session2);
        MSDK_CHECK_POINTER(m_pVPP2, MFX_ERR_MEMORY_ALLOC);

        sts = m_pVPP2->QueryIOSurf(m_pmfxVPP2Param, vpp2Request);
        MSDK_CHECK_STATUS(sts, "m_pVPP2->QueryIOSurf failed");

        plgAllocRequest1.Type = (mfxU16) ((vpp2Request[1].Type & ~MFX_MEMTYPE_EXTERNAL_FRAME) | MFX_MEMTYPE_INTERNAL_FRAME);
        plgAllocRequest1.NumFrameSuggested = plgAllocRequest1.NumFrameSuggested + vpp2Request[0].NumFrameSuggested;
        plgAllocRequest1.NumFrameMin = plgAllocRequest1.NumFrameSuggested;

        // check if opaque memory was requested for this pipeline by the application
        bool bIsOpaque = par->IOPattern & MFX_IOPATTERN_OUT_OPAQUE_MEMORY ? true : false;

        // check that IOPattern of VPP is consistent
        if (bIsOpaque && !(par2->IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY))
            return MFX_ERR_UNSUPPORTED;

        // exposed to application
        request[1] = vpp2Request[1];
    }

    return MFX_ERR_NONE;
}

mfxStatus MFXVideoVPPPlugin::Init(mfxVideoParam *par, mfxVideoParam *par1, mfxVideoParam *par2)
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = AllocateFrames(par, par1, par2);
    MSDK_CHECK_STATUS(sts, "AllocateFrames failed");

    // create and init Plugin
    if (m_pPlugin)
    {
        m_pmfxPluginParam = par;

        // Register plugin, acquire mfxCore interface which will be needed in Init
        mfxPlugin plg = make_mfx_plugin_adapter(m_pPlugin);
        sts = MFXVideoUSER_Register(m_session, 0, &plg);
        MSDK_CHECK_STATUS(sts, "MFXVideoUSER_Register failed");

        // Init plugin
        sts = m_pPlugin->Init(m_pmfxPluginParam);
        MSDK_CHECK_STATUS(sts, "m_pPlugin->Init failed");

        sts = m_pPlugin->SetAuxParams(m_pAuxParam, m_AuxParamSize);
        MSDK_CHECK_STATUS(sts, "m_pPlugin->SetAuxParams failed");
    } else
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    if (par1 && m_pVPP1)
    {
        m_pmfxVPP1Param = par1;
        sts = m_pVPP1->Init(m_pmfxVPP1Param);
        MSDK_CHECK_STATUS(sts, "m_pVPP1->Init failed");
    }
    else if (par1 || m_pVPP1) // configuration differs from the one provided in QueryIOSurf
    {
        return MFX_ERR_UNSUPPORTED;
    }

    if (par2 && m_pVPP2)
    {
        m_pmfxVPP2Param = par2;
        sts = m_pVPP2->Init(m_pmfxVPP2Param);
        MSDK_CHECK_STATUS(sts, "m_pVPP2->Init failed");
    }
    else if (par2 || m_pVPP2)// configuration differs from the one provided in QueryIOSurf
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

mfxStatus MFXVideoVPPPlugin::Reset(mfxVideoParam *par, mfxVideoParam *par1, mfxVideoParam *par2)
{
    par;par1;par2;
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus MFXVideoVPPPlugin::Close(void)
{
    if (m_pPlugin)
        MFXVideoUSER_Unregister(m_session, 0);

    MSDK_SAFE_DELETE(m_pPlugin);

    if (m_PluginModule)
    {
        msdk_so_free(m_PluginModule);
        m_PluginModule = NULL;
    }

    MSDK_SAFE_DELETE(m_pVPP1);
    MSDK_SAFE_DELETE(m_pVPP2);

    if (m_session2)
    {
        //TODO: ensure no active tasks
        MFXDisjoinSession(m_session2);

        MFXClose(m_session2);

        m_session2 = NULL;
    }

    return MFX_ERR_NONE;
}

mfxStatus MFXVideoVPPPlugin::RunFrameVPPAsync(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData * /*aux*/, mfxSyncPoint *syncp)
{
    return RunVPP1(in, out, syncp);
}

mfxStatus MFXVideoVPPPlugin::RunVPP1(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxSyncPoint *syncp)
{
    if (!m_pVPP1)
    {
        return RunPlugin(in, out, syncp);
    }

    mfxStatus sts;
    mfxFrameSurface1 *pOutSurface = out;
    mfxSyncPoint local_syncp = NULL;

    pOutSurface = m_SurfacePool1.GetFreeSurface();
    MSDK_CHECK_POINTER(pOutSurface, MFX_ERR_MEMORY_ALLOC);

    for(;;)
    {
        sts = m_pVPP1->RunFrameVPPAsync(in, pOutSurface, NULL, &local_syncp);

        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            MY_WAIT(1); // just wait and then repeat the same call
        }
        else
        {
            break;
        }
    }

    if (MFX_ERR_NONE != sts)
        return sts;

    return RunPlugin(pOutSurface, out, syncp);
}

mfxStatus MFXVideoVPPPlugin::RunPlugin(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxSyncPoint *syncp)
{
    if (!m_pPlugin || !in) // plugin doesn't support NULL input argument, need to go to buffering stage of VPP2
    {
        return RunVPP2(in, out, syncp);
    }

    mfxStatus sts;
    mfxFrameSurface1 *pOutSurface = out;
    mfxSyncPoint local_syncp = NULL;

    if (m_pVPP2)
    {
        pOutSurface = m_SurfacePool2.GetFreeSurface();
        MSDK_CHECK_POINTER(pOutSurface, MFX_ERR_MEMORY_ALLOC);
    }

    for(;;)
    {
        mfxHDL h1 = in;
        mfxHDL h2 = pOutSurface;
        // BE CAREFUL! mfxHDL in this instance represents pointer to mfxFrameSurface1.
        // And ProcessFrameAsync get pointers to mfxHDL objects. It is required for MSDK internal task dependency tracking.
        // So array of mfxFrameSurface1 pointers can be transferred into user plugin.
        sts = MFXVideoUSER_ProcessFrameAsync(m_session, &h1, 1, &h2, 1, &local_syncp);
        if (!sts)
        {
            *syncp = local_syncp;
        }

        pOutSurface->Info.PicStruct = in->Info.PicStruct; // plugin has to passthrough the picstruct parameter if it is in the transcoding pipeline

        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            MY_WAIT(1); // just wait and then repeat the same call
        }
        else
        {
            break;
        }
    }

    if (!m_pVPP2)
    {
        return sts;
    }
    else
    {
        MSDK_CHECK_STATUS(sts, "MFXVideoUSER_ProcessFrameAsync failed");

        return RunVPP2(pOutSurface, out, syncp);
    }
}

mfxStatus MFXVideoVPPPlugin::RunVPP2(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxSyncPoint *syncp)
{
    mfxStatus sts;

    if (!in)
    {
        return MFX_ERR_MORE_DATA;
    }

    for(;;)
    {
        sts = m_pVPP2->RunFrameVPPAsync(in, out, NULL, syncp);

        if (MFX_WRN_DEVICE_BUSY == sts)
        {
            MY_WAIT(1); // just wait and then repeat the same call
        }
        else
        {
            break;
        }
    }

    return sts;
}

mfxStatus MFXVideoVPPPlugin::SurfacePool::Alloc(mfxFrameAllocResponse &response, mfxFrameInfo &info, bool opaque)
{
    Free();

    m_ppSurfacesPool = new mfxFrameSurface1* [response.NumFrameActual];
    MSDK_CHECK_POINTER(m_ppSurfacesPool, MFX_ERR_MEMORY_ALLOC);

    for (int i = 0; i < response.NumFrameActual; i++)
    {
        m_ppSurfacesPool[i] = new mfxFrameSurface1;
        MSDK_CHECK_POINTER(m_ppSurfacesPool[i], MFX_ERR_MEMORY_ALLOC);

        MSDK_MEMCPY_VAR(m_ppSurfacesPool[i]->Info , &info, sizeof(info));
        MSDK_ZERO_MEMORY(m_ppSurfacesPool[i]->Data);
        if (!opaque)
        {
            m_ppSurfacesPool[i]->Data.MemId = response.mids[i];
        }
    }
    m_nPoolSize = response.NumFrameActual;
    return MFX_ERR_NONE;
}

mfxFrameSurface1 *MFXVideoVPPPlugin::SurfacePool::GetFreeSurface()
{
    mfxU32 SleepInterval = 10; // milliseconds

    for (mfxU32 j = 0; j < MSDK_WAIT_INTERVAL; j += SleepInterval)
    {
        for (mfxU16 i = 0; i < m_nPoolSize; i++)
        {
            if (0 == m_ppSurfacesPool[i]->Data.Locked)
            {
                return m_ppSurfacesPool[i];
            }
        }

        //wait if there's no free surface
        MY_WAIT(SleepInterval);
    }

    return NULL;
}
