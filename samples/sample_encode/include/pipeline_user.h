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

#ifndef __PIPELINE_USER_H__
#define __PIPELINE_USER_H__

#include "vm/so_defs.h"
#include "pipeline_encode.h"
#include "mfx_plugin_base.h"
#include "mfx_plugin_module.h"
#include "rotate_plugin_api.h"

/* This class implements the following pipeline: user plugin (frame rotation) -> mfxENCODE */
class CUserPipeline : public CEncodingPipeline
{
public:

    CUserPipeline();
    virtual ~CUserPipeline();

    virtual mfxStatus Init(sInputParams *pParams);
    virtual mfxStatus Run();
    virtual void Close();
    virtual mfxStatus ResetMFXComponents(sInputParams* pParams);
    virtual void PrintInfo();
    virtual mfxStatus FillBuffers();

protected:
    msdk_so_handle          m_PluginModule;
    MFXGenericPlugin*       m_pusrPlugin;
    mfxFrameSurface1*       m_pPluginSurfaces; // frames array for rotate input
    mfxFrameAllocResponse   m_PluginResponse;  // memory allocation response for rotate plugin

    mfxVideoParam                   m_pluginVideoParams;
    RotateParam                     m_RotateParams;

    virtual mfxStatus InitRotateParam(sInputParams *pParams);
    virtual mfxStatus AllocFrames();
    virtual void DeleteFrames();
};


#endif // __PIPELINE_USER_H__
