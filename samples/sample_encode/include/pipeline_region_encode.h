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

#ifndef __PIPELINE_REGION_ENCODE_H__
#define __PIPELINE_REGION_ENCODE_H__

#include "pipeline_encode.h"

class CMSDKResource
{
public:
    CMSDKResource()
    {
        pEncoder=NULL;
        pPlugin=NULL;
    }

    MFXVideoSession Session;
    MFXVideoENCODE* pEncoder;
    MFXPlugin* pPlugin;
    CEncTaskPool TaskPool;
};

class CResourcesPool
{
public:
    CResourcesPool()
    {
        size=0;
        m_resources=NULL;
    }

    ~CResourcesPool()
    {
        delete[] m_resources;
    }

    CMSDKResource& operator[](int index){return m_resources[index];}

    int GetSize(){return size;}

    mfxStatus Init(int size,mfxIMPL impl, mfxVersion *pVer);
    mfxStatus InitTaskPools(CSmplBitstreamWriter* pWriter, mfxU32 nPoolSize, mfxU32 nBufferSize, CSmplBitstreamWriter *pOtherWriter = NULL);
    mfxStatus CreateEncoders();
    mfxStatus CreatePlugins(mfxPluginUID pluginGUID, mfxChar* pluginPath);

    mfxStatus GetFreeTask(int resourceNum, sTask **ppTask);
    void CloseAndDeleteEverything();

protected:
    CMSDKResource* m_resources;
    int size;

private:
    CResourcesPool(const CResourcesPool& src){(void)src;}
    CResourcesPool& operator= (const CResourcesPool& src){(void)src;return *this;}
};



/* This class implements a pipeline with 2 mfx components: vpp (video preprocessing) and encode */
class CRegionEncodingPipeline : public CEncodingPipeline
{
public:
    CRegionEncodingPipeline();
    virtual ~CRegionEncodingPipeline();

    virtual mfxStatus Init(sInputParams *pParams);
    virtual mfxStatus Run();
    virtual void Close();
    virtual mfxStatus ResetMFXComponents(sInputParams* pParams);

    void SetMultiView();
    void SetNumView(mfxU32 numViews) { m_nNumView = numViews; }

protected:
    mfxI64 m_timeAll;
    CResourcesPool m_resources;

    mfxExtHEVCRegion m_HEVCRegion;

    virtual mfxStatus InitMfxEncParams(sInputParams *pParams);

    virtual mfxStatus CreateAllocator();

    virtual MFXVideoSession& GetFirstSession(){return m_resources[0].Session;}
    virtual MFXVideoENCODE* GetFirstEncoder(){return m_resources[0].pEncoder;}
};

#endif // __PIPELINE_REGION_ENCODE_H__
