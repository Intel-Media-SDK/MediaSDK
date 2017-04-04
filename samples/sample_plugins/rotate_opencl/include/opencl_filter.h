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

#pragma once

#include <iostream>
#include <stdexcept>
#include <CL/cl.h>

#include "mfxvideo++.h"
#include "logger.h"

#define INIT_CL_EXT_FUNC(x) x = (x ## _fn)clGetExtensionFunctionAddress(#x);
#define SAFE_OCL_FREE(P, FREE_FUNC) { if (P) { FREE_FUNC(P); P = NULL; } }

typedef struct {
    std::string program_source;
    std::string kernelY_FuncName;
    std::string kernelUV_FuncName;
    cl_program  clprogram;
    cl_kernel   clkernelY;
    cl_kernel   clkernelUV;
} OCL_YUV_kernel;

class OpenCLFilter
{
public:
    virtual ~OpenCLFilter() {}

    virtual mfxStatus SetAllocator(mfxFrameAllocator *pAlloc) = 0;

    virtual cl_int OCLInit(mfxHDL pD3DDeviceManager) = 0;
    virtual cl_int AddKernel(const char* filename, const char* kernelY_name, const char* kernelUV_name) = 0;
    virtual cl_int SelectKernel(unsigned kNo) = 0;
    virtual cl_int ProcessSurface(int width, int height, mfxMemId pSurfIn, mfxMemId pSurfOut) = 0;
};

class OpenCLFilterBase: public OpenCLFilter
{
public:
    OpenCLFilterBase();
    virtual ~OpenCLFilterBase();

    virtual mfxStatus SetAllocator(mfxFrameAllocator *pAlloc);

    virtual cl_int OCLInit(mfxHDL device);
    virtual cl_int AddKernel(const char* filename, const char* kernelY_name, const char* kernelUV_name);
    virtual cl_int SelectKernel(unsigned kNo); // cl
    virtual cl_int ProcessSurface(int width, int height, mfxMemId pSurfIn, mfxMemId pSurfOut);

protected: // functions
    virtual cl_int InitPlatform();
    virtual cl_int BuildKernels();
    virtual cl_int SetKernelArgs();
    virtual cl_int ReleaseResources();
    virtual cl_int ProcessSurface();
    virtual cl_int PrepareSharedSurfaces(int width, int height, mfxMemId surf_in, mfxMemId surf_out);

    virtual cl_int InitDevice() = 0;
    virtual cl_int InitSurfaceSharingExtension() = 0; // vaapi, d3d9, d3d11, etc. specific
    virtual bool   EnqueueAcquireSurfaces(cl_mem* surfaces, int nSurfaces) = 0;
    virtual bool   EnqueueReleaseSurfaces(cl_mem* surfaces, int nSurfaces) = 0;
    virtual cl_mem CreateSharedSurface(mfxMemId mid, int nView, bool bIsReadOnly) = 0;

    inline size_t chooseLocalSize(
        size_t globalSize, // frame width or height
        size_t preferred)  // preferred local size
    {
        size_t ret = 1;
        while ((globalSize % ret == 0) && ret <= preferred)
        {
             ret <<= 1;
        }

        return ret >> 1;
    }

protected: // variables
    bool                m_bInit;
    mfxFrameAllocator*  m_pAlloc;
    cl_platform_id      m_clplatform;
    cl_device_id        m_cldevice;
    cl_context          m_clcontext;
    cl_command_queue    m_clqueue;

    int                                 m_activeKernel;
    int                                 m_currentWidth;
    int                                 m_currentHeight;

    std::vector<OCL_YUV_kernel>         m_kernels;

    static const size_t c_shared_surfaces_num = 2; // In and Out
    static const size_t c_ocl_surface_buffers_num = 2*c_shared_surfaces_num; // YIn, UVIn, YOut, UVOut

    cl_mem              m_clbuffer[c_ocl_surface_buffers_num];

    size_t              m_GlobalWorkSizeY[2];
    size_t              m_GlobalWorkSizeUV[2];
    size_t              m_LocalWorkSizeY[2];
    size_t              m_LocalWorkSizeUV[2];

//  logger for filter depends on build configuration
    Logger<
#ifdef _DEBUG
        Debug
#else
        Error
#endif
    > log;
};

std::string readFile(const char *filename);
std::string getPathToExe();
