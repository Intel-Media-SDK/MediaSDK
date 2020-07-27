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

#if !defined(_WIN32) && !defined(_WIN64)

#include "opencl_filter_va.h"
#include "sample_defs.h"

#include <CL/cl_va_api_media_sharing_intel.h>

using std::endl;

DECL_CL_EXT_FUNC(clGetDeviceIDsFromVA_APIMediaAdapterINTEL);
DECL_CL_EXT_FUNC(clCreateFromVA_APIMediaSurfaceINTEL);
DECL_CL_EXT_FUNC(clEnqueueAcquireVA_APIMediaSurfacesINTEL);
DECL_CL_EXT_FUNC(clEnqueueReleaseVA_APIMediaSurfacesINTEL);

OpenCLFilterVA::OpenCLFilterVA()
{
    m_requiredOclExtensions.push_back("cl_intel_va_api_media_sharing");

    m_vaDisplay = 0;
    for(size_t i = 0; i < c_shared_surfaces_num; i++)
    {
        m_SharedSurfaces[i] = VA_INVALID_ID;
    }
}

OpenCLFilterVA::~OpenCLFilterVA() {
}

cl_int OpenCLFilterVA::OCLInit(mfxHDL device)
{
    MSDK_CHECK_POINTER(device, MFX_ERR_NULL_PTR);

    m_vaDisplay = (VADisplay*)device;

    return OpenCLFilterBase::OCLInit(device);
}

cl_int OpenCLFilterVA::InitSurfaceSharingExtension()
{
    if ( !INIT_CL_EXT_FUNC(m_clplatform, clGetDeviceIDsFromVA_APIMediaAdapterINTEL)
      || !INIT_CL_EXT_FUNC(m_clplatform, clCreateFromVA_APIMediaSurfaceINTEL)
      || !INIT_CL_EXT_FUNC(m_clplatform, clEnqueueAcquireVA_APIMediaSurfacesINTEL)
      || !INIT_CL_EXT_FUNC(m_clplatform, clEnqueueReleaseVA_APIMediaSurfacesINTEL))
    {
        log.error() << "OpenCLFilter: Couldn't get all of the media sharing routines" << endl;
        return CL_INVALID_PLATFORM;
    }

    return CL_SUCCESS;
}

cl_int OpenCLFilterVA::InitDevice()
{
    cl_int error = CL_SUCCESS;
    log.debug() << "OpenCLFilter: Try to init OCL device" << endl;

    cl_uint nDevices = 0;
    error = lin_clGetDeviceIDsFromVA_APIMediaAdapterINTEL(m_clplatform, CL_VA_API_DISPLAY_INTEL,
                                        m_vaDisplay, CL_PREFERRED_DEVICES_FOR_VA_API_INTEL, 1, &m_cldevice, &nDevices);
    if(error) {
        log.error() << "OpenCLFilter: clGetDeviceIDsFromVA_APIMediaAdapterINTEL failed. Error code: " << error << endl;
        return error;
    }

    if (!nDevices)
        return CL_INVALID_PLATFORM;


    // Initialize the shared context
    cl_context_properties props[] = { CL_CONTEXT_VA_API_DISPLAY_INTEL, (cl_context_properties) m_vaDisplay, CL_CONTEXT_INTEROP_USER_SYNC, 1, 0};
    m_clcontext = clCreateContext(props, 1, &m_cldevice, NULL, NULL, &error);

    if(error) {
        log.error() << "OpenCLFilter: clCreateContext failed. Error code: " << error << endl;
        return error;
    }

    log.debug() << "OpenCLFilter: OCL device inited" << endl;

    return error;
}

cl_mem OpenCLFilterVA::CreateSharedSurface(mfxMemId mid, int nView, bool bIsReadOnly)
{
    VASurfaceID* surf = NULL;
    mfxStatus sts = m_pAlloc->GetHDL(m_pAlloc->pthis, mid, reinterpret_cast<mfxHDL*>(&surf));
    if (sts) return 0;

    cl_int error = CL_SUCCESS;
    cl_mem mem = lin_clCreateFromVA_APIMediaSurfaceINTEL(m_clcontext, bIsReadOnly ? CL_MEM_READ_ONLY : CL_MEM_READ_WRITE,
                                            surf, nView, &error);
    if (error) {
        log.error() << "clCreateFromVA_APIMediaSurfaceINTEL failed. Error code: " << error << endl;
        return 0;
    }
    return mem;
}


bool OpenCLFilterVA::EnqueueAcquireSurfaces(cl_mem* surfaces, int nSurfaces)
{
    cl_int error = lin_clEnqueueAcquireVA_APIMediaSurfacesINTEL(m_clqueue, nSurfaces, surfaces, 0, NULL, NULL);
    if (error) {
        log.error() << "clEnqueueAcquireVA_APIMediaSurfacesINTEL failed. Error code: " << error << endl;
        return false;
    }
    return true ;
}

bool OpenCLFilterVA::EnqueueReleaseSurfaces(cl_mem* surfaces, int nSurfaces)
{
    cl_int error = lin_clEnqueueReleaseVA_APIMediaSurfacesINTEL(m_clqueue, nSurfaces, surfaces, 0, NULL, NULL);
    if (error) {
        log.error() << "clEnqueueReleaseVA_APIMediaSurfacesINTEL failed. Error code: " << error << endl;
        return false;
    }
    return true;
}

#endif // #if !defined(_WIN32) && !defined(_WIN64)
