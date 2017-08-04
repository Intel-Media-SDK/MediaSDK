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

#ifndef __SAMPLE_FEI__AUXILIARY_INTERFACES_H__
#define __SAMPLE_FEI__AUXILIARY_INTERFACES_H__

#include "encoding_task.h"

class MFX_VppInterface
{
private:
    MFX_VppInterface(const MFX_VppInterface& other_vpp);             // forbidden
    MFX_VppInterface& operator= (const MFX_VppInterface& other_vpp); // forbidden

public:
    MFXVideoSession* m_pmfxSession;
    MFXVideoVPP*     m_pmfxVPP;
    mfxU32           m_allocId;
    mfxVideoParam    m_videoParams;
    AppConfig*       m_pAppConfig;
    mfxSyncPoint     m_SyncPoint;

    std::vector<mfxExtBuffer*> m_InitExtParams;

    MFX_VppInterface(MFXVideoSession* session, mfxU32 allocId, AppConfig* config);
    ~MFX_VppInterface();

    mfxStatus Init();
    mfxStatus Close();
    mfxStatus Reset(mfxU16 width = 0, mfxU16 height = 0, mfxU16 crop_w = 0, mfxU16 crop_h = 0);
    mfxVideoParam* GetCommonVideoParams();
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);
    mfxStatus FillParameters();
    mfxStatus VPPoneFrame(mfxFrameSurface1* pSurf_in, mfxFrameSurface1* pSurf_out);
};

class MFX_DecodeInterface
{
private:
    MFX_DecodeInterface(const MFX_DecodeInterface& other_decode);             // forbidden
    MFX_DecodeInterface& operator= (const MFX_DecodeInterface& other_decode); // forbidden

public:
    MFXVideoSession*     m_pmfxSession;
    MFXVideoDECODE*      m_pmfxDECODE;
    mfxU32               m_allocId;
    mfxVideoParam        m_videoParams;
    AppConfig*           m_pAppConfig;
    mfxBitstream         m_mfxBS;
    mfxSyncPoint         m_SyncPoint;
    CSmplBitstreamReader m_BSReader;
    ExtSurfPool*         m_pSurfPool;
    bool                 m_bEndOfFile;
    FILE*                m_DecStremout_out;

    std::vector<mfxExtBuffer*> m_InitExtParams;

    MFX_DecodeInterface(MFXVideoSession* session, mfxU32 allocId, AppConfig* config, ExtSurfPool* surf_pool);
    ~MFX_DecodeInterface();

    mfxStatus Init();
    mfxStatus Close();
    mfxStatus Reset();
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);
    mfxVideoParam* GetCommonVideoParams();
    mfxStatus UpdateVideoParam();
    mfxStatus FillParameters();
    mfxStatus GetOneFrame(mfxFrameSurface1* & pSurf);
    mfxStatus DecodeOneFrame(mfxFrameSurface1 * & pSurf_out);
    mfxStatus DecodeLastFrame(mfxFrameSurface1 * & pSurf_out);
    mfxStatus FlushOutput(mfxFrameSurface1* pSurf);
    mfxStatus ResetState();
};

class YUVreader
{
public:
    CSmplYUVReader     m_FileReader;
    AppConfig*         m_pAppConfig;
    ExtSurfPool*       m_pSurfPool;
    bool               m_bExternalAlloc;
    MFXFrameAllocator* m_pMFXAllocator;

    YUVreader(AppConfig* cfg, ExtSurfPool* surf_pool, MFXFrameAllocator* allocator)
        : m_pAppConfig(cfg)
        , m_pSurfPool(surf_pool)
        , m_bExternalAlloc(m_pAppConfig->bUseHWmemory)
        , m_pMFXAllocator(allocator)
    {}

    ~YUVreader()
    {
        Close();
    }

    void Close()
    {
        m_FileReader.Close();
    }

    mfxStatus Init()
    {
        std::list<msdk_string> tmpl;
        tmpl.push_back(msdk_string(m_pAppConfig->strSrcFile));
        return m_FileReader.Init(tmpl,
                                m_pAppConfig->ColorFormat);
    }

    mfxStatus ResetState()
    {
        return Init();
    }

    mfxStatus GetOneFrame(mfxFrameSurface1* & pSurf);
};

#endif // __SAMPLE_FEI__AUXILIARY_INTERFACES_H__