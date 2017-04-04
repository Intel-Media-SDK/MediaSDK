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

#ifndef __MFX_PLUGIN_2_VPP_H
#define __MFX_PLUGIN_2_VPP_H

#include <memory>
#include "mfxvideo++.h"
#include "mfx_multi_vpp.h"
#include "sample_defs.h"
#include "vm/so_defs.h"
#include "mfxplugin++.h"

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);               \
    void operator=(const TypeName&)

class MFXPluginBase;

//////////////////////////////////////////////////////////////////////////////////
// this class implements pipeline vpp->plugin->vpp (both vpp are optional) and
// exposes it under MFXVideoMultiVPP interface with addition functions LoadDLL
// and SetAuxParam used by app on initialization stage

class MFXVideoVPPPlugin : public MFXVideoMultiVPP
{
public:
    MFXVideoVPPPlugin(mfxSession session);
    virtual ~MFXVideoVPPPlugin(void);

    virtual mfxStatus LoadDLL(msdk_char *dll_path);
    virtual mfxStatus SetAuxParam(void* auxParam, int auxParamSize);
    virtual mfxStatus SetFrameAllocator(mfxFrameAllocator *allocator);

    // par1 != null enables first VPP (before Plugin), par2 != null enables second VPP (after Plugin); pipeline Plugin-VPP is not unsupported
    // QueryIOSurf must be called prior to Init to create topology
    mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest request[2], mfxVideoParam *par1 = NULL, mfxVideoParam *par2 = NULL);

    // par1 != null enables first VPP (before Plugin), par2 != null enables second VPP (after Plugin); pipeline Plugin-VPP is not unsupported; topology must be the same as in QueryIOSurf
    mfxStatus Init(mfxVideoParam *par, mfxVideoParam *par1 = NULL, mfxVideoParam *par2 = NULL);
    mfxStatus RunFrameVPPAsync(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp);
    mfxStatus Reset(mfxVideoParam *par, mfxVideoParam *par1 = NULL, mfxVideoParam *par2 = NULL);
    mfxStatus Close(void);

    mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out, mfxU8 component_idx = 0);
    mfxStatus GetVideoParam(mfxVideoParam *par, mfxU8 component_idx = 0);
    mfxStatus GetVPPStat(mfxVPPStat *stat, mfxU8 component_idx = 0);

protected:
    mfxFrameAllocator   m_FrameAllocator;
    mfxHDL              m_mfxHDL;

    // Plugin
    msdk_so_handle          m_PluginModule; // DLL module
    MFXGenericPlugin        *m_pPlugin; // plugin object
    mfxVideoParam           *m_pmfxPluginParam;
    void                    *m_pAuxParam;
    int                     m_AuxParamSize;

    //helper class
    class SurfacePool
    {
    public:
        mfxFrameSurface1 **m_ppSurfacesPool;
        mfxU16            m_nPoolSize;
    public:
        SurfacePool()
            : m_ppSurfacesPool(NULL)
            , m_nPoolSize(0){}
        ~SurfacePool(){
            Free();
        }
        mfxStatus Alloc(mfxFrameAllocResponse &response, mfxFrameInfo &info, bool opaque);
        mfxFrameSurface1* GetFreeSurface();
    private:
        void Free()
        {
            for (mfxU32 i = 0; m_ppSurfacesPool && (i < m_nPoolSize) ; i++)
            {
                MSDK_SAFE_DELETE(m_ppSurfacesPool[i]);
            }
            MSDK_SAFE_DELETE_ARRAY(m_ppSurfacesPool);
        }
        DISALLOW_COPY_AND_ASSIGN(SurfacePool);
    };

    // VPP1
    mfxVideoParam          *m_pmfxVPP1Param;
    MFXVideoVPP            *m_pVPP1;
    SurfacePool             m_SurfacePool1;

    // VPP2
    mfxVideoParam          *m_pmfxVPP2Param;
    MFXVideoVPP            *m_pVPP2;
    SurfacePool             m_SurfacePool2;
    mfxSession              m_session2; //for two vpp case

    std::auto_ptr<mfxFrameAllocResponse>  m_allocResponses[2];
    mfxStatus               AllocateFrames(mfxVideoParam *par, mfxVideoParam *par1, mfxVideoParam *par2);

    // pipeline implementation
    mfxStatus RunVPP1(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxSyncPoint *syncp);
    mfxStatus RunPlugin(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxSyncPoint *syncp);
    mfxStatus RunVPP2(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxSyncPoint *syncp);
};

#endif // __MFX_PLUGIN_2_VPP_H
