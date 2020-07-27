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

#include "mfx_samples_config.h"

#if defined(_WIN32) || defined(_WIN64)

#include "opencl_filter_dx11.h"
#include "sample_defs.h"

// INTEL DX9 sharing functions declared deprecated in OpenCL 1.2
#pragma warning( disable : 4996 )

// define for <CL/cl_ext.h>
#define DX9_MEDIA_SHARING
#define DX_MEDIA_SHARING
#define CL_DX9_MEDIA_SHARING_INTEL_EXT

#include <CL/cl_ext.h>
#include <CL/cl_d3d11.h>

using std::endl;

#define SAFE_OCL_FREE(P, FREE_FUNC)  { if (P) { FREE_FUNC(P); P = NULL; } }

#define MAX_PLATFORMS       32
#define MAX_STRING_SIZE     1024

DECL_CL_EXT_FUNC(clGetDeviceIDsFromD3D11KHR);
DECL_CL_EXT_FUNC(clCreateFromD3D11Texture2DKHR);
DECL_CL_EXT_FUNC(clEnqueueAcquireD3D11ObjectsKHR);
DECL_CL_EXT_FUNC(clEnqueueReleaseD3D11ObjectsKHR);

OpenCLFilterDX11::OpenCLFilterDX11() : m_pDevice(0)
{
    m_requiredOclExtensions.push_back("cl_khr_d3d11_sharing");

    for (int i = 0; i<c_shared_surfaces_num; i++)
        m_pSharedSurfaces[i] = 0;
}

OpenCLFilterDX11::~OpenCLFilterDX11() {

    ReleaseResources();

    for(unsigned i=0;i<m_kernels.size();i++) {
        SAFE_OCL_FREE(m_kernels[i].clprogram, clReleaseProgram);
        SAFE_OCL_FREE(m_kernels[i].clkernelY, clReleaseKernel);
        SAFE_OCL_FREE(m_kernels[i].clkernelUV, clReleaseKernel);
    }

    SAFE_OCL_FREE(m_clqueue,clReleaseCommandQueue);
    SAFE_OCL_FREE(m_clcontext,clReleaseContext);
}

cl_int OpenCLFilterDX11::OCLInit(mfxHDL device) {
    m_pDevice = reinterpret_cast<ID3D11Device*>(device);
    return OpenCLFilterBase::OCLInit(device);
}


cl_int OpenCLFilterDX11::InitSurfaceSharingExtension() {
    if( !INIT_CL_EXT_FUNC(m_clplatform,clGetDeviceIDsFromD3D11KHR)
     || !INIT_CL_EXT_FUNC(m_clplatform,clCreateFromD3D11Texture2DKHR)
     || !INIT_CL_EXT_FUNC(m_clplatform,clEnqueueAcquireD3D11ObjectsKHR)
     || !INIT_CL_EXT_FUNC(m_clplatform,clEnqueueReleaseD3D11ObjectsKHR))
    {
        log.error() << "OpenCLFilter: Couldn't get all of the media sharing routines" << endl;
        return CL_INVALID_PLATFORM;
    }

    return CL_SUCCESS;
}

cl_int OpenCLFilterDX11::InitDevice() {
    cl_int error = CL_SUCCESS;

    cl_uint numDevices = 0;
    error = pfn_clGetDeviceIDsFromD3D11KHR(m_clplatform,
        CL_D3D11_DEVICE_KHR,
        m_pDevice,
        CL_PREFERRED_DEVICES_FOR_D3D11_KHR,
        1,
        &m_cldevice,
        &numDevices);
    if(error) {
        log.error() << "OpenCLFilter: clGetDeviceIDsFromD3D11KHR failed. Error code: " << error << endl;
        return error;
    }

    const cl_context_properties props[] = {CL_CONTEXT_D3D11_DEVICE_KHR, (cl_context_properties)m_pDevice,
        CL_CONTEXT_INTEROP_USER_SYNC, CL_FALSE, NULL };
    m_clcontext = clCreateContext(props, 1, &m_cldevice, NULL, NULL, &error);
    if(error) {
        log.error() << "OpenCLFilter: clCreateContext failed. Error code: " << error << endl;
        return error;
    }

    log.debug() << "OpenCLFilter: OCL device inited" << endl;

    return error;
}

cl_mem OpenCLFilterDX11::CreateSharedSurface(mfxMemId mid, int nView, bool bIsReadOnly)
{
    mfxHDLPair mid_pair = { 0 };
    mfxStatus sts = m_pAlloc->GetHDL(m_pAlloc->pthis, mid, reinterpret_cast<mfxHDL*>(&mid_pair));
    if (sts) return 0;

    ID3D11Texture2D *surf = (ID3D11Texture2D*)mid_pair.first;

    cl_int error = CL_SUCCESS;
    cl_mem mem = pfn_clCreateFromD3D11Texture2DKHR(m_clcontext, bIsReadOnly ? CL_MEM_READ_ONLY : CL_MEM_READ_WRITE,
                                            surf, nView, &error);
    if (error) {
        log.error() << "clCreateFromD3D11Texture2DKHR failed. Error code: " << error << endl;
        return 0;
    }
    return mem;
}

bool OpenCLFilterDX11::EnqueueAcquireSurfaces(cl_mem* surfaces, int nSurfaces)
{
    cl_int error = pfn_clEnqueueAcquireD3D11ObjectsKHR(m_clqueue, nSurfaces, surfaces, 0, NULL, NULL);
    if (error) {
        log.error() << "clEnqueueAcquireD3D11ObjectsKHR failed. Error code: " << error << endl;
        return false;
    }
    return true;
}

bool OpenCLFilterDX11::EnqueueReleaseSurfaces(cl_mem* surfaces, int nSurfaces)
{
    cl_int error = pfn_clEnqueueReleaseD3D11ObjectsKHR(m_clqueue, nSurfaces, surfaces, 0, NULL, NULL);
    if (error) {
        log.error() << "clEnqueueReleaseD3D11ObjectsKHR failed. Error code: " << error << endl;
        return false;
    }
    return true;
}

#endif // #if defined(_WIN32) || defined(_WIN64)

