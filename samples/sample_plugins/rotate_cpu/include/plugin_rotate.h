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

#ifndef __SAMPLE_PLUGIN_H__
#define __SAMPLE_PLUGIN_H__

#include <stdlib.h>
#include <memory.h>

#include "mfx_plugin_base.h"
#include "rotate_plugin_api.h"
#include "sample_defs.h"

typedef struct {
    mfxU32 StartLine;
    mfxU32 EndLine;
} DataChunk;

class Processor
{
public:
    Processor();
    virtual ~Processor();
    virtual mfxStatus SetAllocator(mfxFrameAllocator *pAlloc);
    virtual mfxStatus Init(mfxFrameSurface1 *frame_in, mfxFrameSurface1 *frame_out);
    virtual mfxStatus Process(DataChunk *chunk) = 0;

protected:
    //locks frame or report of an error
    mfxStatus LockFrame(mfxFrameSurface1 *frame);
    mfxStatus UnlockFrame(mfxFrameSurface1 *frame);

    mfxFrameSurface1  *m_pIn;
    mfxFrameSurface1  *m_pOut;
    mfxFrameAllocator *m_pAlloc;

    std::vector<mfxU8> m_YIn, m_UVIn;
    std::vector<mfxU8> m_YOut, m_UVOut;

};

class Rotator180 : public Processor
{
public:
    Rotator180();
    virtual ~Rotator180();

    virtual mfxStatus Process(DataChunk *chunk);
};

typedef struct {
    mfxFrameSurface1 *In;
    mfxFrameSurface1 *Out;
    bool bBusy;
    Processor *pProcessor;
} RotateTask;

class Rotate : public MFXGenericPlugin
{
public:
    Rotate();
    virtual ~Rotate();

    // methods to be called by Media SDK
    virtual mfxStatus PluginInit(mfxCoreInterface *core);
    virtual mfxStatus Init(mfxVideoParam *mfxParam);
    virtual mfxStatus SetAuxParams(void* auxParam, int auxParamSize);
    virtual mfxStatus PluginClose();
    virtual mfxStatus GetPluginParam(mfxPluginParam *par);
    virtual mfxStatus Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task);
    virtual mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a);
    virtual mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts);
    virtual void Release(){}
    // methods to be called by application
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    static MFXGenericPlugin* CreateGenericPlugin() {
        return new Rotate();
    }

    virtual mfxStatus Close();

protected:
    bool m_bInited;

    MFXCoreInterface m_mfxCore;

    mfxVideoParam   m_VideoParam;
    mfxPluginParam  m_PluginParam;
    RotateParam     m_Param;

    RotateTask      *m_pTasks;
    mfxU32          m_MaxNumTasks;

    DataChunk *m_pChunks;

    mfxU32 m_NumChunks;

    mfxStatus CheckParam(mfxVideoParam *mfxParam, RotateParam *pRotatePar);
    mfxStatus CheckInOutFrameInfo(mfxFrameInfo *pIn, mfxFrameInfo *pOut);
    mfxU32 FindFreeTaskIdx();

    bool m_bIsInOpaque;
    bool m_bIsOutOpaque;
};

#endif // __SAMPLE_PLUGIN_H__
