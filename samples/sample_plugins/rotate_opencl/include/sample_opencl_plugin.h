/******************************************************************************\
Copyright (c) 2005-2020, Intel Corporation
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
#include <memory>

#include "mfx_plugin_base.h"
#include "rotate_plugin_api.h"
#include "sample_defs.h"

#if defined(_WIN32) || defined(_WIN64)
#include "opencl_filter_dx9.h"
#include "opencl_filter_dx11.h"
#else
#include "opencl_filter_va.h"
#endif

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
};

class Rotator180 : public Processor
{
public:
    Rotator180();
    virtual ~Rotator180();

    virtual mfxStatus Process(DataChunk *chunk);
};

inline cl::size_t<1> make_size_t(const size_t &v0) {
    cl::size_t<1> ret;
    ret[0] = v0;
    return ret;
}
inline cl::size_t<2> make_size_t(const size_t &v0, const size_t &v1) {
    cl::size_t<2> ret;
    ret[0] = v0;
    ret[1] = v1;
    return ret;
}
inline cl::size_t<3> make_size_t(const size_t &v0, const size_t &v1, const size_t &v2) {
    cl::size_t<3> ret;
    ret[0] = v0;
    ret[1] = v1;
    ret[2] = v2;
    return ret;
}


class OpenCLFilterRotator180 : public Processor
{
public:
    OpenCLFilterRotator180(OpenCLFilter *pOpenCLFilter);
    virtual ~OpenCLFilterRotator180();

    virtual mfxStatus SetAllocator(mfxFrameAllocator *pAlloc);
    virtual mfxStatus Process(DataChunk * /* chunk */); // operates on whole frame
private:
    OpenCLFilter *m_pOpenCLFilter;
};

class OpenCLRotator180Context
{
public:
    OpenCLRotator180Context(const std::string &filename);

    void Rotate(size_t width, size_t height,
                size_t pitchIn, size_t pitchOut,
                void *pInY,  void *pInUV,
                void *pOutY, void *pOutUV);
private:
    inline void CreateBuffers(const cl::size_t<3> &Y_size,
                              const cl::size_t<3> &UV_size);
    inline void SetKernelArgs();

    cl::Platform m_platform;
    cl::Program m_program;
    cl::Device m_device;
    cl::Context m_context;
    cl::CommandQueue m_queue;
    cl::Kernel m_kernelY;
    cl::Kernel m_kernelUV;

    cl::Image2D m_InY;
    cl::Image2D m_InUV;
    cl::Image2D m_OutY;
    cl::Image2D m_OutUV;
};

class OpenCLRotator180 : public Processor
{
public:
    OpenCLRotator180(OpenCLRotator180Context *pOpenCLRotator180Context);
    virtual ~OpenCLRotator180();

    virtual mfxStatus Process(DataChunk * /* chunk */); // operates on whole frame
private:
    OpenCLRotator180Context *m_pOpenCLRotator180Context;
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
    virtual mfxStatus PluginClose();
    virtual mfxStatus GetPluginParam(mfxPluginParam *par);
    virtual mfxStatus Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task);
    virtual mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a);
    virtual mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts);
    virtual mfxStatus Init(mfxVideoParam *mfxParam);
    virtual mfxStatus SetAuxParams(void* auxParam, int auxParamSize);
    virtual void Release(void){};

    // methods to be called by application
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    virtual mfxStatus SetAllocator(mfxFrameAllocator *pAlloc);
    virtual mfxStatus SetHandle(mfxHandleType type, mfxHDL handle);
    virtual mfxStatus Close();

    static MFXGenericPlugin* CreatePlugin() {
        return new Rotate();
    }

protected: // functions
    mfxStatus CheckParam(mfxVideoParam *mfxParam, RotateParam *pRotatePar);
    mfxStatus CheckInOutFrameInfo(mfxFrameInfo *pIn, mfxFrameInfo *pOut);
    mfxU32 FindFreeTaskIdx();

protected: // variables
    bool m_bInited;
    bool m_bOpenCLSurfaceSharing;
    bool m_bIsInOpaque;
    bool m_bIsOutOpaque;

#if defined(_WIN32) || defined(_WIN64)
    std::unique_ptr<OpenCLFilter> m_OpenCLFilter;
#else
    std::unique_ptr<OpenCLFilterVA> m_OpenCLFilter;
#endif
    std::unique_ptr<OpenCLRotator180Context> m_pOpenCLRotator180Context;
    mfxHDL m_device;

    mfxCoreInterface *m_pmfxCore;

    mfxVideoParam   m_VideoParam;
    mfxPluginParam  m_PluginParam;
    RotateParam     m_Param;

    RotateTask      *m_pTasks;
    mfxU32          m_MaxNumTasks;

    mfxFrameAllocator * m_pAlloc;
    DataChunk *m_pChunks;

    mfxU32 m_NumChunks;
    mfxIMPL m_impl;
};

#endif // __SAMPLE_PLUGIN_H__
